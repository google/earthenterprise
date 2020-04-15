// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


// Classes for specifying the part of the quadtree for which data should
// be kept during a cut. Other than the coarse level thresholds, this is
// done by building a simple quadtree whose leaves indicate that the node
// each leaf represents should be kept, as should all of the nodes that
// are descendents of this node in the full quadtree (assuming they
// don't violate those coarse thresholds mentioned above).

#include "fusion/portableglobe/cutspec.h"
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include "common/notify.h"


namespace fusion_portableglobe {

/**
 * Constructor. If this is the last node in the path, mark it as
 * a leaf. Otherwise, recursively call the constructor to make
 * the next node in the path.
 */
HiresNode::HiresNode(std::string path) {
  if (path.empty()) {
    is_leaf = true;
  } else {
    is_leaf = false;
    AddChild(path[0], new HiresNode(path.substr(1)));
  }
}

/**
 * Destructor. Recursively deletes all children.
 */
HiresNode::~HiresNode() {
  if (!IsLeaf()) {
    if (children.size() == 0) {
      return;
    }

    for (int i = 0; i < 4; ++i) {
      if (children[i] != NULL) {
        delete children[i];
      }
    }
  }
}

/**
 * Returns pointer to child node for the given char
 * or returns NULL if there is no such child. The
 * node_num char shoud be '0', '1', '2' or '3'.
 */
HiresNode* const HiresNode::GetChild(char node_num) const {
  size_t idx = GetChildIndex(node_num);
  if (idx < children.size())
    return children[idx];

  return NULL;
}

/**
 * Adds given node as a child in the position specified by
 * node_num. The node_num char shoud be '0', '1', '2' or '3'.
 */
void HiresNode::AddChild(char node_num, HiresNode* child) {
  if (children.size() < 4) {
    children.resize(4);
  }

  children[GetChildIndex(node_num)] = child;
}

/**
 * Debugging routine that recursively shows which nodes are
 * in a tree.
 */
void HiresNode::Show(const std::string& address) const {
  if (IsLeaf()) {
    std::cout << "f";
  }

  if (children.size() == 0) {
    std::cout << "L";
    return;
  }

  for (int i = 0; i < 4; ++i) {
    std::string new_address = address;
    if (children[i] != NULL) {
      std::cout << std::endl << address << i;
      switch (i) {
        case 0:
          new_address += "0";
          break;

        case 1:
          new_address += "1";
          break;

        case 2:
          new_address += "2";
          break;

        case 3:
          new_address += "3";
          break;
      }

      children[i]->Show(new_address);
    }
  }
}


/**
 * Recursively checks if path is in the tree starting with
 * this node as the root.
 * TODO: Consider eliminating this method and just
 * TODO: calculating this iteratively in the tree method.
 *
 * @return whether this path is in the tree.
 */
bool HiresNode::IsTreePath(const std::string& path) const {
  // If path is empty, we have a match.
  if (path.empty()) {
    return true;
  }

  // If we are at a leaf, everything below is a match.
  if (is_leaf) {
    return true;
  }

  // See if we can match child for next node in path.
  const HiresNode* child = GetChild(path[0]);
  if (child != NULL) {
    return child->IsTreePath(path.substr(1));
  }

  // If not, the path is not in the tree.
  return false;
}


size_t HiresNode::GetChildIndex(char node_num) const {
  return (size_t) node_num - ZERO_CHAR;
}


/**
 * Adds node(s) for the given path that are not already in the tree.
 * Used for "strict" trees where every node must be explicitly
 * included.
 */
void HiresTree::AddNode(const std::string& path) {
  HiresNode* node = tree_;
  // Go until we find nodes that haven't been created yet.
  for (size_t i = 0; i < path.size(); ++i) {
    HiresNode* child_node = node->GetChild(path[i]);
    if (child_node == NULL) {
      // Add remaining needed nodes to the tree.
      node->SetLeaf(false);
      node->AddChild(path[i], new HiresNode(path.substr(i + 1)));
      return;
    }

    // Keep descending through the existing tree nodes.
    node = child_node;
  }

  notify(NFY_DEBUG, "%s node already in place.", path.c_str());
}

/**
 * Adds node(s) for the specified path. If an ancestor path is already in the
 * tree, the node is ignored since all descendents are already assumed.
 * For the polygon to qt node algorithm, this would be unexpected because
 * it tries not to pass any redundant information.
 * Used for "encompassing" trees where any node that is a descendent of
 * a leaf in the tree is considered to be included by the tree.
 */
void HiresTree::AddHiResQtNode(HiresNode* node, const std::string& path) {
  // Go until we find nodes that haven't been created yet.
  for (size_t i = 0; i < path.size(); ++i) {
    if (node->IsLeaf()) {
      notify(NFY_WARN, "Lower-level parent node already in place.");
      return;
    }

    HiresNode* child_node = node->GetChild(path[i]);
    if (child_node == NULL) {
      // Add remaining needed nodes to the tree.
      node->AddChild(path[i], new HiresNode(path.substr(i + 1)));
      return;
    }

    // Keep descending through the existing tree nodes.
    node = child_node;
  }

  node->SetLeaf(true);
  notify(NFY_WARN, "%s node already in place, may be losing specificity.",
         path.c_str());
}

void HiresTree::LoadHiResQtNodeFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  LoadHiResQtNodes(&fin);
}

/**
 * Load quadtree node paths into tree structure so we can quick check if a
 * quadtree node is a leaf in the tree or is a child of a leaf in the tree.
 * We don't care about the ordering of the leaves in this stream, the
 * same tree should be built independent of the order if all are leaves.
 * TODO: Use QuadTreePath instead of std::string.
 */
void HiresTree::LoadHiResQtNodes(std::istream* fin) {
  std::string next_node_path;

  while (!fin->eof()) {
    next_node_path = "";
    *fin >> next_node_path;
    if (!next_node_path.empty()) {
      AddHiResQtNode(tree_, next_node_path);
    }
  }
}

bool HiresTree::IsTreePath(const std::string& path) const {
  return TestTreePath(path, true);
}

bool HiresTree::IsInTree(const std::string& path) const {
  return TestTreePath(path, false);
}

bool HiresTree::TestTreePath(
    const std::string& path, bool includeLeafDescendents) const {
  HiresNode* node = tree_;
  std::string remaining_path = path;
  while (true) {
    // If path is empty, we have a match.
    if (remaining_path.empty()) {
      return true;
    }

    // If we are at a leaf, everything below is a match.
    if (node->IsLeaf()) {
      if (includeLeafDescendents) {
        return true;
      } else {
        return true;
      }
    }

    // See if we can match child for next node in path.
    node = node->GetChild(remaining_path[0]);
    if (node == NULL) {
      return false;
    }

    // So far so good: keep going.
    remaining_path = remaining_path.substr(1);
  }
}

void HiresTree::Show() const {
  tree_->Show("");
  std::cout << std::endl;
}

/**
 * Constructor. Builds acceptance tree from quadtree node file and
 * stores cut limits.
 */
CutSpec::CutSpec(const std::string& qt_node_file_name,
                 std::uint16_t min_level,
                 std::uint16_t default_level,
                 std::uint16_t max_level)
    : min_level_(min_level),
      default_level_(default_level),
      max_level_(max_level),
      is_exclusion_enabled_(false) {
  hires_tree_.LoadHiResQtNodeFile(qt_node_file_name);
}


/**
 * Constructor. Builds acceptance tree from quadtree one node file and
 * an exclusion tree from another. Exclusion trumps acceptance.
 * Also stores cut limits.
 */
CutSpec::CutSpec(const std::string& qt_node_file_name,
                 const std::string& exclusion_qt_node_file_name,
                 std::uint16_t min_level,
                 std::uint16_t default_level,
                 std::uint16_t max_level)
    : min_level_(min_level),
      default_level_(default_level),
      max_level_(max_level),
      is_exclusion_enabled_(true) {
  hires_tree_.LoadHiResQtNodeFile(qt_node_file_name);
  exclusion_tree_.LoadHiResQtNodeFile(exclusion_qt_node_file_name);
}


/**
 * Returns whether the given quadtree node should be kept (i.e. not pruned).
 * If the node is at or below (lower resolution) the default level or if
 * it is in the specified high-resolution nodes, then it should not be
 * pruned.
 */
bool CutSpec::KeepNode(const std::string& qtpath) const {
  std::uint16_t level = qtpath.size();

  // If we are below the min level, keep nothing.
  if (level < min_level_) {
    return true;
  }

  // If we are at or below the default level, keep everything.
  if (level <= default_level_) {
    return true;
  }

  // If we are above the max level, keep nothing.
  if (level > max_level_) {
    return false;
  }

  // Check the quadtree path to see if it is one we are keeping.
  return hires_tree_.IsTreePath(qtpath);
}


/**
 * Returns whether data in the given quadtree node should be excluded.
 */
bool CutSpec::ExcludeNode(const std::string& qtpath) const {
  if (!is_exclusion_enabled_) {
    return false;
  }

  // If we are at or below the default level, do not exclude.
  if (qtpath.size() <= default_level_) {
    return false;
  }

  // Check the quadtree path to see if it is one we are excluding.
  return exclusion_tree_.IsTreePath(qtpath);
}

}  // namespace fusion_portableglobe
