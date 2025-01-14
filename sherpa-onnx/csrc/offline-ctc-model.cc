// sherpa-onnx/csrc/offline-ctc-model.cc
//
// Copyright (c)  2022-2023  Xiaomi Corporation

#include "sherpa-onnx/csrc/offline-ctc-model.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>

#include "sherpa-onnx/csrc/macros.h"
#include "sherpa-onnx/csrc/offline-nemo-enc-dec-ctc-model.h"
#include "sherpa-onnx/csrc/onnx-utils.h"

namespace {

enum class ModelType {
  kEncDecCTCModelBPE,
  kUnkown,
};

}

namespace sherpa_onnx {

static ModelType GetModelType(char *model_data, size_t model_data_length,
                              bool debug) {
  Ort::Env env(ORT_LOGGING_LEVEL_WARNING);
  Ort::SessionOptions sess_opts;

  auto sess = std::make_unique<Ort::Session>(env, model_data, model_data_length,
                                             sess_opts);

  Ort::ModelMetadata meta_data = sess->GetModelMetadata();
  if (debug) {
    std::ostringstream os;
    PrintModelMetadata(os, meta_data);
    SHERPA_ONNX_LOGE("%s", os.str().c_str());
  }

  Ort::AllocatorWithDefaultOptions allocator;
  auto model_type =
      meta_data.LookupCustomMetadataMapAllocated("model_type", allocator);
  if (!model_type) {
    SHERPA_ONNX_LOGE(
        "No model_type in the metadata!\n"
        "If you are using models from NeMo, please refer to\n"
        "https://huggingface.co/csukuangfj/"
        "sherpa-onnx-nemo-ctc-en-citrinet-512/blob/main/add-model-metadata.py"
        "\n"
        "for how to add metadta to model.onnx\n");
    return ModelType::kUnkown;
  }

  if (model_type.get() == std::string("EncDecCTCModelBPE")) {
    return ModelType::kEncDecCTCModelBPE;
  } else {
    SHERPA_ONNX_LOGE("Unsupported model_type: %s", model_type.get());
    return ModelType::kUnkown;
  }
}

std::unique_ptr<OfflineCtcModel> OfflineCtcModel::Create(
    const OfflineModelConfig &config) {
  ModelType model_type = ModelType::kUnkown;

  {
    auto buffer = ReadFile(config.nemo_ctc.model);

    model_type = GetModelType(buffer.data(), buffer.size(), config.debug);
  }

  switch (model_type) {
    case ModelType::kEncDecCTCModelBPE:
      return std::make_unique<OfflineNemoEncDecCtcModel>(config);
      break;
    case ModelType::kUnkown:
      SHERPA_ONNX_LOGE("Unknown model type in offline CTC!");
      return nullptr;
  }

  return nullptr;
}

}  // namespace sherpa_onnx
