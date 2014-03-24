/*! \class VInstrumentResponseFunction
    \brief instrument response (e.g. angular resolution)


*/

#include "VInstrumentResponseFunction.h"

VInstrumentResponseFunction::VInstrumentResponseFunction()
{
	fDebug = false;
	
	fName = "";
	fType = "";
	
	fData = 0;
	fAnaCuts = 0;
	fEnergyReconstructionMethod = 0;
	
	fSpectralWeight = new VSpectralWeight();
	
	fDataProduct = 0;
	
	setContainmentProbability();
}

void VInstrumentResponseFunction::setRunParameter( VInstrumentResponseFunctionRunParameter* iRunPara )
{
	if( !iRunPara )
	{
		cout << "VInstrumentResponseFunction::setRunParameter error: no run parameter given" << endl;
		return;
	}
	fEnergyReconstructionMethod = iRunPara->fEnergyReconstructionMethod;
	setEnergyReconstructionMethod( iRunPara->fEnergyReconstructionMethod );
	setMonteCarloEnergyRange( iRunPara->fMCEnergy_min, iRunPara->fMCEnergy_max, TMath::Abs( iRunPara->fMCEnergy_index ) );
	
	fVMinAz = iRunPara->fAzMin;
	fVMaxAz = iRunPara->fAzMax;
	// preli: use only first spectral index bin
	if( iRunPara->fSpectralIndex.size() > 0 )
	{
		fVSpectralIndex.clear();
		fVSpectralIndex.push_back( iRunPara->fSpectralIndex[0] );
	}
}



void VInstrumentResponseFunction::setMonteCarloEnergyRange( double iMin, double iMax, double iMCIndex )
{
	if( fSpectralWeight )
	{
		fSpectralWeight->setMCParameter( iMCIndex, iMin, iMax );
	}
}


bool VInstrumentResponseFunction::initialize( string iName, string iType, unsigned int iNTel, double iMCMaxCoreRadius,
		double iZe, int iNoise, double iPedvars, double iXoff, double iYoff )
{
	fName = iName;
	fType = iType;
	
	cout << "======================================================================================================" << endl;
	cout << endl;
	cout << "initializing instrument response function calculator (" << iName << ", type " << iType << ")" << endl;
	cout << "(ntel=" << iNTel;
	cout << ", MC_core_max=" << iMCMaxCoreRadius;
	cout << ", azimuth bins: " << fVMinAz.size();
	cout << ", spectral index bins: " << fVSpectralIndex.size() << endl;
	cout << " ze=" << iZe << ", wobble (" << iXoff << ", " << iYoff << "), ";
	cout << "noise=" << iNoise << ", pedvars=" << iPedvars;
	cout << ")" << endl;
	
	char hname[800];
	
	for( unsigned int i = 0; i < fVSpectralIndex.size(); i++ )
	{
		vector< VInstrumentResponseFunctionData* > i_irf;
		for( unsigned int j = 0; j < fVMinAz.size(); j++ )
		{
			sprintf( hname, "%s_%d_%d", iName.c_str(), i, j );
			i_irf.push_back( new VInstrumentResponseFunctionData() );
			if( !i_irf.back()->initialize( hname, iType, iNTel, iMCMaxCoreRadius ) )
			{
				return false;
			}
			
			i_irf.back()->setData( iZe, ( int )j, fVMinAz[j], fVMaxAz[j], iNoise, iPedvars, fVSpectralIndex[i], iXoff, iYoff );
			i_irf.back()->setHistogrambinning();
			i_irf.back()->setEnergyReconstructionMethod( fEnergyReconstructionMethod );
		}
		fIRFData.push_back( i_irf );
	}
	
	// output tree
	
	fIRFData_Tree = new VInstrumentResponseFunctionData();
	
	sprintf( hname, "t_%s", fName.c_str() );
	fDataProduct = new TTree( hname, fType.c_str() );
	fDataProduct->Branch( "IRF", "VInstrumentResponseFunctionData", &fIRFData_Tree, 16000, 0 );
	
	return true;
}

bool VInstrumentResponseFunction::fill()
{
	// data is needed
	if( !fData )
	{
		return false;
	}
	// cut are needed
	if( !fAnaCuts )
	{
		return false;
	}
	
	// spectral weight
	double i_weight = 1.;
	// MC energy (log10)
	
	///////////////////////////////////
	// get full data set
	///////////////////////////////////
	Long64_t d_nentries = fData->fChain->GetEntries();
	cout << "VInstrumentResponseFunction " << fName << " (" << fType << "): total number of data events: " << d_nentries << endl;
	for( Long64_t i = 0; i < d_nentries; i++ )
	{
		fData->GetEntry( i );
		
		fAnaCuts->newEvent( false );
		
		// apply MC cuts
		if( !fAnaCuts->applyMCXYoffCut( fData->MCxoff, fData->MCyoff, true ) )
		{
			continue;
		}
		
		////////////////////////////////
		// apply general quality and gamma/hadron separation cuts
		// apply fiducial area cuts
		if( !fAnaCuts->applyInsideFiducialAreaCut( true ) )
		{
			continue;
		}
		
		// apply reconstruction quality cuts
		if( !fAnaCuts->applyStereoQualityCuts( fEnergyReconstructionMethod, true, i , true ) )
		{
			continue;
		}
		
		// apply telescope type cut
		//	 if( fAnaCuts->applyTelTypeTest( true ) ) continue;
		
		// apply gamma/hadron cuts
		if( !fAnaCuts->isGamma( i, true ) )
		{
			continue;
		}
		
		//////////////////////////////////////
		// loop over all az bins
		for( unsigned int i_az = 0; i_az < fVMinAz.size(); i_az++ )
		{
		
			// check which azimuth bin we are
			if( fData->MCze > 3. )
			{
				// confine MC az to -180., 180.
				if( fData->MCaz > 180. )
				{
					fData->MCaz -= 360.;
				}
				// expect bin like [135,-135]
				if( fVMinAz[i_az] > fVMaxAz[i_az] )
				{
					if( fData->MCaz < fVMinAz[i_az] && fData->MCaz > fVMaxAz[i_az] )
					{
						continue;
					}
				}
				// expect bin like [-135,-45.]
				else
				{
					if( fData->MCaz < fVMinAz[i_az] || fData->MCaz > fVMaxAz[i_az] )
					{
						continue;
					}
				}
			}
			// loop over all spectral index
			for( unsigned int s = 0; s < fVSpectralIndex.size(); s++ )
			{
				// weight by spectral index
				if( fSpectralWeight )
				{
					fSpectralWeight->setSpectralIndex( fVSpectralIndex[s] );
					i_weight = fSpectralWeight->getSpectralWeight( fData->MCe0 );
				}
				else
				{
					i_weight = 0.;
				}
				
				// fill histograms
				if( s < fIRFData.size() && i_az < fIRFData[s].size() )
				{
					if( fIRFData[s][i_az] )
					{
						fIRFData[s][i_az]->fill( i_weight );
					}
				}
			}
		}
	}
	//    fAnaCuts->printCutStatistics();
	
	// fill resolution graphs
	cout << "VInstrumentResponseFunction::terminate ";
	cout << " (integration probability: " << fContainmentProbability << ")" << endl;
	for( unsigned int i = 0; i < fIRFData.size(); i++ )
	{
		for( unsigned int j = 0; j < fIRFData[i].size(); j++ )
		{
			if( fIRFData[i][j] )
			{
				fIRFData[i][j]->terminate( fContainmentProbability, fContainmentProbabilityError );
			}
		}
	}
	
	// fill data product tree
	cout << "VInstrumentResponseFunction:: fill data tree ";
	if( fIRFData.size() > 0 )
	{
		cout << "( " << fIRFData.size()*fIRFData[0].size() << " entries)" << endl;
	}
	for( unsigned int i = 0; i < fIRFData.size(); i++ )
	{
		for( unsigned int j = 0; j < fIRFData[i].size(); j++ )
		{
			if( fIRFData[i][j] )
			{
				fIRFData_Tree = ( VInstrumentResponseFunctionData* )fIRFData[i][j]->Clone();
				fDataProduct->Fill();
			}
		}
	}
	
	return true;
}

void VInstrumentResponseFunction::setHistogrambinning( int iN, double iMin, double iMax )
{
	for( unsigned int i = 0; i < fIRFData.size(); i++ )
	{
		for( unsigned int j = 0; j < fIRFData[i].size(); j++ )
		{
			if( fIRFData[i][j] )
			{
				fIRFData[i][j]->setHistogrambinning( iN, iMin, iMax );
			}
		}
	}
}

void VInstrumentResponseFunction::setDataTree( CData* iData )
{
	fData = iData;
	for( unsigned int i = 0; i < fIRFData.size(); i++ )
	{
		for( unsigned int j = 0; j < fIRFData[i].size(); j++ )
		{
			if( fIRFData[i][j] )
			{
				fIRFData[i][j]->setDataTree( fData );
			}
		}
	}
}


void VInstrumentResponseFunction::setEnergyReconstructionMethod( unsigned int iMethod )
{
	for( unsigned int i = 0; i < fIRFData.size(); i++ )
	{
		for( unsigned int j = 0; j < fIRFData[i].size(); j++ )
		{
			if( fIRFData[i][j] )
			{
				fIRFData[i][j]->setEnergyReconstructionMethod( iMethod );
			}
		}
	}
}

void VInstrumentResponseFunction::setCuts( VGammaHadronCuts* iCuts )
{
	fAnaCuts = iCuts;
	
	if( fAnaCuts )
	{
		for( unsigned int i = 0; i < fIRFData.size(); i++ )
		{
			for( unsigned int j = 0; j < fIRFData[i].size(); j++ )
			{
				if( fIRFData[i][j] )
				{
					fIRFData[i][j]->setArrayCentre( fAnaCuts->getArrayCentre_X(), fAnaCuts->getArrayCentre_Y() );
				}
			}
		}
	}
}

TGraphErrors* VInstrumentResponseFunction::getAngularResolutionGraph( unsigned int iAzBin, unsigned int iSpectralIndexBin )
{
	if( iAzBin < fIRFData.size() && iSpectralIndexBin < fIRFData[iAzBin].size() && fIRFData[iAzBin][iSpectralIndexBin] )
	{
		return fIRFData[iAzBin][iSpectralIndexBin]->fResolutionGraph[VInstrumentResponseFunctionData::E_DIFF];
	}
	
	cout << "VInstrumentResponseFunction::getAngularResolutionGraph: warning index out of range ";
	cout << iAzBin << "\t" << iSpectralIndexBin << "\t";
	cout << "(" << fIRFData[iAzBin].size();
	if( fIRFData.size() )
	{
		cout << "\t" << fIRFData[iAzBin].size();
	}
	cout << ")" << endl;
	
	return 0;
}
