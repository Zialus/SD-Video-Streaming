#!/bin/bash

set -x

icebox --Ice.Config="ice_config_dir/config.icebox" &> ~/VideoStreamerFCUP_Logs/icebox.txt &

sleep 2

./executables/portal --Ice.Config="ice_config_dir/config.portal" &> ~/VideoStreamerFCUP_Logs/portal.txt &

sleep 2

./executables/server -f ~/Downloads/PopeyeAliBaba_512kb.mp4 -e libx264 -b 500k -v 640x360 \
--my_port 10066 --ff_port 54321 -k what -k are -k you -k doing -n alibabaNORMAL \
--Ice.Config="ice_config_dir/config.server" &> ~/VideoStreamerFCUP_Logs/serverAliBabaNORMAL.txt &

sleep 2

./executables/server -f ~/Downloads/PopeyeAliBaba_512kb.mp4 -e libx264 -b 500k -v 640x360 \
--my_port 10067 --ff_port 54322 -k what -k are -k you -k doing -n alibabaHLS --hls \
--Ice.Config="ice_config_dir/config.server" &> ~/VideoStreamerFCUP_Logs/serverAliBabaHLS.txt &

sleep 2

./executables/server -f ~/Downloads/Popeye_forPresident_512kb.mp4 -e libx265 -b 500k -v 640x360 \
--my_port 10068 --ff_port 54323 -k basketball -k videogames -n presidentNORMAL \
--Ice.Config="ice_config_dir/config.server" &> ~/VideoStreamerFCUP_Logs/serverPresidentNORMAL.txt &

sleep 2

./executables/server -f ~/Downloads/Popeye_forPresident_512kb.mp4 -e libx265 -b 500k -v 640x360 \
--my_port 10069 --ff_port 54324 -k basketball -k videogames -n presidentDASH --dash \
--Ice.Config="ice_config_dir/config.server" &> ~/VideoStreamerFCUP_Logs/serverPresidentDASH.txt &

sleep 2

./executables/client --Ice.Config="ice_config_dir/config.client" < ./extras/ClientCommands.txt \
&> ~/VideoStreamerFCUP_Logs/client.txt

sleep 2

killall server

sleep 5

killall portal

sleep 5

killall icebox

sleep 2

cat ~/VideoStreamerFCUP_Logs/client.txt

cat ~/VideoStreamerFCUP_Logs/portal.txt

cat ~/VideoStreamerFCUP_Logs/serverAliBabaNORMAL.txt

cat ~/VideoStreamerFCUP_Logs/serverAliBabaHLS.txt

cat ~/VideoStreamerFCUP_Logs/serverPresidentNORMAL.txt

cat ~/VideoStreamerFCUP_Logs/serverPresidentDASH.txt

cat ~/VideoStreamerFCUP_Logs/icebox.txt
