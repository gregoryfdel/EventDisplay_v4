/*! \class VAnaSum
    \brief class for producing an analysis summary from parameterized veritas data

    \author
      Jamie Holder
      Gernot Maier

*/

#include "VAnaSum.h"

VAnaSum::VAnaSum( string i_datadir, unsigned int iAnalysisType )
{
	fAnalysisType = iAnalysisType;
	fAnalysisRunMode = 0;
	
	fDatadir = i_datadir + "/";
	fPrefix = "";
	fSuffix = ".mscw.root";
	
	fRunPara = 0;
	fStereoOn = 0;
	fStereoOff = 0;
	fRatePlots = 0;
	fRunSummary = 0;
	
	fOPfile = 0;
	fTotalDir = 0;
	fTotalDirName = "total_1";
	fStereoTotalDir = 0;
}

/*
   initialize data analysis

   check run mode

   NOTE: mono analysis might not work anymore

*/
void VAnaSum::initialize( string i_LongListFilename, string i_ShortListFilename, int i_singletel,
						  unsigned int iRunType, string i_outfile, int iRandomSeed, string iRunParameterfile )
{
	char i_temp[2000];
	char i_title[200];
	
	///////////////////////////////////////////////////////////////////////////////
	// check analysis run mode
	fAnalysisRunMode = iRunType;
	// merging analysis
	if( fAnalysisRunMode == 1 )
	{
		cout << "merging analysis" << endl << endl;
	}
	// sequentiell analysis
	else
	{
		cout << "sequentiell analysis" << endl << endl;
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// define run parameter, load list of runs
	fRunPara = new VAnaSumRunParameter();
	if( !fRunPara->readRunParameter( iRunParameterfile ) )
	{
		cout << "error while reading run parameters" << endl;
		cout << "...exiting" << endl;
		exit( -1 );
	}
	int i_npair = 0;
	if( i_LongListFilename.size() > 0 )
	{
		i_npair = fRunPara->loadLongFileList( i_LongListFilename, false, ( fAnalysisRunMode == 1 ) );
	}
	else
	{
		i_npair = fRunPara->loadShortFileList( i_ShortListFilename, fDatadir, ( fAnalysisRunMode == 1 ) );
	}
	if( i_npair == 0 )
	{
		cout << "VAnaSum error: no files found in runlist" << endl;
		cout << "...exiting" << endl;
		exit( -1 );
	}
	cout << "Random seed for stereo maps: " << iRandomSeed << endl;
	cout << endl;
	cout << "File with list of runs ";
	if( i_LongListFilename.size() > 0 )
	{
		cout << " (long version): " << i_LongListFilename;
		cout << " (file format version " << fRunPara->getInputFileVersionNumber() << ", " << i_npair << " runs found in list)" << endl;
	}
	else
	{
		cout << " (short version): " << i_ShortListFilename;
		cout << " (" << i_npair << " runs found in list)";
		cout << endl;
	}
	
	if( fAnalysisRunMode != 1 )
	{
		if( fRunPara->getInputFileVersionNumber() > 3 )
		{
			fRunPara->getEventdisplayRunParameter( fDatadir );
		}
		else
		{
			fRunPara->getWobbleOffsets( fDatadir );
		}
	}
	fNumTels = fRunPara->getMaxNumberofTelescopes();
	fTelOffset = 0;
	if( fNumTels == 1 )
	{
		fTelOffset = i_singletel;
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// open output root file for all the results and create directory structure
	fOPfile = new TFile( i_outfile.c_str(), "recreate" );
	if( fOPfile->IsZombie() )
	{
		cout << "Error: cannot open output file " << i_outfile << endl;
		cout << "exiting..." << endl;
		exit( -1 );
	}
	
	// make directories with combined result
	fOPfile->cd();
	fTotalDir = fOPfile->mkdir( fTotalDirName.c_str(), "combined results from all runs" );
	if( !fTotalDir )
	{
		cout << "VAnaSum::initialize error creating directory " << fTotalDirName << " in output file " << fOPfile->GetName() << endl;
		exit( -1 );
	}
	fTotalDir->cd();
	fStereoTotalDir = fTotalDir->mkdir( "stereo", "combined stereo results" );
	if( !fStereoTotalDir )
	{
		cout << "VAnaSum::initialize error creating directory stereo in output file " << fOPfile->GetName() << endl;
		exit( -1 );
	}
	
	// make mono directories
	if( fAnalysisType == 0 || fAnalysisType == 1 || fAnalysisType == 5 )
	{
		for( int i = 0; i < fNumTels; i++ )
		{
			sprintf( i_temp, "mono_Tel_%d", i + 1 + fTelOffset );
			sprintf( i_title, "combined mono results for telescope %d", i + 1 + fTelOffset );
			fTotalDir->cd();
			fMonoTotalDir.push_back( fTotalDir->mkdir( i_temp, i_title ) );
		}
	}
	
	// directories with run wise results
	for( unsigned int j = 0; j < fRunPara->fRunList.size(); j++ )
	{
		sprintf( i_temp, "%s%s%d%s", fDatadir.c_str(), fPrefix.c_str(), fRunPara->fRunList[j].fRunOn, fSuffix.c_str() );
		fRunPara->fRunList[j].fRunOnFileName = i_temp;
		sprintf( i_temp, "%s%s%d%s", fDatadir.c_str(), fPrefix.c_str(), fRunPara->fRunList[j].fRunOff, fSuffix.c_str() );
		fRunPara->fRunList[j].fRunOffFileName = i_temp;
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////
	// merging analysis
	// (combine several anasum files into one analysis file)
	///////////////////////////////////////////////////////////////////////////////////////////
	if( fAnalysisRunMode == 1 )
	{
		// loop over all files in run list and copy histograms
		for( unsigned int j = 0; j < fRunPara->fRunList.size(); j++ )
		{
			// open input file
			sprintf( i_temp, "%s/%d.anasum.root", fDatadir.c_str(), fRunPara->fRunList[j].fRunOn );
			TFile iAnasumInputFile( i_temp );
			if( iAnasumInputFile.IsZombie() )
			{
				cout << "VAnasum::initialize error opening anasum file for run  " << fRunPara->fRunList[j].fRunOn << ": " << endl;
				cout << iAnasumInputFile.GetName() << endl;
				cout << "exiting..." << endl;
				exit( -1 );
			}
			// copying histograms
			cout << "reading in histograms from " << iAnasumInputFile.GetName() << endl;
			fOPfile->cd();
			sprintf( i_temp, "run_%d", fRunPara->fRunList[j].fRunOn );
			TDirectory* iDir = iAnasumInputFile.GetDirectory( i_temp );
			if( iDir )
			{
				cout << "\t copy histograms for run " << fRunPara->fRunList[j].fRunOn << endl;
				copyDirectory( iDir );
				fOldRunList.insert( fRunPara->fRunList[j].fRunOn );
			}
			else
			{
				cout << endl;
				cout << "Fatal error: directory not found for run " << fRunPara->fRunList[j].fRunOn << endl;
				cout << "...exiting" << endl;
				exit( -1 );
			}
			iDir = fOPfile->GetDirectory( i_temp );
			fStereoRunDir.push_back( iDir );
			
			// get list of excluded regions
			// (note: list is read only from first file)
			if( j == 0 )
			{
				fRunPara->getListOfExcludedSkyRegions( &iAnasumInputFile );
			}
			
			// close input file
			iAnasumInputFile.Close();
		}
	}
	//////////////////////////////////////////////////////////////////
	// STANDARD ANALYSIS (analysis all individual runs; sequentiell)
	else
	{
		vector< TDirectory*> i_monoDir;
		for( unsigned int j = 0; j < fRunPara->fRunList.size(); j++ )
		{
			sprintf( i_temp, "run_%d", fRunPara->fRunList[j].fRunOn );
			sprintf( i_title, "results for run %d", fRunPara->fRunList[j].fRunOn );
			fOPfile->cd();
			TDirectory* iDir = fOPfile->GetDirectory( i_temp );
			// update analysis
			if( fAnalysisRunMode == 1 && iDir )
			{
				cout << "Found directory for run " << fRunPara->fRunList[j].fRunOn << endl;
				fOldRunList.insert( fRunPara->fRunList[j].fRunOn );
				fRunDir.push_back( iDir );
				fStereoRunDir.push_back( iDir->GetDirectory( "stereo" ) );
			}
			else
			{
				fRunDir.push_back( fOPfile->mkdir( i_temp, i_title ) );
				if( !fRunDir.back() )
				{
					cout << "VAnaSum::initialize error creating run directory " << i_temp << " in output file " << fOPfile->GetName() << endl;
					cout << "(run " << fRunPara->fRunList[j].fRunOn << ")" << endl;
					exit( -1 );
				}
				
				sprintf( i_temp, "stereo" );
				sprintf( i_title, "stereo results for run %d", fRunPara->fRunList[j].fRunOn );
				fRunDir.back()->cd();
				fStereoRunDir.push_back( fRunDir.back()->mkdir( i_temp, i_title ) );
				
				/*           fStereoRunDir.back()->cd();
				         char i_temp1[2000];
				         sprintf( i_temp1, "%s%s%d%s", fDatadir.c_str(), fPrefix.c_str(), fRunPara->fRunList[j].fRunOn,fSuffix.c_str() );
				         TFile *oldfile = new TFile(i_temp1);
				         TTree *iTree = (TTree*)oldfile->Get("telconfig");
				
				         TTree *newtree = iTree->CloneTree();
				         newtree->Print();
				         newtree->SetDirectory( fStereoRunDir.back() );
				       fStereoRunDir.back()->Write();
				       */
				
				if( !fStereoRunDir.back() )
				{
					cout << "VAnaSum::initialize error creating stereo run directory ";
					cout << i_temp << " in output file " << fOPfile->GetName() << endl;
					cout << "(run " << fRunPara->fRunList[j].fRunOn << ")" << endl;
					exit( -1 );
				}
				// mono directories
				if( fAnalysisType == 0 || fAnalysisType == 1 || fAnalysisType == 5 )
				{
					i_monoDir.clear();
					for( int i = 0; i < fNumTels; i++ )
					{
						sprintf( i_temp, "mono_Tel_%d", i + 1 + fTelOffset );
						sprintf( i_title, "mono results for run %d and telescope %d", fRunPara->fRunList[j].fRunOn, i + 1 + fTelOffset );
						fRunDir.back()->cd();
						i_monoDir.push_back( fRunDir.back()->mkdir( i_temp, i_title ) );
					}
					fMonoRunDir.push_back( i_monoDir );
				}
			}
		}
		
		//////////////////////////////////////////////////
		// set up data chains (all runs combined)
		// get wobble offsets
		// get run duration
		fTotalExposureOn = 0.;
		cout << endl;
		cout << "-----------------------------------------------------------------------(3)" << endl;
		for( unsigned int j = 0; j < fRunPara->fRunList.size(); j++ )
		{
			sprintf( i_temp, "%s%s%d%s", fDatadir.c_str(), fPrefix.c_str(), fRunPara->fRunList[j].fRunOn, fSuffix.c_str() );
			cout << "Chaining run " << j << " of " << fRunPara->fRunList.size() << " runs with source data: " << i_temp << endl;
			
			// get azimuth range
			double azmin, azmax = 0.;
			fRunAzMeanOn[fRunPara->fRunList[j].fRunOn] = getAzRange( fRunPara->fRunList[j].fRunOn, "data", azmin, azmax );
			fRunAzMinOn[fRunPara->fRunList[j].fRunOn] = azmin;
			fRunAzMaxOn[fRunPara->fRunList[j].fRunOn] = azmax;
			
			// get mean noise level
			fRunPedVarsOn[fRunPara->fRunList[j].fRunOn] = getNoiseLevel( fRunPara->fRunList[j].fRunOn );
		}
		fTotalExposureOff = 0.;
		
		cout << "-----------------------------------------------------------------------(4)" << endl;
		for( unsigned int j = 0; j < fRunPara->fRunList.size(); j++ )
		{
			sprintf( i_temp, "%s%s%d%s", fDatadir.c_str(), fPrefix.c_str(), fRunPara->fRunList[j].fRunOff, fSuffix.c_str() );
			cout << "Chaining run " << j << " of " << fRunPara->fRunList.size() << " runs with background data: " << i_temp << endl;
			
			// get azimuth range
			double azmin, azmax = 0.;
			fRunAzMeanOff[fRunPara->fRunList[j].fRunOff] = getAzRange( fRunPara->fRunList[j].fRunOff, "data", azmin, azmax );
			fRunAzMinOff[fRunPara->fRunList[j].fRunOff] = azmin;
			fRunAzMaxOff[fRunPara->fRunList[j].fRunOff] = azmax;
			
			// get noise level
			fRunPedVarsOff[fRunPara->fRunList[j].fRunOff] = getNoiseLevel( fRunPara->fRunList[j].fRunOff );
			
			// copy TTree telconfig to anasum.root file
			fStereoRunDir.back()->cd();
			char i_temp1[2000];
			sprintf( i_temp1, "%s%s%d%s", fDatadir.c_str(), fPrefix.c_str(), fRunPara->fRunList[j].fRunOn, fSuffix.c_str() );
			TFile* oldfile = new TFile( i_temp1 );
			if( !oldfile->IsZombie() )
			{
				TTree* iTree = ( TTree* )oldfile->Get( "telconfig" );
				if( iTree )
				{
					TTree* newtree = iTree->CloneTree();
					newtree->SetDirectory( fStereoRunDir.back() );
				}
				// copy TTree pointingDataReduced to anasum.root file
				TTree* jTree = ( TTree* )oldfile->Get( "pointingDataReduced" );
				if( jTree )
				{
					TTree* newtreej = jTree->CloneTree();
					newtreej->SetDirectory( fStereoRunDir.back() );
				}
			}
			
			// write the TTree's telconfig, pointingDataReduced, and runparameterV2 to the anasum.root file
			fStereoRunDir.back()->Write();
			
			// copy VEvndispRunParameter 'runparameterV2' to anasum.root file
			fStereoRunDir.back()->cd();
			VEvndispRunParameter* evnrunpar = ( VEvndispRunParameter* )oldfile->Get( "runparameterV2" );
			evnrunpar->Write();
			
		}
		
		// set up mono plots
		if( fAnalysisType == 0 || fAnalysisType == 1 || fAnalysisType == 5 )
		{
			for( int i = 0; i < fNumTels; i++ )
			{
				sprintf( i_temp, "on_%d", i + 1 );
				fMonoOn.push_back( new VMonoPlots( true, 0, i_temp, fRunPara, i ) );
				
				sprintf( i_temp, "off_%d", i + 1 );
				fMonoOff.push_back( new VMonoPlots( false, 0, i_temp, fRunPara, i ) );
			}
		}
	}
	cout << "-----------------------------------------------------------------------(5)" << endl;
	
	/////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////
	// VStereoAnalysis will check validity of the data trees.
	fStereoOn = new VStereoAnalysis( true, "on", fRunPara, fStereoRunDir, fStereoTotalDir, fDatadir, iRandomSeed, ( fAnalysisRunMode == 1 ) );
	fRunExposureOn = fStereoOn->getRunDuration(); // Default exposure is run duration (only really necessary for MONO analysis)
	fStereoOff = new VStereoAnalysis( false, "off", fRunPara, fStereoRunDir, fStereoTotalDir, fDatadir, iRandomSeed, ( fAnalysisRunMode == 1 ) );
	// Default exposure is run duration (only really necessary for MONO analysis)
	fRunExposureOff = fStereoOff->getRunDuration();
	
	// rate plots and run summary
	// (rate plot times are relevant for ON runs only)
	fRatePlots = new VRatePlots( fRunPara, fStereoOn->getRunMJD() );
	if( fAnalysisType == 0 || fAnalysisType == 1 || fAnalysisType == 5 )
	{
		for( int i = 0; i < fNumTels; i++ )
		{
			fMonoRatePlots.push_back( new VRatePlots( fRunPara, fStereoOn->getRunMJD() ) );
		}
		// MJD times for mono analysis calculated in VStereoAnalysis!
		// Shouldn't make a difference:
		// - VStereoAnalysis is initialised even in the case of mono analysis
		// - the trees are the same
	}
	fRunSummary = new VRunSummary();
	
	fMeanRawRateOn = 0.;
	fMeanRawRateOff = 0.;
	fMeanElevationOn = 0.;
	fMeanElevationOff = 0.;
	fNMeanElevation = 0.;
	fMeanAzimuthOn = 0.;
	fMeanAzimuthOff = 0.;
	fMeanDeadTimeOn = 0.;
	fMeanDeadTimeOff = 0.;
	fMeanPedVarsOn = 0.;
	fMeanPedVarsOff = 0.;
	
	//////////////////////////////////////////////////////////
	// fill runsummary (running from anasum, new total only)
	if( fAnalysisRunMode == 1 )
	{
		fRunSummary->fill( fDatadir, fTotalDirName, fRunPara->fRunList );
		
		fTotalExposureOn = fRunSummary->fTotalExposureOn;
		fTotalExposureOff = fRunSummary->fTotalExposureOff;
		fRunExposureOn  = fRunSummary->f_exposureOn;
		fRunExposureOff = fRunSummary->f_exposureOff;
		fStereoOn->setRunMJD( fRunSummary->fRunMJD );
		fStereoOff->setRunMJD( fRunSummary->fRunMJD );
		fMeanAzimuthOn  = fRunSummary->fMeanAzimuthOn;
		fMeanAzimuthOff = fRunSummary->fMeanAzimuthOff;
		fMeanElevationOn = fRunSummary->fMeanElevationOn;
		fMeanElevationOff = fRunSummary->fMeanElevationOff;
		fNMeanElevation = fRunSummary->fNMeanElevation;
		fMeanDeadTimeOn = fRunSummary->fMeanDeadTimeOn;
		fMeanDeadTimeOff = fRunSummary->fMeanDeadTimeOff;
		fMeanRawRateOn  = fRunSummary->fMeanRawRateOn;
		fMeanRawRateOff = fRunSummary->fMeanRawRateOff;
		fMeanPedVarsOn  = fRunSummary->fMeanPedVarsOn;
		fMeanPedVarsOff = fRunSummary->fMeanPedVarsOff;
	}
}


/*!
 *   do stereo analysis
 *
 *   loop first over all runs in run list, then do combined analysis
 *
 */
void VAnaSum::doStereoAnalysis( bool iSkyPlots )
{
	// full sky plots or analysis of source region only
	if( !iSkyPlots )
	{
		fStereoOn->setNoSkyPlots( true );
		fStereoOff->setNoSkyPlots( true );
	}
	
	if( fAnalysisRunMode != 1 )
	{
		// do stereo analysis for each run
		for( unsigned int i = 0; i < fRunPara->fRunList.size(); i++ )
		{
			cout << endl;
			cout << "---------------------------------------" << endl;
			fRunPara->printStereoParameter( i );
			
			// analyze run
			doStereoAnalysis( i, fRunPara->fRunList[i].fRunOn, fRunPara->fRunList[i].fRunOff, fStereoRunDir[i] );
			
			cout << "---------------------------------------" << endl;
		}
	}
	fStereoTotalDir->cd();
	fRatePlots->write();
	
	// now combine all runs to give 'total' results
	cout << endl;
	cout << "---------------------------------------" << endl;
	cout << "Stereo analysis for all runs:" << endl;
	
	doStereoAnalysis( -1, -1, -1, fStereoTotalDir );
	
	cout << "---------------------------------------" << endl;
	
	// print table with all results
	if( fRunSummary )
	{
		fRunSummary->print();
	}
}


/*
 * run stereo analysis for the given run (sky map and histogram filling, on-off)
 *
 */

void VAnaSum::doStereoAnalysis( int icounter, int onrun, int offrun, TDirectory* idir )
{
	if( !idir->cd() )
	{
		cout << "VAnaSum::doStereoAnalysis error, directory not found " << endl;
	}
	
	////////////////////////////////////////////////////////////
	// fill on and off histograms and sky maps
	if( onrun  == -1 )
	{
		fStereoOn->fillHistograms( icounter, onrun, -1.e3, 1.e3, -1. );
	}
	else
	{
		fStereoOn->fillHistograms( icounter, onrun, fRunAzMinOn[onrun], fRunAzMaxOn[onrun], fRunPedVarsOn[onrun] );
	}
	
	if( offrun == -1 )
	{
		fStereoOff->fillHistograms( icounter, offrun, -1.e3, 1.e3, -1. );
	}
	else
	{
		fStereoOff->fillHistograms( icounter, offrun, fRunAzMinOff[offrun], fRunAzMaxOff[offrun], fRunPedVarsOff[offrun] );
	}
	
	////////////////////////////////////////////////////////////
	// get exposures
	double iexp_on    = fTotalExposureOn;
	double iexp_off   = fTotalExposureOff;
	if( onrun != -1 )
	{
		iexp_on = fStereoOn->getEffectiveExposure( onrun );
		fRunExposureOn[onrun] = iexp_on;
		fTotalExposureOn += iexp_on;
		
		iexp_off = fStereoOff->getEffectiveExposure( offrun );
		fRunExposureOff[offrun] = iexp_off;
		fTotalExposureOff += iexp_off;
	}
	
	// calculate normalisation taking into account different length of on and off runs
	double i_norm = 1.;;
	if( iexp_off < 10. )
	{
		i_norm = 1.;    // safety net for very short runs (<10 s)
	}
	else
	{
		i_norm = iexp_on / iexp_off;
	}
	
	if( onrun != -1 && offrun != -1 )
	{
		cout << endl << "Mean properties for this pair: ON=" << onrun;
		cout << ", OFF=" << offrun << " -----------------------------" << endl;
	}
	cout << "\t Exposure ON=" << iexp_on << " secs (" << iexp_on / 60. << "min)";
	cout <<               "  OFF=" << iexp_off << " secs (" << iexp_off / 60. << "min),";
	cout << " Normalization=" << i_norm << endl;
	cout << "\t Az range ON [" << fRunAzMinOn[onrun] << "," << fRunAzMaxOn[onrun] << "],";
	cout << " OFF [" << fRunAzMinOff[offrun] << "," << fRunAzMaxOff[offrun] << "]" << endl;
	
	////////////////////////////////////////////////////////////
	// calculate and print mean elevation and raw rate
	if( onrun != -1 )
	{
		cout << "\t mean elevation: " << fStereoOn->getMeanElevation() << " (ON), " << fStereoOff->getMeanElevation() << " (OFF)" << endl;
		cout << "\t mean azimuth: " << fRunAzMeanOn[onrun] << " (ON), " << fRunAzMeanOff[offrun] << " (OFF)" << endl;
		if( fStereoOn->getMeanElevation() > 0. )
		{
			fMeanElevationOn += fStereoOn->getMeanElevation();
			fMeanAzimuthOn += fStereoOn->getMeanAzimuth();
		}
		else if( fStereoOff->getMeanElevation() > 0 )
		{
			fMeanElevationOn += fStereoOff->getMeanElevation();
			fMeanAzimuthOn += fStereoOff->getMeanAzimuth();
		}
		fMeanElevationOff += fStereoOff->getMeanElevation();
		fMeanAzimuthOff += fStereoOff->getMeanAzimuth();
		fNMeanElevation++;
		if( iexp_on > 0. && iexp_off > 0. )
		{
			cout << "\t trigger rate : " << fStereoOn->getRawRate() / iexp_on << " Hz (ON), ";
			cout << fStereoOff->getRawRate() / iexp_off << " Hz (Off)" << endl;
			fMeanRawRateOn += fStereoOn->getRawRate() / iexp_on;
			fMeanRawRateOff += fStereoOff->getRawRate() / iexp_off;
		}
		fMeanPedVarsOn += fRunPedVarsOn[onrun];
		fMeanPedVarsOff += fRunPedVarsOff[offrun];
	}
	else
	{
		if( fNMeanElevation > 0. )
		{
			fMeanElevationOn /= fNMeanElevation;
			fMeanElevationOff /= fNMeanElevation;
			fMeanAzimuthOn /= fNMeanElevation;
			if( fMeanAzimuthOn > 180. )
			{
				fMeanAzimuthOn -= 360.;
			}
			fMeanAzimuthOff /= fNMeanElevation;
			if( fMeanAzimuthOff > 180. )
			{
				fMeanAzimuthOff -= 360.;
			}
			fMeanRawRateOn /= fNMeanElevation;
			fMeanRawRateOff /= fNMeanElevation;
			fMeanPedVarsOn /= fNMeanElevation;
			fMeanPedVarsOff /= fNMeanElevation;
		}
		cout << "\t mean elevation: " << fMeanElevationOn << " (ON), " << fMeanElevationOff << " (OFF)" << endl;
		cout << "\t trigger rate : " << fMeanRawRateOn << " Hz (ON), " << fMeanRawRateOff << " Hz (Off)" << endl;
		cout << "\t mean pedvars: " << fMeanPedVarsOn << " (ON), " << fMeanPedVarsOff << " (OFF)" << endl;
	}
	
	////////////////////////////////////////////////////////////
	// give analyzers pointers to alpha histograms
	if( onrun != -1 )
	{
		fStereoOn->setAlphaOff( fStereoOff->getAlpha(), fStereoOff->getAlphaUC() );
		fStereoOff->setAlphaOff( fStereoOff->getAlpha(), fStereoOff->getAlphaUC() );
	}
	////////////////////////////////////////////////////////////
	// create alpha histogram for significance calculations
	fStereoOff->scaleAlpha( i_norm, fStereoOn->getAlpha(), fStereoOn->getStereoSkyMap(), fStereoOff->getStereoSkyMap(),
							fStereoOn->getMeanSignalBackgroundAreaRatio(), false, icounter );
	fStereoOff->scaleAlpha( i_norm, fStereoOn->getAlphaUC(), fStereoOn->getStereoSkyMap(), fStereoOff->getStereoSkyMap(),
							fStereoOn->getMeanSignalBackgroundAreaRatioUC(), true, icounter );
							
	////////////////////////////////////////////////////////////
	// calulate significance in central bin (source bin)
	
	// number of on events
	double i_nevts_on = fStereoOn->getStereoSkyMap()->GetBinContent( fStereoOn->getStereoSkyMap()->GetXaxis()->FindBin( -1.*fRunPara->fTargetShiftWest ),
						fStereoOn->getStereoSkyMap()->GetYaxis()->FindBin( -1.*fRunPara->fTargetShiftNorth ) );
	// number of off events
	double i_nevts_off = fStereoOff->getStereoSkyMap()->GetBinContent( fStereoOff->getStereoSkyMap()->GetXaxis()->FindBin( -1.*fRunPara->fTargetShiftWest ),
						 fStereoOff->getStereoSkyMap()->GetYaxis()->FindBin( -1.*fRunPara->fTargetShiftNorth ) );
	// normalization
	double i_norm_alpha = i_norm * fStereoOff->getAlphaNorm()->GetBinContent( fStereoOff->getAlphaNorm()->GetXaxis()->FindBin( -1.*fRunPara->fTargetShiftWest ),
						  fStereoOff->getAlphaNorm()->GetYaxis()->FindBin( -1.*fRunPara->fTargetShiftNorth ) );

	double i_sig = VStatistics::calcSignificance( i_nevts_on, i_nevts_off, i_norm_alpha );
	double i_rate = 0.;
        double i_rateE = 0.;
	double i_rateOFF = 0.;
	if( iexp_on > 0. && iexp_off > 0. )
	{
		i_rate = ( i_nevts_on - i_norm_alpha * i_nevts_off ) * 60. / iexp_on;       // rates in 1/min
		i_rateOFF = i_norm_alpha * i_nevts_off * 60. / iexp_off;                    // rates in 1/min
                i_rateE = sqrt( i_nevts_on + i_norm * i_norm * i_norm_alpha * i_norm_alpha * i_nevts_off ) * 60. / iexp_on;
	}
	
	cout << endl;
	cout << "\t ---------------------------- " << endl;
	cout << "\t RESULTS FOR SOURCE POSITION: " << endl;
	cout << "\t ---------------------------- " << endl;
	cout << "\t ON:" << i_nevts_on << "  OFF:" << setprecision( 4 ) << i_nevts_off* i_norm_alpha << " (";
        cout << "off " << i_nevts_off << ", alpha=" << i_norm_alpha << ")" << endl;
	cout << "\t " << setprecision( 4 ) <<  i_sig << " Sigma  " << i_rate << "+/-" << i_rateE << " gammas/min" << endl;
	cout << "\t background rate: " << i_rateOFF << " CR/min" << endl;
	cout << endl;
	
	////////////////////////////////////////////////////////////
	// ON OFF Analysis
	////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////
	// on-off analysis
	
	VOnOff* fstereo_onoff = new VOnOff();
	
	// 1D histograms
	fstereo_onoff->doOnOffforParameterHistograms( fStereoOn->getParameterHistograms(),
			fStereoOff->getParameterHistograms(),
			i_norm, i_norm_alpha, ( onrun == -1 ) );
			
	// correlated maps
	fstereo_onoff->doOnOffforSkyHistograms( fStereoOn->getSkyHistograms( false ),
											fStereoOff->getSkyHistograms( false ),
											i_norm, fStereoOff->getAlphaNorm() );
	// uncorrelated maps
	fstereo_onoff->doOnOffforSkyHistograms( fStereoOn->getSkyHistograms( true ),
											fStereoOff->getSkyHistograms( true ),
											i_norm, fStereoOff->getAlphaNormUC() );
											
	// print out maximum in maps
	cout << "\t Maximum in CORRELATED maps: " << endl;
	TH2D* hStSig = ( TH2D* )fstereo_onoff->do2DSignificance( fStereoOn->getStereoSkyMap(), fStereoOff->getStereoSkyMap(), fStereoOff->getAlphaNorm() );
	cout << "\t Maximum in UNCORRELATED maps: " << endl;
	TH2D* hStSigUC = ( TH2D* )fstereo_onoff->do2DSignificance( fStereoOn->getStereoSkyMapUC(), fStereoOff->getStereoSkyMapUC(), fStereoOff->getAlphaNormUC() );
	
	// calculate q-factors
	fstereo_onoff->doQfactors( fStereoOn->getParameterHistograms(), fStereoOff->getParameterHistograms(), i_norm );
	
	// fill rate graphs by run
	if( onrun != -1 )
	{
		fRatePlots->fill( onrun, fStereoOn->getMJD( onrun ), i_sig, hStSig->GetMaximum(), i_nevts_on, i_nevts_off * i_norm, i_rate );
	}
	
	// rate graphs by interval
	doLightCurves( idir, i_norm_alpha, fStereoOn, fStereoOff );
	
	// fill run summary tree
	fillRunSummary( onrun, offrun, iexp_on, iexp_off, i_nevts_on, i_nevts_off, i_norm_alpha, i_sig, i_rate, i_rateOFF, fstereo_onoff );
	
	/////////////////////////////////////////////////////////
	// finalize and write everything to disk
	idir->cd();
	fstereo_onoff->fill1DSignificanceHistogram();
	fstereo_onoff->writeHistograms( hStSig, hStSigUC );
	fStereoOn->writeHistograms( true );
	fStereoOff->writeHistograms( false );
	if( onrun != -1 )
	{
		fStereoOff->writeDebugHistograms();
	}
	if( onrun == -1 )
	{
		// write run summary to disk
		fRunSummary->write();
		// write list of excluded regions to disk (as a tree)
		fRunPara->writeListOfExcludedSkyRegions();
	}
	
	// close data files, etc.
	fStereoOn->terminate();
	fStereoOff->terminate();
	
	
	// clean up
	delete fstereo_onoff;
}

/*

    single telescope analysis

    (NOTE: PROBABLY DOES NOT WORK ANY MORE; NEED SOME WORK)


*/
void VAnaSum::doMonoAnalysis( bool bFull )
{
	// do stereo anlysis for all runs
	cout << "Mono Analysis" << endl;
	cout << "---------------" << endl << endl;
	
	// do mono analysis for each run
	if( bFull )
	{
		for( unsigned int i = 0; i < fRunPara->fRunList.size(); i++ )
		{
			cout << endl;
			cout << "Mono analysis for runs: " << fRunPara->fRunList[i].fRunOn << "\t" << fRunPara->fRunList[i].fRunOff << endl;
			cout << "---------------------------------------" << endl;
			// analyze run
			doMonoAnalysis( fRunPara->fRunList[i].fRunOn, fRunPara->fRunList[i].fRunOff, fRunExposureOn[fRunPara->fRunList[i].fRunOn], fRunExposureOff[fRunPara->fRunList[i].fRunOff], fMonoRunDir[i] );
			fTotalExposureOn += fRunExposureOn[fRunPara->fRunList[i].fRunOn];
			fTotalExposureOff += fRunExposureOff[fRunPara->fRunList[i].fRunOff];
		}
		for( unsigned int i = 0; i < fMonoRatePlots.size(); i++ )
		{
			fMonoTotalDir[i]->cd();
			fMonoRatePlots[i]->write();
		}
	}
	
	cout << endl;
	cout << "Mono analysis for all runs:" << endl;
	cout << "---------------------------" << endl << endl;
	
	doMonoAnalysis( -1, -1, fTotalExposureOn, fTotalExposureOff, fMonoTotalDir );
}


/*!
 *   mono analysis only on/off
 *
 */
void VAnaSum::doMonoAnalysis( int irunon, int irunoff, double iexp_on, double iexp_off, vector<TDirectory*> irundir )
{
	fOPfile->cd();
	
	double i_norm;
	
	if( iexp_off < 10. )
	{
		i_norm = 1.;
	}
	else
	{
		i_norm = iexp_on / iexp_off;
	}
	cout << "Total Exposure ON=" << iexp_on << " secs  OFF=" << iexp_off << " secs";
	cout << ", Normalization=" << i_norm << endl;
	
	// On/Off analysis
	vector< VOnOff* > fmono_onoff;
	
	TDirectory* iDir = gDirectory;
	
	for( int i = 0; i < fNumTels; i++ )
	{
		iDir->cd();
		
		// fill all histograms and maps
		double i_nevts_on  = fMonoOn[i]->fillHistograms( i, irunon );
		double i_nevts_off = fMonoOff[i]->fillHistograms( i, irunoff );
		
		double i_sig = VStatistics::calcSignificance( i_nevts_on, i_nevts_off, i_norm );
		double i_rate = 0.;
		if( iexp_on > 0. )
		{
			i_rate = ( i_nevts_on - i_norm * i_nevts_off ) * 60. / iexp_on;
		}
		
		cout << endl;
		cout << "\t --------- RESULTS ------------" << endl;
		cout << "\t ON:" << i_nevts_on << "  OFF:" << i_nevts_off* i_norm;
		cout << ",  " <<  i_sig << " Sigma  " << i_rate << "+/-" << i_rate / i_sig << " gammas/min" << endl;
		
		fmono_onoff.push_back( new VOnOff() );
		
		fmono_onoff.back()->doOnOff( fMonoOn[i]->getHisList(), fMonoOff[i]->getHisList(), i_norm );
		
		fmono_onoff.back()->doQfactors( fMonoOn[i]->getHisList(), fMonoOff[i]->getHisList(), i_norm );
		
		cout << "\t maximum in 2D sky plots: " << endl;
		TH2D* hMoSig = ( TH2D* )fmono_onoff.back()->do2DSignificance( fMonoOn[i]->getSkyMap(), fMonoOff[i]->getSkyMap(), i_norm );
		
		irundir[i]->cd();
		
		if( irunon != -1 )
		{
			if( i < ( int )fMonoRatePlots.size() )
			{
				fMonoRatePlots[i]->fill( irunon, i_sig, hMoSig->GetMaximum(), i_nevts_on, i_nevts_off * i_norm, i_rate );
			}
		}
		
		fMonoOn[i]->writeHistograms();
		fMonoOff[i]->writeHistograms();
		fmono_onoff.back()->writeMonoHistograms();
		hMoSig->Write();
		cout << "\t ------------------------------" << endl;
	}
}


/*!
 *
 *  produce light curves in time intervalls given in fRunPar->fTimeIntervall
 *
 */
void VAnaSum::doLightCurves( TDirectory* iDir, double ialpha, VStereoAnalysis* ion, VStereoAnalysis* ioff )
{
	// calculate rates and significances
	vector< double > isig;
	vector< double > irate;
	vector< double > itime;
	
	for( unsigned int i = 0; i < ion->getRateTime().size(); i++ )
	{
		isig.push_back( VStatistics::calcSignificance( ion->getRateCounts()[i], ioff->getRateCounts()[i], ialpha ) );
		if( ion->getRateTimeIntervall()[i] > 0. && ion->getRateCounts()[i] > 0. )
		{
			irate.push_back( ( ion->getRateCounts()[i] - ialpha * ioff->getRateCounts()[i] ) * 60. / ion->getRateTimeIntervall()[i] );
		}
		else
		{
			irate.push_back( 0. );
		}
		// mean time of time intervall
		itime.push_back( ion->getRateTime()[i] + ion->getRateTimeIntervall()[i] / 2. / 86400. );
	}
	
	VRatePlots iRate;
	
	iRate.fill( itime, ion->getRateCounts(), ioff->getRateCounts(), isig, irate );
	
	iDir->cd();
	iRate.write( "rateIntervallPlots" );
}


/*!
 *   from http://root.cern.ch/phpBB2/viewtopic.php?t=2789
 *
 *   Author: Rene Brun
 */
void VAnaSum::copyDirectory( TDirectory* source )
{
	//copy all objects and subdirs of directory source as a subdir of the current directory
	TDirectory* savdir = gDirectory;
	TDirectory* adir = savdir->mkdir( source->GetName() );
	if( !adir )
	{
		cout << "VAnaSum::copyDirectory error creating directory " << source->GetName() << endl;
		exit( -1 );
	}
	adir->cd();
	//loop on all entries of this directory
	TKey* key;
	TIter nextkey( source->GetListOfKeys() );
	while( ( key = ( TKey* )nextkey() ) )
	{
		const char* classname = key->GetClassName();
		TClass* cl = gROOT->GetClass( classname );
		if( !cl )
		{
			continue;
		}
		if( cl->InheritsFrom( "TDirectory" ) )
		{
			source->cd( key->GetName() );
			TDirectory* subdir = gDirectory;
			adir->cd();
			copyDirectory( subdir );
			adir->cd();
		}
		else if( cl->InheritsFrom( "TTree" ) )
		{
			if( !source->Get( "tRE" ) )
			{
				TTree* T = ( TTree* )source->Get( key->GetName() );
				adir->cd();
				if( T )
				{
					TTree* newT = T->CloneTree();
					if( newT )
					{
						newT->Write();
						delete newT;
					}
					delete T;
				}
			}
		}
		else
		{
			source->cd();
			TObject* obj = key->ReadObj();
			adir->cd();
			obj->Write();
			delete obj;
		}
	}
	adir->SaveSelf( kTRUE );
	savdir->cd();
}

/*!

   fill run summary tree

   - one entry per run
   - one entry for combined analysis

*/
void VAnaSum::fillRunSummary( int onrun, int offrun, double iexp_on, double iexp_off,
							  double i_nevts_on, double i_nevts_off, double i_norm_alpha,
							  double i_sig, double i_rate, double i_rateOFF, VOnOff* fstereo_onoff )
{
	if( !fRunSummary )
	{
		return;
	}
	
	// fill results tree
	fRunSummary->runOn = onrun;
	fRunSummary->runOff = offrun;
	if( onrun != -1 )
	{
		fRunSummary->MJDOn = fStereoOn->getMJD( onrun );
	}
	else
	{
		fRunSummary->MJDOn = 0.;    //Could make this the mean MJD of all ON runs included in the summed analysis
	}
	if( offrun != -1 )
	{
		fRunSummary->MJDOff = fStereoOff->getMJD( offrun );
	}
	else
	{
		fRunSummary->MJDOff = 0.;
	}
	if( onrun != -1 && fRunPara->fMapRunList.find( onrun ) != fRunPara->fMapRunList.end() )
	{
		fRunSummary->fTargetRA = fRunPara->fMapRunList[onrun].fTargetRA;
	}
	else
	{
		fRunSummary->fTargetRA = 0.;
	}
	if( onrun != -1 && fRunPara->fMapRunList.find( onrun ) != fRunPara->fMapRunList.end() )
	{
		fRunSummary->fTargetDec = fRunPara->fMapRunList[onrun].fTargetDec;
	}
	else
	{
		fRunSummary->fTargetDec = 0.;
	}
	if( onrun != -1 && fRunPara->fMapRunList.find( onrun ) != fRunPara->fMapRunList.end() )
	{
		fRunSummary->fTargetRAJ2000 = fRunPara->fMapRunList[onrun].fTargetRAJ2000;
	}
	else
	{
		fRunSummary->fTargetRAJ2000 = 0.;
	}
	if( onrun != -1 && fRunPara->fMapRunList.find( onrun ) != fRunPara->fMapRunList.end() )
	{
		fRunSummary->fTargetDecJ2000 = fRunPara->fMapRunList[onrun].fTargetDecJ2000;
	}
	else
	{
		fRunSummary->fTargetDecJ2000 = 0.;
	}
	fRunSummary->fSkyMapCentreRAJ2000 = fRunPara->fSkyMapCentreRAJ2000;
	fRunSummary->fSkyMapCentreDecJ2000 = fRunPara->fSkyMapCentreDecJ2000;
	fRunSummary->fTargetShiftRAJ2000 = fRunPara->fTargetShiftRAJ2000;
	fRunSummary->fTargetShiftDecJ2000 = fRunPara->fTargetShiftDecJ2000;
	if( onrun != -1 && fRunPara->fMapRunList.find( onrun ) != fRunPara->fMapRunList.end() )
	{
		fRunSummary->fTargetShiftWest = fRunPara->fMapRunList[onrun].fTargetShiftWest;
	}
	else
	{
		fRunSummary->fTargetShiftWest = 0.;
	}
	if( onrun != -1 && fRunPara->fMapRunList.find( onrun ) != fRunPara->fMapRunList.end() )
	{
		fRunSummary->fTargetShiftNorth = fRunPara->fMapRunList[onrun].fTargetShiftNorth;
	}
	else
	{
		fRunSummary->fTargetShiftNorth = 0.;
	}
	if( onrun != -1 )
	{
		fRunSummary->fWobbleNorth = fStereoOn->getWobbleNorth();
	}
	else
	{
		fRunSummary->fWobbleNorth = 0.;
	}
	if( onrun != -1 )
	{
		fRunSummary->fWobbleWest = fStereoOn->getWobbleWest();
	}
	else
	{
		fRunSummary->fWobbleWest = 0.;
	}
	if( onrun != -1 )
	{
		fRunSummary->fNTel = fRunPara->fMapRunList[onrun].fNTel;
	}
	else
	{
		fRunSummary->fNTel = 0;
	}
	fRunSummary->tOn = iexp_on;
	fRunSummary->tOff = iexp_off;
	if( onrun != -1 )
	{
		fRunSummary->elevationOn = fStereoOn->getMeanElevation();
	}
	else
	{
		fRunSummary->elevationOn = fMeanElevationOn;
	}
	if( onrun != -1 )
	{
		fRunSummary->elevationOff = fStereoOff->getMeanElevation();
	}
	else
	{
		fRunSummary->elevationOff = fMeanElevationOff;
	}
	if( onrun != -1 )
	{
		fRunSummary->azimuthOn = fRunAzMeanOn[onrun];
	}
	else
	{
		fRunSummary->azimuthOn = fMeanAzimuthOn;
	}
	if( onrun != -1 )
	{
		fRunSummary->azimuthOff = fRunAzMeanOff[offrun];
	}
	else
	{
		fRunSummary->azimuthOff = fMeanAzimuthOff;
	}
	if( onrun != -1 )
	{
		if( iexp_on > 0. )
		{
			fRunSummary->RawRateOn = fStereoOn->getRawRate() / iexp_on;
		}
		else
		{
			fRunSummary->RawRateOn = 0.;
		}
		if( iexp_off > 0. )
		{
			fRunSummary->RawRateOff = fStereoOff->getRawRate() / iexp_off;
		}
		else
		{
			fRunSummary->RawRateOff = 0.;
		}
	}
	else
	{
		fRunSummary->RawRateOn = fMeanRawRateOn;
		fRunSummary->RawRateOff = fMeanRawRateOff;
	}
	if( onrun != -1 )
	{
		fRunSummary->pedvarsOn = fRunPedVarsOn[onrun];
	}
	else
	{
		fRunSummary->pedvarsOn = fMeanPedVarsOn;
	}
	if( onrun != -1 )
	{
		fRunSummary->pedvarsOff = fRunPedVarsOff[offrun];
	}
	else
	{
		fRunSummary->pedvarsOff = fMeanPedVarsOff;
	}
	fRunSummary->NOn = i_nevts_on;
	fRunSummary->NOff = i_nevts_off;
	fRunSummary->NOffNorm = i_nevts_off * i_norm_alpha;
	fRunSummary->OffNorm = i_norm_alpha;
	fRunSummary->Signi = i_sig;
	fRunSummary->Rate = i_rate;
	double i_tnorm = 1.;
	if( fRunSummary->tOff > 0. )
	{
		i_tnorm = fRunSummary->tOn / fRunSummary->tOff;
	}
	if( fRunSummary->tOn > 0. )
	{
		fRunSummary->RateE = sqrt( i_nevts_on + i_tnorm * i_tnorm * i_norm_alpha * i_norm_alpha * i_nevts_off ) / fRunSummary->tOn * 60.;
	}
	else
	{
		fRunSummary->RateE = 0.;
	}
	fRunSummary->RateOff = i_rateOFF;
	if( fRunSummary->tOff > 0. )
	{
		fRunSummary->RateOffE = sqrt( i_tnorm * i_tnorm * i_norm_alpha * i_norm_alpha * i_nevts_off ) / fRunSummary->tOff * 60.;
	}
	else
	{
		fRunSummary->RateOffE = 0.;
	}
	if( onrun != -1 )
	{
		fRunSummary->DeadTimeFracOn = fStereoOn->getDeadTimeFraction();
		fMeanDeadTimeOn += fStereoOn->getDeadTimeFraction() * iexp_on;
		fRunSummary->DeadTimeFracOff = fStereoOff->getDeadTimeFraction();
		fMeanDeadTimeOff += fStereoOff->getDeadTimeFraction() * iexp_off;
	}
	else
	{
		if( iexp_on > 0. )
		{
			fRunSummary->DeadTimeFracOn = fMeanDeadTimeOn / iexp_on;
		}
		if( iexp_off > 0. )
		{
			fRunSummary->DeadTimeFracOff = fMeanDeadTimeOff / iexp_off;
		}
	}
	if( fstereo_onoff )
	{
		fRunSummary->MaxSigni = fstereo_onoff->getMaxSigma();
		fRunSummary->MaxSigniX = fstereo_onoff->getMaxSigmaX();
		fRunSummary->MaxSigniY = fstereo_onoff->getMaxSigmaY();
	}
	fRunSummary->fill();
}


void VAnaSum::terminate()
{
	if( fOPfile )
	{
		fOPfile->Close();
	}
}


/*!
 *
 * get azimuth range of a run
 *
 *
 */
double VAnaSum::getAzRange( int i_run, string i_treename, double& azmin, double& azmax )
{
	double azmean = 0.;
	
	azmin = 1.e3;
	azmax = -1.e3;
	
	char i_temp[2000];
	sprintf( i_temp, "%s%s%d%s", fDatadir.c_str(), fPrefix.c_str(), i_run, fSuffix.c_str() );
	TFile* i_f = new TFile( i_temp );
	if( i_f->IsZombie() )
	{
		cout << "VAnaSum::getAZRange fatal error: file not found, " << i_temp << endl;
		exit( -1 );
	}
	
	TTree* i_tree = ( TTree* )i_f->Get( i_treename.c_str() );
	if( !i_tree )
	{
		cout << "VAnaSum::getAZRange tree not found " << i_treename << endl;
	}
	int i_v = 0;
	if( fRunPara )
	{
		i_v = fRunPara->getEVNDISP_TREE_VERSION( i_tree );
	}
	else
	{
		i_v = VGlobalRunParameter::getEVNDISP_TREE_VERSION( i_tree );
	}
	double telaz[4];
	unsigned int ImgSel = 0;
	ULong64_t ImgSel_64 = 0;
	if( i_v < 6 )
	{
		i_tree->SetBranchAddress( "ImgSel", &ImgSel );
	}
	else
	{
		i_tree->SetBranchAddress( "ImgSel", &ImgSel_64 );
	}
	i_tree->SetBranchAddress( "TelAzimuth", telaz );
	
	bool bSet = false;
	for( int i = 0; i < i_tree->GetEntries(); i++ )
	{
		i_tree->GetEntry( i );
		if( i_v < 6 )
		{
			ImgSel_64 = ( ULong64_t )ImgSel;
		}
		
		if( ImgSel_64 > 0 )
		{
			bitset<8 * sizeof( ULong64_t )> a = ImgSel_64;
			for( unsigned int t = 0; t < 4; t++ )
			{
				if( a.test( t ) )
				{
					azmin = telaz[t];
					bSet = true;
					break;
				}
			}
		}
		if( bSet )
		{
			break;
		}
	}
	bSet = false;
	for( int i = i_tree->GetEntries() - 1; i >= 0; i-- )
	{
		i_tree->GetEntry( i );
		
		if( i_v < 6 )
		{
			ImgSel_64 = ( ULong64_t )ImgSel;
		}
		
		if( ImgSel_64 > 0 )
		{
			bitset<8 * sizeof( ULong64_t )> a = ImgSel_64;
			for( unsigned int t = 0; t < 4; t++ )
			{
				if( a.test( t ) )
				{
					azmax = telaz[t];
					bSet = true;
					break;
				}
			}
		}
		if( bSet )
		{
			break;
		}
	}
	if( azmin > 180. )
	{
		azmin -= 360.;
	}
	if( azmax > 180. )
	{
		azmax -= 360.;
	}
	cout << "\t azimuth range: [" << azmin << "," << azmax << "]" << endl;
	
	// calculate mean az
	// mean azimuth angle
	if( azmin > 120. && azmax < -120. )
	{
		azmax += 360.;
	}
	else if( azmin < -150. && azmax > 120. )
	{
		azmin += 360.;
	}
	
	azmean = 0.5 * ( azmin + azmax );
	if( azmean > 180. )
	{
		azmean -= 360.;
	}
	
	i_f->Close();
	delete i_f;
	
	return azmean;
}

/*!

    get mean noise level for a run

*/
double VAnaSum::getNoiseLevel( int i_run )
{
	char i_temp[2000];
	double ipedv = -1.;
	
	sprintf( i_temp, "%s%s%d%s", fDatadir.c_str(), fPrefix.c_str(), i_run, fSuffix.c_str() );
	TFile* i_f = new TFile( i_temp );
	if( i_f->IsZombie() )
	{
		cout << "VAnaSum::getNoiseLevel fatal error: file not found, " << i_temp << endl;
		exit( -1 );
	}
	VTableLookupRunParameter* fR = ( VTableLookupRunParameter* )i_f->Get( "TLRunParameter" );
	if( fR )
	{
		ipedv = fR->meanpedvars;
	}
	else
	{
		ipedv = -1.;
	}
	i_f->Close();
	cout << "\t mean pedestal variations in run " << i_run << ": " << ipedv << endl;
	return ipedv;
}

