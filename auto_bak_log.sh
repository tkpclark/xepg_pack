#!/bin/bash


########
if [  "$1" = "" ]
then
        echo 'please input the directory which the logfile locate at!'
        exit
fi

#########
if [ ! -f "$1" ]
then
        echo $1 "doesn't exist or denied!"
        exit
fi

#########
filesize=$(ls -l $1|awk '{print $5}')

echo filesize:$filesize

if [ "$filesize" -ge 100000000 ]
then
	bakdate=$(date +%Y%m%d%k%M%S)
	echo bak $1 to $1.$bakdate
	touch $1
	cp $1 $1.$bakdate
  echo 0 > $1
else
	echo still so small
fi


########

deldate=$(date +%Y%m%d -d "-30 day")
echo delete files on date $deldate
touch x
rm `ls $1* |grep $deldate` x
