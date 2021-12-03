//
//  TunerParams.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/3/21.
//

#pragma once

#include <boost/any.hpp>
#include <boost/variant.hpp>
#include <string>
#include <unordered_map>

namespace SDR {

class TunerParams
    : public std::unordered_map<
          std::string,
          boost::variant<bool, unsigned int, double, std::string>> {
 public:
  enum Type { Bool, UInt, Double, String };

 public:
  TunerParams() {}
  TunerParams(const std::string& string);

  void parse(const std::string& string);
  std::unordered_map<std::string, boost::any> toAnyMap() const;

  template <typename T>
  const T& get(const std::string& name) const;

  template <typename T>
  void set(const std::string& name, T value, bool overwrite = false);

 private:
  static void parse(TunerParams& params, const std::string& string);
  static std::unordered_map<std::string, boost::any> toAnyMap(
      const TunerParams& params);
};

inline TunerParams::TunerParams(const std::string& string) {
  parse(string);
}

inline void TunerParams::parse(const std::string& string) {
  parse(*this, string);
}

inline std::unordered_map<std::string, boost::any> TunerParams::toAnyMap()
    const {
  return toAnyMap(*this);
}

template <typename T>
inline const T& TunerParams::get(const std::string& name) const {
  return boost::get<T>(at(name));
}

template <typename T>
inline void
TunerParams::set(const std::string& name, T value, bool overwrite /*= false*/) {
  if (overwrite || !count(name)) {
    insert_or_assign(name, value);
  }
}

std::ostream& operator<<(std::ostream& os, const TunerParams& params);

} // namespace SDR
