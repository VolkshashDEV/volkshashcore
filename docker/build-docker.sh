#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-volkshashpay/volkshashd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/volkshashd docker/bin/
cp $BUILD_DIR/src/volkshash-cli docker/bin/
cp $BUILD_DIR/src/volkshash-tx docker/bin/
strip docker/bin/volkshashd
strip docker/bin/volkshash-cli
strip docker/bin/volkshash-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
