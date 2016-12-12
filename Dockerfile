FROM ubuntu:14.04
MAINTAINER guodong <gd@tongjo.com>
RUN apt-get update
RUN apt-get install -y nodejs npm
RUN apt-get install -y libx11-6 libxcomposite1 libxdamage1 libxext6 libxfixes3 libxtst6 libasound2
RUN npm install --prefix /root ws
COPY pulsar-webrtc /root/pulsar-webrtc
COPY start.sh /root/start.sh
COPY libprotobuf_lite.so /root/libprotobuf_lite.so
COPY libboringssl.so /root/libboringssl.so
RUN chmod u+x /root/start.sh
COPY conn.js /root/conn.js
ENV DISPLAY :0
CMD ["/root/start.sh"]
