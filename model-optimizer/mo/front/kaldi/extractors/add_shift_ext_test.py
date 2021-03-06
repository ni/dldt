"""
 Copyright (c) 2018-2019 Intel Corporation

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
"""

import numpy as np

from mo.front.kaldi.extractors.add_shift_ext import AddShiftFrontExtractor
from mo.front.kaldi.extractors.common_ext_test import KaldiFrontExtractorTest
from mo.front.kaldi.loader.utils_test import TestKaldiUtilsLoading
from mo.ops.op import Op
from mo.ops.scale_shift import ScaleShiftOp


class AddShiftFrontExtractorTest(KaldiFrontExtractorTest):
    @classmethod
    def register_op(cls):
        Op.registered_ops['ScaleShift'] = ScaleShiftOp

    @classmethod
    def create_pb_for_test_node(cls):
        input_shape = cls.test_node.in_node().shape
        pb = cls.write_tag_with_value('<LearnRateCoef>', 0)
        pb += cls.write_tag_with_value('FV', input_shape[1])
        for i in np.zeros(input_shape[1], dtype=np.uint32):
            pb += TestKaldiUtilsLoading.pack_value(i, TestKaldiUtilsLoading.uint32_fmt)
        cls.test_node['parameters'] = TestKaldiUtilsLoading.bytesio_from(pb)
        AddShiftFrontExtractor.extract(cls.test_node)

    def test_assertion(self):
        self.assertRaises(AttributeError, AddShiftFrontExtractor.extract, None)

    def test_extracted_shapes_add_shift(self):
        weights = self.test_node.weights
        biases = self.test_node.biases
        weights_shape = weights.shape[0]
        self.assertEqual(self.test_node.in_node().shape[1], weights_shape)
        self.assertEqual(biases.shape[0], weights_shape)

    def test_extracted_blobs_add_shift(self):
        weights = self.test_node.weights
        biases = self.test_node.biases
        self.assertTrue(np.array_equal(weights, np.ones(weights.shape)))
        self.assertTrue(np.array_equal(biases, np.zeros(biases.shape)))
        self.assertTrue(self.test_node.bias_term)
