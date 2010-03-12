#!/bin/bash

#export CENTRALHOST=imp

TIMESTAMP=`date +%Y-%m-%d-%H-%M-%S`
HOMEDIR="/home/pearl2dev"

cd $HOMEDIR/carmen/bin

xterm -geometry 80x3+0+0 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+67 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+134 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+201 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+268 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+335 -vb -sl 1000 -sb -rightbar &

sleep 5

xterm -geometry 80x3+0+0 -e ./central -lx &
sleep 5
xterm -geometry 80x3+0+67 -e ./param_server -r pearl ../data/longwood2.map.gz &
sleep 5
xterm -geometry 80x3+0+134 -e ./base_services &
sleep 5
xterm -geometry 80x3+0+201 -e ./localize &
sleep 5
xterm -geometry 80x3+0+268 -e ./navigator &
sleep 5
xterm -geometry 80x3+0+335 -e ./pearlguide &

cd -

