#!/bin/bash

set -x

mkdir -p ~/VideoStreamerFCUP_Logs
mkdir -p ~/Downloads

mkdir db
icebox --Ice.Config="ice_config_dir/config.icebox" &
