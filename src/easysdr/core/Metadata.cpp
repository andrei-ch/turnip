//
//  Metadata.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#include "Metadata.hpp"

#include <boost/property_tree/json_parser.hpp>

namespace SDR {

namespace {

struct make_ptree_visitor : public boost::static_visitor<> {
  make_ptree_visitor(boost::property_tree::ptree& pt, const std::string& p)
      : ptree(pt), path(p) {}
  boost::property_tree::ptree& ptree;
  const std::string& path;

  template <typename T>
  void operator()(T v) const {
    ptree.put<T>(path, v);
  }
};

} // namespace

std::string metadataToJsonString(const MetadataPacket& metadata) {
  boost::property_tree::ptree pt;
  for (auto pair : metadata) {
    make_ptree_visitor v(pt, pair.first);
    boost::apply_visitor(v, pair.second);
  }

  std::stringstream stream;
  boost::property_tree::write_json(stream, pt, false);
  return stream.str();
}

} // namespace SDR
