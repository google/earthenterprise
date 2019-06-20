
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





Open GEE New Imagery Resource Command Test
==========================================
Testing the Command: genewimageryresource


GEE new imagery test: Simple Test
--------------------------------
* perform GEE new imagery resource simple test

GEE new imagery test: Root Directory Test
----------------------------------------
* perform GEE new imagery resource root directory test

GEE new imagery test: Multi Level Directory Test
-----------------------------------------------
* perform GEE new imagery resource multi level directory test

GEE new imagery test: Unacceptable Characters Test
-------------------------------------------------
* perform GEE new imagery resource having unacceptable characters test

GEE new imagery test: Unspecified Special Characters Test
--------------------------------------------------------
* perform GEE new imagery resource having unspecified special characters test

GEE new imagery test: Empty Directory Test
-----------------------------------------
* perform GEE new imagery resource having empty directory test

