#!/bin/bash

set -x

mkdir -p ~/VideoStreamerFCUP_Logs

mkdir db
icebox --Ice.Config=config.icebox &
