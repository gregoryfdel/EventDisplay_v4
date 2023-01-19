/*! \file  trainTMVAforGammaHadronSeparation.cpp
    \brief  use TMVA methods for gamma/hadron separation

*/

#include "TChain.h"
#include "TCut.h"
#include "TFile.h"
#include "TH1D.h"
#include "TMath.h"
#include "TSystem.h"
#include "TTree.h"

#include "TMVA/Config.h"
#include "TMVA/DataLoader.h"
#include "TMVA/Factory.h"
#include "TMVA/Reader.h"
#include "TMVA/Tools.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "VTMVARunData.h"

using namespace std;

bool train( VTMVARunData* iRun, unsigned int iEnergyBin, unsigned int iZenithBin, bool iGammaHadronSeparation );
bool trainGammaHadronSeparation( VTMVARunData* iRun, unsigned int iEnergyBin, unsigned int iZenithBin );
bool trainReconstructionQuality( VTMVARunData* iRun, unsigned int iEnergyBin, unsigned int iZenithBin );

/*!

     train the MVA

*/

bool trainGammaHadronSeparation( VTMVARunData* iRun,
								 unsigned int iEnergyBin, unsigned int iZenithBin )
{
	return train( iRun, iEnergyBin, iZenithBin, true );
}

bool trainReconstructionQuality( VTMVARunData* iRun,
								 unsigned int iEnergyBin, unsigned int iZenithBin )
{
	return train( iRun, iEnergyBin, iZenithBin, false );
}


bool train( VTMVARunData* iRun,
			unsigned int iEnergyBin, unsigned int iZenithBin,
			bool iTrainGammaHadronSeparation )
{
	// sanity checks
	if( !iRun )
	{
		return false;
	}
	if( iRun->fEnergyCutData.size() <= iEnergyBin || iRun->fOutputFile.size() <= iEnergyBin )
	{
		cout << "error in train: energy bin out of range " << iEnergyBin;
		return false;
	}
	if( iRun->fZenithCutData.size() < iZenithBin || iRun->fOutputFile[0].size() < iZenithBin )
	{
		cout << "error in train: zenith bin out of range " << iZenithBin;
		return false;
	}
	
	TMVA::Tools::Instance();
	
	// set output directory
	gSystem->mkdir( iRun->fOutputDirectoryName.c_str() );
	TString iOutputDirectory( iRun->fOutputDirectoryName.c_str() );
	gSystem->ExpandPathName( iOutputDirectory );
	( TMVA::gConfig().GetIONames() ).fWeightFileDir = iOutputDirectory;
	
	//////////////////////////////////////////
	// defining training class
	TMVA::Factory* factory = new TMVA::Factory( iRun->fOutputFile[iEnergyBin][iZenithBin]->GetTitle(), iRun->fOutputFile[iEnergyBin][iZenithBin], "V" );
	TMVA::DataLoader* dataloader = new TMVA::DataLoader( "" );
	////////////////////////////
	// train gamma/hadron separation
	if( iTrainGammaHadronSeparation )
	{
		// adding signal and background trees
		for( unsigned int i = 0; i < iRun->fSignalTree.size(); i++ )
		{
			dataloader->AddSignalTree( iRun->fSignalTree[i], iRun->fSignalWeight );
		}
		for( unsigned int i = 0; i < iRun->fBackgroundTree.size(); i++ )
		{
			dataloader->AddBackgroundTree( iRun->fBackgroundTree[i], iRun->fBackgroundWeight );
		}
	}
	////////////////////////////
	// train reconstruction quality
	else
	{
		for( unsigned int i = 0; i < iRun->fSignalTree.size(); i++ )
		{
			dataloader->AddRegressionTree( iRun->fSignalTree[i], iRun->fSignalWeight );
		}
		dataloader->AddRegressionTarget( iRun->fReconstructionQualityTarget.c_str(), iRun->fReconstructionQualityTargetName.c_str() );
	}
	
	// quality cuts before training
	TCut iCutSignal = iRun->fQualityCuts
					  && iRun->fMCxyoffCut &&
					  iRun->fEnergyCutData[iEnergyBin]->fEnergyCut
					  && iRun->fZenithCutData[iZenithBin]->fZenithCut;
					  
	TCut iCutBck = iRun->fQualityCuts && iRun->fQualityCutsBkg
				   && iRun->fEnergyCutData[iEnergyBin]->fEnergyCut
				   && iRun->fZenithCutData[iZenithBin]->fZenithCut;
				   
	if( !iRun->fMCxyoffCutSignalOnly )
	{
		iCutBck = iCutBck && iRun->fMCxyoffCut;
	}
	
	// adding training variables
	if( iRun->fTrainingVariable.size() != iRun->fTrainingVariableType.size() )
	{
		cout << "train: error: training-variable vectors have different size" << endl;
		return false;
	}
	
	// loop over all trainingvariables and add them to TMVA
	// (test first if variable is constant, TMVA will stop when a variable
	//  is constant)
	for( unsigned int i = 0; i < iRun->fTrainingVariable.size(); i++ )
	{
		if( iRun->fTrainingVariable[i].find( "NImages_Ttype" ) != string::npos )
		{
			for( int j = 0; j < iRun->fNTtype; j++ )
			{
				ostringstream iTemp;
				iTemp << iRun->fTrainingVariable[i] << "[" << j << "]";
				ostringstream iTempCut;
				// require at least 2 image per telescope type
				iTempCut << iTemp.str() << ">1";
				TCut iCutCC = iTempCut.str().c_str();
				
				double iSignalMean = 1.;
				double iBckMean    = -1.;
				if( ( TMath::Abs( iSignalMean - iBckMean ) > 1.e-6
						|| TMath::Abs( iSignalMean + 9999. ) < 1.e-2 || TMath::Abs( iBckMean + 9999. ) < 1.e-2 )
						&& iSignalMean != 0 && iBckMean != 0 )
				{
					dataloader->AddVariable( iTemp.str().c_str(), iRun->fTrainingVariableType[i] );
				}
				else
				{
					cout << "warning: removed constant variable " << iTemp.str() << " from training (added to spectators)" << endl;
					dataloader->AddSpectator( iTemp.str().c_str() );
				}
			}
		}
		else
		{
			// check if the training variable is constant
			double iSignalMean = 1.;
			double iBckMean    = -1.;
			
			if( TMath::Abs( iSignalMean - iBckMean ) > 1.e-6
					|| TMath::Abs( iSignalMean + 9999. ) < 1.e-2 || TMath::Abs( iBckMean + 9999. ) < 1.e-2 )
			{
				dataloader->AddVariable( iRun->fTrainingVariable[i].c_str(), iRun->fTrainingVariableType[i] );
			}
			else
			{
				cout << "warning: removed constant variable " << iRun->fTrainingVariable[i] << " from training (added to spectators)" << endl;
				dataloader->AddSpectator( iRun->fTrainingVariable[i].c_str() );
			}
		}
	}
	// adding spectator variables
	for( unsigned int i = 0; i < iRun->fSpectatorVariable.size(); i++ )
	{
		dataloader->AddSpectator( iRun->fSpectatorVariable[i].c_str() );
	}
	
	//////////////////////////////////////////
	// prepare training events
	// nTrain Signal=5000:nTrain Background=5000: nTest Signal=4000:nTest Background=5000
	
	dataloader->PrepareTrainingAndTestTree( iCutSignal,
											iCutBck,
											iRun->fPrepareTrainingOptions );
											
	//////////////////////////////////////////
	// book all methods
	char htitle[6000];
	
	for( unsigned int i = 0; i < iRun->fMVAMethod.size(); i++ )
	{
		TMVA::Types::EMVA i_tmva_type = TMVA::Types::kBDT;
		if( iRun->fMVAMethod[i] == "BDT" )
		{
			if( iTrainGammaHadronSeparation )
			{
				sprintf( htitle, "BDT_0" );
				i_tmva_type = TMVA::Types::kBDT;
			}
			else
			{
				sprintf( htitle, "BDT_RecQuality_0" );
			}
		}
		else if( iRun->fMVAMethod[i] == "MLP" )
		{
			i_tmva_type = TMVA::Types::kMLP;
		}
		
		//////////////////////////
		if( iRun->fMVAMethod[i] != "BOXCUTS" )
		{
			if( iTrainGammaHadronSeparation )
			{
				sprintf( htitle, "%s_%u", iRun->fMVAMethod[i].c_str(), i );
			}
			else
			{
				sprintf( htitle, "%s_RecQuality_%u", iRun->fMVAMethod[i].c_str(), i );
			}
			if( i < iRun->fMVAMethod_Options.size() )
			{
				cout << "Booking method " << htitle << endl;
				factory->BookMethod( dataloader, i_tmva_type, htitle, iRun->fMVAMethod_Options[i].c_str() );
			}
			else
			{
				factory->BookMethod( dataloader, i_tmva_type, htitle );
			}
		}
		//////////////////////////
		// BOX CUTS
		// (note: box cuts needs additional checking, as the code might be outdated)
		else if( iRun->fMVAMethod[i] == "BOXCUTS" )
		{
			stringstream i_opt;
			i_opt << iRun->fMVAMethod_Options[i].c_str();
			for( unsigned int i = 0; i < iRun->fTrainingVariable_CutRangeMin.size(); i++ )
			{
				i_opt << ":CutRangeMin[" << i << "]=" << iRun->fTrainingVariable_CutRangeMin[i];
			}
			for( unsigned int i = 0; i < iRun->fTrainingVariable_CutRangeMax.size(); i++ )
			{
				i_opt << ":CutRangeMax[" << i << "]=" << iRun->fTrainingVariable_CutRangeMax[i];
			}
			for( unsigned int i = 0; i < iRun->fTrainingVariable_VarProp.size(); i++ )
			{
				i_opt << ":VarProp[" << i << "]=" << iRun->fTrainingVariable_VarProp[i];
			}
			sprintf( htitle, "BOXCUTS_%u_%u", iEnergyBin, iZenithBin );
			factory->BookMethod( dataloader, TMVA::Types::kCuts, htitle, i_opt.str().c_str() );
		}
	}
	
	
	//////////////////////////////////////////
	// start training
	
	factory->TrainAllMethods();
	
	//////////////////////////////////////////
	// evaluate results
	
	factory->TestAllMethods();
	
	factory->EvaluateAllMethods();
	
	dataloader->Delete();
	factory->Delete();
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int main( int argc, char* argv[] )
{
	// print version only
	if( argc == 2 )
	{
		string fCommandLine = argv[1];
		if( fCommandLine == "-v" || fCommandLine == "--version" )
		{
			VGlobalRunParameter fRunPara;
			cout << fRunPara.getEVNDISP_VERSION() << endl;
			exit( EXIT_SUCCESS );
		}
	}
	cout << endl;
	cout << "trainTMVAforGammaHadronSeparation " << VGlobalRunParameter::getEVNDISP_VERSION() << endl;
	cout << "----------------------------------------" << endl;
	if( argc != 2 )
	{
		cout << endl;
		cout << "./trainTMVAforGammaHadronSeparation <configuration file>" << endl;
		cout << endl;
		cout << "  (an example for a configuration file can be found in " << endl;
		cout << "   $VERITAS_EVNDISP_AUX_DIR/ParameterFiles/TMVA.BDT.runparameter )" << endl;
		cout << endl;
		exit( EXIT_SUCCESS );
	}
	cout << endl;
	
	//////////////////////////////////////
	// data object
	VTMVARunData* fData = new VTMVARunData();
	fData->fName = "OO";
	
	//////////////////////////////////////
	// read run parameters from configuration file
	if( !fData->readConfigurationFile( argv[1] ) )
	{
		cout << "error opening or reading run parameter file (";
		cout << argv[1];
		cout << ")" << endl;
		exit( EXIT_FAILURE );
	}
	fData->print();
	
	//////////////////////////////////////
	// read and prepare data files
	if( !fData->openDataFiles() )
	{
		cout << "error opening data files" << endl;
		exit( EXIT_FAILURE );
	}
	
	//////////////////////////////////////
	// train MVA
	// (one training per energy and zenith bin)
	cout << "Number of energy bins: " << fData->fEnergyCutData.size();
	cout << ", number of zenith bins: " << fData->fZenithCutData.size();
	cout << endl;
	cout << "================================" << endl << endl;
	for( unsigned int i = 0; i < fData->fEnergyCutData.size(); i++ )
	{
		for( unsigned int j = 0; j < fData->fZenithCutData.size(); j++ )
		{
			if( fData->fEnergyCutData[i]->fEnergyCut && fData->fZenithCutData[j]->fZenithCut )
			{
				cout << "Training energy bin " << fData->fEnergyCutData[i]->fEnergyCut;
				cout << " zenith bin " << fData->fZenithCutData[j]->fZenithCut << endl;
				cout << "===================================================================================" << endl;
				cout << endl;
			}
			// training
			if( fData->fTrainGammaHadronSeparation && !trainGammaHadronSeparation( fData, i, j ) )
			{
				cout << "Error during training...exiting" << endl;
				exit( EXIT_FAILURE );
			}
			if( fData->fTrainReconstructionQuality )
			{
				trainReconstructionQuality( fData, i, j );
			}
			stringstream iTempS;
			stringstream iTempS2;
			if( fData->fEnergyCutData.size() > 1 && fData->fZenithCutData.size() > 1 )
			{
				iTempS << fData->fOutputDirectoryName << "/" << fData->fOutputFileName << "_" << i << "_" << j << ".bin.root";
				iTempS2 << "/" << fData->fOutputFileName << "_" << i << "_" << j << ".root";
			}
			else if( fData->fEnergyCutData.size() > 1 && fData->fZenithCutData.size() <= 1 )
			{
				iTempS << fData->fOutputDirectoryName << "/" << fData->fOutputFileName << "_" << i << ".bin.root";
				iTempS2 << "/" << fData->fOutputFileName << "_" << i << ".root";
			}
			else if( fData->fZenithCutData.size() > 1 &&  fData->fEnergyCutData.size() <= 1 )
			{
				iTempS << fData->fOutputDirectoryName << "/" << fData->fOutputFileName << "_0_" << j << ".bin.root";
				iTempS2 << "/" << fData->fOutputFileName << "_0_" << j << ".root";
			}
			else
			{
				iTempS << fData->fOutputDirectoryName << "/" << fData->fOutputFileName << ".bin.root";
				iTempS2 << fData->fOutputFileName << ".root";
			}
			
			// prepare a short root file with the necessary values only
			// write energy & zenith cuts, plus signal and background efficiencies
			TFile* root_file = fData->fOutputFile[i][j];
			if( !root_file )
			{
				cout << "Error finding tvma root file " << endl;
				continue;
			}
			TFile* short_root_file = TFile::Open( iTempS.str().c_str(), "RECREATE" );
			if( !short_root_file->IsZombie() )
			{
				VTMVARunDataEnergyCut* fDataEnergyCut = ( VTMVARunDataEnergyCut* )root_file->Get( "fDataEnergyCut" );
				VTMVARunDataZenithCut* fDataZenithCut = ( VTMVARunDataZenithCut* )root_file->Get( "fDataZenithCut" );
				TH1D* MVA_BDT_0_effS = ( TH1D* )root_file->Get( "Method_BDT/BDT_0/MVA_BDT_0_effS" );
				TH1D* MVA_BDT_0_effB = ( TH1D* )root_file->Get( "Method_BDT/BDT_0/MVA_BDT_0_effB" );
				fDataEnergyCut->Write();
				fDataZenithCut->Write();
				TDirectory* Method_BDT = short_root_file->mkdir( "Method_BDT" );
				Method_BDT->cd();
				TDirectory* BDT_0 = Method_BDT->mkdir( "BDT_0" );
				BDT_0->cd();
				MVA_BDT_0_effS->Write();
				MVA_BDT_0_effB->Write();
				short_root_file->GetList();
				short_root_file->Write();
				short_root_file->Close();
			}
			else
			{
				cout << "Error: could not create file with energy cuts " << iTempS.str().c_str() << endl;
			}
			// copy complete TMVA output root-file to another directory
			string iOutputFileName( fData->fOutputDirectoryName + "/" + iTempS2.str() );
			string iOutputFileNameCompleteSubDir( "complete_BDTroot" );
			string iOutputFileNameCompleteDir( fData->fOutputDirectoryName + "/" + iOutputFileNameCompleteSubDir + "/" );
			gSystem->mkdir( iOutputFileNameCompleteDir.c_str() );
			string iOutputFileNameComplete( iOutputFileNameCompleteDir + iTempS2.str() );
			rename( iOutputFileName.c_str(), iOutputFileNameComplete.c_str() );
			cout << "Complete TMVA output root-file moved to: " << iOutputFileNameComplete << endl;
			
			// rename .bin.root file to .root-file
			string iFinalRootFileName( iTempS.str() );
			string iBinRootString( ".bin.root" );
			iFinalRootFileName.replace( iFinalRootFileName.find( iBinRootString ), iBinRootString.length(), ".root" );
			rename( iTempS.str().c_str(), iFinalRootFileName.c_str() );
		}
	}
	fData->Delete();
	return 0;
}

