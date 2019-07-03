// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <vpu/frontend/frontend.hpp>

#include <vector>
#include <map>
#include <unordered_set>
#include <memory>
#include <set>

namespace vpu {

namespace {

class NormalizeStage final : public StageNode {
private:
    StagePtr cloneImpl() const override {
        return std::make_shared<NormalizeStage>(*this);
    }

    DataMap<float> propagateScaleFactorsImpl(
            const DataMap<float>&,
            ScalePropagationStep) override {
        IE_ASSERT(_inputEdges.size() == 2);
        IE_ASSERT(_outputEdges.size() == 1);

        auto input = _inputEdges[0]->input();
        auto scales = _inputEdges[1]->input();
        auto output = _outputEdges[0]->output();

        DataMap<float> out;

        out[input] = 1.0f;
        out[scales] = 1.0f;
        out[output] = 1.0f;

        return out;
    }

    DataMap<DimsOrder> propagateDataOrderImpl() const override {
        IE_ASSERT(_inputEdges.size() == 2);
        IE_ASSERT(_outputEdges.size() == 1);

        auto input = _inputEdges[0]->input();
        auto output = _outputEdges[0]->output();

        DataMap<DimsOrder> out;

        out[output] = input->desc().dimsOrder();

        return out;
    }

    DataMap<StridesRequirement> getDataStridesRequirementsImpl() const override {
        return DataMap<StridesRequirement>();
    }

    void finalizeDataLayoutImpl() override {
    }

    DataMap<BatchSupport> getBatchSupportInfoImpl() const override {
        IE_ASSERT(_inputEdges.size() == 2);
        IE_ASSERT(_outputEdges.size() == 1);

        auto input = _inputEdges[0]->input();
        auto output = _outputEdges[0]->output();

        DataMap<BatchSupport> out;

        out[input] = BatchSupport::Split;
        out[output] = BatchSupport::Split;

        return out;
    }

    void finalCheckImpl() const override {
    }

    void serializeParamsImpl(BlobSerializer& serializer) const override {
        auto acrossSpatial = attrs().get<bool>("acrossSpatial");
        auto channelShared = attrs().get<bool>("channelShared");
        auto eps = attrs().get<float>("eps");

        serializer.append(static_cast<int32_t>(acrossSpatial));
        serializer.append(static_cast<int32_t>(channelShared));
        serializer.append(static_cast<float>(eps));
    }

    void serializeDataImpl(BlobSerializer& serializer) const override {
        IE_ASSERT(_inputEdges.size() == 2);
        IE_ASSERT(_outputEdges.size() == 1);
        IE_ASSERT(_tempBufferEdges.empty());

        auto input = _inputEdges[0]->input();
        auto scales = _inputEdges[1]->input();
        auto output = _outputEdges[0]->output();

        input->serializeOldBuffer(handle_from_this(), serializer);
        output->serializeOldBuffer(handle_from_this(), serializer);
        scales->serializeOldBuffer(handle_from_this(), serializer);
    }
};

}  // namespace

void FrontEnd::parseNormalize(
        const Model::Ptr& model,
        const ie::CNNLayerPtr& layer,
        const DataVector& inputs,
        const DataVector& outputs) {
    IE_ASSERT(inputs.size() == 1);
    IE_ASSERT(outputs.size() == 1);

    auto acrossSpatial = layer->GetParamAsInt("across_spatial", 0);
    auto channelShared = layer->GetParamAsInt("channel_shared", 0);
    float eps = layer->GetParamAsFloat("eps", 0.0f);

    auto weightsIt = layer->blobs.find("weights");
    if (weightsIt == layer->blobs.end()) {
        VPU_THROW_EXCEPTION << "Missing weights for " << layer->name << " layer";
    }

    auto weightsBlob = weightsIt->second;
    IE_ASSERT(weightsBlob != nullptr);

    auto output = outputs[0];

    auto scales = model->addConstData(
        layer->name + "@scales",
        DataDesc({weightsBlob->size()}),
        ieBlobContent(weightsBlob));

    auto stage = model->addNewStage<NormalizeStage>(
        layer->name,
        StageType::Normalize,
        layer,
        {inputs[0], scales},
        outputs);

    stage->attrs().set<bool>("acrossSpatial", acrossSpatial);
    stage->attrs().set<bool>("channelShared", channelShared);
    stage->attrs().set<float>("eps", eps);
}

}  // namespace vpu
