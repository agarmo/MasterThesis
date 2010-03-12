#!/bin/bash

export CENTRALHOST=imp

export TIMESTAMP=`date +%Y-%m-%d-%H-%M-%S`
export HOMEDIR="/home/jrod"

if [[ $# < 3 ]]
  then echo "usage:  r03_imp_start_guidance.sh <user number> <walk number> <guidance config>
guidance config:
    baseline
    arrow
    text_ai
    text_woz
    audio
    audio_arrow
    audio_text_ai
    audio_text_woz"
    exit
fi


export USER_NUM=$1
export WALK_NUM=$2
export GUIDANCE_CONFIG=$3

export GOAL=r03.i$(($WALK_NUM+1))

export LOG_DIR=$HOMEDIR/logs/r03/usr$USER_NUM

if [[ ! -d $LOG_DIR ]]
  then mkdir $LOG_DIR
fi

cd $HOMEDIR/carmen/bin

loggerd $LOG_DIR/imp${WALK_NUM}_${GUIDANCE_CONFIG}_$TIMESTAMP.log.gz &
sleep 2
log_forcesd $LOG_DIR/imp${WALK_NUM}_${GUIDANCE_CONFIG}_$TIMESTAMP.flog.gz &

if [[ $GUIDANCE_CONFIG == *audio* ]]
  then echo "audio guidance on" ; GUIDANCE_CONFIG_AUDIO=1
else echo "audio guidance off" ; GUIDANCE_CONFIG_AUDIO=0
fi

GUIDANCE_CONFIG_TEXT=1
if [[ $GUIDANCE_CONFIG == *woz* ]]
  then
    GUIDANCE_CONFIG_TEXT=0
    GUIDANCE_CONFIG_AUDIO=0
    sleep 2
    if [[ $GUIDANCE_CONFIG == *audio* ]]
      then xterm -geometry 80x3+0+737 -e ./woz &
    else xterm -geometry 80x3+0+737 -e ./woz -notheta &
    fi
fi

if [[ $(($GUIDANCE_CONFIG_TEXT)) == 1 ]] ; then
  if [[ $(($GUIDANCE_CONFIG_AUDIO)) == 1 ]] ; then
    ROOMNAV_GUIDANCE_CONFIG_OPTS="-text -audio"
  else
    ROOMNAV_GUIDANCE_CONFIG_OPTS="-text"
  fi
else
  if [[ $(($GUIDANCE_CONFIG_AUDIO)) == 1 ]] ; then
    ROOMNAV_GUIDANCE_CONFIG_OPTS="-audio"
  else
    ROOMNAV_GUIDANCE_CONFIG_OPTS="-none"
  fi
fi

./roomnav_guidance_config $ROOMNAV_GUIDANCE_CONFIG_OPTS

./imp_goto $GUIDANCE_CONFIG $GOAL

cd -
