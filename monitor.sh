#!/bin/sh
#============================================
#   Company: SIEMENS Of SH
#   Author:  Mxin
#   Func:    Monitoring process is running
#============================================
ps -fe|grep server |grep -v grep
if [ $? -ne 0 ]
then
    echo "start process"
    cd '/home/pi/DustSensor'
    nohup ./server &
else
    echo "process is runing....."
fi
#
