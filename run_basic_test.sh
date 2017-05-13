#!/bin/bash

set -x

icebox --Ice.Config="ice_config_dir/config.icebox" &

sleep 2

./executables/portal --Ice.Config="ice_config_dir/config.portal" &> ~/VideoStreamerFCUP_Logs/portal.txt &

sleep 2

./executables/server -f ~/Downloads/PopeyeAliBaba_512kb.mp4 -e libx264 -b 500k -v 640x360 \
--my_port 10066 --ff_port 54321 -k what -k are -k you -k doing -n alibabaNORMAL \
&> ~/VideoStreamerFCUP_Logs/serverAliBabaNORMAL.txt --Ice.Config="ice_config_dir/config.server" &

./executables/server -f ~/Downloads/PopeyeAliBaba_512kb.mp4 -e libx264 -b 500k -v 640x360 \
--my_port 10067 --ff_port 54322 -k what -k are -k you -k doing -n alibabaHLS --hls \
&> ~/VideoStreamerFCUP_Logs/serverAliBabaHLS.txt --Ice.Config="ice_config_dir/config.server" &

./executables/server -f ~/Downloads/Popeye_forPresident_512kb.mp4 -e libx265 -b 500k -v 640x360 \
--my_port 10068 --ff_port 54323 -k basketball -k videogames -n presidentNORMAL \
&> ~/VideoStreamerFCUP_Logs/serverPresidentNORMAL.txt --Ice.Config="ice_config_dir/config.server" &

./executables/server -f ~/Downloads/Popeye_forPresident_512kb.mp4 -e libx265 -b 500k -v 640x360 \
--my_port 10069 --ff_port 54324 -k basketball -k videogames -n presidentDASH --dash \
&> ~/VideoStreamerFCUP_Logs/serverPresidentDASH.txt --Ice.Config="ice_config_dir/config.server" &

sleep 2

./executables/client --Ice.Config="ice_config_dir/config.client" < ./extras/ClientCommands.txt

sleep 2

killall server
killall portal
killall icebox

cat ~/VideoStreamerFCUP_Logs/portal.txt

cat ~/VideoStreamerFCUP_Logs/serverAliBabaNORMAL.txt

cat ~/VideoStreamerFCUP_Logs/serverPresidentNORMAL.txt
