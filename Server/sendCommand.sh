#!/bin/bash

LOCALPATH=/home/beau/Git/SerreConnectee/Server
TALKARDUINO=$LOCALPATH/talkArduino_v4.py
LISTMEGA=$LOCALPATH/listMega.sh

ARDUINO_TARGET=`echo $@ | awk '{print $1}'`
ARGS=`echo $@ | sed 's/'$ARDUINO_TARGET' //'`

TTY=`$LISTMEGA | grep $ARDUINO_TARGET | awk -F: '{print $1}'`

$TALKARDUINO $TTY $ARGS
