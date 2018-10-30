# Integration Tests

This folder contains integration tests for Open GEE.
Tests are written using [Gauge](https://gauge.org/).
Set up and tear down require super user priviledges, so these steps
are performed by separate scripts.
The set up and tear down scripts DELETE assets. Do not run these
scripts on production systems.
To run these tests, do the following:

1. Install test framework
    1. [Install Gauge](https://docs.gauge.org/latest/installation.html)
    2. Install packages used by the tests:

        ```bash
        sudo yum -y install python-pip wget
        sudo pip install getgauge colorama requests
        ```

2. Build and install GEE Fusion and Server
    1. Be sure to leave the asset root at the default location (`/gevol/assets`)
4. Run `test_setup.sh`
5. Run the tests: `gauge run specs`
6. Optionally run `test_teardown.sh` to remove the assets created by the tests.

Note: Gauge supports Python 2.7 and above. CentOS 6 and RHEL 6 ship with Python 2.6,
so Gauge tests will not run on those platforms.
