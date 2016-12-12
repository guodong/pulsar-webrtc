#!/bin/bash
nohup /root/pulsar-webrtc &
sleep 1
nodejs conn.js $1
