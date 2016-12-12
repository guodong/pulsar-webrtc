#!/bin/bash
nohup /root/pulsar-webrtc &
sleep 4
nodejs /root/conn.js $1
