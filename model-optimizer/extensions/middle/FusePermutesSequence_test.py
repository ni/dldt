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
from argparse import Namespace

from extensions.middle.FusePermutesSequence import FusePermutesSequence
from mo.middle.passes.eliminate_test import build_graph
from mo.middle.passes.fusing.fuse_linear_ops_test import compare_graphs

# The dictionary with nodes attributes used to build various graphs. A key is the name of the node and the value is the
# dictionary with node attributes.
nodes_attributes = {
    'placeholder_1': {'name': 'placeholder_1', 'value': None, 'shape': None, 'type': 'Placeholder', 'kind': 'op',
                      'op': 'Placeholder'},
    'placeholder_1_data': {'name': 'placeholder_1_data', 'value': None, 'shape': None, 'kind': 'data',
                           'data_type': None},
    # Permute layers
    'permute_1': {'type': 'Permute', 'value': None, 'kind': 'op', 'op': 'Permute'},
    'permute_1_data': {'value': None, 'shape': None, 'kind': 'data'},

    'permute_2': {'type': 'Permute', 'value': None, 'kind': 'op', 'op': 'Permute'},
    'permute_2_data': {'value': None, 'shape': None, 'kind': 'data'},

    'permute_3': {'type': 'Permute', 'value': None, 'kind': 'op', 'op': 'Permute'},
    'permute_3_data': {'value': None, 'shape': None, 'kind': 'data'},
    'op_output': { 'op': 'OpOutput', 'kind': 'op'}
}


class FusePermutesSequenceTest(unittest.TestCase):
    def test_1(self):
        #
        #    NHWC         NCHW           NHWC
        #   Input->DATA->Permute->DATA->Permute->DATA  => Input->DATA
        #
        graph = build_graph(nodes_attributes,
                            [('placeholder_1', 'placeholder_1_data'),
                             ('placeholder_1_data', 'permute_1'),
                             ('permute_1', 'permute_1_data'),
                             ('permute_1_data', 'permute_2'),
                             ('permute_2', 'permute_2_data'),
                             ('permute_2_data', 'op_output')
                             ],
                            {'placeholder_1_data': {'shape': np.array([1, 227, 227, 3])},

                             'permute_1': {'order': np.array([0, 3, 1, 2])},
                             'permute_1_data': {'shape': np.array([1, 3, 227, 227])},

                             'permute_2': {'order': np.array([0, 2, 3, 1])},
                             'permute_2_data': {'shape': np.array([1, 227, 227, 3])},
                             }, nodes_with_edges_only=True)

        graph.graph['layout'] = 'NHWC'
        graph.graph['cmd_params'] = Namespace(keep_shape_ops=False)

        graph_ref = build_graph(nodes_attributes,
                                [('placeholder_1', 'placeholder_1_data'),
                                 ('placeholder_1_data', 'op_output')
                                 ],
                                {'placeholder_1_data': {'shape': np.array([1, 227, 227, 3])}},
                                nodes_with_edges_only=True)

        pattern = FusePermutesSequence()
        pattern.find_and_replace_pattern(graph)

        (flag, resp) = compare_graphs(graph, graph_ref, 'placeholder_1_data', check_op_attrs=True)
        self.assertTrue(flag, resp)

    def test_2(self):
        #
        #   Input->DATA->Permute->DATA->Permute->DATA  => Input->DATA->Permute->DATA
        #
        graph = build_graph(nodes_attributes,
                            [('placeholder_1', 'placeholder_1_data'),
                             ('placeholder_1_data', 'permute_1'),
                             ('permute_1', 'permute_1_data'),
                             ('permute_1_data', 'permute_2'),
                             ('permute_2', 'permute_2_data'),
                             ('permute_2_data', 'op_output')
                             ],
                            {'placeholder_1_data': {'shape': np.array([1, 227, 227, 3])},

                             'permute_1': {'order': np.array([0, 3, 1, 2])},
                             'permute_1_data': {'shape': np.array([1, 3, 227, 227])},

                             'permute_2': {'order': np.array([0, 1, 2, 3])},
                             'permute_2_data': {'shape': np.array([1, 3, 227, 227])},
                             }, nodes_with_edges_only=True)

        graph.graph['layout'] = 'NHWC'
        graph.graph['cmd_params'] = Namespace(keep_shape_ops=False)

        graph_ref = build_graph(nodes_attributes,
                                [('placeholder_1', 'placeholder_1_data'),
                                 ('placeholder_1_data', 'permute_1'),
                                 ('permute_1', 'permute_1_data'),
                                 ('permute_1_data', 'op_output')
                                 ],
                                {'placeholder_1_data': {'shape': np.array([1, 227, 227, 3])},
                                 'permute_1': {'order': np.array([0, 3, 1, 2])},
                                 'permute_1_data': {'shape': np.array([1, 3, 227, 227])},
                                 }, nodes_with_edges_only=True)

        pattern = FusePermutesSequence()
        pattern.find_and_replace_pattern(graph)

        (flag, resp) = compare_graphs(graph, graph_ref, 'placeholder_1_data', check_op_attrs=True)
        self.assertTrue(flag, resp)


if __name__ == '__main__':
    unittest.main()
