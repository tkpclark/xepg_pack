#!/bin/bash

cd /home/app/pack/src/
kill `cat dtbcheck.pid `
kill `cat updater.pid`

