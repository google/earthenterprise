# Integration Tests

This folder contains integration tests for Google Earth Enterprise.
Tests are written using [Gauge](https://gauge.org/).
Set up and clean up require super user priviledges, so these steps
are performed by separate scripts.
To run these tests, do the following:

1. Install test framework
    1. [Install Gauge](https://gauge.org/get_started/#yum)
    2. Install gauge plugins using python pip: `getguage`, `colorama` and `requests`
2. Build and install GEE Fusion and Server
    1. Be sure to leave the asset root at the default location (`/gevol/assets`)
4. Run `test_setup.sh`
    1. This may delete some assets. Do not run these tests on a production system.
5. Run the tests: `gauge run specs`
6. Optionally run `test_teardown.sh` to remove the assets created by the tests.
