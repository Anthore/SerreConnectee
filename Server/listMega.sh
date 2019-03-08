#!/bin/bash

DBFILE=/home/beau/Git/SerreConnectee/Server/arduino_db.txt
f=`mktemp`

for a in `\ls /dev/ttyACM*`
do
  udevadm info --query property --name $a > $f
  vendor=`cat $f | grep ID_VENDOR_ID= | awk -F= '{print $2}' `
  if [[ $vendor -eq 2341 ]]
  then
    model=`cat $f  | grep ID_MODEL= | awk -F= '{print $2}' `
    if [[ $model -eq 0042 ]]
    then
      sn=`cat $f | grep ID_SERIAL_SHORT= | awk -F= '{print $2}' `
      shortname=`grep $sn < $DBFILE | awk '{print $2}'`
      if [[ $shortname != "" ]] 
      then
        echo $a:$shortname
      else
        echo $sn is not a known S/N of arduino mega board 1>&2
      fi 
    fi 
  fi 
done

rm -f $f

#0042
