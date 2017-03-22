#!/bin/bash
nohup Xorg $DISPLAY -noreset -logfile /root/0.log -config /root/xorg.conf & #noreset must be add, otherwise cannot connect by tcp
sleep 2
xhost +
nohup /root/pulsar-webrtc $SIGNAL_ADDR &
sleep 2
#nohup nodejs /root/conn.js $PORT &
sleep 10
xfwm4 &
eval $APP &
/bin/bash
