#!/bin/sh
#set -x

cd $(dirname $0)

APP=$(sh ./name.sh)

strings ../$APP | grep AVS: | awk -F 'AVS: ' '{print $2}'