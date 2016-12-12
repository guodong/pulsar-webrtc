#!/bin/bash
nohup /root/pulsar-webrtc &
nodejs conn.js $1
