# Integration Tests

This folder contains integration tests for Google Earth Enterprise.
Tests are written using [Gauge](https://gauge.org/).
Set up and clean up require super user priviledges, so these steps
are performed by separate scripts.
To run these tests, do the following:

1. [Install Gauge](https://gauge.org/get_started/#yum)
1. Build and install GEE Fusion and Server
    1. Be sure to leave the asset root at the default location (`/gevol/assets`)
1. Download the tutorial files to the default location by running `/opt/google/share/tutorials/fusion/download_tutorial.sh`
1. Run `clean_test_setup.sh`
    1. This may delete some assets. Do not run these tests on a production system.
1. Run the tests: `gauge run specs`
1. Optionally run `clean_test_setup.sh` again to remove the assets created by the tests.
