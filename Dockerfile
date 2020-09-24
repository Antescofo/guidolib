FROM ubuntu:18.04

ENV LD_LIBRARY_PATH=/usr/local/lib

WORKDIR /
RUN apt-get update && apt-get install -y git make g++ libcairo2-dev openjdk-8-jdk default-jdk cmake libmagick++-dev libmicrohttpd-dev libcurl4-openssl-dev libssl-dev valgrind ffmpeg fluid-soundfont-gm libfluidsynth-dev sox python3-pip
RUN ln -s /usr/bin/pip3 /usr/bin/pip
RUN ln -s /usr/bin/python3 /usr/bin/python

ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

RUN pip3 install requests
RUN pip install httplib2 google-api-python-client oauth2client progressbar2

COPY tools/ssh /root/.ssh
RUN chmod 600 /root/.ssh/id_rsa
RUN ssh-keyscan bitbucket.org >> /root/.ssh/known_hosts
RUN echo checking out fc7bd88170477b9ffc13a5fee902ea68dc8406e5
RUN git clone git@bitbucket.org:antescofo/libmusicxml.git
WORKDIR /libmusicxml
RUN git checkout antescofo-develop
RUN rm -rf /root/.ssh
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

RUN apt-get update
RUN apt-get install -y curl unzip qrencode
RUN curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
RUN unzip awscliv2.zip
RUN ./aws/install

WORKDIR /app/tools
RUN cmake .
RUN make -j 4
RUN apt-get install -y imagemagick
RUN cp /app/tools/videogeneration /usr/bin/videogeneration
RUN cp /app/tools/audioselector /usr/bin/audioselector

RUN ln -s /app/tools/videogen.sh /usr/bin/videogen.sh
RUN ln -s /app/tools/generate_video.py /usr/bin/generate_video
RUN ln -s /app/tools/upload_video.py /usr/bin/upload_video.py
ENV LANG=C.UTF-8
