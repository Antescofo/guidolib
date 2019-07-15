FROM ubuntu:latest

ENV LD_LIBRARY_PATH=/usr/local/lib

WORKDIR /
RUN apt-get update && apt-get install -y git make g++ libcairo2-dev openjdk-8-jdk default-jdk cmake libmagick++-dev libmicrohttpd-dev libcurl4-openssl-dev libssl-dev
ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

RUN git clone https://github.com/grame-cncm/libmusicxml.git
WORKDIR /libmusicxml/build
RUN make -j 4 && make install

WORKDIR /grame
RUN git clone --single-branch -b dev --depth 1 https://github.com/grame-cncm/midishare.git
WORKDIR /grame/midishare/midisharelight/cmake
RUN cmake . && make -j 4 && make install

WORKDIR /app
COPY doc /app/doc
COPY environments /app/environments
COPY gmn-examples /app/gmn-examples
COPY lang /app/lang
COPY misc /app/misc
COPY open-issues /app/open-issues
COPY package /app/package
COPY platforms /app/platforms
COPY regression-tests /app/regression-tests
COPY server /app/server
COPY validate /app/validate
COPY version /app/version
COPY build /app/build
COPY src /app/src

WORKDIR /app/build
RUN make -j 4 && make install

COPY tools /app/tools
WORKDIR /app/tools

RUN make -j 4 && make install
