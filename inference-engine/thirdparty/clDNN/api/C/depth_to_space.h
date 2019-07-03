/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef DEPTH_TO_SPACE_H
#define DEPTH_TO_SPACE_H

#include "cldnn.h"


/// @addtogroup c_api C API
/// @{
/// @addtogroup c_topology Network Topology
/// @{
/// @addtogroup c_primitives Primitives
/// @{

#ifdef __cplusplus
extern "C" {
#endif

CLDNN_BEGIN_PRIMITIVE_DESC(depth_to_space)
/// @brief Size of spatial block in the output tensor. Should be >= 2.
size_t block_size;
CLDNN_END_PRIMITIVE_DESC(depth_to_space)

CLDNN_DECLARE_PRIMITIVE_TYPE_ID(depth_to_space);

#ifdef __cplusplus
}
#endif

/// @}
/// @}
/// @}
#endif // DEPTH_TO_SPACE_H
