#!/bin/bash

set -x

sleep 1

./executables/portal 1> /dev/null 2> ~/VideoStreamerFCUP_Logs/portal.txt &

sleep 1

./executables/server -f ~/Downloads/PopeyeAliBaba_512kb.mp4 -e libx264 -b 500k -v 640x360 \
--my_port 10066 --ff_port 54321 -k what -k are -k you -k doing -n alibabaNORMAL \
1> /dev/null 2> ~/VideoStreamerFCUP_Logs/serverAliBaba.txt &

./executables/server -f ~/Downloads/PopeyeAliBaba_512kb.mp4 -e libx264 -b 500k -v 640x360 \
--my_port 10066 --ff_port 54321 -k what -k are -k you -k doing -n alibabaHLS --hls \
1> /dev/null 2> ~/VideoStreamerFCUP_Logs/serverAliBaba.txt &

sleep 1

./executables/server -f ~/Downloads/Popeye_forPresident_512kb.mp4 -e libx265 -b 500k -v 640x360 \
--my_port 10067 --ff_port 54322 -k basketball -k videogames -n presidentNORMAL \
1> /dev/null 2> ~/VideoStreamerFCUP_Logs/serverPresident.txt &

./executables/server -f ~/Downloads/Popeye_forPresident_512kb.mp4 -e libx265 -b 500k -v 640x360 \
--my_port 10067 --ff_port 54322 -k basketball -k videogames -n presidentDASH --dash \
1> /dev/null 2> ~/VideoStreamerFCUP_Logs/serverPresident.txt &

sleep 1

./executables/client < ./extras/ClientCommands.txt

sleep 2

killall portal
killall server
killall icebox

cat ~/VideoStreamerFCUP_Logs/portal.txt
