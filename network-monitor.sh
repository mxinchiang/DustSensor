#!/bin/bash
#============================================
#   Company: SIEMENS Of SH
#   Author:  Mxin
#   Func:    Monitoring wifi is running
#============================================
if ifconfig wlan0 | grep -q "inet addr:" ; then
      exit
   else
      echo "Network connection down! Attempting reconnection."
      ifup --force wlan0
   fi
#crontab -e
#*/1 * * * * bash /home/network-monitor.sh 
