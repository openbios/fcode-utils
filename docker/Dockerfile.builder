FROM debian:12.5

RUN apt-get -y update && \
    apt-get -y install build-essential git
