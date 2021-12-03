//
//  HDRadioTuner.cpp
//  Turnip
//
//  Created by Andrei Chtcherbatchenko on 1/20/21.
//

#include "HDRadioTuner.hpp"

namespace SDR {

HDRadioTuner::HDRadioTuner(
    SDRDevice* device,
    const TunerParams& params,
    const HDRadioTunerAdvancedParams&
        advancedParams /*= HDRadioTunerAdvancedParams{}*/)
    : HDRadioTuner(
          device,
          params.get<double>("freq"),
          params.get<unsigned int>("program"),
          advancedParams) {}

HDRadioTuner::HDRadioTuner(
    SDRDevice* device,
    double frequency,
    unsigned int program,
    const HDRadioTunerAdvancedParams&
        advancedParams /*= HDRadioTunerAdvancedParams{}*/)
    : sdrInput_(device, deviceSamplingRate(), frequency),
      nrsc5Decoder_(program),
      mp3Encoder_(audioSamplingRate(), 2, advancedParams.outputBitrateKbps),
      mp3Output_(*audioQueue()),
      sdrMetadataOutput_(*metadataQueue()),
      nrsc5MetadataOutput_(*metadataQueue()) {
  // Assemble graph
  graph()
      .connectQueued(sdrInput_, nrsc5Decoder_)
      .connect(nrsc5Decoder_, mp3Encoder_)
      .connect<DecodeNRSC5::OUT_DISCONTINUITY, MP3Encode::IN_DISCONTINUITY>(
          nrsc5Decoder_, mp3Encoder_)
      .connect<MP3Encode::OUT_OUTPUT, QueueOut<MP3Packet>::IN_INPUT_VECTOR>(
          mp3Encoder_, mp3Output_)
      .connect<SDRPlayInput::OUT_METADATA, QueueOut<MetadataPacket>::IN_INPUT>(
          sdrInput_, sdrMetadataOutput_)
      .connect<DecodeNRSC5::OUT_METADATA, QueueOut<MetadataPacket>::IN_INPUT>(
          nrsc5Decoder_, nrsc5MetadataOutput_);

  // Bind controls
  graph()
      .bind<DecodeNRSC5::CTRL_PROGRAM>(nrsc5Decoder_, "program")
      .bind<SDRPlayInput::CTRL_LNA_STATE>(sdrInput_, "lna_state");
}

} // namespace SDR
