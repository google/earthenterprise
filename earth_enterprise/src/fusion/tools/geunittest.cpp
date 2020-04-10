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


#include <math.h>
#include <vector>

#include <notify.h>
#include <khStringUtils.h>

#include <gstGeode.h>
#include <gstRecord.h>
#include <gstValue.h>
#include <gstSelector.h>
#include "fusion/gst/gstSequenceAlg.h"
#include "fusion/gst/vectorprep/PolylineJoiner.h"

#include <projection.h>


void TestComputeAngle() {
  {
    // test a real world example
    gstVertex v0(0.16001252238456939, 0.60495449748102914, 0);
    gstVertex v1(0.78088253201610214, 0.62467789395185214, 0);

    gstGeodeHandle ah = gstGeodeImpl::Create(gstStreet);
    gstGeode *a = static_cast<gstGeode*>(&(*ah));
    a->AddVertex(v0);
    a->AddVertex(v1);
    gstGeodeHandle bh = gstGeodeImpl::Create(gstStreet);
    gstGeode *b = static_cast<gstGeode*>(&(*bh));
    b->AddVertex(v0);
    b->AddVertex(v1);
    assert(ComputeAngle(ah, bh, v0) == 0.0);

    gstGeodeHandle ch = gstGeodeImpl::Create(gstStreet);
    gstGeode *c = static_cast<gstGeode*>(&(*ch));
    c->AddVertex(v1);
    c->AddVertex(v0);
    assert(ComputeAngle(ah, ch, v0) == 0.0);
  }

  {
    // Test all combinations of segments A,B,C & D
    //
    //         2
    //         |B
    //     A   |
    //    1----0----3
    //         |   C
    //        D|
    //         4
    //
    gstVertex v0(0, 0, 0);
    gstVertex v1(-10, 0, 0);
    gstVertex v2(0, -10, 0);
    gstVertex v3(10, 0, 0);
    gstVertex v4(0, 10, 0);

    gstGeodeHandle ah = gstGeodeImpl::Create(gstStreet);
    gstGeode *a = static_cast<gstGeode*>(&(*ah));
    a->AddVertex(v0);
    a->AddVertex(v1);
    gstGeodeHandle bh = gstGeodeImpl::Create(gstStreet);
    gstGeode *b = static_cast<gstGeode*>(&(*bh));
    b->AddVertex(v0);
    b->AddVertex(v2);
    gstGeodeHandle ch = gstGeodeImpl::Create(gstStreet);
    gstGeode *c = static_cast<gstGeode*>(&(*ch));
    c->AddVertex(v0);
    c->AddVertex(v3);
    gstGeodeHandle dh = gstGeodeImpl::Create(gstStreet);
    gstGeode *d = static_cast<gstGeode*>(&(*dh));
    d->AddVertex(v0);
    d->AddVertex(v4);

    assert(ComputeAngle(ah, ah, v0) == 0.0);
    assert(ComputeAngle(bh, bh, v0) == 0.0);
    assert(ComputeAngle(ch, ch, v0) == 0.0);
    assert(ComputeAngle(dh, dh, v0) == 0.0);

    assert(ComputeAngle(ah, bh, v0) == M_PI_2);
    assert(ComputeAngle(bh, ch, v0) == M_PI_2);
    assert(ComputeAngle(ch, dh, v0) == M_PI_2);
    assert(ComputeAngle(dh, ah, v0) == M_PI_2);

    assert(ComputeAngle(ah, ch, v0) == M_PI);
    assert(ComputeAngle(bh, dh, v0) == M_PI);
    assert(ComputeAngle(ch, ah, v0) == M_PI);
    assert(ComputeAngle(dh, bh, v0) == M_PI);

    assert(ComputeAngle(ah, dh, v0) == M_PI_2);
    assert(ComputeAngle(bh, ah, v0) == M_PI_2);
    assert(ComputeAngle(ch, bh, v0) == M_PI_2);
    assert(ComputeAngle(dh, ch, v0) == M_PI_2);
  }
}

// testing gstRecord, gstHandle and gstValue
void TestRecordAndFriends() {
  {
    assert(gstHeaderImpl::hcount == 0);
    gstHeaderHandle header1 = gstHeaderImpl::Create();
    assert(gstHeaderImpl::hcount == 1);
    gstHeaderHandle header2 = gstHeaderImpl::Create();
    assert(gstHeaderImpl::hcount == 2);
  }

  {
    assert(gstHeaderImpl::hcount == 0);
    assert(gstRecordImpl::rcount == 0);
    gstHeaderHandle header = gstHeaderImpl::Create();
    header->addSpec("Name", gstTagString);
    gstRecordHandle record = header->NewRecord();
    assert(gstRecordImpl::rcount == 1);
  }
}

void TestValue() {
  {
    // pull subpieces
    const char buf[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::string piece = ExtractString(buf, 10);
    assert(piece == std::string("0123456789"));
    piece = ExtractString(buf + 10, 5);
    assert(piece == std::string("abcde"));
    piece = ExtractString(buf + 26, 10);
    assert(piece == std::string("qrstuvwxyz"));
  }

  {
    // validate encoding requirement
    char ubuf[] = "abcdefg";
    ubuf[4] = 200;
    bool need_codec;
    std::string piece = ExtractString(ubuf, 4, &need_codec);
    assert(need_codec == false);
    assert(piece == std::string("abcd"));
    piece = ExtractString(ubuf, -1, &need_codec);
    assert(need_codec == true);
  }

  {
    // handle double-quoted text
#ifndef NDEBUG
    char buf[] = "\"abcdefg\"";
#endif
    assert(ExtractString(buf, -1) == std::string("abcdefg"));
    assert(ExtractString(buf, 3) == std::string("\"ab"));
    assert(ExtractString(buf + 3, -1) == std::string("cdefg\""));
  }

  {
    // strip trailing spaces
#ifndef NDEBUG
    char buf[] = "\"  xx  oo  \"";
#endif
    assert(ExtractString(buf, -1) == std::string("  xx  oo  "));
    assert(ExtractString(buf + 1, 10) == std::string("  xx  oo"));
  }

  {
    // wildcard matching
    gstValue hello("hello world");
    assert(hello.MatchesWildcard("*world") == true);
    assert(hello.MatchesWildcard("hello*") == true);
    assert(hello.MatchesWildcard("hello*x") != true);
  }

  {
    // alpha comparisons
    assert(gstValue("Kansas") < gstValue("Montana"));
    assert(gstValue("Nevada") > gstValue("Colorado"));
  }
}

void TestSelector() {
  {  // Test removal of duplicates
    GeodeList glist;
    gstVertex v0(khTilespace::Normalize(19.99961895),
                 khTilespace::Normalize(-25.48416138), 0);
    gstVertex v1(khTilespace::Normalize(19.99961537),
                 khTilespace::Normalize(-25.48553467), 0);

    gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
    gstGeode *a = static_cast<gstGeode*>(&(*ah));
    a->AddVertex(v0);
    a->AddVertex(v1);
    glist.push_back(ah);

    gstGeodeHandle bh = gstGeodeImpl::Create(gstPolyLine);
    gstGeode *b = static_cast<gstGeode*>(&(*bh));
    b->AddVertex(v1);
    b->AddVertex(v0);
    glist.push_back(bh);

    gstGeodeHandle ch = gstGeodeImpl::Create(gstPolyLine);
    gstGeode *c = static_cast<gstGeode*>(&(*ch));
    c->AddVertex(v0);
    c->AddVertex(v1);
    glist.push_back(ch);

    gstGeodeHandle dh = gstGeodeImpl::Create(gstPolyLine);
    gstGeode *d = static_cast<gstGeode*>(&(*dh));
    d->AddVertex(v1);
    d->AddVertex(v0);
    glist.push_back(dh);

    std::uint64_t num_duplicates = 0;
    std::uint64_t num_joined = 0;
    vectorprep::PolylineJoiner<GeodeList>::
        RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices(
            glist, &num_duplicates, &num_joined);
    assert(num_duplicates == 3);
    assert(num_joined == 0);
    assert(glist[0]->TotalVertexCount() == 2);
    assert(glist[1]->TotalVertexCount() == 0);
    assert(glist[2]->TotalVertexCount() == 0);
    assert(glist[3]->TotalVertexCount() == 0);
  }
  GeodeList backup_for_remove_empty;
  {  // Test join neighbors at degree two vertices
    gstVertex v0(0, 0, 0), v1(10, 0, 0), v2(10, -5, 0), v3(15, -5, 0),
              v4(15, 5, 0), v5(15, 25, 0), v6(12, 24, 0), v7(18, 3, 0),
              v8(20, 3, 0), v9(22, 5, 0), v10(24, 5, 0);
    // Simple one
    gstVertex v11(9, 0, 0);
    GeodeList glist;
    GeodeList backup_glist;
    {  // [0] Add a simple line segment v0-v1
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v0);
      a->AddVertex(v1);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [0] Add a duplicate line segment v0-v1
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v0);
      a->AddVertex(v1);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [1] Add a simple line segment v11-v1
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v11);
      a->AddVertex(v1);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    std::uint64_t num_duplicates = 0;
    std::uint64_t num_joined = 0;
    vectorprep::PolylineJoiner<GeodeList>::
        RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices(
            glist, &num_duplicates, &num_joined);
    // We expect G7 to be deleted as a duplicate, G4-G5 to be merged
    // (none-cyclic degree 2), G2-G3 to be merged (cyclic degree 2).
    assert(num_joined == 1);
    assert(num_duplicates == 1);
    assert(glist.size() == 3);
    assert(glist[0]->TotalVertexCount() == 3);

#ifndef NDEBUG
    gstGeode *g0 = static_cast<gstGeode*>(&(*glist[0]));
#endif
    assert(v0 == g0->GetVertex(0, 0));
    assert(v1 == g0->GetVertex(0, 1));
    assert(v11 == g0->GetVertex(0, 2));
    assert(glist[1]->TotalVertexCount() == 0);

    // Complex one with more vertices and cycles
    glist.clear();
    backup_glist.clear();
    {  // [0] Add a simple line segment v0-v1
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v0);
      a->AddVertex(v1);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [1] Add a cycle at v1, v1-v2-v3-v1
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v1);
      a->AddVertex(v2);
      a->AddVertex(v3);
      a->AddVertex(v1);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [2] Add a half-cycle at v1, v1-v4-v5
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v1);
      a->AddVertex(v4);
      a->AddVertex(v5);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [3] Add another half-cycle at v1, v5-v6-v1
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v5);
      a->AddVertex(v6);
      a->AddVertex(v1);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [4] Add a simple segment at v1, v1-v7
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v1);
      a->AddVertex(v7);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [5] Add a simple segment at v7, v7-v8
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v7);
      a->AddVertex(v8);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [6] Add a cycle  at v8, v8-v9-v10-v8
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v8);
      a->AddVertex(v9);
      a->AddVertex(v10);
      a->AddVertex(v8);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }
    {  // [7] Add a duplicate segment at v7, v7-v8
      gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
      gstGeode *a = static_cast<gstGeode*>(&(*ah));
      a->AddVertex(v7);
      a->AddVertex(v8);
      glist.push_back(ah);
      backup_glist.push_back(ah->Duplicate());
    }

    assert(glist.size() == 8);

    assert(glist[0]->TotalVertexCount() == 2);
    assert(glist[1]->TotalVertexCount() == 4);
    assert(glist[2]->TotalVertexCount() == 3);
    assert(glist[3]->TotalVertexCount() == 3);
    assert(glist[4]->TotalVertexCount() == 2);
    assert(glist[5]->TotalVertexCount() == 2);
    assert(glist[6]->TotalVertexCount() == 4);
    assert(glist[7]->TotalVertexCount() == 2);
    num_duplicates = 0;
    num_joined = 0;
    vectorprep::PolylineJoiner<GeodeList>::
        RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices(
            glist, &num_duplicates, &num_joined);
    // We expect G7 to be deleted as a duplicate, G4-G5 to be merged
    // (none-cyclic degree 2), G2-G3 to be merged (cyclic degree 2).
    assert(num_joined == 2);
    assert(num_duplicates == 1);
    assert(glist.size() == 8);
    assert(glist[0]->Equals(backup_glist[0]));
    assert(glist[1]->Equals(backup_glist[1]));
    assert(glist[2]->TotalVertexCount() == 5);

#ifndef NDEBUG
    gstGeode *g2 = static_cast<gstGeode*>(&(*glist[2]));
#endif
    assert(v1 == g2->GetVertex(0, 0));
    assert(v4 == g2->GetVertex(0, 1));
    assert(v5 == g2->GetVertex(0, 2));
    assert(v6 == g2->GetVertex(0, 3));
    assert(v1 == g2->GetVertex(0, 4));
    assert(glist[3]->TotalVertexCount() == 0);
    assert(glist[4]->TotalVertexCount() == 3);

#ifndef NDEBUG
    gstGeode *g4 = static_cast<gstGeode*>(&(*glist[4]));
#endif
    assert(v1 == g4->GetVertex(0, 0));
    assert(v7 == g4->GetVertex(0, 1));
    assert(v8 == g4->GetVertex(0, 2));

    assert(glist[5]->TotalVertexCount() == 0);
    assert(glist[6]->Equals(backup_glist[6]));
    assert(glist[7]->TotalVertexCount() == 0);
    backup_for_remove_empty = glist;
  }
  {  // Test gstGeodeImpl::Simplify
    gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
    gstGeode *a = static_cast<gstGeode*>(&(*ah));
    gstVertex v0(0, 0, 0), v1(10, 0, 0), v2(10, -5, 0), v3(15, -5, 0),
              v4(15, 5, 0), v5 (15, 25, 0), v6(12, 24, 0), v7(18, 3, 0),
              v8(20, 3, 0), v9(22, 5, 0), v10(24, 5, 0);
    a->AddVertex(v0);
    a->AddVertex(v1);
    a->AddVertex(v2);
    a->AddVertex(v3);
    a->AddVertex(v4);
    a->AddVertex(v5);
    a->AddVertex(v6);
    a->AddVertex(v7);
    a->AddVertex(v8);
    a->AddVertex(v9);
    a->AddVertex(v10);
    std::vector<int> vertices;
    vertices.push_back(0);
    vertices.push_back(2);
    vertices.push_back(5);
    assert(a->Simplify(vertices) == (11 -3));
    assert(a->TotalVertexCount() == 3);
    assert(v0 == a->GetVertex(0, 0));
    assert(v2 == a->GetVertex(0, 1));
    assert(v5 == a->GetVertex(0, 2));
  }
  {  // Test ShrinkToRemaining and RemoveAtIndices
    const int orig_array[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    const size_t orig_array_size = (sizeof(orig_array) / sizeof(orig_array[0]));
    const int delete_indices[] = {0, 4, 9};
    const size_t delete_indices_size = (sizeof(delete_indices) /
                                        sizeof(delete_indices[0]));

    std::vector<int> const orig_vector(
        orig_array, orig_array + orig_array_size);
    std::vector<int> const delete_vector(
        delete_indices, delete_indices + delete_indices_size);
    std::vector<int> expected_result_vector(orig_vector);
    std::vector<bool> remaining(orig_vector.size(), true);
    std::vector<bool> deleted(orig_vector.size(), false);
    for (int i = delete_vector.size(); --i >= 0; ) {
      expected_result_vector.erase(expected_result_vector.begin() +
                                   delete_vector[i]);
      remaining[delete_vector[i]] = false;
      deleted[delete_vector[i]] = true;
    }
    typedef std::vector<std::vector<int> > VectorOfVector;
    VectorOfVector orig_vector_of_vectors(10, orig_vector);
    VectorOfVector expected_result_vector_of_vectors(7, orig_vector);
    {  // ShrinkToRemaining
      std::vector<int> tmp(orig_vector);
      ShrinkToRemaining<std::vector<int>, std::vector<int>::iterator, true>(
          remaining, &tmp);
      assert(tmp == expected_result_vector);
    }
    {  // ShrinkToRemaining
      std::vector<int> tmp(orig_vector);
      ShrinkToRemaining<std::vector<int>, std::vector<int>::iterator, false>(
          deleted, &tmp);
      assert(tmp == expected_result_vector);
    }
    {  // RemoveAtIndices
      std::vector<int> tmp(orig_vector);
      RemoveAtIndices<std::vector<int>, std::vector<int>::const_iterator,
                      std::vector<int>, std::vector<int>::iterator>(
          delete_vector, &tmp);
      assert(tmp == expected_result_vector);
    }
    {  // ShrinkToRemainingUsingSwap
      VectorOfVector tmp(orig_vector_of_vectors);
      ShrinkToRemainingUsingSwap<VectorOfVector, VectorOfVector::iterator,
                                 true>(remaining, &tmp);
      assert(tmp == expected_result_vector_of_vectors);
    }
    {  // ShrinkToRemainingUsingSwap
      VectorOfVector tmp(orig_vector_of_vectors);
      ShrinkToRemainingUsingSwap<VectorOfVector, VectorOfVector::iterator,
                                 false>(deleted, &tmp);
      assert(tmp == expected_result_vector_of_vectors);
    }
    {  // RemoveAtIndicesUsingSwap
      VectorOfVector tmp(orig_vector_of_vectors);
      RemoveAtIndicesUsingSwap<std::vector<int>,
          std::vector<int>::const_iterator, VectorOfVector,
          VectorOfVector::iterator>(delete_vector, &tmp);
      assert(tmp == expected_result_vector_of_vectors);
    }
  }
  {  // gstSelector::RemoveDuplicateSites and RemoveEmpty
    const gstVertex v0(0, 0, 0), v1(10, 0, 0), v2(10, -5, 0), v3(15, -5, 0);
    gstValue r0("0"), r1("1"), r2("2"), r3("3");
    VertexList vlist;
    vlist.push_back(v0);
    vlist.push_back(v0);
    vlist.push_back(v1);
    vlist.push_back(v2);
    vlist.push_back(v2);
    vlist.push_back(v3);
    vlist.push_back(v3);
    RecordList rlist;
    gstHeaderHandle dummy = gstHeaderImpl::Create();
    gstRecordImpl* r0_ptr = new gstRecordImpl(1, dummy);
    r0_ptr->AddField(new gstValue(r0));
    rlist.push_back(khRefGuardFromNew(r0_ptr));
    rlist.push_back(rlist.back());
    gstRecordImpl* r1_ptr = new gstRecordImpl(1, dummy);
    r1_ptr->AddField(new gstValue(r1));
    rlist.push_back(khRefGuardFromNew(r1_ptr));
    gstRecordImpl* r2_ptr = new gstRecordImpl(1, dummy);
    r2_ptr->AddField(new gstValue(r2));
    rlist.push_back(khRefGuardFromNew(r2_ptr));
    rlist.push_back(rlist.back());
    gstRecordImpl* r3_ptr = new gstRecordImpl(1, dummy);
    r3_ptr->AddField(new gstValue(r3));
    rlist.push_back(khRefGuardFromNew(r3_ptr));
    rlist.push_back(rlist.back());
    {  // gstSelector::RemoveDuplicateSites
      VertexList vlist_copy(vlist);
      RecordList rlist_copy(rlist);
      assert(3 == gstSelector::RemoveDuplicateSites(&vlist_copy, &rlist_copy));
      assert(vlist_copy.size() == 4);
      assert(rlist_copy.size() == 4);
      assert(vlist_copy[0] == v0);
      assert(vlist_copy[1] == v1);
      assert(vlist_copy[2] == v2);
      assert(vlist_copy[3] == v3);
      assert(&(*rlist_copy[0]) == r0_ptr);
      assert(&(*rlist_copy[1]) == r1_ptr);
      assert(&(*rlist_copy[2]) == r2_ptr);
      assert(&(*rlist_copy[3]) == r3_ptr);
      assert(0 == gstSelector::RemoveDuplicateSites(&vlist_copy, &rlist_copy));
    }
    {  // gstSelector::RemoveEmptyFeatures
      gstVertex v0(0, 0, 0), v1(10, 0, 0), v2(10, -5, 0), v3(15, -5, 0);
      GeodeList glist;
      // Choose the Geodes such that for different min_vertex_count values
      // for gstSelector::RemoveEmptyFeatures different things happen:
      // 0 -> no deletion
      // 1 -> deletion at the middle i.e index 2
      // 2 -> deletion at the middle and last i.e 2, 6
      // 3 -> deletion at 1, 2, 3, 6 entries
      // 4 -> deletion at 0, 1, 2, 3, 4, 6 entries
      // 5 -> deletion at all 0, 1, 2, 3, 4, 5, 6 entries
      {  // [0] Add a 3 vertex gstGeode
        gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
        gstGeode *a = static_cast<gstGeode*>(&(*ah));
        a->AddVertex(v1);
        a->AddVertex(v2);
        a->AddVertex(v3);
        glist.push_back(ah);
      }
      {  // [1] Add a 2 vertex gstGeode
        gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
        gstGeode *a = static_cast<gstGeode*>(&(*ah));
        a->AddVertex(v3);
        a->AddVertex(v2);
        glist.push_back(ah);
      }
      {  // [2] Add a 0 vertex gstGeode
        gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
        gstGeode *a = static_cast<gstGeode*>(&(*ah));
        a->AddVertex(v3);
        a->DeleteVertex(0,0);
        glist.push_back(ah);
      }
      {  // [3] Add a 2 vertex gstGeode
        gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
        gstGeode *a = static_cast<gstGeode*>(&(*ah));
        a->AddVertex(v2);
        a->AddVertex(v3);
        glist.push_back(ah);
      }
      {  // [4] Add a 3 vertex gstGeode
        gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
        gstGeode *a = static_cast<gstGeode*>(&(*ah));
        a->AddVertex(v1);
        a->AddVertex(v2);
        a->AddVertex(v3);
        glist.push_back(ah);
      }
      {  // [5] Add a 4 vertex gstGeode
        gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
        gstGeode *a = static_cast<gstGeode*>(&(*ah));
        a->AddVertex(v0);
        a->AddVertex(v1);
        a->AddVertex(v2);
        a->AddVertex(v3);
        glist.push_back(ah);
      }
      {  // [6] Add a 1 vertex gstGeode
        gstGeodeHandle ah = gstGeodeImpl::Create(gstPolyLine);
        gstGeode *a = static_cast<gstGeode*>(&(*ah));
        a->AddVertex(v3);
        glist.push_back(ah);
      }
      assert(glist.size() == rlist.size());
      std::map<size_t, std::vector<size_t> > n_vertex_indices;
      for (size_t i = 0; i < glist.size(); ++i) {
        gstGeode *g = static_cast<gstGeode*>(&(*glist[i]));
        n_vertex_indices[g->VertexCount(0)].push_back(i);
      }
      assert(n_vertex_indices[n_vertex_indices.rbegin()->first + 1].size()
             == 0);
      size_t num_geodes_with_less_than_next = 0;
      for (std::map<size_t, std::vector<size_t> >::iterator i =
           n_vertex_indices.begin(); i != n_vertex_indices.end();
           num_geodes_with_less_than_next += i->second.size(), ++i) {
        size_t num_vertices_to_be_not_empty = i->first;
        GeodeList glist_copy(glist);
        RecordList rlist_copy(rlist);
        size_t remaining_vertices = gstSelector::RemoveEmptyFeatures(
            &glist_copy, &rlist_copy, num_vertices_to_be_not_empty);
        assert(glist_copy.size() == rlist_copy.size());
        assert(glist_copy.size() == (glist.size() -
                                     num_geodes_with_less_than_next));
        for (size_t i = 0; i < glist_copy.size(); ++i) {
          gstGeode *gcopy = static_cast<gstGeode*>(&(*glist_copy[i]));
          assert(gcopy->VertexCount(0) >= num_vertices_to_be_not_empty);
          remaining_vertices -= gcopy->VertexCount(0);
        }
        assert(remaining_vertices == 0);
      }
    }
  }
}

void TestMercatorProjection() {
#ifndef NDEBUG
  // use tilesize = 256 to test against maps javascript values
  const unsigned int tile_size = 256;
  const unsigned int num_levels = 4;

  // boundaries of each row
  // from south to north
  double lats_per_level[num_levels][9] = {
    { -85.051129, 85.051129 },
    { -85.051129, 0, 85.051129 },
    { -85.051129, -66.513260, 0, 66.513260, 85.051129 },
    { -85.051129, -79.171335, -66.513260, -40.979898, 0, 40.979898, 66.513260, 79.171335, 85.051129 }
  };
  const double precision = 0.000001;

  // validate lat/lng -> pixel
  MercatorProjection* proj = new MercatorProjection(tile_size);
  for (unsigned int lev = 0; lev < num_levels; ++lev) {
    for (unsigned int d = 0; d <= (1U << lev); ++d) {
      Projection::Point p = proj->FromLatLngToPixel(Projection::LatLng(lats_per_level[lev][d], 0),lev);
      assert(p.X() == (tile_size << lev) >> 1U);
      assert(p.Y() == d * tile_size);
    }
  }

  // validate pixel -> lat/lng
  for (unsigned int lev = 0; lev < num_levels; ++lev) {
    double step = 360.0 / static_cast<double>(0x01UL << lev);
    for (int x = 0; x <= 1 << lev; ++x) {
      for (int y = 0; y <= 1 << lev; ++y) {
        Projection::LatLng ll =
          proj->FromPixelToLatLng(Projection::Point(tile_size * x, tile_size * y), lev);
        assert(fabs(ll.Lng() - (-180.0 + (step * x))) < precision);
        assert(fabs((ll.Lat() - lats_per_level[lev][y])) < precision);
      }
    }
  }
#endif
}

void TestIntPacker() {
#ifndef NDEBUG
  assert(IntToBase62(static_cast<int>(pow(62, 0)) - 1) == "0");
  assert(IntToBase62(static_cast<int>(pow(62, 1)) - 1) == "Z");
  assert(IntToBase62(static_cast<int>(pow(62, 2)) - 1) == "ZZ");
  assert(IntToBase62(static_cast<int>(pow(62, 3)) - 1) == "ZZZ");
  assert(IntToBase62(static_cast<int>(pow(62, 4)) - 1) == "ZZZZ");
  assert(IntToBase62(static_cast<int>(pow(62, 5)) - 1) == "ZZZZZ");

  assert(IntToBase62(static_cast<int>(pow(62, 0))) == "1");
  assert(IntToBase62(static_cast<int>(pow(62, 1))) == "10");
  assert(IntToBase62(static_cast<int>(pow(62, 2))) == "100");
  assert(IntToBase62(static_cast<int>(pow(62, 3))) == "1000");
  assert(IntToBase62(static_cast<int>(pow(62, 4))) == "10000");
  assert(IntToBase62(static_cast<int>(pow(62, 5))) == "100000");
#endif
}

int main(int argc, char **argv) {
  fprintf(stderr, "Testing vertex angle algorithms\n");
  TestComputeAngle();
  fprintf(stderr, "Testing record and header objects\n");
  TestRecordAndFriends();
  fprintf(stderr, "Testing various value operations\n");
  TestValue();
  fprintf(stderr, "Testing various selector operations\n");
  TestSelector();
  fprintf(stderr, "Testing MercatorProjection\n");
  TestMercatorProjection();
  fprintf(stderr, "Testing IntPacker\n");
  TestIntPacker();
}
