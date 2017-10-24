#! /bin/bash

# This script deletes Docker containers that have exited.  Docker leaves them
# behind, so you can inspect them after they've shut down.  Other than that,
# they're probably just taking up more and more space.

docker rm $(docker ps -qa --no-trunc --filter "status=exited")