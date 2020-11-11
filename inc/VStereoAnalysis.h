//! VStereoAnalysis    class for producing one dimensional histograms from parameterized VERITAS data.

#ifndef VStereoAnalysis_H
#define VStereoAnalysis_H

#include "CData.h"
#include "VGammaHadronCuts.h"
#include "VAnaSumRunParameter.h"
#include "VASlalib.h"
#include "VEvndispRunParameter.h"
#include "VTimeMask.h"
#include "VDeadTime.h"
#include "VEffectiveAreaCalculator.h"
#include "VStereoHistograms.h"
#include "VStereoMaps.h"
#include "VSkyCoordinates.h"
#include "VSkyCoordinatesUtilities.h"
#include "VTargets.h"
#include "VPointingDB.h"

#include "TDirectory.h"
#include "TDirectoryFile.h"
#include "TGraphAsymmErrors.h"
#include "TGraph2DErrors.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TList.h"
#include "TMath.h"
#include "TRandom3.h"
#include "TTree.h"
#include "TParameter.h"
#include "TObject.h"
#include "TNamed.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

class VStereoAnalysis
{
	public:
	
		VStereoAnalysis( bool isOnrun, string i_hsuffix, VAnaSumRunParameter* irunpara,
						 vector< TDirectory* > iRDir, TDirectory* iDir, string iDataDir, int iRandomSeed, bool iTotalAnalysisOnly );
		~VStereoAnalysis() {}
		double fillHistograms( int icounter, int irun, double AzMin = -1.e3, double AzMax = 1.e3, double iPedVar = -1. );
		TH2D*  getAlpha();
		TH2D*  getAlphaUC();
		TH2D*  getAlphaNorm();
		TH2D*  getAlphaNormUC();
		double getDeadTimeFraction();
		double getEffectiveExposure( int i_run )
		{
			return ( fRunExposure.find( i_run ) != fRunExposure.end() ? fRunExposure[i_run] : 0. );
		}
		TList* getEnergyHistograms();
		TList* getHisList();
		double getMeanAzimuth()
		{
			return fMeanAzimuth;
		}
		double getMeanElevation()
		{
			return fMeanElevation;
		}
		TH1D*  getMeanSignalBackgroundAreaRatio();
		TH1D* getMeanSignalBackgroundAreaRatioUC();
		double getMJD( int i_run )
		{
			return ( fRunMJD.find( i_run ) != fRunMJD.end() ? fRunMJD[i_run] : 0. );
		}
		TList* getParameterHistograms();
		double getRawRate();                      //! return number of entries in rate histograms
		vector< double > getRateCounts();
		vector< double > getRateTime();
		vector< double > getRateTimeIntervall();
		map< int, double > getRunMJD() const
		{
			return fRunMJD;
		}
		map< int, double > getRunDuration() const
		{
			return fRunDuration;
		}
		TList* getSkyHistograms( bool bUC );
		TH2D*  getStereoSkyMap();
		TH2D*  getStereoSkyMapUC();
		TH1D*  getTheta2();
		double getWobbleNorth();
		double getWobbleWest();
		TTree* getTreeWithSelectedEvents()
		{
			return fTreeSelectedEvents;
		}
		void   scaleAlpha( TH2D* halpha_on, bool bUC );
		void   setCuts( VAnaSumRunParameterDataClass iL, int irun );
		void   setNoSkyPlots( bool iS )
		{
			fNoSkyPlots = iS;
		}
		void   setRunExposure( map< int, double > iExpl )
		{
			fRunExposure = iExpl;
		}
		void   setRunMJD( map< int, double > iRunMJD )
		{
			fRunMJD = iRunMJD;
		}
		void   setRunTimes();
		string setRunTimes( CData* iData );
		bool   terminate();
		void   writeDebugHistograms();
		void   writeHistograms( bool bOn );
		
		
	private:
	
		bool fDebug;
		bool bIsGamma;
		bool bTotalAnalysisOnly;
		
		bool fIsOn;
		bool fNoSkyPlots;                         //! do full sky plots (if false, analysed source region only)
		
		VAnaSumRunParameter* fRunPara;
		
		TGraphAsymmErrors* gMeanEffectiveArea;
		TGraph2DErrors*    gTimeBinnedMeanEffectiveArea;
		TGraphErrors*      gMeanEsys_MC;
		
		TGraphAsymmErrors* gMeanEffectiveAreaMC;
		TH2D*			   hResponseMatrix;

		VStereoMaps* fMap;
		VStereoMaps* fMapUC;
		int fHisCounter;
		map< int, double > fRunMJDStart;
		map< int, double > fRunMJDStopp;
		map< int, double > fRunMJD;               // Default value is mid-point; modified to mean time of accepted events by fillHistograms
		// If fRunMJD is defined from a VRunSummary.fRunMJD it will contain extra ON/OFF runs
		// because the VRunSummary.fRunMJD is a union of all the runs:
		// always access via the run index to be sure of getting the correct run
		map< int, double > fRunDuration;          // Raw run length from data tree
		map< int, double > fRunExposure;          // Open portion of time mask
		vector< VStereoHistograms* > fHisto;
		VStereoHistograms* fHistoTot;
		
		TTree* fTreeSelectedEvents;
		int    fTreeSelected_runNumber;
		int    fTreeSelected_eventNumber;
		int    fTreeSelected_MJD;
		double fTreeSelected_Time;
		int    fTreeSelected_NImages;
		ULong64_t fTreeSelected_ImgSel;
		double fTreeSelected_theta2;
		double fTreeSelected_Xoff;
		double fTreeSelected_Yoff;
		double fTreeSelected_Xoff_derot;
		double fTreeSelected_Yoff_derot;
		double fTreeSelected_Xcore;
		double fTreeSelected_Ycore;
		double fTreeSelected_MSCW;
		double fTreeSelected_MSCL;
		double fTreeSelected_MWR;
		double fTreeSelected_MLR;
		double fTreeSelected_Erec;
		double fTreeSelected_EChi2;
		double fTreeSelected_ErecS;
		double fTreeSelected_EChi2S;
		float fTreeSelected_EmissionHeight;
		float fTreeSelected_EmissionHeightChi2;
		double fTreeSelected_SizeSecondMax;
		UInt_t fTreeSelected_IsGamma;
		
		TTree* fTreeWithEventsForCtools ;
		int     fTreeCTOOLS_runNumber;
		int     fTreeCTOOLS_eventNumber;
		double  fTreeCTOOLS_Time;
		int     fTreeCTOOLS_MJD;
		double  fTreeCTOOLS_Xoff;
		double  fTreeCTOOLS_Yoff;
		double  fTreeCTOOLS_Xderot;
		double  fTreeCTOOLS_Yderot;
		double  fTreeCTOOLS_TargetRA;
		double  fTreeCTOOLS_TargetDEC;
		double  fTreeCTOOLS_RA;
		double  fTreeCTOOLS_DEC;
		double  fTreeCTOOLS_Erec;
		double  fTreeCTOOLS_ErecS;
		double  fTreeCTOOLS_Erec_Err;
		double  fTreeCTOOLS_ErecS_Err;
		double  fTreeCTOOLS_XGroundCore;
		double  fTreeCTOOLS_YGroundCore;
		int     fTreeCTOOLS_NImages;
		int     fTreeCTOOLS_ImgSel;
		double  fTreeCTOOLS_MSCW;
		double  fTreeCTOOLS_MSCL;
		double  fTreeCTOOLS_MWR;
		double  fTreeCTOOLS_MLR;
		double  fTreeCTOOLS_WobbleNorth ;
		double  fTreeCTOOLS_WobbleWest ;
		double  fTreeCTOOLS_Az ;
		double  fTreeCTOOLS_El ;
		double  fTreeCTOOLS_EmissionHeight ;
		int     fTreeCTOOLS_GregYear  ;
		int     fTreeCTOOLS_GregMonth ;
		int     fTreeCTOOLS_GregDay   ;
		double  fTreeCTOOLS_Acceptance ;
		VRadialAcceptance* fCTOOLSAcceptance ;
		
		double  fDeadTimeStorage ;
		//double fullMJD ;
		VSkyCoordinates* fVsky ;  // for RADec to AzimElev conversion
		
		double fTreeSelected_MVA;
		
		double fTotCount;
		
		map < int, double > f_t_in_s_min;
		map < int, double > f_t_in_s_max;
		double fMeanAzimuth;
		double fMeanElevation;
		double fNMeanElevation;
		
		CData* fDataRun;
		TTree* fDataRunTree;
		TTree* fDataFrogsTree;
		TFile* fDataFile;
		string fInstrumentEpoch;
		vector< unsigned int > fTelToAnalyze;
		
		vector< VSkyCoordinates* > fAstro;        //!< Astronomical source parameters for this analysis
		VGammaHadronCuts* fCuts;                  //!< Parameter Cuts
		VTimeMask* fTimeMask;                     //!< Time Cuts
		
		// dead time calculators
		vector< VDeadTime* > fDeadTime;
		
		// rate counters
		vector< vector< double > > fRateCounts;
		vector< vector< double > > fRateTime;
		vector< vector< double > > fRateTimeIntervall;
		vector< double > fRateCountsTot;
		vector< double > fRateTimeTot;
		vector< double > fRateTimeIntervallTot;
		
		// directories
		TDirectory* fDirTot;
		vector< TDirectory* > fDirTotRun;
		
		double combineHistograms();
		void   defineAstroSource();
		bool   closeDataFile();
		CData* getDataFromFile( int i_runNumber );
		
		void fill_TreeWithSelectedEvents( CData*, double, double, double );
		bool init_TreeWithSelectedEvents( int, bool );
		void reset_TreeWithSelectedEvents();
		
		void fill_TreeWithEventsForCtools( CData* c , double i_xderot, double i_yderot, unsigned int icounter, double i_UTC , double fEVDVersionSign ) ; // WRITEEVENTTREEFORCTOOLS
		bool init_TreeWithEventsForCtools( int irun ) ; // WRITEEVENTTREEFORCTOOLS
		void save_TreeWithEventsForCtools() ;           // WRITEEVENTTREEFORCTOOLS
		
		// derotation and J2000
		void getDerotatedCoordinates( unsigned int, double i_UTC, double x, double y, double& x_derot, double& y_derot );
		
		int  getDataRunNumber() const;            // Check for existence of fDataRun and try to retrieve run number from first entry of the tree
};
#endif

