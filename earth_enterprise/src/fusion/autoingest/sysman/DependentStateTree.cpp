/*
 * Copyright 2019 The Open GEE Contributors
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

#include "DependentStateTree.h"
#include "common/notify.h"

#include <map>
#include <set>

using namespace boost;
using namespace std;

class DependentStateTreeFactory
{
  private:
    struct VertexData {
      DependentStateTreeVertexDescriptor vertex;
      bool includeConnections;
    };

    SharedString ref;
    std::function<bool(AssetDefs::State)> includePredicate;
    bool includeDependentChildren;
    bool includeAllChildrenAndInputs;
    StorageManagerInterface<AssetVersionImpl> * const storageManager;
    map<SharedString, VertexData> vertices;
    size_t index;
    set<DependentStateTreeVertexDescriptor> toFillInNext;
    DependentStateTree tree;

    inline bool IsDependent(DependencyType type)
        { return type == DEPENDENT || type == DEPENDENT_AND_CHILD; }

    DependentStateTreeVertexDescriptor AddOrUpdateVertex(
        const SharedString & ref,
        bool inDepTree,
        bool includeConnections);
    void FillInVertex(DependentStateTreeVertexDescriptor myVertex);
    void AddEdge(
        DependentStateTreeVertexDescriptor from,
        DependentStateTreeVertexDescriptor to,
        AssetEdge data);
  public:
    DependentStateTreeFactory(
        const SharedString &ref,
        std::function<bool(AssetDefs::State)> includePredicate,
        bool includeDependentChildren,
        bool includeAllChildrenAndInputs,
        StorageManagerInterface<AssetVersionImpl> *sm) :
      ref(ref), includePredicate(includePredicate), 
      includeDependentChildren(includeDependentChildren),
      includeAllChildrenAndInputs(includeAllChildrenAndInputs), storageManager(sm), index(0) {}
    DependentStateTree BuildTree();
};

// Builds a tree containing the specified asset, its depedent children, their
// dependent children, and so on, along with any other assets that are needed
// to update the state of these assets. That includes their inputs and children,
// their parents and listeners all the way up the tree, and the inputs and
// children for their parents and listeners.
DependentStateTree DependentStateTreeFactory::BuildTree() {
  // First create an empty vertex for the provided asset. Then fill it in,
  // which includes adding its connections to other assets. Every time we fill
  // in a node we will get new assets to add to the tree until all assets have
  // been added. This basically builds the tree using a breadth first search,
  // which allows us to keep memory usage (relatively) low by not forcing
  // assets to stay in the cache and limiting the size of the toFillIn and
  // toFillInNext lists.
  AddOrUpdateVertex(ref, true, true);
  while (toFillInNext.size() > 0) {
    set<DependentStateTreeVertexDescriptor> toFillIn = std::move(toFillInNext);
    toFillInNext.clear();
    for (auto vertex : toFillIn) {
      FillInVertex(vertex);
    }
  }
  return tree;
}

// Creates an "empty" node for this asset if it has not already been added to
// the tree. The node has a default state and doesn't include links to
// inputs/children/etc. The vertex must be "filled in" by calling FillInVertex
// before it can be used.
DependentStateTreeVertexDescriptor
DependentStateTreeFactory::AddOrUpdateVertex(
    const SharedString & ref,
    bool inDepTree,
    bool includeConnections) {
  auto myVertexIter = vertices.find(ref);
  if (myVertexIter == vertices.end()) {
    // I'm not in the graph yet, so make a new empty vertex and let the caller
    // know we need to load it with the correct information
    auto myVertex = add_vertex(tree);
    tree[myVertex] = {ref, AssetDefs::New, inDepTree, index};
    ++index;
    vertices[ref] = {myVertex, includeConnections};
    toFillInNext.insert(myVertex);
    return myVertex;
  }
  else {
    // I'm already in the graph. If we learned new information that we didn't
    // know when I was added to the tree (e.g., I'm in the dependent tree),
    // add this to the list of vertices to fill in so that we can add its
    // connections. Then return the existing descriptor.
    auto myVertex = myVertexIter->second.vertex;
    auto & myVertexData = tree[myVertex];
    if (inDepTree && !myVertexData.inDepTree) {
      myVertexData.inDepTree = true;
      toFillInNext.insert(myVertex);
    }
    if (includeConnections && !myVertexIter->second.includeConnections) {
      myVertexIter->second.includeConnections = true;
      toFillInNext.insert(myVertex);
    }
    return myVertex;
  }
}

// "Fills in" an existing vertex with the state of an asset and its connections
// to other assets. Adds any new nodes that need to be filled in to toFillInNext.
void DependentStateTreeFactory::FillInVertex(
    DependentStateTreeVertexDescriptor myVertex) {
  SharedString name = tree[myVertex].name;
  notify(NFY_PROGRESS, "Loading '%s' for state update", name.toString().c_str());
  auto version = storageManager->Get(name);
  if (!version) {
    notify(NFY_WARN, "Could not load asset '%s' which is referenced by another asset.",
           name.toString().c_str());
    // Set it to a bad state, but use a state that can be fixed by another
    // rebuild operation.
    tree[myVertex].state = AssetDefs::Blocked;
    return;
  }
  tree[myVertex].state = version->state;
  // If this vertex is in the dependent tree but doesn't need to be included,
  // act as though it's not in the dependency tree. We'll leave it in the tree
  // to avoid messing up the index numbering and in case it is needed as an
  // input, but we won't bring in any of its dependents.
  if (tree[myVertex].inDepTree && !includePredicate(tree[myVertex].state)) {
    tree[myVertex].inDepTree = false;
    vertices[name].includeConnections = false;
  }
  // If I'm in the dependency tree, and if the new state propagates to dependents,
  // then I need to add my dependents to the graph and the dependency tree.
  if (tree[myVertex].inDepTree && includeDependentChildren) {
    vector<SharedString> dependents;
    version->DependentChildren(dependents);
    for (const auto & dep : dependents) {
      auto depVertex = AddOrUpdateVertex(dep, true, true);
      AddEdge(myVertex, depVertex, {DEPENDENT});
    }
  }
  // If I need to recalculate my state, I need to have my children and inputs
  // in the tree because my state is based on them. In addition, I need my
  // parents and listeners, because they may also have to recalculate their
  // state. If I don't need to recalculate my state, I don't need to add any of
  // my connections. I'm only used to calculate someone else's state.
  if (vertices[name].includeConnections) {
    // In some cases, assets don't need all their inputs and children to
    // calculate their states.
    if (includeAllChildrenAndInputs) {
      for (const auto &child : version->children)
      {
        auto childVertex = AddOrUpdateVertex(child, false, false);
        AddEdge(myVertex, childVertex, {CHILD});
      }
      for (const auto & input : version->inputs) {
        auto inputVertex = AddOrUpdateVertex(input, false, false);
        AddEdge(myVertex, inputVertex, {INPUT});
      }
    }
    for (const auto & parent : version->parents) {
      auto parentVertex = AddOrUpdateVertex(parent, false, true);
      AddEdge(parentVertex, myVertex, {CHILD});
    }
    for (const auto & listener : version->listeners) {
      auto listenerVertex = AddOrUpdateVertex(listener, false, true);
      AddEdge(listenerVertex, myVertex, {INPUT});
    }
  }
}

void DependentStateTreeFactory::AddEdge(
    DependentStateTreeVertexDescriptor from,
    DependentStateTreeVertexDescriptor to,
    AssetEdge data) {
  auto edgeData = add_edge(from, to, tree);
  if (edgeData.second) {
    // This is a new edge
    tree[edgeData.first] = data;
  }
  else {
    // Check if this is both a dependent and a child
    DependencyType currentType = tree[edgeData.first].type;
    DependencyType newType = data.type;
    if ((currentType == DEPENDENT && newType == CHILD) ||
        (currentType == CHILD && newType == DEPENDENT)) {
      tree[edgeData.first].type = DEPENDENT_AND_CHILD;
    }
  }
}

DependentStateTree BuildDependentStateTree(
    const SharedString & ref,
    std::function<bool(AssetDefs::State)> includePredicate,
    bool includeDependentChildren,
    bool includeAllChildrenAndInputs,
    StorageManagerInterface<AssetVersionImpl> * sm) {
  DependentStateTreeFactory factory(ref, includePredicate, includeDependentChildren, includeAllChildrenAndInputs, sm);
  return factory.BuildTree();
}
