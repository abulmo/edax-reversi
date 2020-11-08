# simple sample

FROM gcc:4.9

RUN apt-get update && \
    apt-get install make && \
    apt-get install p7zip-full
