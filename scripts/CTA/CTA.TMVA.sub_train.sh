#!/bin/sh
#
# script to train cuts/MVAs with TMVA
#
#
# Author: Gernot Maier
#

if [ ! -n "$1" ] || [ ! -n "$2" ] || [ ! -n "$3" ] || [ ! -n "$4" ] || [ ! -n "$5" ] || [ ! -n "$5" ]
then
   echo
   echo "CTA.TMVA.sub_train.sh <subarray list> <run parameter filename> <directory for run parameter and log files> <output file name> <onSource/cone10> <data set>"
   echo ""
   echo "  <subarray list>   text file with list of subarray IDs"
   echo
   echo "  <run parameter filename> without .runparameter"
   echo
   echo "  <onSource/cone10>    calculate tables for on source or different wobble offsets"
   echo
   echo "  <data set>         e.g. cta-ultra3, ISDC3700, ...  "
   echo
   echo "   note 1: keywords ENERGYBINS and OUTPUTFILE are ignored in the runparameter file"
   echo
   echo "   note 2: energy and wobble offset bins are hardwired in this scripts"
   echo
   echo "   note 3: adjust h_cpu depending on your MVA method"
   echo
   exit
fi

RPAR=$2
RXPAR=`basename $RPAR.runparameter runparameter`
DDIR=$3
OFIL=$4
# energy reconstruction method
EREC=0
CONE="FALSE"
if [ $5 == "cone10" ] || [ $5 == "cone" ]
then
  CONE="TRUE"
fi
DSET=$6
VARRAY=`awk '{printf "%s ",$0} END {print ""}' $1`

#####################################
# energy bins
EMIN=( -2.50 -1.75 -1.25 -1.00 -0.75 -0.50 -0.25 0.00 0.25 0.50 0.75 1.00 )
EMAX=( -1.25 -1.00 -0.75 -0.50 -0.25  0.00  0.25 0.50 0.75 1.00 1.50 2.50 )
NENE=${#EMIN[@]}
#####################################
# 
if [ $CONE == "TRUE" ]
then
   OFFMIN=( 0.0 1.0 2.0 3.00 3.50 4.00 4.50 5.00 5.50 )
   OFFMAX=( 1.0 2.0 3.0 3.50 4.00 4.50 5.00 5.50 6.00 )
   OFFMEA=( 0.5 1.5 2.5 3.25 3.75 4.25 4.75 5.25 5.75 )
   DSUF="gamma_cone10"
else
   OFFMIN=( "-1.e10" )
   OFFMAX=( "1.e10" )
   OFFMEA=( 0.0 )
   DSUF="gamma_onSource"
fi
NOFF=${#OFFMIN[@]}


# checking the path for binary
if [ -z $EVNDISPSYS ]
then
    echo "no EVNDISPSYS env variable defined"
    exit
fi

# log files
DATE=`date +"%y%m%d"`
FDIR=$CTA_USER_LOG_DIR/queueTMVA/$DATE
mkdir -p $FDIR
echo "log directory: " $FDIR

# script name template
FSCRIPT="CTA.TMVA.qsub_train"

###############################################################
# loop over all arrays
for ARRAY in $VARRAY
do
   echo "STARTING ARRAY $ARRAY"

   for (( W = 0; W < $NOFF; W++ ))
   do
      ODIR=$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$ARRAY/TMVA/$DDIR-${OFFMEA[$W]}
      mkdir -p $ODIR
# copy run parameter file
      cp -f $RPAR.runparameter $ODIR

# signal and background files
      SFIL=`ls -1 $CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$ARRAY/Analysis/$DSUF."$ARRAY"_ID0*.root`
      BFIL=`ls -1 $CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$ARRAY/Analysis/proton."$ARRAY"_ID0*.root`

###############################################################
# loop over all energy bins and submit a job for each bin
      for ((i=0; i < $NENE; i++))
      do

# run parameter file
	 RFIL=$ODIR/$RXPAR$ARRAY"_$i"
	 echo $RFIL
	 rm -f $RFIL
	 echo "* ENERGYBINS $EREC ${EMIN[$i]} ${EMAX[$i]}" > $RFIL.runparameter
	 echo "* MCXYOFF (MCxoff*MCxoff+MCyoff*MCyoff)>=${OFFMIN[$W]}&&(MCxoff*MCxoff+MCyoff*MCyoff)<${OFFMAX[$W]}" >> $RFIL.runparameter
	 grep "*" $RPAR.runparameter | grep -v ENERGYBINS | grep -v OUTPUTFILE | grep -v SIGNALFILE | grep -v BACKGROUNDFILE | grep -v MCXYOFF >> $RFIL.runparameter
	 echo "* OUTPUTFILE $ODIR $OFIL"_$i" " >> $RFIL.runparameter
	 for arg in $SFIL
	 do
	    echo "* SIGNALFILE $arg" >> $RFIL.runparameter
	 done
	 for arg in $BFIL
	 do
	    echo "* BACKGROUNDFILE $arg" >> $RFIL.runparameter
	 done

# run script
	 FNAM=$ODIR/$FSCRIPT.$ARRAY"_$i"

	 sed -e "s|RUNPARA|$RFIL|" $FSCRIPT.sh > $FNAM-1.sh
	 sed -e "s|OFIL|$ODIR/$OFIL"_$i"|" $FNAM-1.sh > $FNAM.sh
	 rm -f $FNAM-1.sh

	 chmod u+x $FNAM.sh
	 echo $FNAM.sh

#################################
# submit job to queue
# medium queue: BDT 
	 qsub -V -l os="sl*"  -l h_cpu=10:29:00 -l h_vmem=8000M -l tmpdir_size=5G -o $FDIR -e $FDIR "$FNAM.sh"
# long queue: needed for box cut optimization
#         qsub -V -l h_cpu=38:00:00 -l h_vmem=8000M -l tmpdir_size=5G -o $FDIR -e $FDIR "$FNAM.sh"
     done
  done
done

exit
