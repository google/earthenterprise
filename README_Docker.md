# Docker Image

The Docker image is based on Ubuntu 16.04 & includes the development & runtime environment: git, git lfs, Python 2.7, Python pexpect, Python tornado, Swig, gcc, g++ & scons.

# Quick Start

## On a local Docker host

### Prerequisites

Docker, Git & Git LFS

- If you're working on a macOS use the `Git LFS` installation instructions at: https://help.github.com/articles/installing-git-large-file-storage/#platform-mac.
- For Linux: https://help.github.com/articles/installing-git-large-file-storage/#platform-linux
- For Windows: https://help.github.com/articles/installing-git-large-file-storage/#platform-windows

### Get the Earth Enterprise project locally

On a terminal session:

- Clone the Earth Enterprise repo using `GIT_LFS_SKIP_SMUDGE=1 git clone https://[GITHUB USER NAME]:[GITHUB PASSWORD]@github.com/trilogy-group/earthenterprise.git`
- Change into the project root folder: `cd earthenterprise`
- Run: `git lfs pull`

### Create the Docker image & container

- Open a terminal session to the project root folder
- Execute `docker-compose -p earth-enterprise build` to build the Docker image

### Build the project in the Docker container

- Execute `docker-compose -p earth-enterprise up -d` to start the container in a detached mode
- Execute `docker exec -it earth-enterprise bash` to login to the container interactively
- While inside the docker container execute:
    - `cd /data/earth_enterprise` & then `scons -j[NUMBER OF BUILD CORES] release=1 build`, (note: `[NUMBER OF BUILD CORES]` must be at most equal to the number of processing cores of the Docker container) to build Earth Enterprise Fusion and Server, and
	- `cd /data/earth_enterprise/src/portableserver` & `python build.py`, to build Earth Enterprise Portable Server.
	- To exit the container, type `exit`
- Execute `docker-compose -p earth-enterprise down` to stop the container & service.

## On a remote Docker host

### Remote Docker host Prerequisites

Docker

### Create the Docker image & container on the remote host

- Copy the Docker deliverables (`Dockerfile` & `docker-compose.yml`) from `github.com/trilogy-group/earthenterprise` to a local folder.
- Open a terminal session to the folder
- Set the remote docker registry environment variable: `export PRIVATE_DOCKER_REGISTRY="[DOCKER REGISTRY]/"` (please, note the trailing slash)
- Execute `docker-compose -H tcp://[REMOTE DOCKER HOST IP OR DNS NAME]:[REMOTE DOCKER HOST PORT] -p earth-enterprise build` to build the Docker image

### Build the project in the remote Docker container

- Execute `docker-compose -H tcp://[REMOTE DOCKER HOST IP OR DNS NAME]:[REMOTE DOCKER HOST PORT] -p earth-enterprise up -d` to start the container in a detached mode
- Execute `docker -H tcp://[REMOTE DOCKER HOST IP OR DNS NAME] exec -it earth-enterprise bash` to login to the container interactively
- While inside the docker container:
	- Optionally run `cd /data`, since this is already the working directory
	- Execute `GIT_LFS_SKIP_SMUDGE=1 git clone https://[GITHUB USER NAME]:[GITHUB PASSWORD]@github.com/trilogy-group/earthenterprise.git` to clone the Earth Enterprise repo
	- `cd /data/earthenterprise` to change to the project root folder that was created by cloning
	- Execute `git lfs pull`
	- Call `cd /data/earthenterprise/earth_enterprise` & then `scons -j[NUMBER OF BUILD CORES] release=1 build`, (note: `[NUMBER OF BUILD CORES]` must be at most equal to the number of processing cores of the Docker container) to build Earth Enterprise Fusion and Server, and
	- Call `cd /data/earthenterprise/earth_enterprise/src/portableserver` & then `python build.py`, to build Earth Enterprise Portable Server.
	- To exit the container, type `exit`
- Execute `docker-compose -H tcp://[REMOTE DOCKER HOST IP OR DNS NAME]:[REMOTE DOCKER HOST PORT] -p earth-enterprise down` to stop the container & service.

# Working with a local Docker host

Clone the git repo to the same machine as the local Docker host as in the `Quickstart` section above or by following the instructions in `https://github.com/trilogy-group/earthenterprise/blob/master/earth_enterprise/BUILD.md`.

## Building the local Docker image

In the project root folder, execute:

```bash
docker-compose -p earth-enterprise build
```

This will create the `earth-enterprise:latest` Docker image in the local Docker host.

## Running the local Docker container

In the project root folder, run:

```bash
docker-compose -p earth-enterprise up -d
```

Note: Parameter `-d` runs the container in detached mode.

This will start container in detached mode on the local Docker host, called `earth-enterprise`.
You can check the status of the container by running `docker ps`.

## Initiating a session in the local Docker container

In a terminal session run:

```bash
docker exec -it earth-enterprise bash
```

Note: Parameters `-it` start an interactive TTY session on the Docker container

## docker-compose.yml

The docker-compose.yml file contains a single service: `earth-enterprise`.
This is used to build & execute the Earth Enterprises source from the local environment by mounting the local project root folder `.` to the container's `/data` folder:

```yaml
    volumes:
      - .:/data:Z
```

# Working with a remote Docker host

Copy the Docker deliverables (`Dockerfile` & `docker-compose.yml`) from `github.com/trilogy-group/earthenterprise` to a local folder. For the commands defined below, `[DOCKER REGISTRY]` must be a valid Docker registry path to which you have access & `[REMOTE DOCKER HOST IP OR DNS NAME]` must point to a running, remote & accessible Docker host, which can run Docker images pushed to the `[DOCKER REGISTRY]`. The `[REMOTE DOCKER HOST PORT]` is usually `2375`. 

## Building the Docker image

In the folder where you copied the Docker deliverables execute:

```bash
export PRIVATE_DOCKER_REGISTRY="[DOCKER REGISTRY]/"
docker-compose -H tcp://[REMOTE DOCKER HOST IP OR DNS NAME]:[REMOTE DOCKER HOST PORT] -p earth-enterprise build
```

This will create the Docker image & push it as `[DOCKER REGISTRY]/earth-enterprise:latest` to the Docker registry.

## Running the remote Docker container

In the folder where you copied the Docker deliverables, run:

```bash
docker-compose -H tcp://[REMOTE DOCKER HOST IP OR DNS NAME]:[REMOTE DOCKER HOST PORT] -p earth-enterprise up -d
```

Note: Parameter `-d` runs the container in detached mode.

This will start container called `earth-enterprise` to the remote Docker host in `[REMOTE DOCKER HOST IP OR DNS NAME]`.
You can check the status of the container by running `docker -H tcp://[REMOTE DOCKER HOST IP OR DNS NAME] ps`.

## Initiating a session in the remote Docker container

In a terminal session run:

```bash
docker -H tcp://[REMOTE DOCKER HOST IP OR DNS NAME] exec -it earth-enterprise bash
```

Note: Parameters `-it` start an interactive TTY session on the Docker container

# Building

## Earth Enterprise Fusion & Server in a local Docker container

```bash
cd /data/earth_enterprise
scons -j[NUMBER OF BUILD CORES] release=1 build
```

Note: `[NUMBER OF BUILD CORES]` must be at most equal to the number of processing cores of the Docker container. To find the number of cores used by the container run `lscpu`.

## Earth Enterprise Fusion & Server in a remote Docker container

```bash
cd /data/earthenterprise/earth_enterprise
scons -j[NUMBER OF BUILD CORES] release=1 build
```

Note: `[NUMBER OF BUILD CORES]` must be at most equal to the number of processing cores of the Docker container. To find the number of cores used by the container run `lscpu`.

## Earth Enterprise Portable Server in a local Docker container

```bash
cd /data/earth_enterprise/src/portableserver
python build.py
```

## Earth Enterprise Portable Server in a remote Docker container

```bash
cd /data/earthenterprise/earth_enterprise/src/portableserver
python build.py
```

# Installing & Running

## Earth Enterprise Fusion & Server in a local Docker container

```bash
cd /data/earth_enterprise
scons -j[NUMBER OF BUILD CORES] release=1 stage_install
cd /data/earth_enterprise/src/installer
./install_fusion.sh
./install_server.sh
```

Note: `[NUMBER OF BUILD CORES]` must be at most equal to the number of processing cores of the Docker container. To find the number of cores used by the container run `lscpu`.

## Earth Enterprise Fusion & Server in a remote Docker container

```bash
cd /data/earthenterprise/earth_enterprise
scons -j[NUMBER OF BUILD CORES] release=1 stage_install
cd /data/earthenterprise/earth_enterprise/src/installer
./install_fusion.sh
./install_server.sh
```

Note: `[NUMBER OF BUILD CORES]` must be at most equal to the number of processing cores of the Docker container. To find the number of cores used by the container run `lscpu`.

## Earth Enterprise Portable Server

While inside the Docker container & assuming you have already built the Portable Server following the build steps outline above:

### In a local Docker container

```bash
cd /data/earth_enterprise/src/portableserver/build/portableserver-linux-*/server/
python portable_server.py
```

### In a remote Docker container

```bash
cd /data/earthenterprise/earth_enterprise/src/portableserver/build/portableserver-linux-*/server/
python portable_server.py
```
