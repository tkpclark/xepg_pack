#!/bin/bash

echo "installing pack...."
sleep 1

app_path=/home/app
pack_path=$app_path/pack

rm *.pid 2> /dev/null
cd ..
mkdir $pack_path/log $pack_path/taskts
touch $pack_path/log/sys.log
