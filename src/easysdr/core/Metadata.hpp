//
//  Metadata.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/1/21.
//

#pragma once

#include <boost/variant.hpp>
#include <string>
#include <unordered_map>

namespace SDR {

using MetadataItem = boost::variant<bool, unsigned int, double, std::string>;
using MetadataPacket = std::unordered_map<std::string, MetadataItem>;

std::string metadataToJsonString(const MetadataPacket& metadata);

} // namespace SDR
