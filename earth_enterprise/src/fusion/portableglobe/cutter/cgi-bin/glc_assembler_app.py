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


"""Supports web interface for assembling a glc."""

import cgi
from common import form_wrap
from common import utils
from core import glc_assembler


def main():

  assembler = glc_assembler.GlcAssembler()
  utils.WriteHeader("text/plain")
  form = form_wrap.FormWrap(cgi.FieldStorage(), do_sanitize=True)
  cmd = form.getvalue("cmd")

  if cmd == "ASSEMBLE_JOBINFO":
    msg = assembler.GetJobInfo(form)

  elif cmd == "ASSEMBLE_GLC":
    msg = assembler.AssembleGlc(form)

  elif cmd == "ASSEMBLE_DONE":
    msg = assembler.IsDone("geportableglcpacker", form)

  elif cmd == "BUILD_SIZE":
    msg = assembler.GetSize(form)

  elif cmd == "CLEAN_UP":
    msg = assembler.CleanUp(form)

  elif cmd == "GLOBE_INFO":
    msg = assembler.GetGlobeInfo(form)

  elif cmd == "DISASSEMBLE_GLC":
    msg = assembler.DisassembleGlc(form)

  else:
    msg = "Unknown command: %s" % cmd

  if msg:
    print msg

if __name__ == "__main__":
  main()
