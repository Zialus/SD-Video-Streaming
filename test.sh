#!/bin/bash

set -x

./portal &
sleep 1
./server
sleep 1
./client
killall portal
