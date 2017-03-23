#!/bin/bash
nohup Xorg $DISPLAY -noreset -logfile /root/0.log -config /root/xorg.conf & #noreset must be add, otherwise cannot connect by tcp
sleep 1
nohup /root/pulsar-webrtc $SIGNAL_ADDR &
/bin/bash
