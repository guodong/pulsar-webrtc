FROM ubuntu:14.04
MAINTAINER guodong <gd@tongjo.com>
RUN apt-get update
RUN apt-get install -y xorg xserver-xorg-video-dummy wget
RUN apt-get install -y curl && curl -sL https://deb.nodesource.com/setup_7.x | sudo -E bash -
RUN apt-get install -y nodejs
RUN apt-get install -y libx11-6 libxcomposite1 libxdamage1 libxext6 libxfixes3 libxtst6 libasound2
RUN wget https://gist.githubusercontent.com/guodong/91b631bdfa42e5e72f21/raw/8c942883b96e2996fa9cf541c6bf6150a1c3afb9/xorg-dummy.conf -O /root/xorg.conf
EXPOSE 6000
EXPOSE 8080
RUN apt-get install -y gnome-themes-standard xfwm4 gedit
RUN npm install --prefix /root ws
COPY pulsar-webrtc /root/pulsar-webrtc
COPY start.sh /root/start.sh
COPY libprotobuf_lite.so /root/libprotobuf_lite.so
COPY libboringssl.so /root/libboringssl.so
RUN chmod u+x /root/start.sh
COPY conn.js /root/conn.js
ENV DISPLAY :1
RUN sed -i '2s/.*/gtk-theme-name = Adwaita/' /etc/gtk-3.0/settings.ini
CMD ["/root/start.sh"]
