FROM debian:11.2

RUN apt-get -y update && \
    apt-get -y install build-essential git
