/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

export const TreeView2 = ({ nodes }: { nodes: TreeNode[] }) => {
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

  // Keyboard navigation.
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

  // Determine the icon to display for a node.
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
          className={`flex rounded hover:bg-blue-200 focus:outline ${
            node === selectedNode ? "bg-gray-300" : ""
          }`}
        >
          {[...Array<number>(depth)].map(() => (
            <div key={depth} className="ml-2.5 mr-0 w-0.5 bg-gray-200" />
          ))}
          <div className="flex-grow">
            <div
              className="flex items-center px-1.5 text-sm"
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
