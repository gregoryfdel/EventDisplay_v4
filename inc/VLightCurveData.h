//! VLightCurveData  data class for light curve calculations
// Revision $Id: VLightCurveData.h,v 1.1.2.2 2011/06/16 14:53:04 gmaier Exp $

#ifndef VLightCurveData_H
#define VLightCurveData_H

#include <string>
#include <vector>

#include <TObject.h>

#include "VFluxCalculation.h"

using namespace std;

class VLightCurveData : public TObject
{
   private:

   public:

   string fName;
   string fDataFileName;

   double fMJD_min;
   double fMJD_max;

   double fEnergy_min_TeV;
   double fEnergy_max_TeV;
   double fMinEnergy;
   double fMaxEnergy;
   double fE0;
   double fAlpha;

   vector< double > fRunList;
   double fMJD_Data_min;
   double fMJD_Data_max;
   double fRunTime;
   double fNon;
   double fNoff;
   double fNoffAlpha;
   double fSignificance;
   double fFlux;
   double fFluxError;
   double fUpperFluxLimit;
   double fRunFluxCI_lo_1sigma;
   double fRunFluxCI_up_1sigma;
   double fRunFluxCI_lo_3sigma;
   double fRunFluxCI_up_3sigma;

   VLightCurveData( string iName = "lightcurvedata" );
  ~VLightCurveData() {}
   bool   fillTeVEvndispData( string iAnaSumFile, double iThresholdSignificance = -99999., double iMinEvents = -9999., 
                              double iUpperLimit = 0.99, int iUpperlimitMethod = 0, int iLiMaEqu = 17, double iMinEnergy = 0., 
			      double E0 = 1., double alpha = -2.5 );
   double getMJD();
   double getMJDError();
   void   setFluxCalculationEnergyInterval( double iEnergy_min_TeV = 1., double iEnergy_max_TeV = -1. );
   void   setMJDInterval( double iMJD_min, double iMJD_max ) { fMJD_min = iMJD_min; fMJD_max = iMJD_max; }

   ClassDef( VLightCurveData, 1 );
};

#endif
