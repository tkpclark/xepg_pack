#!/bin/bash

if [  "$1" = "" ]
then
        echo 'please input the version_number!!!'
        exit
fi
echo "making pack system version:"$1......
sleep 2

echo $1 > version
tar cvf /home/app/Mycctv_PACK_$1.tar ../../pack/src
