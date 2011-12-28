//! VMathsandFunctions collects math utilities and external function to be used by TF1/TF2/etc
// Revision $Id: VMathsandFunctions.h,v 1.1.2.1 2011/01/21 10:10:57 gmaier Exp $

#ifndef VMathsandFunctions_h
#define VMathsandFunctions_h

#include "TMath.h"

#include "VAnalysisUtilities.h"
#include "VPlotUtilities.h"

using namespace std;

class VDouble_gauss : public VAnalysisUtilities, public VPlotUtilities
{
   public:
 
   double operator() (double *x, double *par);
      
   ClassDef(VDouble_gauss,1);

};

class VFun_gauss : public VAnalysisUtilities, public VPlotUtilities
{
   public:
 
   double operator() (double *x, double *par);
      
   ClassDef(VFun_gauss,1);
};

namespace VMathsandFunctions
{
   double getBaryCentricMeanEnergy( double e_min_log10, double e_max_log10, double iSpectralIndex );
   double getMeanEnergy( double e_min_log10, double e_max_log10 );
   double getSpectralWeightedMeanEnergy( double e_min_log10, double e_max_log10, double iSpectralIndex );
}

#endif

