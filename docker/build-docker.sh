#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-ukkeypay/ukkeyd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/ukkeyd docker/bin/
cp $BUILD_DIR/src/ukkey-cli docker/bin/
cp $BUILD_DIR/src/ukkey-tx docker/bin/
strip docker/bin/ukkeyd
strip docker/bin/ukkey-cli
strip docker/bin/ukkey-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
