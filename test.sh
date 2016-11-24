#!/bin/bash

set -x

mkdir -p ~/VideoStreamerFCUP_Logs

cd ~/Downloads
wget 'https://archive.org/download/Popeye_forPresident/Popeye_forPresident_512kb.mp4'
cd

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
