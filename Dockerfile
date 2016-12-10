FROM ubuntu:16.04
MAINTAINER guodong <gd@tongjo.com>
RUN apt-get update
RUN apt-get install -y xserver-xorg xserver-xorg-video-dummy wget git
ENV DISPLAY :0
