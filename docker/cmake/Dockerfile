FROM ubuntu
LABEL maintainer="Ahsan Khan" \
      description="Basic C++ stuff for  repo." \
      version="0.1.0"
ARG DEBIAN_FRONTEND=noninteractive
    
RUN    apt-get autoclean && \
    apt-get autoremove && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN apt-get update -y
RUN apt-get install -y tzdata

RUN mkdir /scripts
COPY install-dependencies.sh /scripts
RUN /scripts/install-dependencies.sh

