/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


// Classes for specifying the part of the quadtree for which data should
// be kept during a cut.
//
// Adapted from code in partialglobebuilder.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_CUTSPEC_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_CUTSPEC_H_

#include <string>
#include <vector>
#include "common/base/macros.h"

namespace fusion_portableglobe {

/**
 * Class for determining if a quadtree node is one of the high resolution
 * nodes to be saved, or whether it should be pruned away. The nodes
 * can be used to define "strict" or "encompassing" trees.
 * "Strict" trees explicitly define each node of a tree. They are used
 * to create an efficient in-memory representation of available data,
 * equivalent to a single channel quadtree.
 * "Encompassing" trees define a set of leaves. The leaves indicate nodes
 * and all of their descendents that are to be saved in a Cut.
 */
class HiresNode {
 public:
  /**
   * Empty contructor is only used for the root of the tree.
   */
  HiresNode() : is_leaf(false) { }

  explicit HiresNode(std::string path);

  ~HiresNode();

  /**
   * Returns child node for node nums 0-3. If there is no child node
   * for that node num, it returns null.
   */
  HiresNode* const GetChild(char node_num) const;

  /**
   * Adds node as a child of this node in the given node_num (0-3)
   * position.
   */
  void AddChild(char node_num, HiresNode* child);

  /**
   * Returns whether this node is a leaf (i.e. it has no
   * children).
   */
  bool IsLeaf() const { return is_leaf; }

  /**
   * Make node a leaf.
   */
  void SetLeaf(bool is_leaf_) { is_leaf = is_leaf_; }

  /**
   * Returns whether the given path specifies a node that is encompassed
   * by the tree for which this node is the root.
   */
  bool IsTreePath(const std::string& path) const;

  /**
   * Debugging routine for showing the content of the tree starting
   * from this node as the root.
   */
  void Show(const std::string& indent) const;

 private:
  static const size_t ZERO_CHAR = (size_t) '0';

  bool is_leaf;
  // If not a leaf, then it must have between 1 and 4 children (inclusive).
  std::vector<HiresNode*> children;

  /**
   * Returns index associated with the given path character.
   */
  size_t GetChildIndex(char node_num) const;

  DISALLOW_COPY_AND_ASSIGN(HiresNode);
};


/**
 * Class for maintaining a tree of HiresNodes.
 *
 * The leaf nodes are nodes that have been explicitly marked to be kept.
 * The paths to the leaf nodes consist of nodes at higher levels that
 * encompass the specified leaf node. The path encodes the expanding area
 * being represented that ensures that insets in specified hi-res areas
 * are visible at all levels of the globe.
 *
 * "Encompassing" trees are used to detect what should be kept in a cut.
 * The leaves of the encompassing tree indicate that data at an address
 * that is a subset of the leaf address or a descendent of the leaf address
 * should be kept.
 *
 * "Strict" trees explicitly indicate the addresses where data should
 * be kept. The strict tree will include all nodes down to the root.
 */
class HiresTree {
 public:
  explicit HiresTree()
      : tree_(new HiresNode)
        { }

  ~HiresTree() { delete tree_; }

  /**
   * Load hi-res nodes from a list in a file.
   */
  void LoadHiResQtNodeFile(const std::string& file_name);

  /**
   * Load hi-res nodes from an input stream.
   */
  void LoadHiResQtNodes(std::istream* fin);

  /**
   * Returns whether the given path specifies a node that is encompassed
   * by the tree; i.e. any node in the tree or a descendent of any node
   * in the tree. If the tree contains a node for the address "023321131",
   * then this method returns true for any address with that address
   * as its prefix.
   */
  bool IsTreePath(const std::string& path) const;

  /**
   * Returns whether the given path specifies a node that is
   * strictly in the tree. I.e. is the address of a node
   * within the tree.
   */
  bool IsInTree(const std::string& path) const;

  /**
   * Adds nodes for the given path that are not already in the tree.
   * Used for creating "strict" trees rather than "encompassing"
   * trees.
   */
  void AddNode(const std::string& path);

  /**
   * Debugging routine for showing the content of the tree.
   */
  void Show() const;

  /**
   * Returns the root node of the tree.
   */
  HiresNode* Root() const {
    return tree_;
  }

 private:
  HiresNode* tree_;

  void AddHiResQtNode(HiresNode* node, const std::string& path);
  bool TestTreePath(const std::string& path, bool includeLeafDescendents) const;
};


class CutSpec {
 public:
  CutSpec(const std::string& qt_node_file_name,
          std::uint16_t min_level,
          std::uint16_t default_level,
          std::uint16_t max_level);

  CutSpec(const std::string& qt_node_file_name,
          const std::string& exclusion_qt_node_file_name,
          std::uint16_t min_level,
          std::uint16_t default_level,
          std::uint16_t max_level);

  // KeepNode shoud be used on the quadtree, whereas KeepNode
  // AND ExcludeNode are used on the data packets with ExcludeNode
  // trumping KeepNode.

  // Returns whether node is in the cut.
  bool KeepNode(const std::string& qtpath) const;
  // Returns whether data in node should be excluded.
  bool ExcludeNode(const std::string& qtpath) const;

 private:
  // Level below which no assets are kept.
  std::uint16_t min_level_;
  // Level at or below which all assets are kept.
  std::uint16_t default_level_;
  // Level above which no assets are kept.
  std::uint16_t max_level_;
  // Tree identifying quadtree nodes to keep.
  HiresTree hires_tree_;
  // Whether we are excluding any nodes.
  bool is_exclusion_enabled_;
  // Tree identifying quadtree nodes never to keep above default resolution.
  HiresTree exclusion_tree_;
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_CUTSPEC_H_
