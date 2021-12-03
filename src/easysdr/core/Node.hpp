//
//  Node.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#pragma once

#include <functional>
#include <memory>
#include <tuple>

namespace SDR {

class BaseNode {
 public:
  BaseNode() {}
  virtual ~BaseNode() {}

  virtual void init() {}
  virtual void reset() = 0;
  virtual void process() = 0;
  virtual void destroy() {}
};

template <typename T>
class Input final {
 public:
  using Type = T;

  Type** dataPtr() const {
    return dataPtr_;
  }

  void setDataPtr(Type** dataPtr) {
    dataPtr_ = dataPtr;
  }

  void reset() {}

 private:
  Type** dataPtr_ = nullptr;
};

template <typename T>
class Output final {
 public:
  using Type = T;

  void storeData(T&& data) {
    if (dataPtr_) {
      throw std::runtime_error("data already stored");
    }
    data_ = std::move(data);
    dataPtr_ = &data_;
  }

  Type** getDataPtr() {
    return &dataPtr_;
  }

  void reset() {
    dataPtr_ = nullptr;
  }

 private:
  Type data_;
  Type* dataPtr_ = nullptr;
};

template <typename T>
class Control final {
 public:
  using Type = T;
  using ObserverType = std::function<void(const T&)>;

  const T& value() const {
    return value_;
  }

  void setValue(T value) {
    value_ = value;
    if (observer_) {
      observer_(value_);
    }
  }

  void observe(ObserverType observer) {
    if (observer_) {
      throw std::runtime_error("already observing");
    }
    observer_ = observer;
  }

  void unobserve() {
    if (!observer_) {
      throw std::runtime_error("not observing");
    }
    observer_ = nullptr;
  }

  void reset() {}

 private:
  Type value_;
  ObserverType observer_;
};

template <class... Args>
class Node : public BaseNode {
 public:
  using DataTuple = std::tuple<Args...>;

  template <size_t Idx>
  using PortType = typename std::tuple_element<Idx, DataTuple>::type;

  template <size_t Idx>
  using DataType = typename PortType<Idx>::Type;

  template <size_t Idx>
  using ObserverType = typename PortType<Idx>::ObserverType;

  template <size_t Idx>
  PortType<Idx>& portAt() {
    return std::get<Idx>(data_);
  }

  template <size_t Idx>
  const PortType<Idx>& portAt() const {
    return std::get<Idx>(data_);
  }

  template <size_t Idx>
  bool isConnected() const {
    return portAt<Idx>().dataPtr() != nullptr;
  }

  template <size_t Idx>
  bool hasData() const {
    auto** dataPtr = portAt<Idx>().dataPtr();
    if (!dataPtr) {
      throw std::runtime_error("unconnected input");
    }
    return *dataPtr != nullptr;
  };

  template <size_t Idx>
  DataType<Idx>& getData() {
    auto** dataPtr = portAt<Idx>().dataPtr();
    if (!dataPtr) {
      throw std::runtime_error("unconnected input");
    }
    if (!*dataPtr) {
      throw std::runtime_error("no data");
    }
    return **dataPtr;
  };

  template <size_t Idx>
  void setData(DataType<Idx>&& data) {
    portAt<Idx>().storeData(std::move(data));
  };

  template <size_t Idx>
  void observe(ObserverType<Idx> observer) {
    portAt<Idx>().observe(observer);
  }

  template <size_t Idx>
  void unobserve() {
    portAt<Idx>().unobserve();
  }

  virtual void reset() override {
    std::apply([](auto&... port) { (port.reset(), ...); }, data_);
  }

 private:
  DataTuple data_;
};

} // namespace SDR
