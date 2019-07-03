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

import unittest

import numpy as np

from extensions.middle.MulQuantizeFuse import MulQuantizeFuse
from mo.middle.passes.eliminate_test import build_graph
from mo.middle.passes.fusing.fuse_linear_ops_test import compare_graphs

# The dictionary with nodes attributes used to build various graphs. A key is the name of the node and the value is the
# dictionary with node attributes.
nodes = {
    'x': {'type': 'Placeholder', 'kind': 'op', 'op': 'Placeholder'},
    'x_data': {'value': None, 'shape': np.array([1, 64, 56, 56]), 'kind': 'data'},

    'mul_const': {'op': 'Const', 'type': 'Const', 'kind': 'op'},
    'mul_const_data': {'value': np.array([]), 'shape': np.array([]), 'kind': 'data'},

    'mul': {'op': 'Mul', 'kind': 'op'},
    'mul_data': {'value': np.array([]), 'shape': np.array([]), 'kind': 'data'},

    'mi_i': {'op': 'Const', 'type': 'Const', 'kind': 'op'},
    'mi_i_data': {'value': np.array([-10]), 'shape': np.array([]), 'kind': 'data'},

    'ma_i': {'op': 'Const', 'type': 'Const', 'kind': 'op'},
    'ma_i_data': {'value': np.array([10]), 'shape': np.array([]), 'kind': 'data'},

    'mi_o': {'op': 'Const', 'type': 'Const', 'kind': 'op'},
    'mi_o_data': {'value': np.array([]), 'shape': np.array([]), 'kind': 'data'},

    'ma_o': {'op': 'Const', 'type': 'Const', 'kind': 'op'},
    'ma_o_data': {'value': np.array([]), 'shape': np.array([]), 'kind': 'data'},

    'quantize': {'type': 'Quantize', 'value': None, 'kind': 'op', 'op': 'Quantize', 'levels': 2},
    'quantize_data': {'value': None, 'shape': np.array([1, 64, 56, 56]), 'kind': 'data'},

    'output': {'op': 'OpOutput', 'kind': 'op'},
}

edges = [
    ('x', 'x_data'),
    ('mul_const', 'mul_const_data'),
    ('mul', 'mul_data'),
    ('mi_i', 'mi_i_data'),
    ('ma_i', 'ma_i_data'),
    ('mi_o', 'mi_o_data'),
    ('ma_o', 'ma_o_data'),
    ('quantize', 'quantize_data'),
    ('quantize_data', 'output'),

    ('x_data', 'mul', {'in': 0}),
    ('mul_const_data', 'mul', {'in': 1}),

    ('mul_data', 'quantize', {'in': 0}),
    ('mi_i_data', 'quantize', {'in': 1}),
    ('ma_i_data', 'quantize', {'in': 2}),
    ('mi_o_data', 'quantize', {'in': 3}),
    ('ma_o_data', 'quantize', {'in': 4}),
]

edges_ref = [
    ('x', 'x_data'),
    ('mi_i', 'mi_i_data'),
    ('ma_i', 'ma_i_data'),
    ('mi_o', 'mi_o_data'),
    ('ma_o', 'ma_o_data'),
    ('quantize', 'quantize_data'),
    ('quantize_data', 'output'),

    ('x_data', 'quantize', {'in': 0}),
    ('mi_i_data', 'quantize', {'in': 1}),
    ('ma_i_data', 'quantize', {'in': 2}),
    ('mi_o_data', 'quantize', {'in': 3}),
    ('ma_o_data', 'quantize', {'in': 4}),
]


class MulQuantizeFuseTest(unittest.TestCase):
    def test_1(self):
        graph = build_graph(nodes, edges, {
            'mul_const_data': {'shape': np.array([3, 1, 1]), 'value': np.broadcast_to(np.array([1]), (3, 1, 1))},
            'quantize_data': {'shape': np.array([2, 3, 4, 4])},
            'mi_o_data': {'shape': np.array([1, 1, 1, 1]), 'value': np.broadcast_to(np.array([0]), (1, 1, 1, 1))},
            'ma_o_data': {'shape': np.array([1, 3, 1, 1]), 'value': np.broadcast_to(np.array([1]), (1, 3, 1, 1))},
        }, nodes_with_edges_only=True)
        graph.stage = 'middle'
        graph_ref = build_graph(nodes, edges_ref, {
            'quantize_data': {'shape': np.array([2, 3, 4, 4])},
            'mi_o_data': {'shape': np.array([1, 1, 1, 1]), 'value': np.broadcast_to(np.array([0]), (1, 1, 1, 1))},
            'ma_o_data': {'shape': np.array([1, 3, 1, 1]), 'value': np.broadcast_to(np.array([1]), (1, 3, 1, 1))},
            'mi_i_data': {'shape': np.array([3, 1, 1]), 'value': np.broadcast_to(np.array([-10]), (3, 1, 1))},
            'ma_i_data': {'shape': np.array([3, 1, 1]), 'value': np.broadcast_to(np.array([10]), (3, 1, 1))},
        }, nodes_with_edges_only=True)

        MulQuantizeFuse().find_and_replace_pattern(graph)

        (flag, resp) = compare_graphs(graph, graph_ref, 'output', check_op_attrs=True)

        self.assertTrue(flag, resp)

    def test_2(self):
        graph = build_graph(nodes, edges, {
            'mul_const_data': {'shape': np.array([1]), 'value': np.array([-1])},
            'quantize_data': {'shape': np.array([2, 3, 4, 4])},
            'mi_o_data': {'shape': np.array([1]), 'value': np.array([0])},
            'ma_o_data': {'shape': np.array([1]), 'value': np.array([1])},
        }, nodes_with_edges_only=True)
        graph.stage = 'middle'
        graph_ref = build_graph(nodes, edges_ref, {
            'quantize_data': {'shape': np.array([2, 3, 4, 4])},
            'mi_o_data': {'shape': np.array([1]), 'value': np.array([1])},
            'ma_o_data': {'shape': np.array([1]), 'value': np.array([0])},
            'mi_i_data': {'shape': np.array([1]), 'value': np.array([10])},
            'ma_i_data': {'shape': np.array([1]), 'value': np.array([-10])},
        }, nodes_with_edges_only=True)

        MulQuantizeFuse().find_and_replace_pattern(graph)

        (flag, resp) = compare_graphs(graph, graph_ref, 'output', check_op_attrs=True)

        self.assertTrue(flag, resp)

    def test_3(self):
        graph = build_graph(nodes, edges, {
            'mul_const_data': {'shape': np.array([3, 1, 1]), 'value': np.array([[[-1]], [[1]], [[-1]]])},
            'quantize_data': {'shape': np.array([2, 3, 4, 4])},
            'mi_o_data': {'shape': np.array([1, 1, 1, 1]), 'value': np.broadcast_to(np.array([0]), (1, 1, 1, 1))},
            'ma_o_data': {'shape': np.array([1, 1, 1, 1]), 'value': np.broadcast_to(np.array([1]), (1, 1, 1, 1))},
        }, nodes_with_edges_only=True)
        graph.stage = 'middle'
        graph_ref = build_graph(nodes, edges_ref, {
            'quantize_data': {'shape': np.array([2, 3, 4, 4])},
            'mi_o_data': {'shape': np.array([1, 3, 1, 1]), 'value': np.array([[[1]], [[0]], [[1]]])},
            'ma_o_data': {'shape': np.array([1, 3, 1, 1]), 'value': np.array([[[0]], [[1]], [[0]]])},
            'mi_i_data': {'shape': np.array([1, 3, 1, 1]), 'value': np.array([[[10]], [[-10]], [[10]]])},
            'ma_i_data': {'shape': np.array([1, 3, 1, 1]), 'value': np.array([[[-10]], [[10]], [[-10]]])},
        }, nodes_with_edges_only=True)

        MulQuantizeFuse().find_and_replace_pattern(graph)

        (flag, resp) = compare_graphs(graph, graph_ref, 'output', check_op_attrs=True)

        self.assertTrue(flag, resp)
