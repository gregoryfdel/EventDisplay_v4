#!/bin/sh
#
# run laser/flasher calibration over a bunch of laser runs 
# read a calibration file produced by write_analysis_scripts.pl
# (do this only if gain files don't exist yet)
#
# Author: Gernot Maier
#


if [ ! -n "$1" ] 
then
   echo "VTS.EVNDISP.analyse_laser_runs_from_calibfile.sh <calib file>"
   exit
fi

DDIR=$VERITAS_DATA_DIR"/data/"

RLIST=$1
FILES=`grep LASER $RLIST | awk '{print $2"_"$3}'`

# checking the path for binary
if [ -z $EVNDISPSYS ]
then
    echo "no EVNDISPSYS env variable defined"
    exit
fi


for AFIL in $FILES
do
    RUN=${AFIL:0:5}
    DTEL=${AFIL:6}
    echo $AFIL $RUN $DTEL
    DFIL=`find -L $DDIR -name "$RUN.cvbf"`

    if [ -e $DFIL ]
	 then
	    $EVNDISPSYS/scripts/VTS/VTS.EVNDISP.analyse_laser_run $DTEL $DFIL
         else
	    echo "missing laser/flasher file $FIL"
	    echo "please download"
    fi
done

exit
