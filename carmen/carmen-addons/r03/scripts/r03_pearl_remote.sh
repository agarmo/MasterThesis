#!/bin/bash

export CENTRALHOST=pearl1

HOMEDIR="/home/jrod"

cd $HOMEDIR/carmen/bin

xterm -geometry 80x3+500+0 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+67 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+134 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+201 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+268 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+335 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+402 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+469 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+536 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+603 -vb -sl 1000 -sb -rightbar &

sleep 5

xterm -geometry 80x3+500+0 -e ./navigator_panel &
sleep 10
xterm -geometry 80x3+500+67 -e ./roomnav &
sleep 5
xterm -geometry 80x3+500+134 -e ./param_edit &
sleep 5
xterm -geometry 80x3+500+201 -e ./robotgraph &

cd -

