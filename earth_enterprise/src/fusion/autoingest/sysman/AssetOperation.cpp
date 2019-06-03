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

#include "AssetOperation.h"
#include "AssetVersionD.h"

#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <list>
#include <map>

using namespace boost;
using namespace std;

struct AssetVertex {
  SharedString name;
  AssetDefs::State state;
};

enum DependencyType { INPUT, CHILD };

struct AssetEdge {
  DependencyType type;
};

typedef adjacency_list<setS, setS, directedS, AssetVertex, AssetEdge> AssetTree;

// Convenience class for building a graph representation of a version tree
// for a specified version. You call it like a function. Writing a class
// instead of a function is useful to maintain temporary state needed to
// build the graph.
class BuildAssetTree {
  private:
    typedef map<SharedString, AssetTree::vertex_descriptor> VertexMap;
    SharedString version;
    AssetTree graph;
    VertexMap vertices;
    AssetTree::vertex_descriptor GetVertex(AssetVersion version) {
      VertexMap::iterator vertexIter = vertices.find(version->GetRef());
      if (vertexIter != vertices.end()) {
        return vertexIter->second;
      }
      else {
        AssetTree::vertex_descriptor vertex = add_vertex({version->GetRef(), version->state}, graph);
        vertices[version->GetRef()] = vertex;
        return vertex;
      }
    }
    void AddSelfAndDescendants(AssetVersion version) {
      AssetTree::vertex_descriptor vertex = GetVertex(version);
      for (const auto & child : version->children) {
        AssetTree::vertex_descriptor childVertex = GetVertex(child);
        add_edge(vertex, childVertex, {CHILD}, graph);
        AddSelfAndDescendants(child);
      }
      for (const auto & input : version->inputs) {
        AssetTree::vertex_descriptor childVertex = GetVertex(input);
        add_edge(vertex, childVertex, {CHILD}, graph);
      }
    }
  public:
    BuildAssetTree(const SharedString & version) : version(version) {
      AddSelfAndDescendants(version);
    }
    operator AssetTree() { return graph; }
};

void UpdateStateForSelfAndDependentChildren(
    MutableAssetVersionD version,
    AssetDefs::State newState,
    int statesToInclude) {
  if (version->state & statesToInclude) {
    // Don't call SetState - we don't want to trigger callbacks yet
    version->state = newState;
    vector<SharedString> dependents;
    version->DependentChildren(dependents);
    for (auto & child : dependents) {
      UpdateStateForSelfAndDependentChildren(child, newState, statesToInclude);
    }
  }
}

class UpdateStateVisitor : public default_dfs_visitor {
  private:
    class InputStates {
      private:
        uint numinputs = 0;
        uint numgood = 0;
        uint numblocking = 0;
        uint numoffline = 0;
      public:
        void Add(AssetDefs::State inputState) {
          ++numinputs;
          if (inputState == AssetDefs::Succeeded) {
            ++numgood;
          } else if (inputState == AssetDefs::Offline) {
            ++numblocking;
            ++numoffline;
          } else if (inputState & (AssetDefs::Blocked |
                                   AssetDefs::Failed |
                                   AssetDefs::Canceled |
                                   AssetDefs::Bad)) {
            ++numblocking;
          }
        }
        void GetOutputs(AssetDefs::State & stateByInputs, bool & blockersAreOffline, uint32 & numWaitingFor) {
          if (numinputs == numgood) {
            stateByInputs = AssetDefs::Queued;
          } else if (numblocking) {
            stateByInputs = AssetDefs::Blocked;
          } else {
            stateByInputs = AssetDefs::Waiting;
          }
          blockersAreOffline = numblocking == numoffline;
          if (stateByInputs == AssetDefs::Waiting) {
            numWaitingFor = (numinputs - numgood);
          } else {
            numWaitingFor = 0;
          }
        }
    };
    class ChildStates {
      private:
        uint numkids = 0;
        uint numgood = 0;
        uint numblocking = 0;
        uint numinprog = 0;
        uint numfailed = 0;
      public:
        void Add(AssetDefs::State childState) {
          ++numkids;
          if (childState == AssetDefs::Succeeded) {
            ++numgood;
          } else if (childState == AssetDefs::Failed) {
            ++numfailed;
          } else if (childState == AssetDefs::InProgress) {
            ++numinprog;
          } else if (childState & (AssetDefs::Blocked  |
                                   AssetDefs::Canceled |
                                   AssetDefs::Offline  |
                                   AssetDefs::Bad)) {
            ++numblocking;
          }
        }
        void GetOutputs(AssetDefs::State stateByChildren) {
          if (numkids == numgood) {
            stateByChildren = AssetDefs::Succeeded;
          } else if (numblocking || numfailed) {
            stateByChildren = AssetDefs::Blocked;
          } else if (numgood || numinprog) {
            stateByChildren = AssetDefs::InProgress;
          } else {
            stateByChildren = AssetDefs::Queued;
          }
        }
    };

    void CalcStateByInputsAndChildren(
        AssetTree::vertex_descriptor vertex,
        const AssetTree & tree,
        AssetDefs::State &stateByInputs,
        AssetDefs::State &stateByChildren,
        bool & blockersAreOffline,
        uint32 & numWaitingFor) const {
      InputStates inputStates;
      ChildStates childStates;

      auto edgeIters = out_edges(vertex, tree);
      auto edgeBegin = edgeIters.first;
      auto edgeEnd = edgeIters.second;
      for (auto i = edgeBegin; i != edgeEnd; ++i) {
        DependencyType type = tree[*i].type;
        AssetTree::vertex_descriptor dep = i->m_target;
        AssetDefs::State depState = tree[dep].state;
        switch(type) {
          case INPUT:
            inputStates.Add(depState);
            break;
          case CHILD:
            childStates.Add(depState);
            break;
        }
      }

      inputStates.GetOutputs(stateByInputs, blockersAreOffline, numWaitingFor);
      childStates.GetOutputs(stateByChildren);
    }
  public:
    // Update the state of an asset after we've updated the state of its
    // inputs and children.
    virtual void finish_vertex(AssetTree::vertex_descriptor vertex, AssetTree & tree) const {
      MutableAssetVersionD version(tree[vertex].name);
      if (!version->NeedComputeState()) return;
      AssetDefs::State stateByInputs;
      AssetDefs::State stateByChildren;
      bool blockersAreOffline;
      uint32 numWaitingFor;
      CalcStateByInputsAndChildren(vertex, tree, stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
      AssetDefs::State newState;
      newState = version->CalcStateByInputsAndChildren(stateByInputs, stateByChildren, blockersAreOffline, numWaitingFor);
      // Update the state in the tree because other assets may need it to compute
      // their own states.
      tree[vertex].state = newState;
      // false -> don't propagate state. We've already taken care of that with
      // the asset tree.
      version->SetState(newState, nullptr, false);
    }
};

void RebuildVersion(const AssetVersion & version) {
  // Rebuilding an already succeeded asset is quite dangerous!
  // Those who depend on me may have already finished their work with me.
  // If I rebuild, they have the right to recognize that nothing has
  // changed (based on my version # and their config) and just reuse their
  // previous results. Those previous results may reference my outputs.
  // But if my inputs reference disk files that could change (sources
  // overwritten with new versions), I may change some of my outputs,
  // thereby invalidating the cached work from later stages.
  //
  // For this reason, requests to rebuild 'Succeeded' versions will fail.
  // Assets marked 'Bad' were once succeeded, so they too are disallowed.
  // The same logic could hold true for 'Offline' as well.
  if (version->state & (AssetDefs::Succeeded | AssetDefs::Offline | AssetDefs::Bad)) {
    throw khException(kh::tr("%1 marked as %2. Refusing to resume.")
                      .arg(ToQString(version->GetRef()), ToQString(version->state)));
  }

  UpdateStateForSelfAndDependentChildren(version->GetRef(), AssetDefs::New, AssetDefs::Canceled | AssetDefs::Failed);
  AssetTree assets = BuildAssetTree(version->GetRef());
  // Set the states of the version and children, grandchildren, etc, to new
  // Create a graph using input->listener and parent->child relationships
  // for all descendants (use boost graph library).
  //    May be able to ignore parent->child relationships if they are
  //    included as input->listener operations.
  // Calculate and update state starting at the sinks
  // Additional optimizations
  //    Many assets have significant overlap in their inputs. It would be great
  //    if we can calculate the overlapping state only once.
}
