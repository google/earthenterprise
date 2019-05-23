Copyright 2019 Thermopylae Sciences and Technology

text underlined with = chars denotes the spec name
text underlined with - chars denotes the scenario name
text that starts with * chars denote steps

' Guideline for format:
PRE-TEST steps go here.   These steps will be run before EACH and EVERY test (scenario)
' * Example setup step to be performed.


Example for scenario including cleanups (aka POST-STEP):
' Testing RPM install of OpenGee
' ------------------------------
' Tags: tbd
' * tbd
' ----------------------
' POST-STEPs for this SCENARIO go here.

'##########################################
'# DO NOT DELETE ANYTHING ABOVE THIS LINE #
'##########################################





Ge New Imagery Resource command Test
====================================
Testing the Command: genewimageryresource


ge new imagery test: Simple Test
--------------------------------
Tags: OGCommandgnir01
* perform ge new imagery resource simple test

ge new imagery test: Root Directory Test
----------------------------------------
Tags: OGCommandgnir02
* perform ge new imagery resource root directory test

ge new imagery test: Directory Traversal Test
---------------------------------------------
Tags: OGCommandgnir03
* perform ge new imagery resource directory traversal test

ge new imagery test: Multi Level Directory Test
-----------------------------------------------
Tags: OGCommandgnir04
* perform ge new imagery resource multi level directory test

ge new imagery test: Unacceptable Characters Test
-------------------------------------------------
Tags: OGCommandgnir05
* perform ge new imagery resource having unacceptable characters test

ge new imagery test: Unspecified Special Characters Test
--------------------------------------------------------
Tags: OGCommandgnir06
* perform ge new imagery resource having unspecified special characters test

ge new imagery test: Empty Directory Test
-----------------------------------------
Tags: OGCommandgnir07
* perform ge new imagery resource having empty directory test

