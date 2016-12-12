#!/bin/bash
nohup /root/pulsar-webrtc &
sleep 1
nodejs /root/conn.js $1
