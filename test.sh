#!/bin/bash

set -x

mkdir db

mkdir -p ~/VideoStreamerFCUP_Logs

cd ~/Downloads
wget -nc 'https://archive.org/download/PopeyeAliBaba/PopeyeAliBaba_512kb.mp4'
wget -nc 'https://archive.org/download/Popeye_forPresident/Popeye_forPresident_512kb.mp4'

cd -

icebox --Ice.Config=config.icebox &

sleep 1

./executables/portal 1> /dev/null 2> ~/VideoStreamerFCUP_Logs/portal.txt &

sleep 1

./executables/server localhost 54321 10066 640x360 500k libx264 ~/Downloads/PopeyeAliBaba_512kb.mp4 tcp 1> /dev/null 2> ~/VideoStreamerFCUP_Logs/serverAliBaba.txt &

sleep 1

./executables/server localhost 54322 10067 640x360 500k libx265 ~/Downloads/Popeye_forPresident_512kb.mp4 tcp 1> /dev/null 2> ~/VideoStreamerFCUP_Logs/serverPresident.txt &

sleep 1

./executables/client 1> /dev/null 2> ~/VideoStreamerFCUP_Logs/client.txt &

 sleep 1
 killall portal
 killall server
 killall icebox

