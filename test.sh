#!/bin/bash

set -x

./portal &
sleep 1
./server
