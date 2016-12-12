FROM ubuntu:14.04
MAINTAINER guodong <gd@tongjo.com>
RUN apt-get update
RUN apt-get install -y nodejs npm
RUN npm install --prefix /root ws
COPY pulsar-webrtc /root/pulsar-webrtc
COPY start.sh /root/start.sh
RUN chmod u+x /root/start.sh
COPY conn.js /root/conn.js
ENV DISPLAY :0
CMD ["/root/start.sh"]
