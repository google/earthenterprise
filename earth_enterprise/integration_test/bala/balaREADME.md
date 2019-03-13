# genewimageryresource function test

This folder contains integration tests for Open GEE tool named genewimageryresource.
Tests are written using Bash and Python
To run these tests, do the following:

1. Execute: ./balaOutputPathTestMain.sh [ This will be changed to a python script soon ]
    1. No inputs are necessary
    2. Internal Controls:
       The location of tool can be controlled through the script.
       The directory where the output of the genewimageryresource should go to, is controlled through the script.
        ```bash
        ./balaOutputPathTestMain.sh
        ```
2. Results will be populated with PASS or FAIL, and the description should be able ot help identify the testcases that are failing.
3. TBD: Need to perform testing for all other parameters including direction, and everything else.
