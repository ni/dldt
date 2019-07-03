// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vpu/model/stage.hpp>

namespace vpu {

class PostOpStage : public StageNode {
protected:
    DataMap<float> propagateScaleFactorsImpl(
            const DataMap<float>& inputScales,
            ScalePropagationStep step) override;

    DataMap<DimsOrder> propagateDataOrderImpl() const override;

    DataMap<StridesRequirement> getDataStridesRequirementsImpl() const override;

    void finalizeDataLayoutImpl() override;

    DataMap<BatchSupport> getBatchSupportInfoImpl() const override;

    StageSHAVEsRequirements getSHAVEsRequirementsImpl() const override;

    void finalCheckImpl() const override;

    void serializeDataImpl(BlobSerializer& serializer) const override;
};

}  // namespace vpu
