FROM debian:stretch
LABEL maintainer="Ukkey Developers <dev@ukkey.org>"
LABEL description="Dockerised UkkeyCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
