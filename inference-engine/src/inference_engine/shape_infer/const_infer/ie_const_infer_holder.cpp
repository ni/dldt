// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#ifdef __INTEL_COMPILER
#pragma warning disable: 2586
#endif


#include "ie_const_infer_holder.hpp"
#include "ie_mul_const_infer.hpp"
#include "ie_add_const_infer.hpp"
#include "ie_div_const_infer.hpp"
#include "ie_const_const_infer.hpp"
#include "ie_shape_const_infer.hpp"
#include "ie_power_const_infer.hpp"
#include "ie_tile_const_infer.hpp"
#include "ie_reshape_const_infer.hpp"
#include "ie_gather_const_infer.hpp"
#include "ie_split_const_infer.hpp"
#include "ie_concat_const_infer.hpp"
#include "ie_in_place_const_infer.hpp"
#include "ie_strided_slice_const_infer.hpp"
#include "ie_fill_const_infer.hpp"
#include "ie_range_const_infer.hpp"
#include <list>
#include <memory>
#include <string>

namespace InferenceEngine {
namespace ShapeInfer {

ConstInferHolder::ImplsHolder::Ptr ConstInferHolder::GetImplsHolder() {
    static ImplsHolder::Ptr localHolder;
    if (localHolder == nullptr) {
        localHolder = std::make_shared<ImplsHolder>();
    }
    return localHolder;
}

void ConstInferHolder::AddImpl(const std::string& name, const IConstInferImpl::Ptr& impl) {
    GetImplsHolder()->list[name] = impl;
}

std::list<std::string> ConstInferHolder::getConstInferTypes() {
    std::list<std::string> types;
    auto& factories = GetImplsHolder()->list;
    for (const auto& factory : factories) {
        types.push_back(factory.first);
    }
    return types;
}

IConstInferImpl::Ptr ConstInferHolder::getConstInferImpl(const std::string& type) {
    auto& impls = ConstInferHolder::GetImplsHolder()->list;
    if (impls.find(type) != impls.end()) {
        return impls[type];
    }
    return nullptr;
}

REG_CONST_INFER_FOR_TYPE(MulConstInfer, Mul);
REG_CONST_INFER_FOR_TYPE(AddConstInfer, Add);
REG_CONST_INFER_FOR_TYPE(DivConstInfer, Div);
REG_CONST_INFER_FOR_TYPE(ShapeConstInfer, Shape);
REG_CONST_INFER_FOR_TYPE(ConstConstInfer, Const);
REG_CONST_INFER_FOR_TYPE(PowerConstInfer, Power);
REG_CONST_INFER_FOR_TYPE(TileConstInfer, Tile);
REG_CONST_INFER_FOR_TYPE(ReshapeConstInfer, Reshape);
REG_CONST_INFER_FOR_TYPE(GatherConstInfer, Gather);
REG_CONST_INFER_FOR_TYPE(SplitConstInfer, Split);
REG_CONST_INFER_FOR_TYPE(ConcatConstInfer, Concat);
REG_CONST_INFER_FOR_TYPE(InPlaceConstInfer, Unsqueeze);
REG_CONST_INFER_FOR_TYPE(InPlaceConstInfer, Squeeze);
REG_CONST_INFER_FOR_TYPE(StridedSliceConstInfer, StridedSlice);
REG_CONST_INFER_FOR_TYPE(FillConstInfer, Fill);
REG_CONST_INFER_FOR_TYPE(RangeConstInfer, Range);

}  // namespace ShapeInfer
}  // namespace InferenceEngine
