#!/bin/sh
CONTAINER_NAME="$1"
docker exec -ti "$CONTAINER_NAME" geth --exec 'admin.peers' attach
