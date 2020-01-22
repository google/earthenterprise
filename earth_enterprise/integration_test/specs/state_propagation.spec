State Propagation Correctness Tests
===================================

Test handling and propagation of state changes.

These tests assume you have installed Fusion and that the asset root is at
/gevol/assets.

Before running these tests you must run `test_setup.sh` located in the same
directory as this file.  You can also run `test-teardown.sh` to clean up after
these tests. WARNING: these scripts DELETE assets under /gevol/assets and
require privileged access to run.

Some of these tests are timing dependent. For example, if we are testing that
the state of a particular asset is "InProgress", that step will fail if the
build completes too quickly and the asset goes to the "Succeeded" state.

## Build, Clean and Rebuild Project Test
Tags: basic, build, clean, rebuild

Build
* Create and build default project "StatePropagationTest_BasicBuild"
* Wait for imagery project "StatePropagationTest_BasicBuild" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BasicBuild" is "Succeeded"

Clean
* Clean imagery project "StatePropagationTest_BasicBuild"
* Verify that the state of imagery project "StatePropagationTest_BasicBuild" is "Cleaned"
* Verify that the state of images for default project "StatePropagationTest_BasicBuild" is "Succeeded"

Rebuild
* Build imagery project "StatePropagationTest_BasicBuild"
* Wait for imagery project "StatePropagationTest_BasicBuild" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BasicBuild" is "Succeeded"

Build after build finished
* Build imagery project "StatePropagationTest_BasicBuild"
* Verify that the state of images for default project "StatePropagationTest_BasicBuild" is "Succeeded"

## Cancel and Resume Project Before Packgen
Tags: basic, build, clean, rebuild

Build
* Create and build default project "StatePropagationTest_CancelProjectEarly"

Cancel
* Cancel imagery project "StatePropagationTest_CancelProjectEarly"
* Verify that the state of images for default project "StatePropagationTest_CancelProjectEarly" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
* Verify that the state of imagery project "StatePropagationTest_CancelProjectEarly" is "Canceled"

Resume
* Resume imagery project "StatePropagationTest_CancelProjectEarly"
* Verify that the state of imagery project "StatePropagationTest_CancelProjectEarly" is "Waiting"
* Wait for imagery project "StatePropagationTest_CancelProjectEarly" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CancelProjectEarly" is "Succeeded"

## Cancel, Resume, Clean, and Rebuild Resource While Project is Building
Tags: cancel, clean, resume, rebuild

Build
* Create and build default project "StatePropagationTest_CancelResourceDuringBuild"

Cancel
* Cancel imagery resource "BlueMarble_CancelResourceDuringBuild"
* Verify that the state of imagery resource "BlueMarble_CancelResourceDuringBuild" is "Canceled"
* Verify that the state of imagery resource "i3SF15meter_CancelResourceDuringBuild" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "USGSLanSat_CancelResourceDuringBuild" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "SFHiRes_CancelResourceDuringBuild" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery project "StatePropagationTest_CancelResourceDuringBuild" is "Blocked"

Resume
* Resume imagery resource "BlueMarble_CancelResourceDuringBuild"
* Verify that the state of imagery resource "BlueMarble_CancelResourceDuringBuild" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
* Verify that the state of imagery project "StatePropagationTest_CancelResourceDuringBuild" is "Waiting"
* Wait for imagery project "StatePropagationTest_CancelResourceDuringBuild" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CancelResourceDuringBuild" is "Succeeded"

Clean
* Clean imagery resource "BlueMarble_CancelResourceDuringBuild"
* Verify that the state of imagery resource "BlueMarble_CancelResourceDuringBuild" is "Cleaned"
* Verify that the state of imagery project "StatePropagationTest_CancelResourceDuringBuild" is "Succeeded"
* Verify that the state of imagery resource "i3SF15meter_CancelResourceDuringBuild" is "Succeeded"
* Verify that the state of imagery resource "USGSLanSat_CancelResourceDuringBuild" is "Succeeded"
* Verify that the state of imagery resource "SFHiRes_CancelResourceDuringBuild" is "Succeeded"

Rebuild
* Build imagery project "StatePropagationTest_CancelResourceDuringBuild"
* Verify that the state of imagery project "StatePropagationTest_CancelResourceDuringBuild" is "Waiting"
* Wait for imagery project "StatePropagationTest_CancelResourceDuringBuild" to reach state "Succeeded"
* Verify that the state of imagery resource "BlueMarble_CancelResourceDuringBuild" is "Succeeded"

## Cancel, Resume, Clean, and Rebuild Project During Packgen
Tags: cancel, clean, resume, rebuild

Build
* Create and build default project "StatePropagationTest_CancelProjectLate"
* Wait for imagery project "StatePropagationTest_CancelProjectLate" to reach state "Queued"

Cancel
* Cancel imagery project "StatePropagationTest_CancelProjectLate"
* Verify that the state of images for default project "StatePropagationTest_CancelProjectLate" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_CancelProjectLate" is "Canceled"

Resume
* Resume imagery project "StatePropagationTest_CancelProjectLate"
* Verify that the state of images for default project "StatePropagationTest_CancelProjectLate" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_CancelProjectLate" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Wait for imagery project "StatePropagationTest_CancelProjectLate" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CancelProjectLate" is "Succeeded"

Clean
* Clean imagery project "StatePropagationTest_CancelProjectLate"
* Verify that the state of imagery project "StatePropagationTest_CancelProjectLate" is "Cleaned"
* Verify that the state of images for default project "StatePropagationTest_CancelProjectLate" is "Succeeded"

Rebuild
* Build imagery project "StatePropagationTest_CancelProjectLate"
* Wait for imagery project "StatePropagationTest_CancelProjectLate" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CancelProjectLate" is "Succeeded"

## Clean After Cancel
Tags: cancel, clean, rebuild

Build
* Create and build default project "StatePropagationTest_CleanAfterCancel"

Cancel Resource
* Cancel imagery resource "BlueMarble_CleanAfterCancel"
* Verify that the state of imagery resource "BlueMarble_CleanAfterCancel" is "Canceled"
* Verify that the state of imagery resource "i3SF15meter_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "USGSLanSat_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "SFHiRes_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery project "StatePropagationTest_CleanAfterCancel" is "Blocked"

Clean Resource
* Clean imagery resource "BlueMarble_CleanAfterCancel"
* Verify that the state of imagery resource "BlueMarble_CleanAfterCancel" is "Cleaned"
* Verify that the state of imagery project "StatePropagationTest_CleanAfterCancel" is "Blocked"
* Verify that the state of imagery resource "i3SF15meter_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "USGSLanSat_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "SFHiRes_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |

Rebuild Resource
* Build imagery resource "BlueMarble_CleanAfterCancel"
* Verify that the state of images for default project "StatePropagationTest_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
The project will not start building again without user input
* Resume imagery project "StatePropagationTest_CleanAfterCancel"

Cancel Project
* Cancel imagery project "StatePropagationTest_CleanAfterCancel"
* Verify that the state of images for default project "StatePropagationTest_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery project "StatePropagationTest_CleanAfterCancel" is "Canceled"

Clean Project
* Clean imagery project "StatePropagationTest_CleanAfterCancel"
* Verify that the state of images for default project "StatePropagationTest_CleanAfterCancel" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery project "StatePropagationTest_CleanAfterCancel" is "Cleaned"

Rebuild Project
* Build imagery project "StatePropagationTest_CleanAfterCancel"
* Wait for imagery project "StatePropagationTest_CleanAfterCancel" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CleanAfterCancel" is "Succeeded"

## Clean and Cancel Project and Resource, Then Rebuild Project
Tags: cancel, clean, resume, rebuild

Build
* Create and build default project "StatePropagationTest_CleanCancelRebuildProject"

Cancel Resource and Project
* Cancel imagery resource "BlueMarble_CleanCancelRebuildProject"
* Verify that the state of imagery resource "BlueMarble_CleanCancelRebuildProject" is "Canceled"
* Cancel imagery project "StatePropagationTest_CleanCancelRebuildProject"
* Verify that the state of imagery project "StatePropagationTest_CleanCancelRebuildProject" is "Canceled"

Rebuild Project
* Build imagery project "StatePropagationTest_CleanCancelRebuildProject"
* Verify that the state of imagery project "StatePropagationTest_CleanCancelRebuildProject" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of imagery resource "BlueMarble_CleanCancelRebuildProject" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |

Cancel Resource and Project
* Cancel imagery resource "BlueMarble_CleanCancelRebuildProject"
* Verify that the state of imagery resource "BlueMarble_CleanCancelRebuildProject" is "Canceled"
* Cancel imagery project "StatePropagationTest_CleanCancelRebuildProject"
* Verify that the state of imagery project "StatePropagationTest_CleanCancelRebuildProject" is "Canceled"

Clean Resource and Project
* Clean imagery resource "BlueMarble_CleanCancelRebuildProject"
* Verify that the state of imagery resource "BlueMarble_CleanCancelRebuildProject" is "Cleaned"
* Clean imagery project "StatePropagationTest_CleanCancelRebuildProject"
* Verify that the state of imagery project "StatePropagationTest_CleanCancelRebuildProject" is "Cleaned"

Rebuild Project
* Build imagery project "StatePropagationTest_CleanCancelRebuildProject"
* Verify that the state of imagery project "StatePropagationTest_CleanCancelRebuildProject" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "BlueMarble_CleanCancelRebuildProject" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Wait for imagery project "StatePropagationTest_CleanCancelRebuildProject" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CleanCancelRebuildProject" is "Succeeded"

## Resources Belonging to Multiple Projects
Tags: multiproject, build, cancel, clean, rebuild

Set up
* Create imagery project "StatePropagationTest_MultiProject1"
* Create imagery project "StatePropagationTest_MultiProject2"
* Create imagery resource "BlueMarble_MultiProject" from "Imagery/bluemarble_4km.tif"
* Add imagery resource "BlueMarble_MultiProject" to project "StatePropagationTest_MultiProject1"
* Add imagery resource "BlueMarble_MultiProject" to project "StatePropagationTest_MultiProject2"
* Create imagery resource "USGSLanSat_MultiProject" from "Imagery/usgsLanSat.tif"
* Add imagery resource "USGSLanSat_MultiProject" to project "StatePropagationTest_MultiProject1"
* Add imagery resource "USGSLanSat_MultiProject" to project "StatePropagationTest_MultiProject2"

Build
* Build imagery project "StatePropagationTest_MultiProject1"
* Build imagery project "StatePropagationTest_MultiProject2"

Cancel Resource
* Cancel imagery resource "USGSLanSat_MultiProject"
* Verify that the state of imagery resource "USGSLanSat_MultiProject" is "Canceled"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Blocked"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Blocked"

Build Resource
* Build imagery resource "USGSLanSat_MultiProject"
* Verify that the state of imagery resource "USGSLanSat_MultiProject" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Waiting"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Waiting"

Cancel and Clean Resource
* Cancel imagery resource "USGSLanSat_MultiProject"
* Clean imagery resource "USGSLanSat_MultiProject"
* Verify that the state of imagery resource "USGSLanSat_MultiProject" is "Cleaned"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Blocked"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Blocked"

Build Resource
* Build imagery resource "USGSLanSat_MultiProject"
* Verify that the state of imagery resource "USGSLanSat_MultiProject" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
* Build imagery project "StatePropagationTest_MultiProject1"
* Build imagery project "StatePropagationTest_MultiProject2"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Waiting"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Waiting"

Cancel Project
* Cancel imagery project "StatePropagationTest_MultiProject1"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Canceled"
* Verify that the state of imagery resource "USGSLanSat_MultiProject" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Waiting"

Clean Project
* Clean imagery project "StatePropagationTest_MultiProject1"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Cleaned"
* Verify that the state of imagery resource "USGSLanSat_MultiProject" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Waiting"

Wait for project 2 to build
* Wait for imagery project "StatePropagationTest_MultiProject2" to reach state "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Cleaned"
* Verify that the state of imagery resource "BlueMarble_MultiProject" is "Succeeded"
* Verify that the state of imagery resource "USGSLanSat_MultiProject" is "Succeeded"

Finish building project 1
* Build imagery project "StatePropagationTest_MultiProject1"
* Wait for imagery project "StatePropagationTest_MultiProject1" to reach state "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Succeeded"
* Verify that the state of imagery resource "BlueMarble_MultiProject" is "Succeeded"
* Verify that the state of imagery resource "USGSLanSat_MultiProject" is "Succeeded"

Clean resource after build
* Clean imagery resource "USGSLanSat_MultiProject"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Succeeded"
* Build imagery resource "USGSLanSat_MultiProject"
* Build imagery project "StatePropagationTest_MultiProject1"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Waiting"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Succeeded"
* Cancel imagery resource "USGSLanSat_MultiProject"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Blocked"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Succeeded"
* Build imagery resource "USGSLanSat_MultiProject"
* Verify that the state of imagery project "StatePropagationTest_MultiProject1" is "Waiting"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Succeeded"
* Wait for imagery project "StatePropagationTest_MultiProject1" to reach state "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_MultiProject2" is "Succeeded"

## Set Bad and Good
Tags: bad, build 

Set up and build
* Create and build default project "StatePropagationTest_BadAndGood" 
* Create database "Database_BadAndGood" from imagery project "StatePropagationTest_BadAndGood"
* Build database "Database_BadAndGood"
* Wait for database "Database_BadAndGood" to reach state "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_BadAndGood" is "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BadAndGood" is "Succeeded"

Mark resource bad
* Mark imagery resource "BlueMarble_BadAndGood" bad
* Verify that the state of database "Database_BadAndGood" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_BadAndGood" is "Succeeded"
* Verify that the state of imagery resource "BlueMarble_BadAndGood" is "Bad"
* Verify that the state of imagery resource "i3SF15meter_BadAndGood" is "Succeeded"
* Verify that the state of imagery resource "USGSLanSat_BadAndGood" is "Succeeded"
* Verify that the state of imagery resource "SFHiRes_BadAndGood" is "Succeeded"

Try to build the bad resource
* Build imagery resource "BlueMarble_BadAndGood"
* Verify that the state of imagery resource "BlueMarble_BadAndGood" is "Bad"

Mark resource good
* Mark imagery resource "BlueMarble_BadAndGood" good
* Verify that the state of database "Database_BadAndGood" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_BadAndGood" is "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BadAndGood" is "Succeeded"

Mark project bad
* Mark imagery project "StatePropagationTest_BadAndGood" bad
* Verify that the state of database "Database_BadAndGood" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_BadAndGood" is "Bad"
* Verify that the state of images for default project "StatePropagationTest_BadAndGood" is "Succeeded"

Mark project good
* Mark imagery project "StatePropagationTest_BadAndGood" good
* Verify that the state of database "Database_BadAndGood" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_BadAndGood" is "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BadAndGood" is "Succeeded"

Mark database bad
* Mark database "Database_BadAndGood" bad
* Verify that the state of database "Database_BadAndGood" is "Bad"
* Verify that the state of imagery project "StatePropagationTest_BadAndGood" is "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BadAndGood" is "Succeeded"

Mark database good
* Mark database "Database_BadAndGood" good
* Verify that the state of database "Database_BadAndGood" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_BadAndGood" is "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BadAndGood" is "Succeeded"

## Cancel Resource Before Building Database or Project
Tags: cancel, build 

Set up
* Create imagery project "StatePropagationTest_CancelBeforeBuild"
* Create imagery resource "BlueMarble_CancelBeforeBuild" from "Imagery/bluemarble_4km.tif" and add to project "StatePropagationTest_CancelBeforeBuild"
* Create database "Database_CancelBeforeBuild" from imagery project "StatePropagationTest_CancelBeforeBuild"
* Build imagery resource "BlueMarble_CancelBeforeBuild"

Cancel resource before the project or database is built
* Cancel imagery resource "BlueMarble_CancelBeforeBuild"
* Verify that the state of imagery resource "BlueMarble_CancelBeforeBuild" is "Canceled"
* Verify that the state of imagery project "StatePropagationTest_CancelBeforeBuild" is "Bad"
* Verify that the state of database "Database_CancelBeforeBuild" is "Bad"

Build the database
* Build database "Database_CancelBeforeBuild"
* Verify that the state of imagery resource "BlueMarble_CancelBeforeBuild" is in
  | State      |
  |------------|
  | Queued     |
  | InProgress |
* Verify that the state of imagery project "StatePropagationTest_CancelBeforeBuild" is "Waiting"
* Verify that the state of database "Database_CancelBeforeBuild" is "Waiting"
* Wait for database "Database_CancelBeforeBuild" to reach state "Succeeded"
* Verify that the state of imagery resource "BlueMarble_CancelBeforeBuild" is "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_CancelBeforeBuild" is "Succeeded"

## Build Database with Failed Resources
Tags: failed, build, rebuild
Try to build failed resource
* Build imagery resource "StatePropagationTest_FailedImageryResource"
* Verify that the state of imagery resource "StatePropagationTest_FailedImageryResource" is "Failed"

Create project with failed resource
* Create imagery project "StatePropagationTest_FailedResources"
* Add imagery resource "StatePropagationTest_FailedImageryResource" to project "StatePropagationTest_FailedResources"
* Create imagery resource "BlueMarble_FailedResources" from "Imagery/bluemarble_4km.tif" and add to project "StatePropagationTest_FailedResources"
* Build imagery project "StatePropagationTest_FailedResources"
* Wait for imagery project "StatePropagationTest_FailedResources" to reach state "Blocked"

Try to build blocked project
* Build imagery project "StatePropagationTest_FailedResources"
* Verify that the state of imagery project "StatePropagationTest_FailedResources" is "Blocked"

Build database with failed imagery resource project
* Create database "StatePropagationTest_Database_FailedResources" from imagery project "StatePropagationTest_FailedResources"
* Build database "StatePropagationTest_Database_FailedResources"
* Wait for database "StatePropagationTest_Database_FailedResources" to reach state "Blocked"

Rebuild failed database without failed resource
* Drop imagery resource "StatePropagationTest_FailedImageryResource" from project "StatePropagationTest_FailedResources"
* Build database "StatePropagationTest_Database_FailedResources"
* Wait for database "StatePropagationTest_Database_FailedResources" to reach state "Succeeded"


## Map Projects and Mercator Assets
Tags: mercator, build
We do not test Map and Mercator assets as thoroughly as flat assets since much
of the processing is the same.

Set up
* Create mercator imagery project "StatePropagationTest_Mercator"
* Create mercator imagery resource "BlueMarble_Mercator" from "Imagery/bluemarble_4km.tif"
* Add mercator imagery resource "BlueMarble_Mercator" to project "StatePropagationTest_Mercator"
* Create vector resource "CA_POIs_Merc" from "Vector/california_popplaces.csv"
* Build vector resource "CA_POIs_Merc"
* Wait for vector resource "CA_POIs_Merc" to reach state "Succeeded"
* Create map layer "StatePropagationTest_Mercator" from resource "CA_POIs_Merc"
* Create map project "StatePropagationTest_Mercator" from layer "StatePropagationTest_Mercator"
* Create map database "Database_Mercator" from imagery project "StatePropagationTest_Mercator" and map project "StatePropagationTest_Mercator"

Build database
* Build database "Database_Mercator"
* Verify that the state of mercator database "Database_Mercator" is "Waiting"
* Verify that the state of map project "StatePropagationTest_Mercator" is "Waiting"
* Verify that the state of map layer "StatePropagationTest_Mercator" is in
  | State      |
  |------------|
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of vector resource "CA_POIs_Merc" is "Succeeded"
* Verify that the state of mercator imagery project "StatePropagationTest_Mercator" is "Waiting"
* Verify that the state of mercator imagery resource "BlueMarble_Mercator" is "InProgress"

Wait for success
* Wait for mercator database "Database_Mercator" to reach state "Succeeded"
* Verify that the state of mercator database "Database_Mercator" is "Succeeded"
* Verify that the state of map project "StatePropagationTest_Mercator" is "Succeeded"
* Verify that the state of map layer "StatePropagationTest_Mercator" is "Succeeded"
* Verify that the state of vector resource "CA_POIs_Merc" is "Succeeded"
* Verify that the state of mercator imagery project "StatePropagationTest_Mercator" is "Succeeded"
* Verify that the state of mercator imagery resource "BlueMarble_Mercator" is "Succeeded"

## Cancel and Modify
Tags: cancel, modify, rebuild

Build
* Create and build default project "StatePropagationTest_CancelModify"

Cancel
* Cancel imagery project "StatePropagationTest_CancelModify"
* Verify that the state of imagery project "StatePropagationTest_CancelModify" is "Canceled"

Remove and cancel resource
* Drop imagery resource "i3SF15meter_CancelModify" from project "StatePropagationTest_CancelModify"
* Cancel imagery resource "i3SF15meter_CancelModify"
* Verify that the state of imagery resource "i3SF15meter_CancelModify" is "Canceled"

Rebuild (resume won't work because we need to create a new version)
* Build imagery project "StatePropagationTest_CancelModify"
* Verify that the state of imagery project "StatePropagationTest_CancelModify" is in
  | State      |
  |------------|
  | Waiting    |
  | Queued     |
  | InProgress |
* Verify that the state of imagery resource "i3SF15meter_CancelModify" is "Canceled"

Cancel
* Cancel imagery project "StatePropagationTest_CancelModify"
* Verify that the state of imagery project "StatePropagationTest_CancelModify" is "Canceled"

Add resource
* Add imagery resource "i3SF15meter_CancelModify" to project "StatePropagationTest_CancelModify"

Rebuild
* Build imagery project "StatePropagationTest_CancelModify"
* Verify that the state of imagery project "StatePropagationTest_CancelModify" is in
  | State      |
  |------------|
  | Waiting    |
  | Queued     |
  | InProgress |
* Wait for imagery project "StatePropagationTest_CancelModify" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CancelModify" is "Succeeded"

## Build and modify
Tags: build, modify, rebuild

Build
* Create and build default project "StatePropagationTest_BuildModify"
* Wait for imagery project "StatePropagationTest_BuildModify" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BuildModify" is "Succeeded"

Remove and clean resource
* Drop imagery resource "i3SF15meter_BuildModify" from project "StatePropagationTest_BuildModify"
* Clean imagery resource "i3SF15meter_BuildModify"
* Verify that the state of imagery resource "i3SF15meter_BuildModify" is "Cleaned"

Rebuild
* Build imagery project "StatePropagationTest_BuildModify"
* Verify that the state of imagery project "StatePropagationTest_BuildModify" is in
  | State      |
  |------------|
  | Waiting    |
  | Queued     |
  | InProgress |
* Verify that the state of imagery resource "i3SF15meter_BuildModify" is "Cleaned"
* Wait for imagery project "StatePropagationTest_BuildModify" to reach state "Succeeded"

Add resource
* Add imagery resource "i3SF15meter_BuildModify" to project "StatePropagationTest_BuildModify"

Rebuild
* Build imagery project "StatePropagationTest_BuildModify"
* Verify that the state of imagery project "StatePropagationTest_BuildModify" is in
  | State      |
  |------------|
  | Waiting    |
  | Queued     |
  | InProgress |
* Wait for imagery project "StatePropagationTest_BuildModify" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_BuildModify" is "Succeeded"

## Clean and modify
Tags: build, clean modify, rebuild

Build
* Create and build default project "StatePropagationTest_CleanModify"
* Wait for imagery project "StatePropagationTest_CleanModify" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CleanModify" is "Succeeded"

Clean
* Clean imagery project "StatePropagationTest_CleanModify"
* Verify that the state of imagery project "StatePropagationTest_CleanModify" is "Cleaned"

Remove and clean resource
* Drop imagery resource "i3SF15meter_CleanModify" from project "StatePropagationTest_CleanModify"
* Clean imagery resource "i3SF15meter_CleanModify"
* Verify that the state of imagery resource "i3SF15meter_CleanModify" is "Cleaned"

Rebuild
* Build imagery project "StatePropagationTest_CleanModify"
* Verify that the state of imagery project "StatePropagationTest_CleanModify" is in
  | State      |
  |------------|
  | Waiting    |
  | Queued     |
  | InProgress |
* Verify that the state of imagery resource "i3SF15meter_CleanModify" is "Cleaned"
* Wait for imagery project "StatePropagationTest_CleanModify" to reach state "Succeeded"

Clean
* Clean imagery project "StatePropagationTest_CleanModify"
* Verify that the state of imagery project "StatePropagationTest_CleanModify" is "Cleaned"

Add resource
* Add imagery resource "i3SF15meter_CleanModify" to project "StatePropagationTest_CleanModify"

Rebuild
* Build imagery project "StatePropagationTest_CleanModify"
* Verify that the state of imagery project "StatePropagationTest_CleanModify" is in
  | State      |
  |------------|
  | Waiting    |
  | Queued     |
  | InProgress |
* Wait for imagery project "StatePropagationTest_CleanModify" to reach state "Succeeded"
* Verify that the state of images for default project "StatePropagationTest_CleanModify" is "Succeeded"

## Database, Terrain, and Vector Tests
Tags: terrain, vector, build
We do not test terrain and vector assets as thoroughly as imagery assets since much of the
processing will be the same.

Set up and Build
* Create imagery project "StatePropagationTest_Database"
* Create terrain project "StatePropagationTest_Database"
* Create vector project "StatePropagationTest_Database"
* Create imagery resource "BlueMarble_Database" from "Imagery/bluemarble_4km.tif" and add to project "StatePropagationTest_Database"
* Create terrain resource "GTopo_Database" from "Terrain/gtopo30_4km.tif" and add to project "StatePropagationTest_Database"
* Create and build vector resource "CA_POIs_Database" from "Vector/california_popplaces.csv" and add to project "StatePropagationTest_Database"
* Create database "Database_DatabaseTest" from imagery project "StatePropagationTest_Database", terrain project "StatePropagationTest_Database", and vector project "StatePropagationTest_Database"
* Build database "Database_DatabaseTest"
* Verify that the state of imagery project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Waiting"

Cancel project
* Cancel imagery project "StatePropagationTest_Database"
* Verify that the state of imagery project "StatePropagationTest_Database" is "Canceled"
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Blocked"

Rebuild project
* Build imagery project "StatePropagationTest_Database"
* Build database "Database_DatabaseTest"
* Verify that the state of imagery project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Waiting"

Cancel resource
* Cancel imagery resource "BlueMarble_Database"
* Verify that the state of imagery resource "BlueMarble_Database" is "Canceled"
* Verify that the state of imagery project "StatePropagationTest_Database" is "Blocked"
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Blocked"

Clean resource
* Clean imagery resource "BlueMarble_Database"
* Verify that the state of imagery resource "BlueMarble_Database" is "Cleaned"
* Verify that the state of imagery project "StatePropagationTest_Database" is "Blocked"
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Blocked"

Rebuild resource
* Build imagery resource "BlueMarble_Database"
* Build imagery project "StatePropagationTest_Database"
* Build database "Database_DatabaseTest"
* Verify that the state of imagery project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Waiting"

Cancel database
* Cancel database "Database_DatabaseTest"
* Verify that the state of imagery project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "BlueMarble_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
* Verify that the state of terrain resource "GTopo_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
* Verify that the state of vector resource "CA_POIs_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Canceled"

Clean database
* Clean database "Database_DatabaseTest"
* Verify that the state of imagery project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "BlueMarble_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
* Verify that the state of terrain resource "GTopo_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
* Verify that the state of vector resource "CA_POIs_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Cleaned"

Rebuild Database
* Build database "Database_DatabaseTest"
* Verify that the state of imagery project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of terrain project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
* Verify that the state of vector project "StatePropagationTest_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Queued     |
  | Succeeded  |
* Verify that the state of imagery resource "BlueMarble_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
* Verify that the state of terrain resource "GTopo_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
* Verify that the state of vector resource "CA_POIs_Database" is in
  | State      |
  |------------|
  | Waiting    |
  | InProgress |
  | Succeeded  |
* Verify that the state of database "Database_DatabaseTest" is "Waiting"

Wait for database to succeed
* Wait for database "Database_DatabaseTest" to reach state "Succeeded"
* Verify that the state of imagery project "StatePropagationTest_Database" is "Succeeded"
* Verify that the state of terrain project "StatePropagationTest_Database" is "Succeeded"
* Verify that the state of vector project "StatePropagationTest_Database" is "Succeeded"
* Verify that the state of imagery resource "BlueMarble_Database" is "Succeeded"
* Verify that the state of terrain resource "GTopo_Database" is "Succeeded"
* Verify that the state of vector resource "CA_POIs_Database" is "Succeeded"
