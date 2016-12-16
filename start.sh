#!/bin/bash
nohup Xorg :1 -noreset -logfile /root/0.log -config /root/xorg.conf & #noreset must be add, otherwise cannot connect by tcp
sleep 2
xhost +
nohup /root/pulsar-webrtc &
sleep 2
nodejs /root/conn.js $1
