//
//  Graph.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#pragma once

#include "Latch.hpp"
#include "Node.hpp"
#include "Queue.hpp"
#include "QueueIn.hpp"
#include "QueueOut.hpp"

#include <boost/any.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SDR {

class Graph final {
 public:
  Graph() {}

  template <class FromNode, class ToNode>
  Graph& connect(FromNode& fromNode, ToNode& toNode);

  template <class FromNode, class ToNode>
  Graph& connectQueued(FromNode& fromNode, ToNode& toNode);

  template <size_t FromIdx, size_t ToIdx, class FromNode, class ToNode>
  Graph& connect(FromNode& fromNode, ToNode& toNode);

  template <size_t FromIdx, size_t ToIdx, class FromNode, class ToNode>
  Graph&
  connectQueued(FromNode& fromNode, ToNode& toNode, size_t queueSize = 100);

  template <typename DataType>
  using BindingValidator = std::function<DataType(const DataType&)>;

  template <
      size_t Idx,
      class Node,
      typename DataType = typename Node::template DataType<Idx>>
  Graph& bind(
      Node& node,
      const std::string& name,
      BindingValidator<DataType> validator = nullptr);
  Graph& unbindAll();

  void postUpdates(std::unordered_map<std::string, boost::any>&& updates);

  void startRunning();
  void stopRunning();

 private:
  struct ControlActions {
    std::mutex mutex;
    std::vector<std::function<void()>> data;
  };

  struct Subgraph {
    std::unique_ptr<BaseQueue> inQueue;
    std::unordered_set<BaseNode*> nodes;
    std::unordered_map<BaseNode*, std::unordered_set<BaseNode*>> edges;
    std::unique_ptr<ControlActions> actions;
  };

  using BindingSetterType = std::function<void(const boost::any&)>;

  struct Binding {
    const BaseNode& node;
    BindingSetterType setter;
  };

 private:
  void runner(const Subgraph& sub);
  Subgraph* createSubgraph();
  Subgraph* findSubgraph(const BaseNode* node);
  void addNodeToSubgraph(BaseNode* node, Subgraph* sub);
  void ensureSameSubgraph(BaseNode* node0, BaseNode* node1);
  void ensureDisconnectedSubgraphs(BaseNode* node0, BaseNode* node1);
  void mergeSubgraphs(Subgraph* sub0, Subgraph* sub1);
  std::vector<BaseNode*> topologicalSort(const Subgraph& topology);
  void initNodes(const std::vector<BaseNode*>& nodes);
  void destroyNodes(const std::vector<BaseNode*>& nodes);
  void runActions(const Subgraph& topology);
  bool waitForData(const Subgraph& topology);

 private:
  std::vector<std::thread> threads_;
  std::atomic<bool> stopping_;
  Latch initLatch_;
  Latch destroyLatch_;
  std::unordered_set<std::shared_ptr<Subgraph>> subgraphs_;
  std::unordered_map<const BaseNode*, Subgraph*> nodeToSubgraph_;
  std::vector<std::unique_ptr<BaseNode>> extraNodes_;
  std::unordered_map<std::string, std::vector<Binding>> bindings_;
};

template <class FromNode, class ToNode>
Graph& Graph::connect(FromNode& fromNode, ToNode& toNode) {
  connect<FromNode::OUT_OUTPUT, ToNode::IN_INPUT, FromNode, ToNode>(
      fromNode, toNode);
  return *this;
}

template <class FromNode, class ToNode>
Graph& Graph::connectQueued(FromNode& fromNode, ToNode& toNode) {
  connectQueued<FromNode::OUT_OUTPUT, ToNode::IN_INPUT, FromNode, ToNode>(
      fromNode, toNode);
  return *this;
}

template <size_t FromIdx, size_t ToIdx, class FromNode, class ToNode>
Graph& Graph::connect(FromNode& fromNode, ToNode& toNode) {
  if (reinterpret_cast<void*>(&fromNode) == reinterpret_cast<void*>(&toNode)) {
    throw std::runtime_error("bad topology");
  }

  ensureSameSubgraph(&fromNode, &toNode);

  const auto sub = findSubgraph(&fromNode);
  sub->edges[&fromNode].insert(&toNode);

  auto& port = toNode.template portAt<ToIdx>();
  if (port.dataPtr()) {
    throw std::runtime_error("already connected");
  }
  port.setDataPtr(fromNode.template portAt<FromIdx>().getDataPtr());
  return *this;
}

template <size_t FromIdx, size_t ToIdx, class FromNode, class ToNode>
Graph& Graph::connectQueued(
    FromNode& fromNode,
    ToNode& toNode,
    size_t queueSize /*= 100*/) {
  if (reinterpret_cast<void*>(&fromNode) == reinterpret_cast<void*>(&toNode)) {
    throw std::runtime_error("bad topology");
  }

  ensureDisconnectedSubgraphs(&fromNode, &toNode);

  Subgraph* sub = findSubgraph(&toNode);
  if (sub->inQueue) {
    throw std::runtime_error("bad topology");
  }

  using DataType = typename FromNode::template DataType<FromIdx>;
  using QueueInNode = QueueIn<DataType>;
  using QueueOutNode = QueueOut<DataType>;

  auto queue = std::make_unique<Queue<DataType>>(queueSize);
  auto queueIn = std::make_unique<QueueInNode>(*queue);
  auto queueOut = std::make_unique<QueueOutNode>(*queue);

  connect<FromIdx, QueueOutNode::IN_INPUT>(fromNode, *queueOut);
  connect<QueueInNode::OUT_OUTPUT, ToIdx>(*queueIn, toNode);

  sub->inQueue = std::move(queue);
  extraNodes_.push_back(std::move(queueIn));
  extraNodes_.push_back(std::move(queueOut));
  return *this;
}

template <size_t Idx, class Node, typename DataType>
Graph& Graph::bind(
    Node& node,
    const std::string& name,
    std::function<DataType(const DataType&)> validator /*= nullptr*/) {
  const auto setter = [&node, validator](const boost::any& value) {
    const auto& typedValue = boost::any_cast<DataType>(value);
    node.template portAt<Idx>().setValue(
        validator ? validator(typedValue) : typedValue);
  };

  bindings_[name].push_back(Binding{node, setter});
  return *this;
}

inline Graph& Graph::unbindAll() {
  bindings_.clear();
  return *this;
}

inline Graph::Subgraph* Graph::findSubgraph(const BaseNode* node) {
  const auto found = nodeToSubgraph_.find(node);
  return found != nodeToSubgraph_.end() ? found->second : nullptr;
}

inline Graph::Subgraph* Graph::createSubgraph() {
  return subgraphs_.insert(std::make_shared<Subgraph>()).first->get();
}

} // namespace SDR
