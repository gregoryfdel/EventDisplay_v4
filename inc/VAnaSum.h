//! VAnaSum    class for producing an analysis summary from parameterized veritas data.
//  Revision $Id: VAnaSum.h,v 1.16.2.5.2.1.2.2.12.1.4.1.2.1.2.2.8.1.2.1 2010/03/08 07:45:08 gmaier Exp $

#ifndef VANASUM_H
#define VANASUM_H

#include "VMonoPlots.h"
#include "VOnOff.h"
#include "VRatePlots.h"
#include "VAnaSumRunParameter.h"
#include "VRunSummary.h"
#include "VStatistics.h"
#include "VStereoAnalysis.h"
#include "VTableLookupRunParameter.h"

#include "TBranch.h"
#include "TChain.h"
#include "TDirectory.h"
#include "TDirectoryFile.h"
#include "TFile.h"
#include "TGraphErrors.h"
#include "TH1.h"
#include "TH2.h"
#include "TKey.h"
#include "TLatex.h"
#include "TLeaf.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include <bitset>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>
#include <set>

using namespace std;

class VAnaSum
{
    public:
        VAnaSum( string i_datadir, unsigned int fMode );
        ~VAnaSum() {}

        void doMonoAnalysis( bool iFull );
        void doStereoAnalysis( bool iSkyPlots );
        void initialize(string i_listfilename, int i_singletel, unsigned int iRunType, string i_outfile, int iRandomSeed, string fRunParameterfile );
        void terminate();

    private:

        void copyDirectory( TDirectory* );
        void doLightCurves( TDirectory *iDir, double ialpha, VStereoAnalysis *ion, VStereoAnalysis *ioff );
        void doMonoAnalysis( int irunon, int irunoff, double iexp_on, double i_exp_off, vector<TDirectory *> idir );
        void doStereoAnalysis( int icounter, int irunon, int irunoff, TDirectory *idir );
        void fillRunSummary( int onrun, int offrun, double iexp_on, double iexp_off, double i_nevts_on, double i_nevts_off, double i_norm_alpha, double i_sig, double i_rate, double i_rateOFF, VOnOff *fstereo_onoff );
        double getAzRange( int i_run, string i_treename, double &azmin, double &azmax );
        double getNoiseLevel( int i_run, double iNoiseLevel );
        void makeEnergySpectrum( int ionrun, TList*, double );
	void make2DEnergySpectrum( int ionrun, TList* l);

        bool bTotalAnalysisOnly;
        bool bUpdateAnalysis;
	unsigned int fAnalysis;
        set< int > fOldRunList;

	unsigned int fAnalysisType;               // (see anasum.cpp)
        unsigned int fAnalysisRunMode;            // 0: loop over all files (sequentiell)
						  // 1: combine several anasum result file and merge analysis results

        VAnaSumRunParameter *fRunPara;            //!< all run parameters (run numbers, background models, etc.)
        string fDatadir;                          //!< Directory containing the parameter data files
        string fPrefix;                           //!< Data file prefix
        string fSuffix;                           //!< Data file suffix
        int fNumTels;                             //!< Number of Telescopes to Analyze
        int fTelOffset;                           //!< Offset Telescope Number to Start Analysis From

        double fTotalExposureOn;                  //!< Total time spent ON Source
        double fTotalExposureOff;                 //!< Total time spent OFF Source

        map< int, double > fRunExposureOn;        //!< exposure per run (on)
        map< int, double > fRunExposureOff;       //!< exposure per run (off)

//map< int, double >  fRunMJD;	//Moved to VStereoAnalysis
//map< int, double >  fRunMJDStart;	//Moved to VStereoAnalysis
//map< int, double >  fRunMJDStopp;	//Moved to VStereoAnalysis
//Perhaps the following should go to VStereoAnalysis too ...
        map< int, double > fRunAzMeanOn;
        map< int, double > fRunAzMinOn;
        map< int, double > fRunAzMaxOn;
        map< int, double > fRunAzMeanOff;
        map< int, double > fRunAzMinOff;
        map< int, double > fRunAzMaxOff;
        map< int, double > fRunPedVarsOn;
        map< int, double > fRunPedVarsOff;

// output file and directories
        TFile *fOPfile;                           //!< Output file

        TDirectory *fTotalDir;                    //!< directory with combined results (output directory)
        string fTotalDirName;
        vector< TDirectory* > fRunDir;            //!< directory with results per run

        TDirectory *fStereoTotalDir;              //!< directory with combined stereo results
        vector<TDirectory *> fMonoTotalDir;       //!< directory with combined mono results (one per telescope)

        vector< TDirectory* > fStereoRunDir;      //!< directory with stereo results per run
                                                  //!< single telescope directories, one per run and telescope (first index: run, second index: telescope)
        vector< vector< TDirectory* > > fMonoRunDir;

// Mono Analysis
        vector<VMonoPlots *> fMonoOn;             //!< Mono analysis plots for ON source (one per telescope)
        vector<VMonoPlots *> fMonoOff;            //!< Mono analysis plots for OFF source (one per telescope)

// Stereo analysis
        VStereoAnalysis *fStereoOn;               //!< Analysis plots for stereo parameters
        VStereoAnalysis *fStereoOff;              //!< Analysis plots for stereo parameters

// summary
        VRatePlots *fRatePlots;                   //!< significance and rate plots vs time
        vector< VRatePlots* > fMonoRatePlots;
        VRunSummary *fRunSummary;

        double fMeanRawRateOn;
        double fMeanRawRateOff;
        double fMeanAzimuthOn;
        double fMeanAzimuthOff;
        double fMeanElevationOn;
        double fMeanElevationOff;
        double fNMeanElevation;
        double fMeanDeadTimeOn;                   //!< mean dead time (weighted mean, weight is length of a run)
        double fMeanDeadTimeOff;                  //!< mean dead time (weighted mean, weight is length of a run)
        double fMeanPedVarsOn;
        double fMeanPedVarsOff;

};
#endif
