//
//  TunerParams.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/20/21.
//

#include "TunerParams.hpp"

#include <boost/algorithm/string.hpp>
#include <iomanip>

namespace SDR {

namespace {

template <typename T>
inline T coerce_type(const std::string& value);

template <>
inline bool coerce_type(const std::string& value) {
  if (value.length() != 1) {
    throw std::runtime_error("Invalid value " + value);
  }
  switch (value[0]) {
    case '0':
      return false;
    case '1':
      return true;
    default:
      throw std::runtime_error("Invalid value " + value);
  }
}

template <>
inline unsigned int coerce_type(const std::string& value) {
  const auto v = std::atol(value.c_str());
  if (v < 0) {
    throw std::runtime_error("Invalid value " + value);
  }
  return static_cast<unsigned int>(v);
}

template <>
inline double coerce_type(const std::string& value) {
  return std::atof(value.c_str());
}

template <>
inline std::string coerce_type(const std::string& value) {
  return value;
}

struct variant_to_any_convertor : public boost::static_visitor<> {
  variant_to_any_convertor(
      std::unordered_map<std::string, boost::any>& map,
      const std::string& key)
      : map_(map), key_(key) {}

  template <typename T>
  void operator()(T v) const {
    map_.emplace(key_, v);
  }

 private:
  std::unordered_map<std::string, boost::any>& map_;
  const std::string& key_;
};

} // namespace

void TunerParams::parse(TunerParams& params, const std::string& string) {
  static const std::unordered_map<std::string, TunerParams::Type> schema = {
      {"freq", TunerParams::Double},
      {"modem", TunerParams::String},
      {"bw", TunerParams::Double},
      {"mono", TunerParams::Bool},
      {"program", TunerParams::UInt},
      {"lna_state", TunerParams::UInt},
      {"mode", TunerParams::UInt},
  };

  std::vector<std::string> pairs;
  boost::split(pairs, string, boost::is_any_of("&"));
  for (auto& pair : pairs) {
    std::vector<std::string> name_and_value;
    boost::split(name_and_value, pair, boost::is_any_of("="));

    if (name_and_value.size() != 2) {
      throw std::runtime_error("invalid uri");
    }

    const auto& name = name_and_value[0];
    const auto& value = name_and_value[1];

    const auto found = schema.find(name);
    if (found == schema.end()) {
      throw std::runtime_error("Unknown parameter " + name);
    }

    switch (found->second) {
      case TunerParams::Bool:
        params[name] = coerce_type<bool>(value);
        break;
      case TunerParams::UInt:
        params[name] = coerce_type<unsigned int>(value);
        break;
      case TunerParams::Double:
        params[name] = coerce_type<double>(value);
        break;
      case TunerParams::String:
        params[name] = coerce_type<std::string>(value);
        break;
      default:
        throw std::runtime_error("Unknown type " + name);
    }
  }
}

std::unordered_map<std::string, boost::any> TunerParams::toAnyMap(
    const TunerParams& params) {
  std::unordered_map<std::string, boost::any> any_map;
  for (const auto& pair : params) {
    variant_to_any_convertor visitor(any_map, pair.first);
    boost::apply_visitor(visitor, pair.second);
  }
  return any_map;
}

std::ostream& operator<<(std::ostream& os, const TunerParams& params) {
  std::vector<TunerParams::key_type> keys;
  keys.reserve(params.size());
  for (const auto& pair : params) {
    keys.push_back(pair.first);
  }
  std::sort(keys.begin(), keys.end());
  os << "{" << std::endl;
  for (const auto& key : keys) {
    os << "  " << key << " = " << std::fixed << std::setprecision(1)
       << params.at(key) << std::endl;
  }
  os << "}";
  return os;
}

} // namespace SDR
