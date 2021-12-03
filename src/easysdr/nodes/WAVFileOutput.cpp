//
//  WAVFileOutput.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/2/21.
//

#include "WAVFileOutput.hpp"

namespace SDR {

void WAVFileOutput::writeHeader(uint32_t fileLength /*= 0*/) {
  constexpr uint16_t BitsPerSample = 16;
  constexpr uint16_t NumChannels = 1;
  constexpr uint32_t SampleRate = 48000;

#pragma pack(push, 1)
  struct {
    uint32_t signature0 = 'FFIR';
    uint32_t chunkSize = 0;
    uint32_t signature1 = 'EVAW';
    uint32_t signature2 = ' tmf';
    uint32_t subchunk1Size = 16; // PCM
    uint16_t audioFormat = 1; // PCM
    uint16_t channels = NumChannels;
    uint32_t sampleRate = SampleRate;
    uint32_t byteRate = SampleRate * BitsPerSample * NumChannels / 8;
    uint16_t blockAlign = BitsPerSample * NumChannels / 8;
    uint16_t bitsPerSample = BitsPerSample;
    uint32_t signature3 = 'atad';
    uint32_t subchunk2Size = 0;
  } header;
#pragma pack(pop)

  static_assert(sizeof(header) == 44, "");

  if (fileLength) {
    header.chunkSize = fileLength - 8;
    header.subchunk2Size = fileLength - 44;
  }

  stream_.write(reinterpret_cast<const char*>(&header), sizeof(header));
}

void WAVFileOutput::init() {
  stream_.open(path_, std::ios::binary);
  writeHeader();
}

void WAVFileOutput::process() {
  auto& inData = getData<IN_INPUT>();
  for (int16_t sample : inData) {
    stream_.write(reinterpret_cast<const char*>(&sample), sizeof(int16_t));
  }
}

void WAVFileOutput::destroy() {
  // TODO: .wav files cannot grow larger than 4GB so break streams into multiple
  // files
  const uint32_t fileLength = static_cast<uint32_t>(stream_.tellp());
  stream_.seekp(0);
  writeHeader(fileLength);
  stream_.close();
}

} // namespace SDR
