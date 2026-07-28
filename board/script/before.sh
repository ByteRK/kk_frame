#!/bin/bash
#set -x

cd $(dirname $0)

# 限制core文件大小为0
ulimit -c 0