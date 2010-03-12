#!/bin/bash

export CENTRALHOST=imp

HOMEDIR="/home/jrod"

cd $HOMEDIR/carmen/bin

xterm -geometry 80x3+500+0 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+67 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+134 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+500+201 -vb -sl 1000 -sb -rightbar &

sleep 5

xterm -geometry 80x3+500+0 -e ./navigator_panel &
sleep 10
xterm -geometry 80x3+500+67 -e ./robotgraph &
sleep 2
xterm -geometry 80x3+500+134 -e ./param_edit &

cd -

