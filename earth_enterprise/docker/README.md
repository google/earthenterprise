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
with a temporary assets directory, and commit the running to container to a
new image after you've build your assets.

To run the Docker container using a persistent globe assets directory outside 
the container, use the following script.  Set the `HOST_GEVOL_PATH`
environment variable to the path outside the container where you want to store
your persistent assets.

```BASH
HOST_GEVOL_PATH=/persistent/gevol/ ./bin/start-docker-image.sh
```

You can optionally set the `GEE_SERVER_PORT` and even  `IMAGE_TAG` to override
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


#### Using GEE Tutorial Files in the Docker Container

The Docker image is built without the GEE tutorial files, since people using
the image for production may not want to carry the 600+ MB files they wouldn't
use.

##### Deriving an Image that Includes the Tutorial Files

You can create an image based on the provided one that adds the tutorial
files. All you need to do to construct the new image is to run:

```Dockerfile
CMD /bin/bash /opt/google/share/tutorials/fusion/download_tutorial.sh
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
Docker image.  You need slightly more than 6 GB of free space to build
the Docker build image stages, and another 2+ GB to produce a flattened
image.  You can remove the larger build images which contain Docker layer
history after creating the flattened one.

To build the two stages of images with layer history, and flattened final
image:

```BASH
./bin/build-docker-image.sh
```

You can optionally set the `OUTPUT_IMAGE_NAME` shell variable to override the
default name used for the final Docker image.


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
