#!/usr/bin/python2.4
#
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
#

"""Script to create and display dynamic content on messagebox to user."""


import os
import sys
import Tkinter


class PortableWidgets(object):
  """Create Widget for handling user inputs."""

  def __init__(self, root, input_str):
    """initialize widget veriables."""
    frame = Tkinter.Frame(root)
    self.InfoTextFrame(frame, input_str)
    frame.grid(sticky=Tkinter.E + Tkinter.W + Tkinter.N + Tkinter.S)
    frame.rowconfigure(0, weight=1)
    frame.rowconfigure(1, weight=0)
    frame.columnconfigure(0, weight=1)
    frame.columnconfigure(1, weight=0)
    return

  def InfoTextFrame(self, frame, input_str):
    """Create the textframe for  diaplying information."""
    # put a scroll bar in the frame
    scroll = Tkinter.Scrollbar(frame)
    scroll.grid(row=0, column=1, sticky=Tkinter.E + Tkinter.N + Tkinter.S)
    width_ = 80
    length_ = len(input_str)
    # Create a text linked to the scroll and write string to a Text
    if length_ > 125:
      width_ = 125
    height_ = length_ / width_
    if height_ < 4:
      height_ = 4
    elif height_ > 40:
      height_ = 40
    text = Tkinter.Text(frame, height=height_, width=width_,
                        yscrollcommand=scroll.set, background='white',
                        wrap=Tkinter.WORD)
    text.grid(row=0, column=0,
              sticky=Tkinter.E + Tkinter.W + Tkinter.N + Tkinter.S)

    text.insert(Tkinter.END, input_str)
    text.config(state=Tkinter.DISABLED)

    # With scroll button click, let the text scroll
    scroll.config(command=text.yview)

    # Add the choice buttons
    button_frame = Tkinter.Frame(frame)
    button_frame.grid(row=1,
                      sticky=Tkinter.E + Tkinter.W + Tkinter.N + Tkinter.S)
    button_frame.columnconfigure(0, weight=1)
    button_frame.columnconfigure(1, weight=1)
    w1 = Tkinter.Button(button_frame, text='Yes', command=self.ResponceForYes)
    w1.grid(row=1, column=0, sticky=Tkinter.E, padx=8, pady=8)
    w2 = Tkinter.Button(button_frame, text='No', command=self.ResponceForNo)
    w2.grid(row=1, column=1, sticky=Tkinter.W, padx=8, pady=8)
    return

  def ResponceForYes(self):
    sys.exit(0)

  def ResponceForNo(self):
    sys.exit(1)


def main(argv):
  """Accept user inputs retrun user responce in status code to batch file."""
  if os.path.isfile(argv[2]):
    fp = open(argv[2], 'r')
    input_str = fp.read()
    fp.close()
  else:
    input_str = argv[2]
  root = Tkinter.Tk()
  PortableWidgets(root, input_str)
  root.title(argv[1])
  root.rowconfigure(0, weight=1)
  root.columnconfigure(0, weight=1)
  root.mainloop()

if __name__ == '__main__':
  main(sys.argv)
