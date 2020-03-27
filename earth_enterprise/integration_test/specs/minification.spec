
text underlined with = chars denotes the spec name
text underlined with - chars denotes the scenario name
text that starts with * chars denote steps

' Guideline for format:
PRE-TEST steps go here.   These steps will be run before EACH and EVERY test (scenario)
' * Example setup step to be performed.


Example for scenario including cleanups (aka POST-STEP):
' * Example clean up steps to be performed.
' -----------------------------------------
' Tags: Comma separated list of Tag Names
' * Example steps
' -----------------------------------------
' POST-STEPs for this SCENARIO go here.

'##########################################
'# DO NOT DELETE ANYTHING ABOVE THIS LINE #
'##########################################


Test Open GEE Minification Settings
==========================================
Test that Fusion respects the UseMinification setting in misc.xml



GEE minification test: Test that images built with minification have more dependencies
------------------------------------------------
Tags: build, minification, minify, resources, misc.xml
* Turn minification "on"
* Restart fusion
* Create imagery resource "blue_marble_minify" from "Imagery/bluemarble_4km.tif"
* Create imagery resource "sf_hires_minify" from "Imagery/usgsSFHiRes.tif"
* Create imagery project "with_minification"
* Add imagery resource "blue_marble_minify" to project "with_minification"
* Add imagery resource "sf_hires_minify" to project "with_minification"
* Build imagery project "with_minification"
* Wait for imagery project "with_minification" to reach state "Succeeded"
* Turn minification "off"
* Restart fusion
* Create imagery resource "blue_marble_no_minify" from "Imagery/bluemarble_4km.tif"
* Create imagery resource "sf_hires_no_minify" from "Imagery/usgsSFHiRes.tif"
* Create imagery project "without_minification"
* Add imagery resource "blue_marble_no_minify" to project "without_minification"
* Add imagery resource "sf_hires_no_minify" to project "without_minification"
* Build imagery project "without_minification"
* Wait for imagery project "without_minification" to reach state "Succeeded"
* Verify project "with_minification" has "119" dependencies and project "without_minification" has "101"
* Turn minification "on"
* Restart fusion
