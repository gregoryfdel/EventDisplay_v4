#!/bin/sh
#
# calculate effective areas and instrument response functions for CTA
#
#
# Author: Gernot Maier
#
##############################################################################


if [ $# -ne 7 ] && [ $# -ne 8 ] && [ $# -ne 9 ]
then
   echo
   echo "CTA.EFFAREA.sub_analyse.sh <subarray> <recid> <particle> <cutfile template> <scripts input parameter file> <outputsubdirectory> <data set> [filling mode] [direction (e.g. _180deg)]"
   echo "================================================================================"
   echo
   echo "calculate effective areas and instrument response functions for CTA"
   echo 
   echo
   echo "<subarray>"
   echo "     subarray for analysis (e.g. array E)"
   echo
   echo "<recid>"
   echo "     reconstruction ID from array reconstruction (mscw stage)"
   echo
   echo "<particle>"
   echo "     allowed particle types are:"
   echo "     gamma_onSource / gamma_cone / electron / electron_onSource / proton / proton_onSource / helium "
   echo
   echo "<cutfile template>"
   echo "     template for gamma/hadron cut file (full path and file name)"
   echo "     (examples can be found in $CTA_EVNDISP_AUX_DIR/GammaHadronCutFiles)"
   echo
   echo "<scripts parameter file>"
   echo "     file with analysis parameter"
   echo "     see e.g. CTA_EVNDISP_AUX_DIR/ParameterFiles/scriptsInput.runparameter"
   echo 
   echo "<outputsubdirectory>"
   echo "     directory with all result and log files (full path)"
   echo
   echo "<data set>         e.g. cta-ultra3, ISDC3700m, ...  "
   echo
   echo "[filling mode]"
   echo "     effective area filling mode (use 2 to calculate angular resolution only)"
   echo "     (optional, for default full calculation no option is needed)"
   echo
   echo "[direction]        e.g. for north: \"_180deg\", for south: \"_0deg\", for all directions: no option"
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
echo "reading analysis parameter from $ANAPAR"
NIMAGESMIN=`grep NIMAGESMIN $ANAPAR | awk {'print $2'}`
NTYPEMIN_0=`grep NTYPEMIN_0 $ANAPAR | awk {'print $2'}`
NTYPEMIN_1=`grep NTYPEMIN_1 $ANAPAR | awk {'print $2'}`
NTYPEMIN_2=`grep NTYPEMIN_2 $ANAPAR | awk {'print $2'}`
if [ -z "$NTYPEMIN_2" ]
then
   NTYPEMIN_2=0
fi
ANADIR=`grep MSCWSUBDIRECTORY  $ANAPAR | awk {'print $2'}`
EREC=`grep ENERGYRECONSTRUCTIONMETHOD $ANAPAR | awk {'print $2'}`
TMVACUT=`grep TMVASUBDIR $ANAPAR | awk {'print $2'}`
EFFAREADIR=`grep EFFAREASUBDIR $ANAPAR | awk {'print $2'}`
OBSTIME=`grep OBSERVINGTIME_H $ANAPAR | awk {'print $2'}`
if [ -z "$ANADIR" ] || [ -z "$NIMAGESMIN" ] || [ -z "$EREC" ] || [ -z "$TMVACUT" ] || [ -z "$EFFAREADIR" ] || [ -z "$OBSTIME" ] || [ -z "$NTYPEMIN_0" ] || [ -z "$NTYPEMIN_1" ] || [ -z "$NTYPEMIN_2" ]
then
  echo "error: analysis parameter file not correct: $ANAPAR" 
  echo " one variable missing"
  echo $NIMAGESMIN $ANADIR $EREC $TMVACUT $EFFAREADIR $OBSTIME
  exit
fi
echo "Input parameters read from $ANAPAR"
echo $NIMAGESMIN $ANADIR $EREC $TMVACUT $EFFAREADIR $OBSTIME
# parameters from the command line
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
if [ -n "$9" ]
then
  MCAZ=$9
fi

####################################
# check particle type
if [ $PART != "gamma_onSource" ] && [ $PART != "gamma_cone" ] && [ $PART != "proton" ] && [ $PART != "electron" ] &&  [ $PART != "electron_onSource" ] && [ $PART != "helium" ] && [ $PART != "proton_onSource" ] && [ $PART != "helium_onSource" ]
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
echo "directory for qsub shell scripts"
QSHELLDIR=$CTA_USER_DATA_DIR"/queueShellDir"
echo $QSHELLDIR
mkdir -p $QSHELLDIR
echo "data (input) directory"
if [ $PART = "gamma_onSource" ] || [ $PART = "electron_onSource" ] || [ $PART = "proton_onSource" ] || [ $PART == "helium_onSource" ]
then
  DDIR=$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$ARRAY/$ANADIR-onAxis/
else
  DDIR=$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$ARRAY/$ANADIR/
fi
echo $DDIR
mkdir -p $DDIR
echo "output log directory"
QDIR=$CTA_USER_LOG_DIR"/$DATE/EFFAREA/"
echo $QDIR
mkdir -p $QDIR
QDIR="/dev/null"
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
   MSCFILE=$DDIR/gamma_onSource."$ARRAY"_ID"$RECID$MCAZ"*.mscw.root
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=gamma_onSource."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. )
   OFFMAX=( 100000. )
   OFFMEA=( "0.0" )
# NOTE: this is theta2
   THETA2MIN=( -1. )
# using TMVA or angular resolution
   THETA2MAX=( -1. )
   ISOTROPY="0"
   AZBINS="0"
   TELTYPECUTS="1"
   DIRECTIONCUT="2"
fi
# isotropic gamma-rays: analyse in rings in camera distance
if [ $PART = "gamma_cone" ]
then
   MSCFILE=$DDIR/gamma_cone."$ARRAY"_ID"$RECID$MCAZ"*.mscw.root
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=gamma_cone."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. 1. 2. 3.0 3.5 4.0 4.5 5.0 )
   OFFMAX=( 1. 2. 3. 3.5 4.0 4.5 5.0 5.5 )
   OFFMEA=( 0.5 1.5 2.5 3.25 3.75 4.25 4.75 5.25 )
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
   MSCFILE=$DDIR/electron."$ARRAY"_ID"$RECID$MCAZ"*.mscw.root
   EFFFILE=$DDIR/EffectiveAreas/
   OFFMIN=( 0. )
   OFFMAX=( 100000. )
# NOTE: this is theta and not theta2
   if [ $PART = "electron" ]
   then
      OFIL=electron."$ARRAY"_ID"$RECID".eff
      THETA2MIN=( 0. 1. 2. 3.0 3.5 4.0 4.5 5.0  )
      THETA2MAX=( 1. 2. 3. 3.5 4.0 4.5 5.0 5.5 )
      OFFMEA=( 0.5 1.5 2.5 3.25 3.75 4.25 4.75 5.25  )
   else
      OFIL=electron_onSource."$ARRAY"_ID"$RECID".eff
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
   MSCFILE=$DDIR/proton*."$ARRAY"_ID"$RECID$MCAZ"*.mscw.root
   if [ $ARRAY = "V5" ]
   then
      MSCFILE=$DDIR/proton."$ARRAY"_ID"$RECID$MCAZ".mscw.root
   fi
   EFFFILE=$DDIR/EffectiveAreas/
   OFIL=proton."$ARRAY"_ID"$RECID".eff
   OFFMIN=( 0. )
   OFFMAX=( 100000. )
# NOTE: this is theta and not theta2
   if [ $PART = "proton" ] 
   then
      OFIL=proton."$ARRAY"_ID"$RECID".eff
      THETA2MIN=( 0. 1. 2. 3.0 3.5 4.0 4.5 5.0 )
      THETA2MAX=( 1. 2. 3. 3.5 4.0 4.5 5.0 5.5 )
      OFFMEA=( 0.5 1.5 2.5 3.25 3.75 4.25 4.75 5.25 )
   else
      OFIL=proton_onSource."$ARRAY"_ID"$RECID".eff
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
   MSCFILE=$DDIR/helium."$ARRAY"_ID"$RECID$MCAZ"*.mscw.root
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
      pwd

# wobble offset
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone" ] 
      then
         WOBBLEOFFSET=${OFFMEA[$i]}
      else
         WOBBLEOFFSET=${OFFMEA[$j]}
      fi
# angular resolution file
      if [ $PART = "gamma_onSource" ] 
      then
	 ANGRESFILE="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/AngularResolution/gamma_onSource."$ARRAY"_ID"$RECID".eff-0.root"
      else
	 ANGRESFILE="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/AngularResolution/gamma_cone."$ARRAY"_ID"$RECID".eff-$i.root"
      fi
# particle number file
      if [ $PART = "gamma_onSource" ] || [ $PART = "electron_onSource" ] || [ $PART = "proton_onSource" ]
      then
	 PNF="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/QualityCuts001CU/ParticleNumbers."$ARRAY".00.root"
      elif [ $PART = "gamma_cone" ]
      then
	 PNF="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/QualityCuts001CU/ParticleNumbers."$ARRAY".$i.root"
      else
	 PNF="$CTA_USER_DATA_DIR/analysis/AnalysisData/$DSET/$EFFAREADIR/QualityCuts001CU/ParticleNumbers."$ARRAY".$j.root"
      fi
      

      sed -i -e "s|OFFMIN|$iMIN|" \
             -e "s|OFFMAX|$iMAX|" \
             -e "s|THETA2MIN|$jMIN|" \
             -e "s|THETA2MAX|$jMAX|" \
             -e "s|DIRECTIONCUT|$DIRECTIONCUT|" \
             -e "s|SUBARRAY|$ARRAY|" \
             -e "s|MINIMAGES|$NIMAGESMIN|" \
	     -e "s|NTELTYPE0|$NTYPEMIN_0|" \
	     -e "s|NTELTYPE1|$NTYPEMIN_1|" \
	     -e "s|NTELTYPE2|$NTYPEMIN_2|" \
             -e "s|WOBBLEOFFSET|$WOBBLEOFFSET|" \
             -e "s|TMVACUTDIR|$TMVACUT|" \
             -e "s|DATASET|$DSET|" \
             -e "s|ANGRESFILE|$ANGRESFILE|" \
             -e "s|PARTICLENUMBERFILE|$PNF|" \
             -e "s|OBSERVINGTIME_H|$OBSTIME|" $iCFIL
      iCFIL=$ODIR/effectiveArea-CTA-$DSET-$PART-$i-$j.$iCBFILE
      cd $EVNDISPSYS/scripts/CTA/
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
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone" ]
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
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone" ]
      then
         echo "* ENERGYSPECTRUMINDEX  1 2.5 0.1" >> $MSCF
      fi
# first half of data set is not used (as these events are used for the TMVA training)
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone" ]
      then
        echo "* IGNOREFRACTIONOFEVENTS 0.5" >> $MSCF
      fi
      if [ $PART = "proton" ] || [ $PART = "proton_onSource" ]
      then
# first half of data set is not used (as these events are used for the TMVA training)
        if [ $DSET != "v_leeds" ]
	then
	   echo "* IGNOREFRACTIONOFEVENTS 0.5" >> $MSCF
        fi
      fi
      echo "* CUTFILE $iCFIL" >> $MSCF
      echo "* SIMULATIONFILE_DATA $MSCFILE" >> $MSCF

# output file
      if [ $PART = "gamma_onSource" ] || [ $PART = "gamma_cone" ]
      then
         OFIX=$ODIR/$OFIL-$i
      else
         OFIX=$ODIR/$OFIL-$j
      fi

# create run script
      FNAM="CTAeffArea-$DSET-$PART-$ARRAY-$i-$j-$EREC"
# run parameter file
      sed -e "s|IIIIFIL|$MSCF|" $FSCRIPT.sh > $QSHELLDIR/$FNAM-1.sh
# output file
      sed -e "s|TTTTFIL|$OFIX|" $QSHELLDIR/$FNAM-1.sh > $QSHELLDIR/$FNAM.sh
      rm -f $QSHELLDIR/$FNAM-1.sh
      chmod u+x $QSHELLDIR/$FNAM.sh

      echo
      echo "preparing new analysis run"
      echo "--------------------------"
      echo
      echo "gamma/hadron separation file"
      echo $iCFIL
      echo $PNF
      echo "run script is $QSHELLDIR/$FNAM.sh"
      echo "batch log and error files are written to $QDIR"
      echo "parameter files write written to $QDIR"

# submit the job
     if [ $GFILLING = "2" ]
     then
	qsub -l os="sl6" -l h_cpu=5:29:00 -l h_vmem=4000M -l tmpdir_size=1G  -V -o $QDIR -e $QDIR "$QSHELLDIR/$FNAM.sh"
     else
	qsub -l os="sl6" -l h_cpu=11:29:00 -l h_vmem=6000M -l tmpdir_size=1G  -V -o $QDIR -e $QDIR "$QSHELLDIR/$FNAM.sh"
     fi
   done
done

exit
