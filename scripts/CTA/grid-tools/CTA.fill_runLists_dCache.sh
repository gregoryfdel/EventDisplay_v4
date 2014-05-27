#!/bin/bash
#
# script to check which files are available on local dCache
#
#

if [ ! -n "$1" ] && [ ! -n "$2" ] 
then
   echo "./CTA.fill_runLists_dCache.sh <run list> <output list file directory>"
   echo
   exit
fi

# dcache client
export DCACHE_CLIENT_ACTIVE=1

# output list file directory
mkdir -p $2
TLIS=`basename $1`
FILELO="$2/$TLIS"
rm -f $FILELO.*
touch $FILELO.dcache

# loop over all files in the list
FILEL=`cat $1`
for i in $FILEL
do
   OFIL=`basename $i`
# check if the file is on the local dCache
   DC="/acs/grid/cta/$i"
   if [ -e $DC ]
   then
      echo $DC >> $FILELO.dcache
   fi
done

# sort according to particle types
grep proton $FILELO.dcache > $FILELO.proton.dcache.list
grep electron $FILELO.dcache > $FILELO.electron.dcache.list
grep gamma_ptsrc $FILELO.dcache > $FILELO.gamma_onSource.dcache.list
grep -v gamma_ptsrc $FILELO.dcache | grep gamma > $FILELO.gamma_cone.dcache.list

exit
