#!/bin/bash

HOMEDIR="/home/pearldev"

if [[ $HOME != "/root" ]]
  then echo "error:  must be root to run r03_pearl1_local.sh!"
  exit
fi

if [[ $# < 1 ]]
  then echo "usage:  r03_pearl1_local.sh <intro_script_filename>"
  exit
fi

INTRO_FILE=$1

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

sleep 2

xterm -geometry 80x3+0+402 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+469 -vb -sl 1000 -sb -rightbar &
xterm -geometry 80x3+0+536 -vb -sl 1000 -sb -rightbar &

sleep 2

xterm -geometry 80x3+0+402 -e "export DISPLAY=:0.0 ; ./neck | $HOMEDIR/demos/expressionsFace.sh" &
sleep 5
xterm -geometry 80x3+0+469 -e ./roomnav_guidance &
sleep 5
xterm -geometry 80x3+0+536 -e "export DISPLAY=:0.0 ; ./pearl_intro $INTRO_FILE | $HOMEDIR/demos/ShowWords.sh" &

cd -

