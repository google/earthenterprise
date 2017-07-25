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
outside of the Docker container.

To run the Docker container using a persistent globe assets directory outside 
the container, use the script:

```BASH
./bin/start-docker-image.sh
```

You can optionally set the `HOST_GEVOL_PATH`, `GEE_SERVER_PORT` and even 
`IMAGE_TAG` to  override the default values.  You likely want to override, at 
least, `HOST_GEVOL_PATH` to contain the path to the directory outside of the 
container where you store your persistent globe assets in.


#### Running Open GEE Server with Temporary Globe and Map Databases

To run the Docker container using temporary assets that disapper when the 
container is stopped, set the `TEMPORARY_SERVERS` shell valiable to true when 
executing the above command.

```BASH
TEMPORARY_SERVERS=true ./bin/start-docker-image.sh
```


#### Running the Fusion UI in the Docker Container

To start the Fusion UI in the Open GEE Docker container, you need to have
the GEE servers running in the container, e.g. by running
`./bin/start-docker-image.sh`, and an X11 server outside the container that
the Fusion UI can connect to.  If you started the container on a Linux 
distribution running X11 listening to socket connections to `/tmp/.X11-unix`,
you should only need to run the following command to bring up the Fusion UI:

```BASH
./bin/attach-fusion.sh
```

Running the Fusion UI on a non-Linux Docker host running X11 has not been
tested, although it should be theoretially possible.


#### Pointers for Running Open GEE in Custom Setup

There's a sample `docker-compose.yml` you can have a look at.  Note, however,
that Open GEE doesn't currently support clustering.


## Building the Image

If you want to build the Docker image yourself (perhaps, after customization),
you can use the commands that follow.

Make sure you have, at least, about 10 GB of free space to create a new GEE 
Docker image.  You need slightly more than 8.6 GB of free space to build
the Docker build image stages, and another 1.03+ GB to produce a flattened
image.  You can remove the larger build images which contains Docker layer
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
