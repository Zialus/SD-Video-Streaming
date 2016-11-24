#!/bin/bash

set -x

mkdir -p ~/VideoStreamerFCUP_Logs

./portal 1> /dev/null 2> ~/VideoStreamerFCUP_Logs/portal.txt &
sleep 1
./server 1> /dev/null 2> ~/VideoStreamerFCUP_Logs/server.txt &
sleep 1
./client 1> /dev/null 2> ~/VideoStreamerFCUP_Logs/client.txt &
sleep 2
killall portal
killall server
killall client
sleep 2
killall ffplay
