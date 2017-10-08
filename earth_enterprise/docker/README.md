# Docker Image for Running GE Open Source Earth Server and Fusion Server

## Setting up Docker

### Ubuntu

Install Docker as root:

```BASH
# apt-get install docker.io
```

Add users that you want to have access to Docker to the `docker` group:

```BASH
# adduser muser docker
```

### Cent OS 7

Install Docker as root:

```BASH
yum install docker
# In case your Docker deamon isn't running:
service docker start
```

Now you can use Docker as the root user.


## Pulling an Image from Docker Cloud

You can get a pre-built image from a Docker registry.

```BASH
docker pull thermopylae/opengee-experimental
```


## Running the Container

### Running the Open GEE Servers

#### Running Open GEE Server with Persistent Globe and Map Databases

In order to be able to build and serve Earth databases that persist between 
instantiations of the Docker image, you need a persistent assets directory 
outside of the Docker container.  Another alternative is to run a container
with a temporary assets directory, and commit the running container to a new
image after you've built your assets.

To run the Docker container using a persistent globe assets directory outside 
the container, use the following script.  Set the `HOST_GEVOL_PATH`
environment variable to the path outside the container where you want to store
your persistent assets.

```BASH
HOST_GEVOL_PATH=/persistent/gevol/ ./bin/start-docker-image.sh
```

You can optionally set the `GEE_SERVER_PORT` and even `IMAGE_TAG` to override
the default values.


##### Initializing a New Location for Persistsing Databases

If you want to create the directory structure needed for storing globe and map
databases in a new location, you can use the following commands (after
substituting the desired path), and follow the prompts.

```BASH
sudo mkdir -p /persistent/gevol/assets
docker run --cap-add=DAC_READ_SEARCH -v /persistent/gevol:/gevol -ti opengee-experimental /opt/google/bin/geconfigureassetroot --new --assetroot /gevol/assets
```


#### Running Open GEE Server with Temporary Globe and Map Databases

To run the Docker container using temporary assets that disapper when the 
container is stopped, leave the `HOST_GEVOL_PATH` environment valiable empty.
This is the default behavior

```BASH
HOST_GEVOL_PATH='' ./bin/start-docker-image.sh
```


#### Running the Fusion UI in the Docker Container

To start the Fusion UI in the Open GEE Docker container, you need to have
the GEE servers running in the container, e.g. by running
`./bin/start-docker-image.sh`, and an X11 server outside the container that
the Fusion UI can connect to.  If you started the container on a Linux 
distribution running X11 listening for socket connections to `/tmp/.X11-unix`,
you should only need to run the following command to bring up the Fusion UI:

```BASH
./bin/attach-fusion.sh
```

Running the Fusion UI on a non-Linux Docker host running X11 has not been
tested, although it should be theoretially possible.


#### Running Portable Server in the Docker Container

Portable Server is installed under an `/opt/google` directory which has a name
that looks like `/opt/google/portableserver-linux-5.2.0-20170812`.  You can
start Portable Server by running the following command in a started Open GEE
Docker container:

```BASH
/opt/google/bin/portableserver
```


#### Using GEE Tutorial Files in the Docker Container

The Docker image is built without the GEE tutorial files, since people using
the image for production may not want to carry the 600+ MB files they wouldn't
use.

##### Deriving an Image that Includes the Tutorial Files

You can create an image based on the provided one that adds the tutorial
files.

```BASH
docker build -f derived-images/Dockerfile.opengee-tutorial-resources -t opengee-tutorial-resources .
```

You could then, re-use the `start-docker-image.sh` and `attach-fusion.sh`
scripts by supplying the tag of the new image you built. E.g.:

```BASH
IMAGE_TAG=opengee-tutorial-resources ./bin/start-docker-image.sh
```

and

```BASH
IMAGE_TAG=opengee-tutorial-resources ./bin/attach-fusion.sh
```


##### Using Tutorial Files from Outside the Docker Image

If you have already unpacked the tutorial files in host operating system, you
can add them as a volume accessible inside the Docker container.  E.g., if you
have the files stored at `/opt/google/share/tutorials/fusion/` outside the
container:

```BASH
DOCKER_RUN_FLAGS="-v /opt/google/share/tutorials/fusion/:/opt/google/share/tutorials/fusion/" ./bin/start-docker-image.sh
```


#### Pointers for Running Open GEE in Custom Setup

There's a sample `docker-compose.yml` you can have a look at.  Note, however,
that Open GEE doesn't currently support clustering.


## Building the Image

If you want to build the Docker image yourself (perhaps, after customization),
you can use the commands that follow.

Make sure you have, at least, about 8.1 GB of free space to create a new GEE 
Docker image.  (You need slightly more than 6 GB of free space to build
the Docker build image stages, and another 2+ GB to produce a flattened
image.  You can remove the larger build images which contain Docker layer
history after creating the flattened one.)

Building a Docker image with Open GEE requires a Docker image with the GEE
build environment for the platform you're building on.  The next section
covers build environment images.  You'll need to read it, if you don't have a
build environment image you can pull from a Docker registry.

To build the two stages of images with layer history, and flattened final
image:

```BASH
./bin/build-docker-image.sh
```

You can optionally set the `OUTPUT_IMAGE_NAME` shell variable to override the
default name used for the final Docker image.  You can use the
`OS_DISTRIBUTION` variable to set the platform you want to build on, the
`STAGE_1_NAME` to set whether to build from a clean clone of the GEE
repository, or from you current clone, and `CLEAN_CLONE_BRANCH` to build from
the most recent commit on a given Git branch when building a clean clone.

Have a look at the files under the `config` directory for a more convenient
way of setting build variables.

### Examples:

Build from a clean repsitory clone on Cent OS 7:

```BASH
OS_DISTRIBUTION=centos-7 ./bin/build-docker-image.sh
```


Build from your current clone of the repository on Ubuntu 16:

```BASH
OS_DISTRIBUTION=ubuntu-16 STAGE_1_NAME=current-clone ./bin/build-docker-image.sh
```


Build a clean clone of branch `release_5.2.0` on Cent OS 7:

```BASH
OS_DISTRIBUTION=centos-7 CLEAN_CLONE_BRANCH=release_5.2.0 ./bin/build-docker-image.sh
```


Build a clean clone of branch `release_5.2.0` from user `unameit`'s GitHub
repository on Cent OS 7:

```BASH
OS_DISTRIBUTION=centos-7 CLEAN_CLONE_URL=https://github.com/unameit/earthenterprise.git CLEAN_CLONE_BRANCH=release_5.2.0 ./bin/build-docker-image.sh
```

### Building a Build-environment Image

The Dockerfiles used by the `build-docker-image.sh` script require Docker
images with build environments for each platform you are trying to build on.

The build environment image is reused every time you rebuild Open GEE.  This
should save you time during normal development targetting another platform,
since only the steps of transferring the modified source code, and running the
Open GEE build would need to be repeated.

Normally, a build environment image for each supported platform should be
published to Docker cloud, so one would be downloaded when you try to build,
if you don't have it locally.  However, if you want to build a new image to
publish to Docker cloud, or want to use a custom build environment, you can
have a look at the `build-build-env-image.sh` script.  It's used as:

```BASH
OS_DISTRIBUTION=centos-7 ./bin/build-build-env-image.sh
```

With the value of `OS_DISTRIBUTION` that you need substituted above.  You can
also override the `IMAGE_NAME` variable, but don't forget that the Dockerfiles
used for the final platform images are looking for images with the default
image name tags.


#### Recovering a RHEL Subscription for a Deleted Image

If a build on RHEL fails during package building and installation, the build
script may terminate while the RHEL installation in the Docker container is
still registered with `subscription-manager`.  If you still have the layer of
the terminated container, you can `commit` it to a new image, run a shell in
it, and perform `subscription-manager unregister`.

If you have already deleted the terminated container, you can log in the
Red Hat portal online, and follow this answer to unsubscribe the deleted
image: https://access.redhat.com/solutions/776723.



### Delete the Temporary Build Docker Images

To delete the intermediary Docker images used during the build:

```BASH
./bin/clean-build-images.sh
```


### Publish the Image to Docker Store / Docker Cloud

If you are GEE developer, and need to publish a new version of the Docker 
image to the Docker Store, or Docker Cloud registry, you can use the commands 
below.

```BASH
docker login  # You should only need to do this once.
docker tag opengee-experimental thermopylae/opengee-experimental
docker push thermopylae/opengee-experimental
```

