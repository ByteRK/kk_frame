#!/bin/sh
#set -x
# Copyright (c) 2026 by Ricken, All Rights Reserved. 

cd $(dirname $0)

chmod +x ./*.sh
chmod +x ./script/*.sh

sh ./script/run.sh &