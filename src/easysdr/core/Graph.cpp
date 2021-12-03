//
//  Graph.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#include "Graph.hpp"

#include "Node.hpp"

#include <chrono>
#include <iostream>

namespace SDR {

void Graph::addNodeToSubgraph(BaseNode* node, Subgraph* sub) {
  const auto prev = findSubgraph(node);
  if (prev) {
    assert(prev == sub);
    return;
  }
  sub->nodes.insert(node);
  nodeToSubgraph_[node] = sub;
}

void Graph::mergeSubgraphs(Subgraph* sub0, Subgraph* sub1) {
  if (sub0 == sub1) {
    return;
  }
  if (sub0->inQueue && sub1->inQueue) {
    throw std::runtime_error("bad topology");
  }
  // TODO handle additional "bad topology" scenarios:
  // 1. looping back to SDR input thread
  // 2. attempting to directly connect subgraphs which are already connected via
  // queue
  for (auto node : sub1->nodes) {
    nodeToSubgraph_[node] = sub0;
  }
  sub0->nodes.merge(sub1->nodes);
  sub0->edges.merge(sub1->edges);
  if (sub1->inQueue) {
    sub0->inQueue = std::move(sub1->inQueue);
  }
  subgraphs_.erase(std::shared_ptr<Subgraph>(sub1, [](auto) {}));
}

void Graph::ensureSameSubgraph(BaseNode* node0, BaseNode* node1) {
  const auto sub0 = findSubgraph(node0);
  const auto sub1 = findSubgraph(node1);
  if (sub0 && sub1) {
    if (sub0 != sub1) {
      mergeSubgraphs(sub0, sub1);
    }
  } else {
    const auto sub = sub0 ?: sub1 ?: createSubgraph();
    addNodeToSubgraph(node0, sub);
    addNodeToSubgraph(node1, sub);
  }
}

void Graph::ensureDisconnectedSubgraphs(BaseNode* node0, BaseNode* node1) {
  const auto sub0 = findSubgraph(node0);
  const auto sub1 = findSubgraph(node1);
  if (sub0 && sub1) {
    if (sub0 == sub1) {
      throw std::runtime_error("bad topology");
    }
  } else {
    if (!sub0) {
      addNodeToSubgraph(node0, createSubgraph());
    }
    if (!sub1) {
      addNodeToSubgraph(node1, createSubgraph());
    }
  }
}

std::vector<BaseNode*> Graph::topologicalSort(const Subgraph& topology) {
  std::vector<BaseNode*> orderedNodes;
  std::unordered_set<BaseNode*> currentNodes;
  std::unordered_map<BaseNode*, size_t> edgeCounts;

  for (auto pair : topology.edges) {
    for (auto node : pair.second) {
      edgeCounts[node]++;
    }
  }

  for (auto node : topology.nodes) {
    if (edgeCounts.find(node) == edgeCounts.end()) {
      currentNodes.insert(node);
    }
  }

  while (!currentNodes.empty()) {
    BaseNode* node = *currentNodes.begin();
    currentNodes.erase(currentNodes.begin());

    orderedNodes.push_back(node);

    const auto edges = topology.edges.find(node);
    if (edges == topology.edges.end()) {
      continue;
    }

    for (auto nextNode : edges->second) {
      if (--edgeCounts[nextNode] == 0) {
        currentNodes.insert(nextNode);
        edgeCounts.erase(nextNode);
      }
    }
  }

  if (!edgeCounts.empty()) {
    throw std::runtime_error("cycle topology");
  }
  return orderedNodes;
}

void Graph::startRunning() {
  assert(!subgraphs_.empty());
  initLatch_.reset(subgraphs_.size());
  stopping_ = false;
  for (auto& t : subgraphs_) {
    t->actions = std::make_unique<ControlActions>();
    threads_.emplace_back([this, t]() { runner(*t); });
  }
  initLatch_.wait();
}

void Graph::stopRunning() {
  destroyLatch_.reset(subgraphs_.size());
  stopping_ = true;
  destroyLatch_.wait();
  for (auto& thread : threads_) {
    thread.join();
  }
  threads_.clear();
}

void Graph::runner(const Subgraph& topology) {
  const auto orderedNodes = topologicalSort(topology);

  initNodes(orderedNodes);

  while (!stopping_) {
    try {
      runActions(topology);
      if (!waitForData(topology)) {
        continue;
      }
      for (auto node : orderedNodes) {
        node->reset();
      }
      for (auto node : orderedNodes) {
        node->process();
      }
    } catch (const std::exception& ex) {
      std::cerr << "Graph node exception in process(): " << ex.what()
                << std::endl;
    }
  }

  destroyNodes(orderedNodes);
}

void Graph::initNodes(const std::vector<BaseNode*>& nodes) {
  for (auto node : nodes) {
    try {
      node->init();
    } catch (const std::exception& ex) {
      std::cerr << "Graph node exception in init(): " << ex.what() << std::endl;
    }
  }
  initLatch_.arrive_and_wait();
}

void Graph::destroyNodes(const std::vector<BaseNode*>& nodes) {
  destroyLatch_.arrive_and_wait();
  for (auto node : nodes) {
    try {
      node->destroy();
    } catch (const std::exception& ex) {
      std::cerr << "Graph node exception in destroy(): " << ex.what()
                << std::endl;
    }
  }
}

void Graph::runActions(const Subgraph& topology) {
  auto& actions = topology.actions;
  std::lock_guard<std::mutex> lock(actions->mutex);
  for (const auto& action : actions->data) {
    action();
  }
  actions->data.clear();
}

bool Graph::waitForData(const Subgraph& topology) {
  const auto& queue = topology.inQueue;
  if (!queue) {
    return true;
  }
  using namespace std::chrono_literals;
  return queue->waitForData(10ms);
}

void Graph::postUpdates(std::unordered_map<std::string, boost::any>&& updates) {
  struct SetterWithValue {
    const BindingSetterType& setter;
    const boost::any& value;
  };

  // Map updates to threads
  std::unordered_map<Subgraph*, std::vector<SetterWithValue>> threads;
  for (const auto& pair : updates) {
    const auto found = bindings_.find(pair.first);
    if (found == bindings_.end()) {
      continue;
    }
    for (const auto& binding : found->second) {
      auto* subgraph = findSubgraph(&binding.node);
      if (!subgraph) {
        throw std::runtime_error("invalid control node");
      }
      threads[subgraph].push_back(SetterWithValue{
          .setter = binding.setter,
          .value = pair.second,
      });
    }
  }

  if (threads.empty()) {
    return;
  }

  // TODO implement transaction support: stop all threads

  const auto storage =
      std::make_shared<std::unordered_map<std::string, boost::any>>(
          std::move(updates));

  // Execute updates in threads
  for (const auto& pair : threads) {
    auto* subgraph = pair.first;

    const auto refs =
        std::make_shared<std::vector<SetterWithValue>>(pair.second);

    auto& actions = subgraph->actions;
    std::lock_guard<std::mutex> lock(actions->mutex);
    actions->data.push_back([storage, refs] {
      for (const auto& ref : *refs) {
        try {
          ref.setter(ref.value);
        } catch (const std::exception& ex) {
          std::cerr << "Exception setting control value: " << ex.what()
                    << std::endl;
        }
      }
    });
  }
}

} // namespace SDR
