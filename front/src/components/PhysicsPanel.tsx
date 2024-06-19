/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import React from "react";
import { TreeView2 } from "./common/TreeView";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const treeData = [
  {
    id: "14",
    name: "Kernel",
    descr:
      "A very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
  },
  {
    id: "1",
    name: "Parent 1",
    descr:
      "A very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
    children: [
      {
        id: "2",
        value: "123",
        name: "Child 1-1",
      },
      {
        id: "3",
        name: "Child 1-2",
        descr:
          "A very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
        children: [
          {
            id: "4",
            name: "Child 1-2-1",
            descr:
              "A very very very **very** very very very very very very very very very very very very very very very very very very very very very very very long description.",
          },
        ],
      },
    ],
  },
  {
    id: "5",
    name: "Parent 2",
    descr:
      "A very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
    children: [
      {
        id: "6",
        name: "Child 2-1",
        descr:
          "A very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
      },
    ],
  },
  {
    id: "7",
    name: "Parent 3",
    descr:
      "A very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
    children: [
      {
        id: "8",
        name: "Child 3-1",
        descr:
          "A very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
      },
      {
        id: "9",
        name: "Child 3-2",
        descr:
          "A very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
        children: [
          {
            id: "10",
            name: "Child 3-2-1",
            descr:
              "A very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
            children: [
              {
                id: "11",
                name: "Child 3-2-1-1",
                descr:
                  "A very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long description.",
              },
            ],
          },
        ],
      },
    ],
  },
];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const PhysicsPanel: React.FC = () => {
  return <TreeView2 nodes={treeData} />;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
