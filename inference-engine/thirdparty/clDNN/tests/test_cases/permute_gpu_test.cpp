/*
// Copyright (c) 2016 Intel Corporation
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
#include <gtest/gtest.h>
#include "api/CPP/memory.hpp"
#include <api/CPP/input_layout.hpp>
#include "api/CPP/permute.hpp"
#include "api/CPP/reorder.hpp"
#include <api/CPP/topology.hpp>
#include <api/CPP/network.hpp>
#include <api/CPP/engine.hpp>
#include "test_utils/test_utils.h"
#include <api/CPP/data.hpp>
#include <api/CPP/fully_connected.hpp>
#include <api/CPP/reshape.hpp>
#include <api/CPP/crop.hpp>
#include <cmath>
#include <gmock/gmock.h>
#include <limits>

using namespace cldnn;
using namespace tests;
using namespace testing;


TEST(permute_gpu_f32, output_ordering_test)
{
    const auto& engine = get_test_engine();


    std::vector<std::vector<int32_t>> input_tensors =
    {
        { 10, 5, 15, 2 },{ 2, 4, 6, 8 },{ 2, 2, 3, 2 },{ 9, 8, 7, 4 }
    };
    std::vector<std::vector<uint16_t>> permutations =
    {
        { 0, 1, 2, 3 }, //do nothing
    { 0, 1, 3, 2 }, //replace x with y
    { 1, 0, 3, 2 }, //replace b with f
    { 0, 2, 3, 1 }  //big permutation
    };
    std::vector<format> input_formats = { format::bfyx, format::yxfb };

    auto get_permutation = [&](const std::vector<int32_t>& inp1, const std::vector<uint16_t>& order)
    {
        EXPECT_EQ(inp1.size(), order.size());
        std::vector<int32_t> output;
        for (auto const& o : order)
        {
            output.push_back(inp1.at(o));
        }
        return output;
    };

    for (auto const& fr : input_formats)
    {
        for (auto const& inp_t : input_tensors)
        {
            for (auto const& perm : permutations)
            {

                auto input = memory::allocate(engine, { data_types::f32, fr, tensor(inp_t) });
                topology topology(
                    input_layout("input", input.get_layout()),
                    permute("permute", "input", perm));

                network network(engine, topology);
                network.set_input_data("input", input);
                auto outputs = network.execute();
                auto output = outputs.at("permute");
                auto output_mem = output.get_memory();
                EXPECT_EQ(outputs.size(), size_t(1));
                auto ref_tensor = get_permutation(inp_t, perm);
                auto out_tensor = output_mem.get_layout().size;
                EXPECT_EQ(out_tensor.batch[0], ref_tensor[0]);
                EXPECT_EQ(out_tensor.feature[0], ref_tensor[1]);
                EXPECT_EQ(out_tensor.spatial[0], ref_tensor[2]);
                EXPECT_EQ(out_tensor.spatial[1], ref_tensor[3]);
            }
        }
    }
}

TEST(permute_gpu_f32, basic_bfyx_permute_0_1_2_3)
{
    //  Input               : bfyx:2x2x3x2
    //  Permute order       : { 0,1,3,2 }
    //
    //  Input:
    //  f0: b0:  1    2   -15  b1:   0    0     -15
    //  f0: b0:  3    4   -15  b1:   0.5 -0.5   -15
    //  f1: b0:  5    6   -15  b1:   1.5  5.2   -15
    //  f1: b0:  7    8   -15  b1:   12   8     -15
    //
    //  Output = input


    const auto& engine = get_test_engine();

    auto input = memory::allocate(engine, { data_types::f32, format::bfyx,{ 2, 2, 3, 2 } });

    std::vector<float> values =
    {
        1.0f,  2.0f, -15.f,
        3.0f,  4.0f, -15.f,

        5.0f,  6.0f, -15.f,
        7.0f,  8.0f, -15.f,

        0.0f,  0.0f, -15.f,
        0.5f, -0.5f, -15.f,

        1.5f,  5.2f, -15.f,
        12.0f, 8.0f, -15.f
    };

    set_values(input, values);

    topology topology(
        input_layout("input", input.get_layout()),
        permute("permute", "input", { 0, 1, 2, 3 }));

    network network(engine, topology);
    network.set_input_data("input", input);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "permute");

    auto output = outputs.begin()->second.get_memory();


    auto output_ptr = output.pointer<float>();
    for (int i = 0; i < 24; i++)
    {
        EXPECT_FLOAT_EQ(values[i], output_ptr[i]);
    }

}

TEST(permute_gpu_f32, basic_bfyx_permute_0_1_3_2)
{
    //  Input               : bfyx:2x2x3x2
    //  Permute order       : { 0,1,3,2 }
    //
    //  Input:
    //  f0: b0:  1    2   -15  b1:   0    0     -15
    //  f0: b0:  3    4   -15  b1:   0.5 -0.5   -15
    //  f1: b0:  5    6   -15  b1:   1.5  5.2   -15
    //  f1: b0:  7    8   -15  b1:   12   8     -15
    //
    //  Output
    //  f0: b0:  1    3  b1:   0    0.5
    //  f0: b0:  2    4  b1:   0    -0.5
    //  f0: b0:  -15 -15 b1:   -15  -15
    //  f1: b0:  5    7  b1:   1.5  12
    //  f1: b0:  6    8  b1:   5.2   8
    //  f1: b0:  -15 -15 b1:   -15   -15
    //

    const auto& engine = get_test_engine();

    auto input = memory::allocate(engine, { data_types::f32, format::bfyx,{ 2, 2, 3, 2 } });

    set_values(input, {
        1.0f,  2.0f, -15.f,
        3.0f,  4.0f, -15.f,

        5.0f,  6.0f, -15.f,
        7.0f,  8.0f, -15.f,

        0.0f,  0.0f, -15.f,
        0.5f, -0.5f, -15.f,

        1.5f,  5.2f, -15.f,
        12.0f, 8.0f, -15.f,
        });

    topology topology(
        input_layout("input", input.get_layout()),
        permute("permute", "input", { 0, 1, 3, 2 }));

    network network(engine, topology);
    network.set_input_data("input", input);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "permute");

    auto output = outputs.begin()->second.get_memory();

    float answers[24] = {
        1.0f,  3.0f,
        2.0f,  4.0f,
        -15.0f,  -15.0f,

        5.0f,  7.0f,
        6.0f,  8.0f,
        -15.0f,  -15.0f,

        0.0f,  0.5f,
        0.0f, -0.5f,
        -15.0f,  -15.0f,

        1.5f,  12.0f,
        5.2f, 8.0f,
        -15.0f,  -15.0f,
    };

    auto output_ptr = output.pointer<float>();
    for (int i = 0; i < 24; i++)
    {
        EXPECT_FLOAT_EQ(answers[i], output_ptr[i]);
    }

}

TEST(permute_gpu_f32, basic_yxfb_permute_1_0_2_3)
{
    const auto& engine = get_test_engine();

    auto input_mem = memory::allocate(engine, { data_types::f32, format::yxfb,{ 1, 100, 64, 1 } });

    tests::set_random_values<float>(input_mem);

    topology topology(
        input_layout("input", input_mem.get_layout()),
        permute("permute", "input", { 1, 0, 2, 3 }));

    network network(engine, topology);
    network.set_input_data("input", input_mem);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "permute");

    auto output = outputs.begin()->second.get_memory();

    auto output_ptr = output.pointer<float>();
    auto input_ptr = input_mem.pointer<float>();
    for (int i = 0; i < 6400; i++)
    {
        EXPECT_FLOAT_EQ(input_ptr[i], output_ptr[i]);
    }

}

TEST(permute_gpu_f32, basic_bfyx_permute_0_1_3_2_input_padding)
{
    //  Input               : bfyx:2x2x3x2
    //  Permute order       : { 0,1,3,2 }
    //  Input padding       : 2x1
    //
    //  Input:
    //  f0: b0:  1    2   -15  b1:   0    0     -15
    //  f0: b0:  3    4   -15  b1:   0.5 -0.5   -15
    //  f1: b0:  5    6   -15  b1:   1.5  5.2   -15
    //  f1: b0:  7    8   -15  b1:   12   8     -15
    //
    //  Input:
    //  f0: b0:  1    3  b1:   0    0.5
    //  f0: b0:  2    4  b1:   0    -0.5
    //  f0: b0:  -15 -15 b1:   -15  -15
    //  f1: b0:  5    7  b1:   1.5  12
    //  f1: b0:  6    8  b1:   5.2   8
    //  f1: b0:  -15 -15 b1:   -15   -15
    //

    const auto& engine = get_test_engine();

    auto input = memory::allocate(engine, { data_types::f32, format::bfyx,{ 2, 2, 3, 2 } });

    set_values(input, {
        1.0f,  2.0f, -15.f,
        3.0f,  4.0f, -15.f,

        5.0f,  6.0f, -15.f,
        7.0f,  8.0f, -15.f,

        0.0f,  0.0f, -15.f,
        0.5f, -0.5f, -15.f,

        1.5f,  5.2f, -15.f,
        12.0f, 8.0f, -15.f,
        });

    topology topology(
        input_layout("input", input.get_layout()),
        reorder("reorder", "input", input.get_layout().with_padding(padding{ { 0, 0, 2, 1 }, 0 })),
        permute("permute", "reorder", { 0, 1, 3, 2 }));

    network network(engine, topology);
    network.set_input_data("input", input);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "permute");

    auto output = outputs.begin()->second.get_memory();

    float answers[24] = {
        1.0f,  3.0f,
        2.0f,  4.0f,
        -15.0f,  -15.0f,

        5.0f,  7.0f,
        6.0f,  8.0f,
        -15.0f,  -15.0f,

        0.0f,  0.5f,
        0.0f, -0.5f,
        -15.0f,  -15.0f,

        1.5f,  12.0f,
        5.2f, 8.0f,
        -15.0f,  -15.0f,
    };

    auto output_ptr = output.pointer<float>();
    for (int i = 0; i < 24; i++)
    {
        EXPECT_FLOAT_EQ(answers[i], output_ptr[i]);
    }

}

TEST(permute_gpu_f32, basic_yxfb_permute_batch_with_feature)
{
    //  Input               : yxfb:8x2x1x1
    //  Permute order       : { 1, 0, 2, 3 }
    //  Output              : yxfb:2x8x1x1

    const auto& engine = get_test_engine();

    auto input = memory::allocate(engine, { data_types::f32, format::yxfb,{ 8, 2, 1, 1 } });

    set_values(input, {
        //b0 - b7 for f=0
        1.f, 0.f, 5.f, 1.5f, 2.f, 0.f, 6.f, 5.2f,

        //b0 - b7 for f=1
        3.f, 0.5f, 7.f, 12.f, 4.f, -0.5f, 8.f, 8.f
        });

    topology topology(
        input_layout("input", input.get_layout()),
        permute("permute", "input", { 1, 0, 2, 3 }));

    network network(engine, topology);
    network.set_input_data("input", input);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "permute");

    auto output = outputs.begin()->second.get_memory();
    auto out_tensor = output.get_layout().size;
    EXPECT_EQ(out_tensor.batch[0], 2);
    EXPECT_EQ(out_tensor.feature[0], 8);
    EXPECT_EQ(out_tensor.spatial[0], 1);
    EXPECT_EQ(out_tensor.spatial[1], 1);

    float answers[16] = {
        1.0f, 3.0f,
        0.0f, 0.5f,
        5.f, 7.f,
        1.5f, 12.f,
        2.f, 4.f,
        0.f, -0.5f,
        6.f, 8.f,
        5.2f, 8.f
    };

    auto output_ptr = output.pointer<float>();
    for (int i = 0; i < 16; i++)
    {
        EXPECT_FLOAT_EQ(answers[i], output_ptr[i]);
    }

}

TEST(permute_gpu_f32, basic_bfyx_permute_batch_with_feature)
{
    //  Input               : yxfb:8x2x1x1
    //  Permute order       : { 1, 0, 2, 3 }
    //  Output              : yxfb:2x8x1x1

    const auto& engine = get_test_engine();

    auto input = memory::allocate(engine, { data_types::f32, format::bfyx,{ 2, 8, 1, 1 } });

    set_values(input, {
        //f0 - f7 for b=0
        1.f, 0.f, 5.f, 1.5f, 2.f, 0.f, 6.f, 5.2f,

        //f0 - f7 for b=1
        3.f, 0.5f, 7.f, 12.f, 4.f, -0.5f, 8.f, 8.f
        });

    topology topology(
        input_layout("input", input.get_layout()),
        permute("permute", "input", { 1, 0, 2, 3 }));

    network network(engine, topology);
    network.set_input_data("input", input);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "permute");

    auto output = outputs.begin()->second.get_memory();
    auto out_tensor = output.get_layout().size;
    EXPECT_EQ(out_tensor.batch[0], 8);
    EXPECT_EQ(out_tensor.feature[0], 2);
    EXPECT_EQ(out_tensor.spatial[0], 1);
    EXPECT_EQ(out_tensor.spatial[1], 1);

    float answers[16] = {
        1.0f, 3.0f,
        0.0f, 0.5f,
        5.f, 7.f,
        1.5f, 12.f,
        2.f, 4.f,
        0.f, -0.5f,
        6.f, 8.f,
        5.2f, 8.f
    };

    auto output_ptr = output.pointer<float>();
    for (int i = 0; i < 16; i++)
    {
        EXPECT_FLOAT_EQ(answers[i], output_ptr[i]);
    }

}

template<data_types DType>
void permute_test_with_reorder()
{
    const auto& engine = get_test_engine();

    auto input = memory::allocate(engine, { data_types::f32, format::bfyx,{ 2, 2, 3, 2 } });

    set_values(input, {
        1.0f,  2.0f, -15.f,
        3.0f,  4.0f, -15.f,

        5.0f,  6.0f, -15.f,
        7.0f,  8.0f, -15.f,

        0.0f,  0.0f, -15.f,
        0.0f,  0.0f, -15.f,

        1.0f,  5.0f, -15.f,
        12.0f, 8.0f, -15.f,
        });

    topology topology(
        input_layout("input", input.get_layout()),
        reorder("reorder", "input", { DType, format::bfyx,{ 2, 2, 3, 2 } }),
        permute("permute", "reorder", { 0, 1, 3, 2 }),
        reorder("reorder_out", "permute", { data_types::f32, format::bfyx,{ 2, 2, 3, 2 } }));

    network network(engine, topology);
    network.set_input_data("input", input);

    auto outputs = network.execute();
    ASSERT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "reorder_out");

    auto output = outputs.begin()->second.get_memory();

    float answers[24] = {
        1.0f,  3.0f,
        2.0f,  4.0f,
        -15.0f,  -15.0f,

        5.0f,  7.0f,
        6.0f,  8.0f,
        -15.0f,  -15.0f,

        0.0f,  0.0f,
        0.0f,  0.0f,
        -15.0f,  -15.0f,

        1.0f,  12.0f,
        5.0f, 8.0f,
        -15.0f,  -15.0f,
    };

    auto output_ptr = output.pointer<float>();
    for (int i = 0; i < 24; i++)
    {
        EXPECT_FLOAT_EQ(answers[i], output_ptr[i]);
    }
}

TEST(permute_gpu_i8, basic_bfyx_permute_0_1_3_2) {
    permute_test_with_reorder<data_types::i8>();
}

TEST(permute_gpu_i32, basic_bfyx_permute_0_1_3_2) {
    permute_test_with_reorder<data_types::i32>();
}

TEST(permute_gpu_i64, basic_bfyx_permute_0_1_3_2) {
    permute_test_with_reorder<data_types::i64>();
}

TEST(fc_permute_crop_gpu, basic_permute_yxfb)
{
    const auto& engine = get_test_engine();

    auto input_mem = memory::allocate(engine, { data_types::f32, format::yxfb,{ 1, 5, 1, 512 } });

    //Topolgy creates permute which "repalces" the batch with the feature.
    topology topology(
        input_layout("input", input_mem.get_layout()),  // yxfb {1, 5, 1, 512 }}
        permute("permute", "input", { 1, 0, 2, 3 })  // yxfb {5, 1, 1, 512}  --- without permute fix yxfb {1, 5, 512, 1}
    );

    network network(engine, topology);
    network.set_input_data("input", input_mem);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "permute");

    auto output = outputs.begin()->second.get_memory();
    auto out_tensor = output.get_layout().size;
    EXPECT_EQ(out_tensor.batch[0], 5);
    EXPECT_EQ(out_tensor.feature[0], 1);
    EXPECT_EQ(out_tensor.spatial[0], 1);
    EXPECT_EQ(out_tensor.spatial[1], 512);
    EXPECT_EQ(output.get_layout().format, cldnn::format::yxfb);
}

TEST(fc_permute_crop_gpu, basic_0)
{

    const auto& engine = get_test_engine();

    auto input_mem = memory::allocate(engine, { data_types::f32, format::bfyx,{ 5, 11264, 1, 1 } });
    auto weights_mem = memory::allocate(engine, { data_types::f32, format::bfyx,{ 512, 11264, 1, 1 } });
    auto bias_mem = memory::allocate(engine, { data_types::f32, format::bfyx,{ 1, 1, 512, 1 } });

    topology topology(
        input_layout("input", input_mem.get_layout()),                   // bfyx {5, 11264, 1, 1}}
        data("weights", weights_mem),
        data("bias", bias_mem),
        fully_connected("fully_connected", "input", "weights", "bias"),  // yxfb {5, 512, 1, 1}
        reshape("reshape", "fully_connected", { 1, 5, 1, 512 }),           // yxfb {1, 5, 1, 512}
        permute("permute", "reshape", { 1, 0, 2, 3 }),                     // yxfb {5, 1, 1, 512}        --- without permute fix yxfb {1, 5, 512, 1}
        crop("crop", "permute", { 1, 1, 1, 512 }, { 4, 0, 0 ,0 })           // without permute fix it will fail "Tensor pitches didn't set correctly"
    );

    network network(engine, topology);
    network.set_input_data("input", input_mem);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "crop");

    auto output = outputs.begin()->second.get_memory();
    auto out_tensor = output.get_layout().size;
    EXPECT_EQ(out_tensor.batch[0], 1);
    EXPECT_EQ(out_tensor.feature[0], 1);
    EXPECT_EQ(out_tensor.spatial[0], 1);
    EXPECT_EQ(out_tensor.spatial[1], 512);
    EXPECT_EQ(output.get_layout().format, cldnn::format::yxfb);
}

TEST(fc_permute_gpu, basic_permute_bfyx)
{
    const auto& engine = get_test_engine();

    auto input_mem = memory::allocate(engine, { data_types::f32, format::bfyx,{ 1, 5, 1, 256 } });

    tests::set_random_values<float>(input_mem);

    //Topolgy creates permute which "repalces" the batch with the feature.
    topology topology(
        input_layout("input", input_mem.get_layout()),
        permute("permute", "input", { 1, 0, 2, 3 })
    );

    network network(engine, topology);
    network.set_input_data("input", input_mem);

    auto outputs = network.execute();
    EXPECT_EQ(outputs.size(), size_t(1));
    EXPECT_EQ(outputs.begin()->first, "permute");

    auto output = outputs.begin()->second.get_memory();
    auto out_tensor = output.get_layout().size;
    EXPECT_EQ(out_tensor.batch[0], 5);
    EXPECT_EQ(out_tensor.feature[0], 1);
    EXPECT_EQ(out_tensor.spatial[0], 1);
    EXPECT_EQ(out_tensor.spatial[1], 256);
    EXPECT_EQ(output.get_layout().format, cldnn::format::bfyx);

    auto input_ptr = input_mem.pointer<float>();
    auto output_ptr = output.pointer<float>();
    for (int i = 0; i < 5 * 256; i++)
        EXPECT_NEAR(input_ptr[i], output_ptr[i], 1e-3f);
}
