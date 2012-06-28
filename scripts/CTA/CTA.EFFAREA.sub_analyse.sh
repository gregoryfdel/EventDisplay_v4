#!/bin/sh
#
# make effective areas for CTA
#
#
# Revision $Id$
#
# Author: Gernot Maier
#


if [ ! -n "$1" ] || [ ! -n "$2" ] || [ ! -n "$3" ] || [ ! -n "$4" ] || [ ! -n "$5" ] || [ ! -n "$6" ] || [ ! -n "$7" ]
then
   echo
   echo "CTA.EFFAREA.sub_analyse.sh <subarray> <recid> <particle> <cutfile template> <analysis parameter file> <outputsubdirectory> <data set> [filling mode]"
   echo "================================================================================"
   echo
   echo "make effective areas for CTA"
   echo 
   echo "(note: shape cuts hardwired)"
   echo
   echo "<subarray>"
   echo "     subarray for analysis (e.g. array E)"
   echo
   echo "<recid>"
   echo "     reconstruction ID from array reconstruction"
   echo
   echo "<particle>"
   echo "     gamma_onSource / gamma_cone10 / electron / electron_onSource / proton / proton_onSource / helium "
   echo
   echo "<cutfile template>"
   echo "     template for gamma/hadron cut file (full path and file name)"
   echo
   echo "<analysis parameter file>"
   echo "     file with analysis parameter"
   echo 
   echo "<outputsubdirectory>"
   echo "     directory with all result and log files (full path)"
   echo
   echo "  <data set>         e.g. cta-ultra3, ISDC3700m, ...  "
   echo
   echo "[filling mode]"
   echo "     effective area filling mode (use 2 to calculate angular resolution only)"
   echo
   exit
fi

echo
echo "make effective areas for CTA: create run scripts"
echo "------------------------------------------------"
echo

######################################################################
# input variables
######################################################################

#######################################
# read values from parameter file
ANAPAR=$5
if [ ! -e $ANAPAR ]
then
  echo "error: analysis parameter file not found: $ANAPAR" 
  exit
fi
echo "reading anaysis parameter from $ANAPAR"
NIMAGESMIN=`grep NIMAGESMIN $ANAPAR | awk {'print $2'}`
ANADIR=`grep MSCWSUBDIRECTORY  $ANAPAR | awk {'print $2'}`
EREC=`grep ENERGYRECONSTRUCTIONMETHOD $ANAPAR | awk {'print $2'}`
TMVACUT=`grep TMVASUBDIR $ANAPAR | awk {'print $2'}`
EFFAREADIR=`grep EFFAREASUBDIR $ANAPAR | awk {'print $2'}`
echo $NIMAGESMIN $ANADIR $EREC $TMVACUT $EFFAREADIR
# parameters from command line
ARRAY=$1
RECID=$2
PART=$3
CFIL=$4
ODIR=$6
GFILLING=0
DSET=$7
if [ -n "$8" ]
then
  GFILLING=$8
fi

# check particle type
if [ $PART != "gamma_onSource" ] && [ $PART != "gamma_cone10" ] && [ $PART != "proton" ] && [ $PART != "electron" ] &&  [ $PART != "electron_onSource" ] && [ $PART != "helium" ] && [ $PART != "proton_onSource" ] && [ $PART != "helium_onSource" ] && [ $PART != "gamma_onSourceDISP" ]
then
   echo "unknown particle type: " $PART
   exit
fi

######################################################################
# qsub script
######################################################################
FSCRIPT="CTA.EFFAREA.qsub_analyse"

######################################################################
# directories
######################################################################
DATE=`date +"%y%m%d"`
echo "directory for qsub output/error files"
QLOGDIR=$CTA_USER_LOG_DIR"/queueLogDir"
echo $QLOGDIR
mkdir -p $QLOGDIR
echo "directory for qsub shell scripts"
QSHELLDIR=$CTA_USER_LOG_DIR"/queueShellDir"
echo $QSHELLDIR
mkdir -p $QSHELLDIR
echo "data (input) directory"
DDIR=$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$ARRAY/$ANADIR/
echo $DDIR
mkdir -p $DDIR
echo "output log directory"
FDIR=$CTA_USER_LOG_DIR"/queueEffArea/$DATE/"
echo $FDIR
mkdir -p $FDIR
echo "output data directory"
echo $ODIR
mkdir -p $ODIR

######################################################################
# input files
######################################################################
# data directory
# on source gamma rays
if [ $PART = "gamma_onSource" ]
then
   MSCFILE=$DDIR/gamma_onSource."$ARRAY"_ID"$RECID"*.mscw.root
   if [ $ARRAY = "V5" ]
   then
      MSCFILE=$DDIR/gamma_onSource."$ARRAY"_ID"$RECID".mscw.root
   fi
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=gamma_onSource."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. )
   OFFMAX=( 100000. )
   OFFMEA=( "0.0" )
# NOTE: this is theta2
   THETA2MIN=( -1. )
#   THETA2MAX=( 0.008 )
#   THETA2MAX=( 0.04 )
# using TMVA or angular resolution
   THETA2MAX=( -1. )
   ISOTROPY="0"
   AZBINS="0"
   TELTYPECUTS="1"
   DIRECTIONCUT="2"
fi
# on source gamma rays
if [ $PART = "gamma_onSourceDISP" ]
then
   MSCFILE=$DDIR/gamma_onSourceDISP."$ARRAY"_ID"$RECID"*.mscw.root
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=gamma_onSourceDISP."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. )
   OFFMAX=( 100000. )
   OFFMEA=( "0.0" )
# NOTE: this is theta2
   THETA2MIN=( -1. )
#   THETA2MAX=( 0.008 )
#   THETA2MAX=( 0.04 )
# using TMVA or angular resolution
   THETA2MAX=( -1. )
   ISOTROPY="0"
   AZBINS="0"
   TELTYPECUTS="1"
   DIRECTIONCUT="2"
fi
# isotropic gamma-rays: analyse in rings in camera distance
if [ $PART = "gamma_cone10" ]
then
   MSCFILE=$DDIR/gamma_cone10."$ARRAY"_ID"$RECID"*.mscw.root
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=gamma_cone10."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. 1. 2. 3.0 3.5 4.0 4.5 5.0 )
   OFFMAX=( 1. 2. 3. 3.5 4.0 4.5 5.0 5.5 )
   OFFMEA=( 0.5 1.5 2.5 3.25 3.75 4.25 4.75 5.25 )
# PRELIMINARY! use on axis TMVA for all off axis gamma/hadron separation
#   OFFMEA=( 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 )
# NOTE: this is theta2
   THETA2MIN=( -1. )
   THETA2MAX=( -1. )
   ISOTROPY="0"
   AZBINS="0"
   TELTYPECUTS="1"
   DIRECTIONCUT="2"
fi
if [ $PART = "electron" ] || [ $PART = "electron_onSource" ]
then
   MSCFILE=$DDIR/electron."$ARRAY"_ID"$RECID"*.mscw.root
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=electron."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. )
   OFFMAX=( 100000. )
# NOTE: this is theta and not theta2
   if [ $PART = "electron" ]
   then
      THETA2MIN=( 0. 1. 2. 3.0 3.5 4.0 4.5 5.0  )
      THETA2MAX=( 1. 2. 3. 3.5 4.0 4.5 5.0 5.5 )
#      OFFMEA=( 0.5 1.5 2.5 3.25 3.75 4.25 4.75 5.25  )
      OFFMEA=( 0.0 1.5 2.5 3.25 3.75 4.25 4.75 5.25  )
# use on axis TMVA for all off axis gamma/hadron separation
#      OFFMEA=( 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 )
   else
      THETA2MIN=( 0. )
      THETA2MAX=( 1. )
      OFFMEA=( 0.0 )
   fi   
   ISOTROPY="1"
   AZBINS="0"
   TELTYPECUTS="1"
   DIRECTIONCUT="0"
fi
if [ $PART = "proton" ] || [ $PART = "proton_onSource" ]
then
   MSCFILE=$DDIR/proton*."$ARRAY"_ID"$RECID"*.mscw.root
   if [ $ARRAY = "V5" ]
   then
      MSCFILE=$DDIR/proton."$ARRAY"_ID"$RECID".mscw.root
   fi
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=proton."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. )
   OFFMAX=( 100000. )
# NOTE: this is theta and not theta2
   if [ $PART = "proton" ] 
   then
      THETA2MIN=( 0. 1. 2. 3.0 3.5 4.0 4.5 5.0 )
      THETA2MAX=( 1. 2. 3. 3.5 4.0 4.5 5.0 5.5 )
#      OFFMEA=( 0.5 1.5 2.5 3.25 3.75 4.25 4.75 5.25 )
      OFFMEA=( 0.0 1.5 2.5 3.25 3.75 4.25 4.75 5.25 )
# use on axis TMVA for all off axis gamma/hadron separation
#      OFFMEA=( 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 )
   else
      THETA2MIN=( 0. )
      THETA2MAX=( 1. )
      OFFMEA=( 0.0 )
   fi
   ISOTROPY="1"
   AZBINS="0"
   TELTYPECUTS="1"
   DIRECTIONCUT="0"
fi
if [ $PART = "helium" ] || [ $PART = "helium_onSource" ]
then
   MSCFILE=$DDIR/helium."$ARRAY"_ID"$RECID"*.mscw.root
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=helium."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. )
   OFFMAX=( 100000. )
# NOTE: this is theta and not theta2
   if [ $PART = "helium" ] 
   then
      THETA2MIN=( 0. 1. 2. 3.0 3.5 4.0 4.5 5.0 )
      THETA2MAX=( 1. 2. 3. 3.5 4.0 4.5 5.0 5.5 )
      OFFMEA=( 0.5 1.5 2.5 3.25 3.75 4.25 4.75 5.25 )
# use on axis TMVA for all off axis gamma/hadron separation
#      OFFMEA=( 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 )
   else
      THETA2MIN=( 0. )
      THETA2MAX=( 1. )
      OFFMEA=( 0.0 )
   fi
   ISOTROPY="1"
   AZBINS="0"
   TELTYPECUTS="1"
   DIRECTIONCUT="0"
fi
NOFF=${#OFFMIN[@]}
NTH2=${#THETA2MIN[@]}
######################################################################


###############################################################################
# loop over all MC cuts
for ((i=0; i < $NOFF; i++))
do
   iMIN=${OFFMIN[$i]}
   iMAX=${OFFMAX[$i]}
# loop over all theta2 cuts
   for ((j=0; j < $NTH2; j++))
   do
     jMIN=${THETA2MIN[$j]}
     jMAX=${THETA2MAX[$j]}

###############################################################################
# theta2 cut of protons and electron should match the rings from the isotropic gammas
      if [ $PART = "proton" ] || [ $PART = "proton_onSource" ] || [ $PART = "electron" ] || [ $PART = "electron_onSource" ] || [ $PART = "helium" ]
      then
         jMIN=$(echo "$jMIN*$jMIN" | bc -l )
         jMAX=$(echo "$jMAX*$jMAX" | bc -l )
      fi

###############################################################################
# create cut file
      iCBFILE=`basename $CFIL`      
      iCFIL=$ODIR/effectiveArea-CTA-$DSET-$PART-$i-$j.$iCBFILE
      if [ ! -e $CFIL ]
      then
        echo "ERROR: cut file does not exist:"
	echo $CFIL
	exit
      fi
      cp -f $CFIL $iCFIL

      sed -e "s|OFFMIN|$iMIN|" $iCFIL > $iCFIL-a
      rm -f $iCFIL
      sed -e "s|OFFMAX|$iMAX|" $iCFIL-a > $iCFIL-b
      rm -f $iCFIL-a
      sed -e "s|THETA2MIN|$jMIN|" $iCFIL-b > $iCFIL-c
      rm -f $iCFIL-b
      sed -e "s|THETA2MAX|$jMAX|" $iCFIL-c > $iCFIL-d
      rm -f $iCFIL-c
      sed -e "s|DIRECTIONCUT|$DIRECTIONCUT|" $iCFIL-d > $iCFIL-e
      rm -f $iCFIL-d
      sed -e "s|SUBARRAY|$ARRAY|" $iCFIL-e > $iCFIL-e1
      rm -f $iCFIL-e
      sed -e "s|MINIMAGES|$NIMAGESMIN|" $iCFIL-e1 > $iCFIL-f
      rm -f $iCFIL-e1
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone10" ] || [ $PART = "gamma_onSourceDISP" ]
      then
         sed -e "s|WOBBLEOFFSET|${OFFMEA[$i]}|" $iCFIL-f > $iCFIL-g
      else
         sed -e "s|WOBBLEOFFSET|${OFFMEA[$j]}|" $iCFIL-f > $iCFIL-g
      fi
      rm -f $iCFIL-f
      sed -e "s|TMVACUTDIR|$TMVACUT|" $iCFIL-g > $iCFIL-h
      rm -f $iCFIL-g
      if [ $DSET = "v_leeds" ]
      then
	 sed -e "s|DATASET|cta-ultra3|" $iCFIL-h > $iCFIL-i
      else
	 sed -e "s|DATASET|$DSET|" $iCFIL-h > $iCFIL-i
      fi
# angular resolution file
      rm -f $iCFIL-h
      if [ $PART = "gamma_onSource" ] 
      then
	 ANGRESFILE="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/AngularResolution/gamma_onSource."$ARRAY"_ID"$RECID".eff-0.root"
      else
	 ANGRESFILE="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/AngularResolution/gamma_cone10."$ARRAY"_ID"$RECID".eff-$i.root"
      fi
      sed -e "s|ANGRESFILE|$ANGRESFILE|" $iCFIL-i > $iCFIL-j
      rm -f $iCFIL-i
# particle number file
      if [ $PART = "gamma_onSource" ] || [ $PART = "electron_onSource" ] || [ $PART = "proton_onSource" ]
      then
	 PNF="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/QualityCuts001CU/ParticleNumbers."$ARRAY".00.root"
      elif [ $PART = "gamma_cone10" ]
      then
	 PNF="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/QualityCuts001CU/ParticleNumbers."$ARRAY".$i.root"
      else
	 PNF="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/QualityCuts001CU/ParticleNumbers."$ARRAY".$j.root"
      fi
      sed -e "s|PARTICLENUMBERFILE|$PNF|" $iCFIL-j > $iCFIL-k
      rm -f $iCFIL-j
      mv -f $iCFIL-k $iCFIL
      echo $iCFIL

###############################################################################
# create run list
      MSCF=$ODIR/effectiveArea-CTA-$DSET-$PART-$i-$j.$ARRAY.dat
      rm -f $MSCF
      echo "effective area data file for $PART $i $j" > $MSCF
###############################################################################
# general run parameters
###############################################################################
# filling mode
###############################################################################
# fill IRFs and effective areas
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone10" ] || [ $PART = "gamma_onSourceDISP" ]
      then
# filling mode 0: fill and use angular resolution for energy dependent theta2 cuts
	 echo "* FILLINGMODE $GFILLING" >> $MSCF
      else
# background: use fixed theta2 cut
	 echo "* FILLINGMODE 3" >> $MSCF
      fi
# fill IRFs only
      echo "* ENERGYRECONSTRUCTIONMETHOD $EREC" >> $MSCF
      echo "* ENERGYAXISBINS 60" >> $MSCF
      echo "* ENERGYRECONSTRUCTIONQUALITY 0" >> $MSCF
# one azimuth bin only
      echo "* AZIMUTHBINS $AZBINS" >> $MSCF
      echo "* ISOTROPICARRIVALDIRECTIONS $ISOTROPY" >> $MSCF
      echo "* TELESCOPETYPECUTS $TELTYPECUTS" >> $MSCF
# do fill analysis (a 1 would mean that MC histograms would be filled only)
      echo "* FILLMONTECARLOHISTOS 0" >> $MSCF
# spectral index
      if [ $PART = "proton" ] || [ $PART = "proton_onSource" ]
      then
         echo "* ENERGYSPECTRUMINDEX  1 2.6 0.1" >> $MSCF
      fi
      if [ $PART = "helium" ] 
      then
         echo "* ENERGYSPECTRUMINDEX  1 2.6 0.1" >> $MSCF
      fi
      if [ $PART = "electron" ] || [ $PART = "electron_onSource" ]
      then
         echo "* ENERGYSPECTRUMINDEX  1 3.0 0.1" >> $MSCF
      fi
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone10" ] || [ $PART = "gamma_onSourceDISP" ]
      then
         echo "* ENERGYSPECTRUMINDEX  1 2.5 0.1" >> $MSCF
      fi
      echo "* CUTFILE $iCFIL" >> $MSCF
      echo "* SIMULATIONFILE_DATA $MSCFILE" >> $MSCF

# output file
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone10" ] || [ $PART = "gamma_onSourceDISP" ]
      then
         OFIX=$ODIR/$OFIL-$i
      else
         OFIX=$ODIR/$OFIL-$j
      fi

# create run script
      FNAM="CTAeffArea-$PART-$ARRAY-$i-$j-$EREC"
# run parameter file
      sed -e "s|IIIIFIL|$MSCF|" $FSCRIPT.sh > $QSHELLDIR/$FSCRIPT-1.sh
# output file
      sed -e "s|TTTTFIL|$OFIX|" $QSHELLDIR/$FSCRIPT-1.sh > $QSHELLDIR/$FSCRIPT-2.sh
      rm -f $QSHELLDIR/$FSCRIPT-1.sh
      mv -f $QSHELLDIR/$FSCRIPT-2.sh $QSHELLDIR/$FNAM.sh

      chmod u+x $QSHELLDIR/$FNAM.sh

      echo
      echo "preparing new analysis run"
      echo "--------------------------"
      echo
      echo "gamma/hadron separation file"
      echo $iCFIL
      echo $PNF
      echo "run script is $QSHELLDIR/$FNAM.sh"
      echo "batch log and error files are written to $FDIR"
      echo "parameter files write written to $FDIR"

# submit the job
#     qsub -l os="sl*" -l h_cpu=0:29:00 -js 200 -l h_vmem=6000M -l tmpdir_size=10G  -V -o $FDIR -e $FDIR "$QSHELLDIR/$FNAM.sh"
     qsub -l os="sl*" -l h_cpu=11:29:00 -js 200 -l h_vmem=6000M -l tmpdir_size=10G  -V -o $FDIR -e $FDIR "$QSHELLDIR/$FNAM.sh"
   done
done

exit

