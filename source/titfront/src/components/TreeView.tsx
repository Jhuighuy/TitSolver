/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

import React, { useState } from "react";
import { FiChevronDown, FiChevronRight, FiDroplet } from "react-icons/fi";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

interface TreeNode {
  name: string;
  descr?: string;
  value?: string;
  parent?: TreeNode;
  children?: TreeNode[];
}

export const TreeView = ({ nodes }: { nodes: TreeNode[] }) => {
  const [expandedNodes, setExpandedNodes] = useState<Set<TreeNode>>(new Set());

  const showNode = (node: TreeNode) => {
    setExpandedNodes((prevExpandedNodes) => {
      const newExpandedNodes = new Set(prevExpandedNodes);
      newExpandedNodes.add(node);
      return newExpandedNodes;
    });
  };

  const hideNode = (node: TreeNode) => {
    setExpandedNodes((prevExpandedNodes) => {
      const newExpandedNodes = new Set(prevExpandedNodes);
      newExpandedNodes.delete(node);
      return newExpandedNodes;
    });
  };

  const toggleNode = (node: TreeNode) => {
    setExpandedNodes((prevExpandedNodes) => {
      const newExpandedNodes = new Set(prevExpandedNodes);
      if (newExpandedNodes.has(node)) {
        newExpandedNodes.delete(node);
      } else {
        newExpandedNodes.add(node);
      }
      return newExpandedNodes;
    });
  };

  const [selectedNode, setSelectedNode] = useState<TreeNode | null>(null);

  const flattenVisibleTree = (
    depth: number,
    nodes: TreeNode[]
  ): [number, TreeNode][] => {
    return nodes.flatMap((node) => [
      [depth, node],
      ...(expandedNodes.has(node) && node.children
        ? flattenVisibleTree(depth + 1, node.children)
        : []),
    ]);
  };
  const visibleNodes = flattenVisibleTree(/*depth=*/ 0, nodes);

  const handleKeyDown = (event: React.KeyboardEvent) => {
    const node = selectedNode || nodes[0];
    if (event.key === "ArrowDown") {
      event.preventDefault();
      const index = visibleNodes.findIndex(([, n]) => n === node);
      const nextIndex = (index + 1) % visibleNodes.length;
      const nextNode = visibleNodes[nextIndex][1];
      setSelectedNode(nextNode);
    } else if (event.key === "ArrowUp") {
      event.preventDefault();
      const index = visibleNodes.findIndex(([, n]) => n === node);
      const prevIndex = (index - 1 + visibleNodes.length) % visibleNodes.length;
      const prevNode = visibleNodes[prevIndex][1];
      setSelectedNode(prevNode);
    } else if (event.key === "ArrowRight") {
      event.preventDefault();
      if (node.children) showNode(node);
    } else if (event.key === "ArrowLeft") {
      event.preventDefault();
      if (node.children) hideNode(node);
    } else if (event.key === "Enter") {
      event.preventDefault();
      if (node.children) {
        if (expandedNodes.has(node)) hideNode(node);
        else showNode(node);
      }
    }
  };

  const nodeIcon = (node: TreeNode) => {
    if (!node.children) {
      return <FiDroplet />;
    }
    return expandedNodes.has(node) ? <FiChevronDown /> : <FiChevronRight />;
  };

  return (
    <div tabIndex={-1} onKeyDown={handleKeyDown} className="outline-none">
      {visibleNodes.map(([depth, node]) => (
        <div
          key={node.name}
          className={`flex rounded hover:bg-blue-900 focus:outline ${
            node === selectedNode ? "bg-gray-900" : ""
          }`}
        >
          {[...Array<number>(depth)].map(() => (
            <div key={depth} className="ml-2.5 mr-0 w-0.5 bg-gray-500" />
          ))}
          <div className="flex-grow">
            <div
              className="flex items-center px-1.5"
              onClick={() => (toggleNode(node), setSelectedNode(node))}
            >
              <span className="mr-1">{nodeIcon(node)}</span>
              <span className="cursor-default">{node.name}</span>
            </div>
          </div>
        </div>
      ))}
    </div>
  );
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const MockData = [
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
