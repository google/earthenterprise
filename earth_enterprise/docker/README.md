# Docker Image for Running Open GEE Earth Server and Fusion Server

## Set up Docker

### Ubuntu

Install Docker as root:

```BASH
# apt-get install docker.io
```

Add users that you want to have access to Docker to the `docker` group:

```BASH
# adduser muser docker
```

### CentOS 7

Install Docker as root:

```BASH
yum install docker

# in case your Docker deamon isn't running:
service docker start
```

Now you can use Docker as the root user.

## Building Images

Before building Open GEE Docker images, make sure you have at least 8 GB of
free disk space.

### Image Configuration

There are a number of options for building and running Open GEE Docker
images:

* Build platform
* Source code origin: from the main repository, a fork, master or another
    branch, or from the current repository clone on your disk
* Port to forward HTTP connections
* Custom image to run, e.g. one that contains extra data such as tutorial
    resources or custom built globes

These options are controlled by environment variables that are passed to the
build scripts. Since it may become inconvenient to pass more than a couple of
values on the command line, you may put the variable definitions in a
shell script file instead, and source it before executing each command.

View the files under
_earthenterprise/earth_enterprise/docker/config_ for examples.

### Build a Build-environment Image

Before being able to compile and build Open GEE in a Docker image, you need a
prerequisite Docker image containing a build environment for the target
platform.

The build environment image is reused every time you rebuild Open GEE. This
should save you time during normal development for targetting another
platform since only the steps of transferring the modified source code and
running the Open GEE build would need to be repeated.

You can build a build-environment image with a command like this:

```BASH
( . config/rhel-7.sh && ./bin/build-build-env-image.sh )
```

#### Recovering a RHEL Subscription for a Deleted Image

If a build on RHEL fails during package building and installation, the build
script may terminate while the RHEL installation in the Docker container is
still registered with `subscription-manager`. If you still have the layer of
the terminated container, you can `commit` it to a new image, run a shell in
it, and perform `subscription-manager unregister`.

If you have already deleted the terminated container, you can sign-in to the
Red Hat portal and follow this answer to unsubscribe the deleted
image: <https://access.redhat.com/solutions/776723>.

### Building the Open GEE Image

To build the flattened final Open GEE image:

```BASH
( . config/rhel-7.sh && ./bin/build-gee-image.sh )
```

You can use the `OS_DISTRIBUTION` variable to set the platform you want to
build on, the `STAGE_1_NAME` to set whether to build from a clean clone of the
GEE repository or from your current clone, and `CLEAN_CLONE_BRANCH` to build
from the most recent commit on a given Git branch when building a clean clone.

### Examples

Build from a clean repsitory clone on CentOS 7:

```BASH
OS_DISTRIBUTION=centos-7 ./bin/build-gee-image.sh
```

Build from your current clone of the repository on Ubuntu 16:

```BASH
OS_DISTRIBUTION=ubuntu-16 STAGE_1_NAME=current-clone ./bin/build-gee-image.sh
```

Build a clean clone of branch `release_5.2.0` on CentOS 7:

```BASH
OS_DISTRIBUTION=centos-7 CLEAN_CLONE_BRANCH=release_5.2.0 ./bin/build-gee-image.sh
```

Build a clean clone of branch `release_5.2.0` from user `unameit`'s GitHub
repository on CentOS 7:

```BASH
OS_DISTRIBUTION=centos-7 CLEAN_CLONE_URL=https://github.com/unameit/earthenterprise.git CLEAN_CLONE_BRANCH=release_5.2.0 ./bin/build-gee-image.sh
```

### Using GEE Tutorial Files in the Docker Container

The Docker image is built without the GEE tutorial files, since people using
the image for production may not want to carry the 600+ MB files they likely
wouldn't use.

#### Deriving an Image that Includes the Tutorial Files

You can create an image based on the provided one that adds the tutorial
files.

```BASH
( . config/rhel-7.sh && ./bin/build-tutorials-image.sh )
```

You could then, reuse the `start-gee-image.sh` and `attach-fusion.sh`
scripts by supplying the tag of the new image you built:

```BASH
(
    . config/rhel-7.sh &&
    GEE_IMAGE_NAME=opengee-experimental-rhel-7-tutorial-resources ./bin/start-gee-image.sh
)
```

and

```BASH
(
    . config/rhel-7.sh &&
    GEE_IMAGE_NAME=opengee-experimental-rhel-7-tutorial-resources ./bin/attach-fusion.sh
)
```

#### Using Tutorial Files from Outside the Docker Image

If you have already unpacked the tutorial files in the host operating system,
you can add them as a volume accessible inside the Docker container. E.g., if
you have the files stored at _/opt/google/share/tutorials/fusion/_ outside the
container:

```BASH
( . config/rhel-7.sh &&
DOCKER_RUN_FLAGS="-v /opt/google/share/tutorials/fusion/:/opt/google/share/tutorials/fusion/" ./bin/start-gee-image.sh )
```

## Running the Container

### Running the Open GEE Servers

#### Running Open GEE Server with Persistent Globe and Map Databases

__NOTE:__ Currently, running running Open GEE Server with persistent globe and
map databases outside the Docker container is not supported by the Open GEE
Docker convenince scripts. You'll have to update the `run` commands on your own.
There are plans to fix this in a future release.

In order to be able to build and serve Earth databases that persist between
instantiations of the Docker image, you need persistent asset, and other
directories outside of the Docker container. Another alternative is to run a
container with temporary data directories and commit the running container to
a new image after building your assets.

#### Running Open GEE Server with Temporary Globe and Map Databases

To run the Docker container using temporary assets that disapper when the
container is stopped, leave the `HOST_GEVOL_PATH`, `HOST_GEHTTPD_PATH`,
`HOST_GOOGLE_LOG_PATH`, and `HOST_PGSQL_DATA_PATH` environment variables
empty. This is the default behavior:

```BASH
(
    . config/rhel-7.sh
    unset HOST_GEVOL_PATH
    unset HOST_GEHTTPD_PATH
    unset HOST_GOOGLE_LOG_PATH
    unset HOST_PGSQL_DATA_PATH
    ./bin/start-gee-image.sh
)
```

#### Running the Fusion UI in the Docker Container

To start the Fusion UI in the Open GEE Docker container, you need to have
the GEE servers running in the container, e.g. by running
`./bin/start-gee-image.sh`, and an X11 server outside the container that the
Fusion UI can connect to. If you started the container on a Linux
distribution running X11 listening for socket connections to `/tmp/.X11-unix`,
you should only need to run the following command to open the Fusion UI:

```BASH
( . config/rhel-7.sh && ./bin/attach-fusion.sh )
```

Running the Fusion UI on a non-Linux Docker host running X11 has not been
tested, although it should be possible.

#### Running Portable Server in the Docker Container

Portable Server is installed under an _/opt/google_ directory which has a name
that looks like _/opt/google/portableserver-linux-5.2.0-20170812_. You can
start Portable Server by running the following command in a started Open GEE
Docker container:

```BASH
/opt/google/bin/portableserver
```
