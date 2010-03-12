#!/bin/bash

export CENTRALHOST=pearl1

TIMESTAMP=`date +%Y-%m-%d-%H-%M-%S`
HOMEDIR="/home/jrod"

if [[ $# < 3 ]]
  then echo "usage:  r03_pearl_start_guidance.sh <user number> <walk number> <guidance config>
guidance config:
    baseline
    neck
    audio
    audio_neck"
    exit
fi


USER_NUM=$1
WALK_NUM=$2
GUIDANCE_CONFIG=$3

GOAL=r03.i$((5-$WALK_NUM))

LOG_DIR=$HOMEDIR/logs/r03/usr$USER_NUM

if [[ ! -d $LOG_DIR ]]
  then mkdir $LOG_DIR
fi

cd $HOMEDIR/carmen/bin

xterm -geometry 80x3+0+603 -e ./logger $LOG_DIR/pearl${WALK_NUM}_${GUIDANCE_CONFIG}_$TIMESTAMP.log.gz &

if [[ $GUIDANCE_CONFIG == *neck ]]
  then echo "neck motion on" ; ./pearlguide_set_neck_motion on
else echo "neck motion off" ; ./pearlguide_set_neck_motion off
fi

if [[ $GUIDANCE_CONFIG == audio* ]]
  then echo "audio guidance on" ; killall theta1.pl ; ./roomnav_guidance_config -audio
else echo "audio guidance off" ; ./roomnav_guidance_config -none
fi

./pearlguide_goto $GOAL

cd -
