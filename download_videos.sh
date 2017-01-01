#!/bin/bash

set -x

pushd .
cd ~/Downloads
wget -nc 'https://archive.org/download/PopeyeAliBaba/PopeyeAliBaba_512kb.mp4'
wget -nc 'https://archive.org/download/Popeye_forPresident/Popeye_forPresident_512kb.mp4'
popd
