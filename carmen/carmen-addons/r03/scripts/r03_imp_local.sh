#!/bin/bash

#export CENTRALHOST=imp

TIMESTAMP=`date +%Y-%m-%d-%H-%M-%S`
HOMEDIR="/home/jrod"

cd $HOMEDIR/carmen/bin

xterm -geometry 80x3+0+0 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+67 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+134 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+201 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+268 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+335 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+402 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+469 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+536 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+603 -vb -sl 1000 -sb -rightbar &

sleep 5

xterm -geometry 80x3+0+0 -e ./central -lx &
sleep 5
xterm -geometry 80x3+0+67 -e ./param_server -imp ../data/longwood2.map.gz &
sleep 5
xterm -geometry 80x3+0+134 -e ./laser &
sleep 5
xterm -geometry 80x3+0+201 -e ./walker_cerebellum &
sleep 5
xterm -geometry 80x3+0+268 -e ./walker_cerebellum3 &
sleep 5
xterm -geometry 80x3+0+335 -e ./walker &
sleep 5
xterm -geometry 80x3+0+402 -e ./robot &
sleep 5
xterm -geometry 80x3+0+469 -e ./localize &
sleep 5
xterm -geometry 80x3+0+536 -e ./roomnav_guidance &

cd -

