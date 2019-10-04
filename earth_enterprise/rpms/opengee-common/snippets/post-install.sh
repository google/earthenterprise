#!/bin/bash
#
# Copyright 2019 the Open GEE Contributors
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


#------------------------------------------------------------------------------
add_python27_libs_to_ldconf()
{
    bash -c 'echo "/opt/rh/python27/root/usr/lib64" >> /etc/ld.so.conf.d/opengee.conf'
    ldconfig
}

#-----------------------------------------------------------------
# Main Function
#-----------------------------------------------------------------

if [[ ! -f /etc/os-release && -f /etc/redhat-release ]]; then
    add_python27_libs_to_ldconf
fi

#-----------------------------------------------------------------