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
//#include "fusion/autoingest/sysman/InsetTilespaceIndex.h"

// // TODO - pack this struct to fit in a tidy 64 bit "size_t" for optimal performance
// struct  TilespaceMBR { 
//     byte projectionType,  // TODO - Flat, Mercator, etc.  Reuse Define  a definition other headers?
//     byte maxDepth,
//     uint32 quadTreeBits
// };
template <class T>
class QuadKeyTreeNode {
//     friend class QuadKeyTree<T>;
// protected:
public:
    //QuadKeyTreeNode() = default {}
    std::array<std::shared_ptr<QuadKeyTreeNode<T>>, 4> nodes {{nullptr, nullptr, nullptr, nullptr}};
    std::vector<const T*> data;
};

template <class T>
class QuadKeyTree {
private:
    std::array<std::shared_ptr<QuadKeyTreeNode<T>>, 4> nodes {{nullptr, nullptr, nullptr, nullptr}};

public:
    void AddElementAtQuadTreePath(QuadtreePath path,  const T *element){
        assert(path.Level() >= 1 && path.Level() <= 24);
        std::cout << "path[0] = " << path[0] << "\n";
        if (nodes[path[0]] == nullptr)
            nodes[path[0]] = std::make_shared<QuadKeyTreeNode<T>>();
        std::shared_ptr<QuadKeyTreeNode<T>> nodeptr = nodes[path[0]];
        std::cout << "nodeptr is " << (nodeptr == nullptr ? "null" : "not null") << "\n";
        for (uint32 level = 1; level < path.Level(); level++) {
            std::cout << "path[" << level << "] = " << path[level] << "\n";
            if (nodeptr->nodes[path[level]] == nullptr)
                nodeptr->nodes[path[level]] = std::make_shared<QuadKeyTreeNode<T>>();
            nodeptr = nodeptr->nodes[path[level]];
            std::cout << "nodeptr is " << (nodeptr == nullptr ? "null" : "not null") << "\n";
        }

        assert(nodeptr != nullptr);
        nodeptr->data.push_back(element);
    }

    std::vector<const T*> GetElementsAtQuadTreePath(QuadtreePath path, bool matchEntirePathUpAndDown = true) 
    {
        std::vector<const T*> vec;
        if (nodes[path[0]] == nullptr)
            return vec;

        std::shared_ptr<QuadKeyTreeNode<T>> nodeptr = nodes[path[0]];
        //for (uint32 level = 1; level < path.Level(); level++) {
        uint32 level = 0;
        while(nodeptr != nullptr && level < path.Level()-1 ) {
            if (matchEntirePathUpAndDown)
                vec.insert(std::end(vec), std::begin(nodeptr->data), std::end(nodeptr->data));
            level++;
            nodeptr = nodeptr->nodes[path[level]];
        }

        if (nodeptr != nullptr){
            if (matchEntirePathUpAndDown) {
                AddAllFromNodeAndChildren(nodeptr, vec);
            }
            else
            {
                vec.insert(std::end(vec), std::begin(nodeptr->data), std::end(nodeptr->data));                    
            }
            
        }

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

    //QuadtreePath add(const khExtents <double> &extents);
    QuadtreePath add(const ExtentContainer &toAdd);

    QuadtreePath getQuadtreeMBR(const khExtents<double> &extents, int &level, const int max_level);

    ContainerVector intersectingExtents(const QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel);

    // std::vector <QuadtreePath>
    // intersectingExtentsQuadtreePaths(const QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel);

protected:

};
// class InsetInfoAndIndex {
// public:
//     InsetInfoAndIndex(const InsetInfo<RasterProductAssetVersion>* _ptrInsetInfo, uint _index)
//     : ptrInsetInfo(_ptrInsetInfo), index(_index) {}
//     const InsetInfo<RasterProductAssetVersion>* ptrInsetInfo;
//     uint index;
// };


#endif 


