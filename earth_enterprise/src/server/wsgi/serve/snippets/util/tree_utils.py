#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


"""The tree utilities."""

import json
from serve.snippets.util import sparse_tree


def CompactTree(snippets_json, log):
  """Converts a sparse tree to a dense one.

  Args:
    snippets_json: the sparse store / multi-level dict representing
      the proto end snippet.
    log: logging object.
  Returns:
    The dense snippet tree.
  """
  log.debug("DenseTree: verify, extract_snippets...")
  log.debug("JSON: %s", json.dumps(snippets_json))
  sparse_snippet_tree = json.loads(snippets_json)
  log.debug("TREE: %s",
            json.dumps(sparse_snippet_tree, indent=2, separators=(",", ":")))
  dense_snippet_tree = (
      sparse_tree.CompactArrayIndices(sparse_snippet_tree))
  log.debug("DenseTree done...")
  log.debug("DENSE TREE: %s", str(dense_snippet_tree))
  log.debug("DENSE TREE JSON: %s",
            json.dumps(dense_snippet_tree, indent=2, separators=(",", ":")))
  return dense_snippet_tree


def main():
  pass


if __name__ == "__main__":
  main()
