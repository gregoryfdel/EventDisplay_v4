#$ -S /bin/tcsh
#
# script to write CTA WP Phys Files
#
#
# Author: Gernot Maier
#
#######################################################################

set AXRRAY=ARRAY
set DXDIR=DDIR
set OXBSTIME=OBSTIME
set OXUTNAME=OUTNAME
set OXFFSET=OFFSET
set RECID=RRRR

# set the right observatory (environmental variables)
source $EVNDISPSYS/setObservatory.tcsh CTA

rm -f $OXUTNAME-$OXBSTIME$AXRRAY-$OXFFSET.log

cd $EVNDISPSYS/
./bin/writeCTAWPPhysSensitivityFiles $AXRRAY $OXBSTIME $DXDIR $OXUTNAME CTA $OXFFSET $RECID > $OXUTNAME-$OXBSTIME$AXRRAY-$OXFFSET.log

############################################################################

exit
