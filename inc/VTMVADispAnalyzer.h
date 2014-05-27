// VTMVADispAnalyzer TMVA based modified disp analysis

#ifndef VTMVADispAnalyzer_H
#define VTMVADispAnalyzer_H

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "TMath.h"

#include "TMVA/Config.h"
#include "TMVA/Factory.h"
#include "TMVA/Reader.h"
#include "TMVA/Tools.h"

using namespace std;

class VTMVADispAnalyzer
{
	private:
	
		bool fDebug;
		bool bZombie;
		
		vector<ULong64_t> fTelescopeTypeList;
		map< ULong64_t, TMVA::Reader* > fTMVAReader;
		
		float fWidth;
		float fLength;
		float fSize;
		float fPedvar;
		float fTGrad;
		float fZe;
		float fAz;
		float fLoss;
		float fDist;
		float fAsymm;
		float fXcore;
		float fYcore;
		float fcross;
		float fLTrig;
		
	public:
	
		VTMVADispAnalyzer( string iFile, vector< ULong64_t > iTelTypeList );
		~VTMVADispAnalyzer() {}
		
		float evaluate( float iWidth, float iLength, float iSize, float iAsymm, float iLoss, float iTGrad, float icen_x, float icen_y, float xoff_4, float yoff_4, ULong64_t iTelType, float iZe, float iAz );
		bool isZombie()
		{
			return bZombie;
		}
		void terminate();
		
};

#endif
