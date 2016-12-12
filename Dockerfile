FROM ubuntu:14.04
MAINTAINER guodong <gd@tongjo.com>
RUN apt-get update
RUN apt-get install -y nodejs
COPY pulsar-webrtc /root/pulsar-webrtc
COPY start.sh /root/start.sh
CMD chmod u+x /root/start.sh
ENV DISPLAY :0
CMD ["/root/start.sh"]
