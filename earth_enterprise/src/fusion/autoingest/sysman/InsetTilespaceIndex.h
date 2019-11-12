// Copyright 2017 Google Inc.
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


/******************************************************************************
File:        InsetInfo.cpp
Description:

Changes:
Description: Support for terrain "overlay" projects.

******************************************************************************/

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_EXTENTINDEXES_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_EXTENTINDEXES_H_

#include <string>
#include <functional>
#include "common/khTileAddr.h"
#include "common/quadtreepath.h"
#include "common/khExtents.h"
#include "common/khInsetCoverage.h"
#include "fusion/autoingest/sysman/InsetInfo.h"
#include <iostream>
template <class T>

class QuadKeyTreeNode {
public:
    std::array<std::shared_ptr<QuadKeyTreeNode<T>>, 4> nodes {{nullptr, nullptr, nullptr, nullptr}};
    std::vector<const T*> data;
};

template <class T>
class QuadKeyTree {
private:
    std::shared_ptr<QuadKeyTreeNode<T>> levelZeroNode = std::make_shared<QuadKeyTreeNode<T>>();
public:
    void AddElementAtQuadTreePath(QuadtreePath path,  const T *element){
        const uint pathLength = path.Level();
        uint pathIndex = 0;
        std::shared_ptr<QuadKeyTreeNode<T>> nodeptr = levelZeroNode;

        while (pathIndex < pathLength){
            uint qtIndexAtLevel = path[pathIndex];     // e.g., if path is "1320", tileAtLevel would be 3 for level=2
            if (nodeptr->nodes[qtIndexAtLevel] == nullptr)
                nodeptr->nodes[qtIndexAtLevel] = std::make_shared<QuadKeyTreeNode<T>>();
            nodeptr = nodeptr->nodes[qtIndexAtLevel];
            pathIndex++;
        }

        assert(nodeptr != nullptr);
        nodeptr->data.push_back(element);
    }

    std::vector<const T*> GetElementsAtQuadTreePath(QuadtreePath path, uint startLevel) 
    {
        std::vector<const T*> vec;

        const uint pathLength = path.Level();
        uint pathIndex = 0;
        std::shared_ptr<QuadKeyTreeNode<T>> nodeptr = levelZeroNode;

        while (pathIndex < pathLength && pathIndex < startLevel && nodeptr != nullptr){
            vec.insert(std::end(vec), std::begin(nodeptr->data), std::end(nodeptr->data));
            uint qtIndexAtLevel = path[pathIndex];     
            nodeptr = nodeptr->nodes[qtIndexAtLevel];
            ++pathIndex;
        }

        if (nodeptr)
            AddAllFromNodeAndChildren(nodeptr, vec);

        return vec;
    }

    void AddAllFromNodeAndChildren(std::shared_ptr<QuadKeyTreeNode<T>> nodeptr, std::vector<const T*> &vec) {
        vec.insert(std::end(vec), std::begin(nodeptr->data), std::end(nodeptr->data));

        for (auto & childnode : nodeptr->nodes) {
            if (childnode != nullptr)
                AddAllFromNodeAndChildren(childnode, vec);
        }
    }
};



const int MAX_LEVEL = 24;

template<class ExtentContainer>
class InsetTilespaceIndex {
public:
    using ContainerVector = typename std::vector<const ExtentContainer*>;
protected:
    //using QuadTreeMap = typename std::map <QuadtreePath, ContainerVector>;
    QuadKeyTree<ExtentContainer> quadTree;
    //QuadTreeMap _mbrExtentsVecMap;
public:
    InsetTilespaceIndex() = default;

    QuadtreePath add(const ExtentContainer &toAdd);

    QuadtreePath getQuadtreeMBR(const khExtents<double> &extents, int &level, const int max_level);

    ContainerVector intersectingExtents(const QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel);
protected:

};

#endif 


