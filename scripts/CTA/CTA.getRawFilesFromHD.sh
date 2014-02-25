#!/bin/bash
#
# simple script to download raw files from Heidelberg
#
# (you have to add the password manually)
#

if [ ! -n "$1" ] || [ ! -n "$2" ]
then
   echo "CTA.getRawFilesFromHD.sh <run list> <target directory>"
   echo
   exit
fi

PWD="1000srcs*"
if [ -z "$PWD" ]
then
   echo "error: please add manually the password"
   exit
fi

if [ -e $2 ]
then
   mkdir -p $2
fi

FILEL=`cat $1`
for j in $FILEL
do
    i=`basename $j`
    if [ -e $2/$i ] && [ -s $2/$i ]
    then
       echo "FILE EXISTS: $2/$i"
    else
#       rm -f $2/$i
#       wget --user=CTAraw --password=$PWD -O $2/$i http://www.mpi-hd.mpg.de/personalhomes/bernlohr/cta-raw/cta-prod2-aar500/$i
       wget --user=CTAraw --password=$PWD -O $2/$i http://www.mpi-hd.mpg.de/personalhomes/bernlohr/cta-raw/cta-prod2-40deg-aar/$i
    fi
done

exit
