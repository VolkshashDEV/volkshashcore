FROM debian:stretch
LABEL maintainer="Volkshash Developers <dev@volkshash.org>"
LABEL description="Dockerised VolkshashCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
