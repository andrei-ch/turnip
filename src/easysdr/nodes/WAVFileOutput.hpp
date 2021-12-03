//
//  WAVFileOutput.hpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#pragma once

#include "easysdr/core/Node.hpp"

#include <fstream>
#include <vector>

namespace SDR {

class WAVFileOutput final : public Node<Input<std::vector<int16_t>>> {
 public:
  WAVFileOutput(const std::string& path) : path_(path) {}

  enum { IN_INPUT = 0 };

  virtual void init() override;
  virtual void process() override;
  virtual void destroy() override;

 private:
  void writeHeader(uint32_t fileLength = 0);

 private:
  std::string path_;
  std::ofstream stream_;
};

} // namespace SDR
