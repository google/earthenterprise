Copyright 2017 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Google Earth Fusion Taskrules for General Use

New: Look towards the bottom of this document and see the combined terrain
taskrule and the map tile generation taskrule

There are a number of taskrules that can be implemented to modify the way
that Fusion performs its processing. The two taskrules of most general use
are the PacketLevel taskrule and the KRP/KRMP taskrules. The PacketLevel
taskrule can speed up the building of imagery projects by having more than
one CPU process individual pack files. Note that this does not
speed the creation of an imagery resource, just the processing of the
project. The KRP/KRMP taskrules assist in managing disk space by writing
the very large raster.kip and mask.kmp directories to a volume other than
the default assetroot directory.  These task rules can be used individually
or all can be used. The task rules will apply to all subsequent tasks that
a Fusion server performs, and can not be applied to individual assets or
projects.



The selected taskrule files must be placed in the .config directory
of the assetroot, and they  must be called PacketLevel.taskrule, KRP.taskrule
and KRMP.taskrule (etc., exactly as named below).
For example:
sudo cp /opt/google/share/taskrules/CombinedTerrain.taskrule /gevol/assets/.config

In order for these taskrules to be used, the system manager
must be restarted on the fusion server. The /etc/init.d/gesystem stop and start
commands will restart the system manager.


WARNING: Be careful when cutting and pasting from Google Docs or any other
formatted document like a PDF. Often times, special characters get embedded
into the resulting text and will throw off the Linux server when you create
the taskrule file. You can "save as" text on the doc, or use "od -c" to look
for non-ascii characters.


PacketLevel.taskrule

This taskrule will use 2 CPU's to work on individual Pack files.
The default for Fusion is to have one cpu working on each Pack file.
It does not need to be modified and can be used as-is. Pack files are created
during the building of a project, so this taskrule should help speed up the
project builds. Google has found that adding more than two CPU's will
not necessarily speed up the pack file processing, so the maxNumCPU value
should be left at 2.

PacketLevel.taskrule

<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<TaskRule>
   <taskname>PacketLevel</taskname>
   <inputConstraints/>
   <outputConstraints/>
   <cpuConstraint>
        <minNumCPU>2</minNumCPU>
        <maxNumCPU>2</maxNumCPU>
   </cpuConstraint>
</TaskRule>

KRP/KRMP taskrules

The KRP taskrule will put the large files that normally are written to
raster.kip  or mask.kmp somewhere other than the asset root during the creation
of an asset.  The KRP taskrule applies to the large files that are normally
placed in a raster.kip directory under the assets directory.
The KRMP taskrule applies to the smaller files that are normally placed in the
mask.kmp directory. These task rules can be used individually, or
both can be used.

The taskrules listed below were written assuming that there is a volume called
products available to Fusion. This volume can be set up using the
khconfigure command. The volumes.xml file will contain an entry that looks
something like:

<products>
      <netpath>/gevol/products</netpath>
      <host>linux</host>
      <localpath>/gevol/products</localpath>
      <reserveSpace>100000</reserveSpace>
      <isTmp>0</isTmp>
</products>

For this example, assume that we are building an asset called TestImage under
the imagery directory of the asset root, and the assetroot is
/gevol/assets. Normally, the large imagery files would be placed under
/gevol/assets/imagery/TestImage.kiasset/product.kia/ver001/raster.kip and
the smaller mask files would be put under
/gevol/assets/imagery/TestImage.kiasset/maskproduct.kia/ver001/mask.kmp.
Using the task rules shown below, the new directories for these files will be:



Imagery:    /gevol/products/imagery/TestImage-001.kip

Mask:       /gevol/products/imagery/TestImage-001.kmp



This taskrule needs to be modified to specify the volume that the  files will
be written to. The <requiredVolume> tag should specify a volume configured for
use by Fusion that has sufficient space to hold the files. Once this volume
fills up, the next available volume can be entered in the requiredVolume tag,
the system manager restarted, and the files will be written to the next volume.

KRP.taskrule

<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<TaskRule>
   <taskname>KRP</taskname>
   <inputConstraints/>
   <outputConstraints>
      <outputConstraint>
         <num>0</num>
         <requiredVolume>products</requiredVolume>
         <pathPattern>${assetref:dirname:sansext}-${vernum}${defaultpath:ext}</pathPattern>
      </outputConstraint>
   </outputConstraints>
   <cpuConstraint>
      <minNumCPU>1</minNumCPU>
      <maxNumCPU>1</maxNumCPU>
   </cpuConstraint>
</TaskRule>

KRMP.taskrule

<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<TaskRule>
   <taskname>KRMP</taskname>
   <inputConstraints/>
   <outputConstraints>
      <outputConstraint>
         <num>0</num>
         <requiredVolume>products</requiredVolume>
         <pathPattern>${assetref:dirname:sansext}-${vernum}${defaultpath:ext}</pathPattern>
      </outputConstraint>
   </outputConstraints>
   <cpuConstraint>
      <minNumCPU>1</minNumCPU>
      <maxNumCPU>1</maxNumCPU>
   </cpuConstraint>
</TaskRule>


Combined Terrain Taskrule:

The recommended usage of combineterrain is to put all available CPUs/cores onto
the build, assuming that 1 core is reserved for the system (i.e., Min/max 7
where maxjobs = 7 on a 2 quad-core CPU system).

CombinedTerrain.taskrule

<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<TaskRule>
   <taskname>CombinedTerrain</taskname>
   <inputConstraints/>
   <outputConstraints/>
   <cpuConstraint>
        <minNumCPU>4</minNumCPU>
        <maxNumCPU>4</maxNumCPU>
   </cpuConstraint>
</TaskRule>

Map Tile Generation Taskrule:

Maptilegen should be set to Min/Max 4 on a 2 dual-core CPU system (3 may work
well enough).  On a 2 quad-core system, I would up this to Min/Max 3 so 2 total
jobs would work.

MapLayerLevel.taskrule

<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<TaskRule>
   <taskname>MapLayerLevel</taskname>
   <inputConstraints/>
   <outputConstraints/>
   <cpuConstraint>
        <minNumCPU>4</minNumCPU>
        <maxNumCPU>4</maxNumCPU>
   </cpuConstraint>
</TaskRule>


