/*! \class VFluxCalculation
    \brief read effective area file from anasum output file, sum them up to mean effective area


*/

#include "VFluxCalculation.h"

/*
   empy constructor
*/

VFluxCalculation::VFluxCalculation()
{
	reset();
	
	setTimeBinnedAnalysis();
	
	bZombie = true;
}

/*
   calculate fluxes using a single anasum result file
*/
VFluxCalculation::VFluxCalculation( string ifile, unsigned int iTot, int iRunMin, int iRunMax, double iMJDMin, double iMJDMax, bool iDebug )
{
	reset();
	
	fDebug = iDebug;
	
	bZombie = openDataFile( ifile.c_str() );
	loadRunList( iRunMin, iRunMax, iTot, iMJDMin, iMJDMax );
}


/*
   calculate fluxes using multiple anasum result files
*/
VFluxCalculation::VFluxCalculation( vector< string > ifile, unsigned int iTot, int iRunMin, int iRunMax, double iMJDMin, double iMJDMax )
{
	reset();
	
	bZombie = openDataFile( ifile );
	loadRunList( iRunMin, iRunMax, iTot, iMJDMin, iMJDMax );
}


VFluxCalculation::~VFluxCalculation()
{
	closeFiles();
}

/*

   close anasum result files

*/
void VFluxCalculation::closeFiles()
{
	for( unsigned int i = 0; i < fFile.size(); i++ )
	{
		if( fFile[i] && !fFile[i]->IsZombie() )
		{
			fFile[i]->Close();
		}
	}
}


void VFluxCalculation::reset()
{
	fFile.clear();
	fData = 0;
	
	fMJD_min = -999.;
	fMJD_max = -999.;
	
	// long printout
	fDebug = true;
	
	// formula for significance calulation from Li & Ma
	fLiMaEqu = 17;
	
	// calculate fluxes above this energy
	fMinEnergy = 0.;
	// maximum energy (not clear what defines this value)
	fMaxEnergy = 300.;
	// assumed spectral parameters
	fE0 = 1.;                                     // [TeV]
	fAlpha = -2.5;
	
	// significance parameters: decide when to calculate fluxes and when upper limits
	setSignificanceParameters( 3., 5., 0.99, 5, 17 );
	
	// default flux calulation method is Poissonian (unbounded)
	setFluxCalculationMethod();
	
	// graphs
	gFluxElevation = 0;
	gFluxWobbleOffset = 0;
	gFluxPedvars = 0;
	gFluxAzimuth = 0;
	fCanvasFluxesVSMJD = 0;
	
	fRXTE = 0;
	
}


void VFluxCalculation::resetRunList()
{
	fRunList.clear();
	fRunMJD.clear();
	fRunTOn.clear();
	fIntraRunTOn.clear();
	fTimeBinDuration.clear();
	fRunDeadTime.clear();
	fRunZe.clear();
	fRunAz.clear();
	fRunWobbleOffset.clear();
	fRunPedvars.clear();
	fRunNdiff.clear();
	fRunNdiffE.clear();
	fIntraRunNdiff.clear();
	fIntraRunNdiffE.clear();
	fRunRate.clear();
	fRunRateE.clear();
	fRunNon.clear();
	fIntraRunNon.clear();
	fRunNoff.clear();
	fIntraRunNoff.clear();
	fRunNorm.clear();
	fRunSigni.clear();
	fIntraRunSigni.clear();
	fRunUFL.clear();
	fIntraRunUFL.clear();
	fRunCI_up_1sigma.clear();
	fRunCI_lo_1sigma.clear();
	fRunCI_up_3sigma.clear();
	fRunCI_lo_3sigma.clear();
	fIntraRunCI_up_1sigma.clear();
	fIntraRunCI_lo_1sigma.clear();
	fIntraRunCI_up_3sigma.clear();
	fIntraRunCI_lo_3sigma.clear();
	fRunFlux.clear();
	fRunFluxE.clear();
	fRunFluxConstant.clear();
	fRunFluxConstantE.clear();
	fRunEffArea.clear();
	fIntraRunFlux.clear();
	fIntraRunFluxE.clear();
	fIntraRunFluxConstant.clear();
	fIntraRunFluxConstantE.clear();
	fIntraRunEffArea.clear();
	fRunFluxCI_lo_1sigma.clear();
	fRunFluxCI_up_1sigma.clear();
	fRunFluxCI_lo_3sigma.clear();
	fRunFluxCI_up_3sigma.clear();
	fIntraRunFluxCI_lo_1sigma.clear();
	fIntraRunFluxCI_up_1sigma.clear();
	fIntraRunFluxCI_lo_3sigma.clear();
	fIntraRunFluxCI_up_3sigma.clear();
	
}


/*
    get run list from tree tRunSummary in anasum output file

    get run duration, zenith angle, excess events from this tree (per run)
*/
unsigned int VFluxCalculation::loadRunList( int iRunMin, int iRunMax, unsigned int iTot, double iMJDMin, double iMJDMax )
{
	if( fDebug )
	{
		cout << "VFluxCalculation::loadRunList" << endl;
	}
	if( bZombie )
	{
		return 0;
	}
	
	fMJD_min = iMJDMin;
	fMJD_max = iMJDMax;
	
	resetRunList();
	
	// loop over all files
	char hname[2000];
	if( !fData )
	{
		TChain* c = new TChain( "tRunSummary" );
		for( unsigned int i = 0; i < fFile.size(); i++ )
		{
			if( iTot > 0 )
			{
				sprintf( hname, "total_%d/stereo", iTot );
			}
			else
			{
				sprintf( hname, "total/stereo" );
			}
			if( !fFile[i]->cd( hname ) )
			{
				cout << "directory " << hname << " not found in file " << fFile[i] << endl;
				return 0;
			}
			if( iTot > 0 )
			{
				sprintf( hname, "%s/total_%d/stereo/tRunSummary", fFile[i]->GetName(), iTot );
			}
			else
			{
				sprintf( hname, "%s/total/stereo/tRunSummary", fFile[i]->GetName() );
			}
			c->Add( hname );
			if( fDebug )
			{
				cout << "\t chaining " << hname << endl;
			}
			fData = new CRunSummary( c );
		}
	}
	
	int nentries = fData->fChain->GetEntries();
	if( fDebug )
	{
		cout << "total number of runs: " << nentries - 1;
		cout << " (run number interval [" << iRunMin << ", " << iRunMax << "])" << endl;
	}
	
	for( int i = 0; i < nentries; i++ )
	{
		fData->GetEntry( i );
		
		// check run min/max requirements
		if( fData->runOn > 0 && iRunMin >= 0 && fData->runOn < iRunMin )
		{
			continue;
		}
		if( fData->runOn > 0 && iRunMax >= 0 && fData->runOn > iRunMax )
		{
			continue;
		}
		// check MJD min/max requirements
		if( iMJDMin > 0. && fData->MJDOn < iMJDMin && fData->runOn != -1 )
		{
			continue;
		}
		if( iMJDMax > 0. && fData->MJDOn > iMJDMax && fData->runOn != -1 )
		{
			continue;
		}
		
		// skip runs with zero elevation (no events)
		if( fData->elevationOn < 5. && fData->elevationOff < 5 )
		{
			if( fDebug )
			{
				cout << "\t skip run " << fData->runOn << ": no entries" << endl;
			}
			continue;
		}
		
		fRunList.push_back( fData->runOn );
		fRunMJD.push_back( fData->MJDOn );
		fRunTOn.push_back( fData->tOn );
		
		fRunDeadTime.push_back( fData->DeadTimeFracOn );
		
		// mean zenith angle for this run
		if( fData->elevationOn > 1. )
		{
			fRunZe.push_back( 90. - fData->elevationOn );
			fRunAz.push_back( fData->azimuthOn );
		}
		else
		{
			fRunZe.push_back( 90. - fData->elevationOff );
			fRunAz.push_back( fData->azimuthOff );
		}
		fRunWobbleOffset.push_back( sqrt( fData->WobbleNorth * fData->WobbleNorth + fData->WobbleWest * fData->WobbleWest ) );
		fRunPedvars.push_back( fData->pedvarsOn );
		// number of gamma-like events per run (this might be overwritten later)
		fRunNdiff.push_back( fData->NOn - fData->NOff * fData->OffNorm );
		// error on number of gamma-like events
		fRunNdiffE.push_back( sqrt( fData->NOn + fData->NOff *  fData->OffNorm *  fData->OffNorm ) );
		if( fData->tOn > 0. )
		{
			fRunRate.push_back( fRunNdiff.back() / fData->tOn * 60. );
		}
		else
		{
			fRunRate.push_back( 0. );
		}
		if( fData->tOn > 0. )
		{
			fRunRateE.push_back( fRunNdiffE.back() / fData->tOn * 60. );
		}
		else
		{
			fRunRateE.push_back( 0. );
		}
		fRunNon.push_back( fData->NOn );
		fRunNoff.push_back( fData->NOff );
		fRunNorm.push_back( fData->OffNorm );
		fRunSigni.push_back( -99. );
		fRunUFL.push_back( -99. );
		fRunCI_lo_1sigma.push_back( -99. );
		fRunCI_up_1sigma.push_back( -99. );
		fRunCI_lo_3sigma.push_back( -99. );
		fRunCI_up_3sigma.push_back( -99. );
		fRunFlux.push_back( -99. );
		fRunFluxE.push_back( -99. );
		fRunFluxConstant.push_back( 0. );
		fRunFluxConstantE.push_back( 0. );
		fRunEffArea.push_back( 0. );
		fRunFluxCI_lo_1sigma.push_back( -99. );
		fRunFluxCI_up_1sigma.push_back( -99. );
		fRunFluxCI_lo_3sigma.push_back( -99. );
		fRunFluxCI_up_3sigma.push_back( -99. );
	}
	
	cleanRunList();
	
	if( fRunNorm.size() == 0 )
	{
		cout << "run list empty - no analysis possible" << endl;
		return 0;
	}
	
	// recalculate some of the total values
	double itot = 0.;
	double itotNorm = 0.;
	double im = 0.;
	double itotze = 0.;
	double itotdead = 0.;
	// calculate mean norm (weighted by observation time)
	for( unsigned int r = 0; r < fRunList.size(); r++ )
	{
		if( fRunList[r] > 0 )
		{
			itot += fRunTOn[r];
			if( fRunNorm[r] > 0. )
			{
				im       += fRunTOn[r] / fRunNorm[r];
				itotNorm += fRunTOn[r];
			}
			itotze   += fRunTOn[r] * fRunZe[r];
			itotdead += fRunTOn[r] * fRunDeadTime[r];
		}
	}
	if( itotNorm > 0. && im > 0. )
	{
		fRunNorm[fRunNorm.size() - 1] = 1. / ( im / itotNorm );
	}
	else
	{
		fRunNorm[fRunNorm.size() - 1] = 1.;
	}
	if( itot > 0. )
	{
		fRunZe[fRunZe.size() - 1] = itotze / itot;
	}
	if( itot > 0. )
	{
		fRunDeadTime[fRunDeadTime.size() - 1] = itotdead / itot;
	}
	fRunTOn[fRunNorm.size() - 1] = itot;
	
	if( fDebug )
	{
		cout << "total number of runs in runlist: ";
		if( fRunList.size() > 0 )
		{
			cout << fRunList.size() - 1 << endl;
		}
		else
		{
			cout << 0 << endl;
		}
	}
	
	return fRunList.size();
}

void VFluxCalculation::printRunList()
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		cout << "RUN " << fRunList[i];
		cout << ",  TOn: " << fRunTOn[i];
		cout << ", Ze [deg]: " << fRunZe[i];
		cout << endl;
	}
}


bool VFluxCalculation::openDataFile( string ifile )
{
	fFile.clear();
	fFile.push_back( new TFile( ifile.c_str() ) );
	if( fFile.back()->IsZombie() )
	{
		return true;
	}
	
	return false;
}


bool VFluxCalculation::openDataFile( vector< string > ifile )
{
	fFile.clear();
	for( unsigned int i = 0; i < ifile.size(); i++ )
	{
		fFile.push_back( new TFile( ifile[i].c_str() ) );
		if( fFile.back()->IsZombie() )
		{
			return true;
		}
	}
	
	return false;
}

/*

    calculate spectral weighted effective area average for this energy interval

*/
double VFluxCalculation::integrateEffectiveAreaInterval( double x0, double x1, double x2, double ieff_mean )
{
	// return value
	double i_InterEffArea = 0.;
	
	// energy bin
	double iE_low = TMath::Power( 10., ( x0 + x1 ) / 2. );
	double iE_up  = TMath::Power( 10., ( x1 + x2 ) / 2. );
	double dE = 0.;
	
	// apply lower energy threshold cut
	if( iE_up  < fMinEnergy )
	{
		return -99.;
	}
	if( iE_low < fMinEnergy )
	{
		iE_low = fMinEnergy;
	}
	// MC statistic only good up to this energy
	if( iE_low > fMaxEnergy )
	{
		return -99.;
	}
	if( iE_up  > fMaxEnergy )
	{
		iE_up = fMaxEnergy;
	}
	
	// width of energy bin (on linear scale)
	dE = iE_up - iE_low;
	
	// integrate spectral weighted effective area
	// using trapezoidal rule
	i_InterEffArea  = ieff_mean * dE * 0.5 * ( TMath::Power( iE_up / fE0, fAlpha ) + TMath::Power( iE_low / fE0, fAlpha ) );
	// (correction term at E_low in order to maximize second derivative)
	i_InterEffArea -=  1. / 12. * dE * dE * dE * ieff_mean * fAlpha * ( fAlpha - 1. ) / fE0 / fE0
					   * TMath::Power( iE_low / fE0, fAlpha - 2. );
					   
	return i_InterEffArea;
}

/*!
    read effective areas from anasum output file for each run
    (interpolate between zenith angle bins)

    calculate spectral weighted effective area average

    < A (E) > = INT_{E}^{INF} A(E) (E/E_0)^{-fAlpha} dE


*/
void VFluxCalculation::getIntegralEffectiveArea()
{
	if( fDebug )
	{
		cout << "calculating spectral weighted integral effective areas" << endl;
		cout << "------------------------------------------------------" << endl;
		cout << endl;
	}
	
	// loop over all files
	for( unsigned int f = 0; f < fFile.size(); f++ )
	{
		if( bZombie || !fFile[f] )
		{
			return;
		}
		
		// loop over all runs in this file
		fFile[f]->cd();
		if( fDebug )
		{
			cout << "now at file " << fFile[f]->GetName() << " (runlist size: " << fRunList.size() << ")" << endl;
		}
		char hname[800];
		for( unsigned int i = 0; i < fRunList.size(); i++ )
		{
			// don't have effective areas for combined run histograms
			if( fDebug && fRunList[i] > 0 )
			{
				cout << "Calculating effective area for run " << fRunList[i] << endl;
			}
			if( fRunList[i] < 0. )
			{
				continue;
			}
			
			if( fDebug )
			{
				cout << "\t Run " << int( fRunList[i] + 0.5 ) << " at ze [deg]: " << fRunZe[i] << "\t";
			}
			
			sprintf( hname, "run_%d/stereo/EffectiveAreas", int( fRunList[i] + 0.5 ) );
			if( !fFile[f]->Get( hname ) )
			{
				continue;
			}
			if( !fFile[f]->cd( hname ) )
			{
				cout << "directory " << hname << " not found" << endl;
				cout << "continue..." << endl;
				continue;
			}
			/////////////////////////////////////////////////////////////////////////////////////////////////
			/////////////////////////////////////////////////////////////////////////////////////////////////
			// graphs with mean effective areas (mean for given run; filled in anasum)
			/////////////////////////////////////////////////////////////////////////////////////////////////
			TGraphAsymmErrors* g = ( TGraphAsymmErrors* )gDirectory->Get( "gMeanEffectiveArea" );
			/////////////////////////////////////////////////////////////////////////////////////////////////
			// found graph with effective areas
			if( !g )
			{
				// try go get off graph
				g = ( TGraphAsymmErrors* )gDirectory->Get( "gMeanEffectiveArea_off" );
				if( !g )
				{
					// backwards compatibility to files prodcued with v35x
					g = ( TGraphAsymmErrors* )gDirectory->Get( "gMeanEffectiveAreaEMC" );
					if( !g )
					{
						g = ( TGraphAsymmErrors* )gDirectory->Get( "gMeanEffectiveAreaEMC_off" );
					}
					if( !g )
					{
						cout << "error: effective area graph not found" << endl;
						cout << "continue..." << endl;
						fRunEffArea[i] = 0.;
						continue;
					}
				}
			}
			
			double ieff_int = 0.;
			double ieff_mean = 0.;
			double ieff_meanA = 0.;
			double x0, x1, x2;
			/////////////////////////////////////////////////////////////////////
			// calculate spectral weighted integral effective area
			fRunEffArea[i] = 0.;
			// loop over all bins in effective area histogram
			for( int b = 1; b < g->GetN() - 1; b++ )
			{
				// get energies and effective areas
				g->GetPoint( b - 1, x0, ieff_mean );
				g->GetPoint( b + 1, x2, ieff_mean );
				g->GetPoint( b, x1, ieff_mean );
				
				ieff_int = integrateEffectiveAreaInterval( x0, x1, x2, ieff_mean );
				if( ieff_int > 0. )
				{
					fRunEffArea[i] += ieff_int;
				}
				else
				{
					continue;
				}
				
				// integral effective area
				ieff_meanA += ieff_mean;
			}
			// debug output
			if( fDebug )
			{
				cout << " t_obs " << setprecision( 5 ) << fRunTOn[i] / 60. << " [min],";
				cout << "< A > " << setprecision( 6 ) << fRunEffArea[i] << " [m^2]";
				cout << ", integral effective area [m^2]: " << ieff_meanA << endl;
			}
			
			/////////////////////////////////////////////////////////////////////
			// TIME dependent effective areas
			/////////////////////////////////////////////////////////////////////
			if( fTimebinned )
			{
				vector < double > i_IntraEffArea;
				vector < double > i_IntraEffArea_before;
				//--- this vector must have the same number of entry as there is time bin
				for( unsigned int ii = 0 ; ii < fIntraRunTOn[i].size(); ii++ )
				{
					i_IntraEffArea.push_back( 0. );
				}
				// read effective area graphs for time binned intra run light curves
				TGraph2DErrors* g_time = ( TGraph2DErrors* )gDirectory->Get( "gTimeBinnedMeanEffectiveArea" );
				// find 2D graph with time dependent effective areas
				if( !g_time )
				{
					// if not try go get off graph
					g_time = ( TGraph2DErrors* )gDirectory->Get( "gTimeBinnedMeanEffectiveArea_off" );
					if( !g_time )
					{
						cout << "error: 2D effective area graph not found" << endl;
						cout << "continue..." << endl;
						fRunEffArea[i] = 0.;
						continue;
					}
				}
				
				// calculate spectral weighted integral effective area
				double* Xaxis = g_time->GetX();
				double* Yaxis = g_time->GetY();
				double* Zaxis = g_time->GetZ();
				double InterEffArea = 0.;
				
				double Uedge_time_bin = fTimeBinDuration[i];
				int time_bin_counter = 0;
				
				// loop over the different point of the 2D graph with Mean effective area per time bin
				//----- during the integration process mixing of effective area value corresponding to different time bin is avoided with the condition:  ieff_int > 0
				for( int pt_2Dgraph = 1 ; pt_2Dgraph < g_time->GetN() - 1; pt_2Dgraph++ )
				{
					//int pt_2Dgraph = 1 + (i_time)*nb_en_bin ;
					double Time = 	Zaxis[pt_2Dgraph];
					// Finding the time bin corresponding to the current Time effective area
					if( Time > Uedge_time_bin )
					{
						Uedge_time_bin += fTimeBinDuration[i];
						time_bin_counter++;
						InterEffArea = 0;
						// Some time bin might be empty of events, there is no effective area for this time bin, so this time bin must be skipped
						while( Time > Uedge_time_bin )
						{
							Uedge_time_bin += fTimeBinDuration[i];
							time_bin_counter++;
							InterEffArea = 0;
						}
					}
					x0 = Xaxis[pt_2Dgraph - 1];
					x2 = Xaxis[pt_2Dgraph + 1];
					x1 = Xaxis[pt_2Dgraph];
					ieff_mean = Yaxis[pt_2Dgraph];
					
					ieff_int = integrateEffectiveAreaInterval( x0, x1, x2, ieff_mean );
					
					if( ieff_int > 0. )
					{
						InterEffArea += ieff_int;
					}
					else
					{
						continue;
					}
					i_IntraEffArea[time_bin_counter] = InterEffArea;
				}
				
				fIntraRunEffArea.push_back( i_IntraEffArea );
			}
		}
	}
	
	//////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////
	
	// calculate mean effective area for all runs (weighted by observation time)
	// (expect run = -1 in last element of vector)
	//(why not weighting by number of event in run since the effective area for the run in average event wise?)
	double iTTot = 0.;
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( fRunList[i] < 0. )
		{
			continue;
		}
		fRunEffArea[fRunList.size() - 1] += fRunEffArea[i] * fRunTOn[i];
		iTTot += fRunTOn[i];
	}
	if( iTTot > 0. )
	{
		fRunEffArea[fRunList.size() - 1] /= iTTot;
	}
	else if( fRunList.size() > 0 )
	{
		fRunEffArea[fRunList.size() - 1] = 0.;
	}
	if( fDebug )
	{
		cout << "Total, t_obs " << iTTot / 60. << " [min], < A > ";
		if( fRunList.size() > 0 )
		{
			cout << fRunEffArea[fRunList.size() - 1] << " [m^2]" << endl;
		}
		else
		{
			cout << " (run list zero size)" << endl;
		}
		cout << endl;
	}
}

/*

   print results for each run (for debugging)

*/
void VFluxCalculation::printDebugSummary()
{
	cout << endl << endl;
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		cout << fixed << " " << setprecision( 0 ) << fRunList[i] << "\t";
		cout << setprecision( 2 ) << fRunMJD[i] << "\t";
		cout << setprecision( 1 ) <<  fRunZe[i] << "\t";
		cout << setprecision( 1 ) << showpos << fRunSigni[i] << "\t";
		if( fFluxCalculationUseRolke )
		{
			cout << scientific << setprecision( 2 ) << fRunFlux[i] << " +- " << noshowpos << fRunFluxE[i] << endl;
		}
		else
		{
			cout << scientific << setprecision( 2 ) << fRunFlux[i];
			cout << " + " << noshowpos << fRunFluxCI_up_1sigma[i];
			cout << " - " << noshowpos << fRunFluxCI_lo_1sigma[i] << endl;
		}
	}
	cout << endl << endl;
}

/*
 * print single line with results
 *
 */
void VFluxCalculation::printECSVLine()
{
	cout << fixed << setprecision( 2 ) << fMinEnergy;
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( fRunList[i] < 0 )
		{
			cout << "   " << fixed << setprecision( 1 ) <<  fMJD_min;
			cout << "   " << fixed << setprecision( 1 ) << fMJD_max;
			cout << fixed << setprecision( 4 ) << "   " << fRunTOn[i] / 3600.;
			cout << scientific << "   " << fRunFlux[i];
			cout << scientific << "   " << fRunFluxE[i];
			cout << fixed << "   " << fRunNon[i];
			cout << fixed << "   " << fRunNoff[i];
			cout << fixed << "   " << fRunNorm[i];
			cout << fixed << "   " << fRunSigni[i];
		}
	}
	cout << endl;
}

/*

   print results for each run

*/
void VFluxCalculation::printResults()
{
	cout << endl << endl;
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		// (GM) how can this happen?
		if( i >= fRunEffArea.size() )
		{
			cout << "FluxCalculation::printResults() error: i >= fRunEffArea.size(): " << fRunEffArea.size() << "\t" << fRunList.size() << endl;
			break;
		}
		
		printf( "RUN %d at MJD %.4f\n", ( int )fRunList[i], fRunMJD[i] );
		printf( "\t ze [deg]: %.1f, ", fRunZe[i] );
		printf( "wobble offset [deg]: %.1f, ", fRunWobbleOffset[i] );
		printf( "pedvars: %.2f, ", fRunPedvars[i] );
		printf( "t_obs [min]: %.1f", fRunTOn[i] / 60. );
		printf( ", dead time fraction: %.2f", fRunDeadTime[i] );
		cout << endl;
		printf( "\t NOn: %d, NOff: %d, Norm: %.2f, NDiff: %.1f +- %.1f", ( int )fRunNon[i], ( int )fRunNoff[i], fRunNorm[i], fRunNdiff[i], fRunNdiffE[i] );
		printf( ", Rate: %.2f +- %.2f gammas/min", fRunRate[i], fRunRateE[i] );
		cout << endl;
		printf( "\t Effective area %.1f m^2", fRunEffArea[i] );
		printf( "\t Significance: %.1f", fRunSigni[i] );
		cout << endl;
		if( fRunUFL[i] < 0. )
		{
			printf( "\t Differential flux normalisation at %.2f TeV [cm^-2 s^-1 TeV^-1]: %.2e +- %.2e ", fE0, fRunFluxConstant[i], fRunFluxConstantE[i] );
			cout << endl;
			printf( "\t Flux F(E > %.3f TeV) [cm^-2 s^-1]: %.3e +- %.3e ", fMinEnergy, fRunFlux[i], fRunFluxE[i] );
			cout << endl;
			printf( "\t Flux F(E > %.3f TeV) [C.U. (Whipple)]: %.4f +- %.4f ", fMinEnergy, getFluxInCrabUnits( fRunFlux[i], fMinEnergy ), getFluxInCrabUnits( fRunFluxE[i], fMinEnergy ) );
			cout << endl;
			printf( "\t Flux F(E > %.3f TeV) [cm^-2 s^-1] 1 sigma confidence interval: [%.3e,%.3e] ", fMinEnergy, fRunFluxCI_lo_1sigma[i], fRunFluxCI_up_1sigma[i] );
			cout << endl;
			printf( "\t Flux F(E > %.3f TeV) [cm^-2 s^-1] 3 sigma confidence interval: [%.3e,%.3e] ", fMinEnergy, fRunFluxCI_lo_3sigma[i], fRunFluxCI_up_3sigma[i] );
			cout << endl;
		}
		else
		{
			printf( "\t Upper limit on events: %.1f", fRunUFL[i] );
			cout << endl;
			printf( "\t Differential flux normalisation at %.2f TeV [cm^-2 s^-1 TeV^-1]: %.2e", fE0, fRunFluxConstant[i] );
			cout << endl;
			printf( "\t Upper Flux Limit F(E > %.3f TeV, %.2f ) [cm^-2 s^-1]: %.2e", fMinEnergy, fUpperLimit, fRunFlux[i] );
			cout << endl;
			printf( "\t Upper Flux Limit F(E > %.3f TeV, %.2f ) [C.U. (Whipple)]: %.4f", fMinEnergy, fUpperLimit, getFluxInCrabUnits( fRunFlux[i], fMinEnergy ) );
			cout << endl;
			VDifferentialFlux a;
			// convert integral flux limit to a differential flux
			double iDF = -1. * fRunFlux[i] * ( fAlpha + 1 ) / fMinEnergy;
			printf( "\t Upper Flux Limit F(E > %e Hz, %.2f ) [ergs/s/cm^2]: %e", a.convertEnergy_TeV_to_Hz( fMinEnergy ), fUpperLimit, a.convertPhotonFlux_to_Ergs( fMinEnergy, iDF, true ) );
			cout << endl;
		}
	}
}


/*

   print results for each run in JSON format

*/

void VFluxCalculation::printResultsJSON()
{
	cout << "[" << endl;
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		// (GM) how can this happen?
		if( i >= fRunEffArea.size() )
		{
			cout << "FluxCalculation::printResultsJSON() error: i >= fRunEffArea.size(): " << fRunEffArea.size() << "\t" << fRunList.size() << endl;
			break;
		}
		
		cout << "\t{" << endl;
		printf( "\t  \"RUN\":%d,", ( int )fRunList[i] );
		cout << endl;
		printf( "\t  \"MJD\":%0.4f,", fRunMJD[i] );
		cout << endl;
		printf( "\t  \"MJD Err\":%0.4f,", fRunTOn[i] / 60. / 60. / 24. );
		cout << endl;
		printf( "\t  \"Energy Threshold\":%0.3f,", fMinEnergy );
		cout << endl;
		// ( int )fRunNon[i], ( int )fRunNoff[i], fRunNorm[i] , fRunNdiff[i], fRunNdiffE[i]
		printf( "\t  \"NOn\":%d,", ( int )fRunNon[i] );
		cout << endl;
		printf( "\t  \"NOff\":%d,", ( int )fRunNoff[i] );
		cout << endl;
		printf( "\t  \"Alpha\":%0.3f,", fRunNorm[i] );
		cout << endl;
		printf( "\t  \"Excess\":%0.3f,", fRunNdiff[i] );
		cout << endl;
		printf( "\t  \"Excess Error\":%0.3f,", fRunNdiffE[i] );
		cout << endl;
		printf( "\t  \"Significance\":%0.3f,", fRunSigni[i] );
		cout << endl;
		printf( "\t  \"Rate\":%0.3f,", fRunRate[i] );
		cout << endl;
		printf( "\t  \"Rate Error\":%0.3f,", fRunRateE[i] );
		cout << endl;
		
		if( fRunUFL[i] < 0. )
		{
			printf(	"\t  \"Upper Limit\":0," );
			cout << endl;
			printf(	"\t  \"Upper Limit Level\":0," );
			cout << endl;
			printf( "\t  \"Flux [cm^-2 s^-1]\":%0.3e,", fRunFlux[i] );
			cout << endl;
			printf( "\t  \"Flux Error [cm^-2 s^-1]\":%0.3e,", fRunFluxE[i] );
			cout << endl;
			printf( "\t  \"Flux Crab [Crab]\":%0.3f,", getFluxInCrabUnits( fRunFlux[i], fMinEnergy ) );
			cout << endl;
			printf( "\t  \"Flux Error [Crab]\":%0.3f", getFluxInCrabUnits( fRunFluxE[i], fMinEnergy ) );
			cout << endl;
		}
		
		else
		{
			printf(	"\t  \"Upper Limit\":1," );
			cout << endl;
			printf(	"\t  \"Upper Limit Level\":%0.3f,", fUpperLimit );
			cout << endl;
			printf( "\t  \"Flux [cm^-2 s^-1]\":%0.3e,", fRunFlux[i] );
			cout << endl;
			printf( "\t  \"Flux Error [cm^-2 s^-1]\":%0.3e,", 0. );
			cout << endl;
			printf( "\t  \"Flux Crab [Crab]\":%d,", ( int )getFluxInCrabUnits( fRunFlux[i], fMinEnergy ) );
			cout << endl;
			printf( "\t  \"Flux Error [Crab]\":%d", 0 );
			cout << endl;
		}
		cout << "\t}" << endl;
		
	}
	cout << "]" << endl;
}

/*

   get flux in Crab Nebula units; using Whipple 1998 result

   (note: spectral index should be changed)

*/
double VFluxCalculation::getFluxInCrabUnits( double iF, double iE, double iGamma )
{
	// Whipple (Hillas 1998)
	double iCrab = 3.2e-7 / ( iGamma - 1. ) * TMath::Power( iE, -iGamma + 1. ) / 1.e4;
	
	return iF / iCrab;
}

/*

   get flux in ergs / s / cm^2

*/
double VFluxCalculation::getFluxInErgs( double iF, double iE )
{
	// TeV to eV
	iE *= 1.e12;
	
	iF *= iE;
	iF *= 1.602176487e-19 / 1.e-7;
	
	return iF;
}

/*

   calculate integral flux for each run and all runs together

   use spectral weighted effective areas from getIntegralEffectiveArea()

*/
void VFluxCalculation::calculateFluxes()
{
	vector < double > IntraFlux;
	vector < double > IntraFluxE;
	vector < double > IntraFluxConstant;
	vector < double > IntraFluxConstantE;
	vector < double > IntraFluxCI_lo_1sigma;
	vector < double > IntraFluxCI_up_1sigma;
	vector < double > IntraFluxCI_lo_3sigma;
	vector < double > IntraFluxCI_up_3sigma;
	
	if( fDebug )
	{
		cout << "calculating fluxes" << endl;
		cout << "------------------" << endl;
	}
	
	// calculate total dead time corrected observation time
	double iTot = 0.;
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( fRunList[i] > 0 )
		{
			iTot += fRunTOn[i] * ( 1. - fRunDeadTime[i] );
		}
	}
	if( fDebug )
	{
		cout << "total dead time corrected observation time [min]: " << iTot / 60. << endl;
	}
	
	double iEffNorm = 0.;
	//////////////////////////////////////////////////////////////
	// loop over all runs in run list
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
	
		IntraFlux.clear();
		IntraFluxE.clear();
		IntraFluxConstant.clear();
		IntraFluxConstantE.clear();
		IntraFluxCI_lo_1sigma.clear();
		IntraFluxCI_up_1sigma.clear();
		IntraFluxCI_lo_3sigma.clear();
		IntraFluxCI_up_3sigma.clear();
		
		// effective area * observation time / dead time
		if( fRunList[i] > 0 )
		{
			iEffNorm = fRunEffArea[i] * fRunTOn[i] * ( 1. - fRunDeadTime[i] );
		}
		else
		{
			iEffNorm = fRunEffArea[i] * iTot;
		}
		
		// calculate fluxes (numbers/norm)
		if( iEffNorm > 0. )
		{
			if( fRunUFL[i] < 0. )
			{
				fRunFluxConstant[i] = fRunNdiff[i] / iEffNorm / 1.e4;            // m^2 to cm^2
				fRunFluxConstantE[i] = fRunNdiffE[i] / iEffNorm / 1.e4;
			}
			else
			{
				fRunFluxConstant[i] = fRunUFL[i] / iEffNorm / 1.e4;
				fRunFluxConstantE[i] = 0.;
			}
			fRunFluxCI_lo_1sigma[i] = fRunCI_lo_1sigma[i] / iEffNorm / 1.e4;
			fRunFluxCI_up_1sigma[i] = fRunCI_up_1sigma[i] / iEffNorm / 1.e4;
			fRunFluxCI_lo_3sigma[i] = fRunCI_lo_3sigma[i] / iEffNorm / 1.e4;
			fRunFluxCI_up_3sigma[i] = fRunCI_up_3sigma[i] / iEffNorm / 1.e4;
		}
		else
		{
			fRunFluxConstant[i] = 0.;
			fRunFluxConstantE[i] = 0.;
			fRunFluxCI_lo_1sigma[i] = 0.;
			fRunFluxCI_up_1sigma[i] = 0.;
			fRunFluxCI_lo_3sigma[i] = 0.;
			fRunFluxCI_up_3sigma[i] = 0.;
		}
		///////////////////////////////////////////////////////////////
		// calculate integral flux
		if( fMaxEnergy == MAX_SAFE_MC_ENERGY )
		{
			if( fMinEnergy > 0. )
			{
				fRunFlux[i]             = -1. / ( fAlpha + 1. ) * fRunFluxConstant[i]
										  * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha );
				fRunFluxE[i]            = -1. / ( fAlpha + 1. ) * fRunFluxConstantE[i]
										  * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha );
				fRunFluxCI_lo_1sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_lo_1sigma[i]
										  * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha );
				fRunFluxCI_up_1sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_up_1sigma[i]
										  * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha );
				fRunFluxCI_lo_3sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_lo_3sigma[i]
										  * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha );
				fRunFluxCI_up_3sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_up_3sigma[i]
										  * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha );
			}
			else if( fRunFluxConstantE[i] > 0. )
			{
				fRunFlux[i] = -1. / ( fAlpha + 1. ) * fRunFluxConstant[i];
				fRunFluxE[i] = -1. / ( fAlpha + 1. ) * fRunFluxConstantE[i];
				fRunFluxCI_lo_1sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_lo_1sigma[i];
				fRunFluxCI_up_1sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_up_1sigma[i];
				fRunFluxCI_lo_3sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_lo_3sigma[i];
				fRunFluxCI_up_3sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_up_3sigma[i];
			}
			else
			{
				fRunFlux[i] = -99.;
				fRunFluxE[i] = -99.;
				fRunFluxCI_lo_1sigma[i] = -99.;
				fRunFluxCI_up_1sigma[i] = -99.;
				fRunFluxCI_lo_3sigma[i] = -99.;
				fRunFluxCI_up_3sigma[i] = -99.;
			}
		}
		else  // fMaxEnergy != MAX_SAFE_MC_ENERGY
		{
			if( fDebug )
			{
				cout << "Calculating Integral fluxes within the restricted range [" << fMinEnergy << " TeV, "  << fMaxEnergy << " TeV]  " << endl;
			}
			if( fMinEnergy > 0. )
			{
				fRunFlux[i]             = -1. / ( fAlpha + 1. ) * fRunFluxConstant[i]
										  * ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha );
				fRunFluxE[i]            = -1. / ( fAlpha + 1. ) * fRunFluxConstantE[i]
										  * ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha );
				fRunFluxCI_lo_1sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_lo_1sigma[i]
										  * ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha );
				fRunFluxCI_up_1sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_up_1sigma[i]
										  * ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha );
				fRunFluxCI_lo_3sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_lo_3sigma[i]
										  * ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha );
				fRunFluxCI_up_3sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_up_3sigma[i]
										  * ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha );
			}
			else if( fRunFluxConstantE[i] > 0. )
			{
				fRunFlux[i] = -1. / ( fAlpha + 1. ) * fRunFluxConstant[i];
				fRunFluxE[i] = -1. / ( fAlpha + 1. ) * fRunFluxConstantE[i];
				fRunFluxCI_lo_1sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_lo_1sigma[i];
				fRunFluxCI_up_1sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_up_1sigma[i];
				fRunFluxCI_lo_3sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_lo_3sigma[i];
				fRunFluxCI_up_3sigma[i] = -1. / ( fAlpha + 1. ) * fRunFluxCI_up_3sigma[i];
			}
			else
			{
				fRunFlux[i] = -99.;
				fRunFluxE[i] = -99.;
				fRunFluxCI_lo_1sigma[i] = -99.;
				fRunFluxCI_up_1sigma[i] = -99.;
				fRunFluxCI_lo_3sigma[i] = -99.;
				fRunFluxCI_up_3sigma[i] = -99.;
			}
			
		}
		
		
		// Time BINs
		if( i < fIntraRunEffArea.size() )
		{
		
			for( unsigned int t = 0; t < fIntraRunEffArea[i].size(); t++ )
			{
			
				if( fRunList[i] > 0 )
				{
					iEffNorm = fIntraRunEffArea[i][t] * fIntraRunTOn[i][t] ;
				}
				else
				{
					iEffNorm = fIntraRunEffArea[i][t] * fIntraRunTOn[i][t] ;
					//iEffNorm = fIntraRunEffArea[i][t] * iTot / ( double )( fIntraRunEffArea[i].size() );
				}
				
				
				// calculate fluxes (numbers/norm)
				if( iEffNorm > 0. )
				{
					if( fIntraRunUFL[i][t] < 0. )
					{
						IntraFluxConstant.push_back( fIntraRunNdiff[i][t] / iEffNorm / 1.e4 );            // m^2 to cm^2
						IntraFluxConstantE.push_back( fIntraRunNdiffE[i][t] / iEffNorm / 1.e4 );
					}
					else
					{
						IntraFluxConstant.push_back( fIntraRunUFL[i][t] / iEffNorm / 1.e4 );
						IntraFluxConstantE.push_back( 0. );
					}
					IntraFluxCI_lo_1sigma.push_back( fIntraRunCI_lo_1sigma[i][t] / iEffNorm / 1.e4 );
					IntraFluxCI_up_1sigma.push_back( fIntraRunCI_up_1sigma[i][t] / iEffNorm / 1.e4 );
					IntraFluxCI_lo_3sigma.push_back( fIntraRunCI_lo_3sigma[i][t] / iEffNorm / 1.e4 );
					IntraFluxCI_up_3sigma.push_back( fIntraRunCI_up_3sigma[i][t] / iEffNorm / 1.e4 );
				}
				else
				{
					IntraFluxConstant.push_back( 0. );
					IntraFluxConstantE.push_back( 0. );
					IntraFluxCI_lo_1sigma.push_back( 0. );
					IntraFluxCI_up_1sigma.push_back( 0. );
					IntraFluxCI_lo_3sigma.push_back( 0. );
					IntraFluxCI_up_3sigma.push_back( 0. );
				}
				///////////////////////////////////////////////////////////////
				// calculate integral flux
				if( fMaxEnergy == MAX_SAFE_MC_ENERGY )
				{
					if( fMinEnergy > 0. )
					{
						IntraFlux.push_back( -1. / ( fAlpha + 1. ) * IntraFluxConstant[t] * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxE.push_back( -1. / ( fAlpha + 1. ) * IntraFluxConstantE[t] * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxCI_lo_1sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_lo_1sigma[t] * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxCI_up_1sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_up_1sigma[t] * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxCI_lo_3sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_lo_3sigma[t] * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxCI_up_3sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_up_3sigma[t] * TMath::Power( fMinEnergy, fAlpha + 1. ) / TMath::Power( fE0, fAlpha ) );
					}
					else if( fIntraRunFluxConstantE[i][t] > 0. )
					{
						IntraFlux.push_back( -1. / ( fAlpha + 1. ) * IntraFluxConstant[t] );
						IntraFluxE.push_back( -1. / ( fAlpha + 1. ) * IntraFluxConstantE[t] );
						IntraFluxCI_lo_1sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_lo_1sigma[t] );
						IntraFluxCI_up_1sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_up_1sigma[t] );
						IntraFluxCI_lo_3sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_lo_3sigma[t] );
						IntraFluxCI_up_3sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_up_3sigma[t] );
					}
					else
					{
						IntraFlux.push_back( -99. );
						IntraFluxE.push_back( -99. );
						IntraFluxCI_lo_1sigma.push_back( -99. );
						IntraFluxCI_up_1sigma.push_back( -99. );
						IntraFluxCI_lo_3sigma.push_back( -99. );
						IntraFluxCI_up_3sigma.push_back( -99. );
					}
				}
				else  // fMaxEnergy != MAX_SAFE_MC_ENERGY)
				{
					if( fDebug )
					{
						cout << "Calculating Integral fluxes within the restricted range [" << fMinEnergy << " TeV, "  << fMaxEnergy << " TeV]  " << endl;
					}
					if( fMinEnergy > 0. )
					{
						IntraFlux.push_back( -1. / ( fAlpha + 1. ) * IntraFluxConstant[t] *
											 ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxE.push_back( -1. / ( fAlpha + 1. ) * IntraFluxConstantE[t] *
											  ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxCI_lo_1sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_lo_1sigma[t] *
														 ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxCI_up_1sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_up_1sigma[t] *
														 ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxCI_lo_3sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_lo_3sigma[t] *
														 ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha ) );
						IntraFluxCI_up_3sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_up_3sigma[t] *
														 ( TMath::Power( fMinEnergy, fAlpha + 1. ) - TMath::Power( fMaxEnergy, fAlpha + 1. ) ) / TMath::Power( fE0, fAlpha ) );
					}
					else if( fIntraRunFluxConstantE[i][t] > 0. )
					{
						IntraFlux.push_back( -1. / ( fAlpha + 1. ) * IntraFluxConstant[t] );
						IntraFluxE.push_back( -1. / ( fAlpha + 1. ) * IntraFluxConstantE[t] );
						IntraFluxCI_lo_1sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_lo_1sigma[t] );
						IntraFluxCI_up_1sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_up_1sigma[t] );
						IntraFluxCI_lo_3sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_lo_3sigma[t] );
						IntraFluxCI_up_3sigma.push_back( -1. / ( fAlpha + 1. ) * IntraFluxCI_up_3sigma[t] );
					}
					else
					{
						IntraFlux.push_back( -99. );
						IntraFluxE.push_back( -99. );
						IntraFluxCI_lo_1sigma.push_back( -99. );
						IntraFluxCI_up_1sigma.push_back( -99. );
						IntraFluxCI_lo_3sigma.push_back( -99. );
						IntraFluxCI_up_3sigma.push_back( -99. );
					}
					
				}
				
			}
			
			fIntraRunFlux.push_back( IntraFlux );
			fIntraRunFluxE.push_back( IntraFluxE );
			fIntraRunFluxConstant.push_back( IntraFluxConstant );
			fIntraRunFluxConstantE.push_back( IntraFluxConstantE );
			fIntraRunFluxCI_lo_1sigma.push_back( IntraFluxCI_lo_1sigma );
			fIntraRunFluxCI_up_1sigma.push_back( IntraFluxCI_up_1sigma );
			fIntraRunFluxCI_lo_3sigma.push_back( IntraFluxCI_lo_3sigma );
			fIntraRunFluxCI_up_3sigma.push_back( IntraFluxCI_up_3sigma );
		}
	}
}

void VFluxCalculation::setSpectralParameters( double iMinEnergy_TeV, double  E0, double alpha, double iMaxEnergy_TeV )
{
	fMinEnergy = iMinEnergy_TeV;
	if( iMaxEnergy_TeV <  MAX_SAFE_MC_ENERGY && iMaxEnergy_TeV > 0. )
	{
		fMaxEnergy = iMaxEnergy_TeV;
	}
	else
	{
		fMaxEnergy = MAX_SAFE_MC_ENERGY;
	}
	fE0 = E0;
	fAlpha = alpha;
}

/*

   setting signficance parameters

   (used for the decision of when to use fluxes and when upper flux limits)

*/
void VFluxCalculation::setSignificanceParameters( double iThresholdSignificance, double iMinEvents, double iUpperLimit, int iUpperlimitMethod, int iLiMaEqu )
{
	fThresholdSignificance = iThresholdSignificance;
	fMinEvents = iMinEvents;
	fUpperLimit = iUpperLimit;
	fUpperLimitMethod = iUpperlimitMethod;
	fLiMaEqu = iLiMaEqu;
}

/*

     get number of on and off events above a given energy threshold [TeV]

*/
void VFluxCalculation::getNumberOfEventsAboveEnergy( double iMinEnergy )
{
	if( fDebug )
	{
		cout << "VFluxCalculation::getNumberOfEventsAboveEnergy " << iMinEnergy << " [TeV]" << endl;
	}
	fMinEnergy = iMinEnergy;
	
	// loop over all files
	for( unsigned int f = 0; f < fFile.size(); f++ )
	{
		if( !fFile[f] || !fFile[f]->cd() )
		{
			cout << "error: no input file found" << endl;
			return;
		}
		double iTotOn = 0.;
		double iTotOff = 0.;
		double iTotOffNorm = 0.;
		double iTotOffNormN = 0.;
		
		TH1D* hon = 0;
		TH2D* hon2DtimeBinned = 0;
		TH1D* hoff = 0;
		TH2D* hoff2DtimeBinned = 0;
		TH1D* honDuration1DtimeBinned = 0;
		TH1D* hoffDuration1DtimeBinned = 0;
		//TH1D* honDeadTimeFraction1DtimeBinned = 0;
		
		
		vector < double > IntraNon;
		vector < double > IntraNoff;
		vector < double > IntraNdiff;
		vector < double > IntraNdiffE;
		
		vector < double > IntraTOn;
		vector < double > IntraDeadTime;
		
		
		char hname[200];
		///////////////////////////////////////////////////
		// loop over all runs in run list
		for( unsigned int i = 0; i < fRunList.size(); i++ )
		{
			IntraNon.clear();
			IntraTOn.clear();
			IntraDeadTime.clear();
			IntraNoff.clear();
			IntraNdiff.clear();
			IntraNdiffE.clear();
			// calculate number of events above given energy threshold for this run
			// also get the time and deadtime fraction for each time bin
			if( ( int )fRunList[i] > 0 )
			{
				sprintf( hname, "run_%d/stereo/energyHistograms", ( int )fRunList[i] );
				
				// OBS!!!
				if( !fFile[f]->Get( hname ) )
				{
					continue;
				}
				if( !fFile[f]->cd( hname ) )
				{
					cout << "error finding directory " << hname << endl;
					return;
				}
				
				hon = ( TH1D* )gDirectory->Get( "hLinerecCounts_on" );
				hoff = ( TH1D* )gDirectory->Get( "hLinerecCounts_off" );
				if( !hon || !hoff )
				{
					cout << "error finding counting histogram (energy): " << hon << "\t" << hoff << "\t" << ( int )fRunList[i] << endl;
					return;
				}
				if( fTimebinned )
				{
					hon2DtimeBinned = ( TH2D* )gDirectory->Get( "hLinerecCounts2DtimeBinned_on" );
					hoff2DtimeBinned = ( TH2D* )gDirectory->Get( "hLinerecCounts2DtimeBinned_off" );
					honDuration1DtimeBinned = ( TH1D* )gDirectory->Get( "hRealDuration1DtimeBinned_on" );
					hoffDuration1DtimeBinned = ( TH1D* )gDirectory->Get( "hRealDuration1DtimeBinned_off" );
					//honDeadTimeFraction1DtimeBinned
					if( !hon2DtimeBinned || !hoff2DtimeBinned || !honDuration1DtimeBinned || !hoffDuration1DtimeBinned )
					{
						cout << "error finding time binned 2D counting histogram (energy): ";
						cout << hon2DtimeBinned << "\t" << hoff2DtimeBinned << "\t" << honDuration1DtimeBinned << "\t" << hoffDuration1DtimeBinned << "\t" << ( int )fRunList[i] << endl;
						return;
					}
				}
				// get number of on events above energy threshold
				fRunNon[i] = 0.;
				for( int b = hon->GetNbinsX(); b > 0; b-- )
				{
					if( hon->GetXaxis()->GetBinLowEdge( b ) < fMinEnergy )
					{
						break;
					}
					if( fMaxEnergy < MAX_SAFE_MC_ENERGY && ( hon->GetXaxis()->GetBinLowEdge( b ) + hon->GetXaxis()->GetBinWidth( b ) ) > fMaxEnergy )
					{
						continue;
					}
					fRunNon[i] += hon->GetBinContent( b );
				}
				iTotOn += fRunNon[i];
				// get number of on events above energy threshold in Time BIN
				if( fTimebinned && hon2DtimeBinned )
				{
					for( int t = 0; t < hon2DtimeBinned->GetNbinsY(); t++ )
					{
						double Non = 0.;
						
						for( int b = hon2DtimeBinned->GetNbinsX(); b > 0; b-- )
						{
							if( hon2DtimeBinned->GetXaxis()->GetBinLowEdge( b ) < fMinEnergy )
							{
								break;
							}
							if( fMaxEnergy < MAX_SAFE_MC_ENERGY && ( hon2DtimeBinned->GetXaxis()->GetBinLowEdge( b ) + hon2DtimeBinned->GetXaxis()->GetBinWidth( b ) ) > fMaxEnergy )
							{
								continue;
							}
							Non += hon2DtimeBinned->GetBinContent( hon2DtimeBinned->GetBin( b, t + 1 ) );
							
						}
						IntraNon.push_back( Non );
					}
				}
				
				
				// get time duration for each time bin
				if( fTimebinned && honDuration1DtimeBinned )
				{
					for( int t = 0; t < honDuration1DtimeBinned->GetNbinsX(); t++ )
					{
						IntraTOn.push_back( honDuration1DtimeBinned->GetBinContent( t + 1 ) );
					}
					fTimeBinDuration.push_back( honDuration1DtimeBinned->GetBinWidth( 1 ) );
					
				}
				
				// get number of off events above energy threshold
				fRunNoff[i] = 0.;
				for( int b = hoff->GetNbinsX(); b > 0; b-- )
				{
					if( hoff->GetXaxis()->GetBinLowEdge( b ) < fMinEnergy )
					{
						break;
					}
					
					if( fMaxEnergy < MAX_SAFE_MC_ENERGY && ( hoff->GetXaxis()->GetBinLowEdge( b ) + hoff->GetXaxis()->GetBinWidth( b ) ) > fMaxEnergy )
					{
						continue;
					}
					fRunNoff[i] += hoff->GetBinContent( b );
				}
				iTotOff += fRunNoff[i];
				
				// get number of off events above energy threshold in Time BIN
				if( fTimebinned && hoff2DtimeBinned )
				{
					for( int t = 0; t < hoff2DtimeBinned->GetNbinsY(); t++ )
					{
						double Noff = 0.;
						for( int b = hoff2DtimeBinned->GetNbinsX(); b > 0; b-- )
						{
							if( hoff2DtimeBinned->GetXaxis()->GetBinLowEdge( b ) < fMinEnergy )
							{
								break;
							}
							if( fMaxEnergy < MAX_SAFE_MC_ENERGY && ( hoff2DtimeBinned->GetXaxis()->GetBinLowEdge( b ) + hoff2DtimeBinned->GetXaxis()->GetBinWidth( b ) ) > fMaxEnergy )
							{
								continue;
							}
							
							Noff += hoff2DtimeBinned->GetBinContent( hoff2DtimeBinned->GetBin( b, t + 1 ) );
						}
						IntraNoff.push_back( Noff );
						IntraNdiff.push_back( IntraNon[t] - IntraNoff[t] * fRunNorm[i] );
						IntraNdiffE.push_back( sqrt( IntraNon[t] + IntraNoff[t] * fRunNorm[i] * fRunNorm[i] ) );
					}
				}
				
				// mean off norm
				if( fRunNorm[i] > 0. )
				{
					iTotOffNorm += 1. / fRunNorm[i];
					iTotOffNormN++;
				}
			}
			////////////////////////
			// calculate number of events above given energy threshold for all runs
			else
			{
				fRunNon[i] = iTotOn;
				fRunNoff[i] = iTotOff;
				if( iTotOffNormN > 0. && iTotOffNorm > 0. )
				{
					iTotOffNorm /= iTotOffNormN;
					fRunNorm[i] = 1. / iTotOffNorm;
				}
			}
			////////////////////////
			// calculate N_diff
			
			// Rolke
			if( fFluxCalculationUseRolke )
			{
				TRolke i_Rolke;
				i_Rolke.SetCLSigmas( 1.e-4 );
				i_Rolke.SetPoissonBkgKnownEff( ( int )fRunNon[i], ( int )fRunNoff[i], 1. / fRunNorm[i], 1. );
				fRunNdiff[i] = 0.5 * ( i_Rolke.GetLowerLimit() + i_Rolke.GetUpperLimit() );
				// symmetric error bar is meaning less for low counts
				i_Rolke.SetCLSigmas( 1. );
				fRunNdiffE[i] =  0.5 * ( i_Rolke.GetLowerLimit() + i_Rolke.GetUpperLimit() );
			}
			// Poisson
			else
			{
				fRunNdiff[i] = fRunNon[i] - fRunNoff[i] * fRunNorm[i];
				// assuming poisson errors
				fRunNdiffE[i] = sqrt( fRunNon[i] + fRunNoff[i] * fRunNorm[i] * fRunNorm[i] );
			}
			if( fRunTOn[i] > 0. )
			{
				fRunRate[i] = fRunNdiff[i] / fRunTOn[i] * 60.;
			}
			else
			{
				fRunRate[i] = 0.;
			}
			if( fRunTOn[i] > 0. )
			{
				fRunRateE[i] = fRunNdiffE[i] / fRunTOn[i] * 60.;
			}
			else
			{
				fRunRateE[i] = 0.;
			}
			
			if( hon )
			{
				gDirectory->RecursiveRemove( hon );
			}
			if( hoff )
			{
				gDirectory->RecursiveRemove( hoff );
			}
			
			// Fill IntraRun Vectors
			fIntraRunNon.push_back( IntraNon );
			fIntraRunNoff.push_back( IntraNoff );
			fIntraRunNdiff.push_back( IntraNdiff );
			fIntraRunNdiffE.push_back( IntraNdiffE );
			fIntraRunTOn.push_back( IntraTOn );
			
			
			
		}
		fFile[f]->cd();
	}                                             // end loop over all files
	if( fDebug )
	{
		cout << "END: VFluxCalculation::getNumberOfEventsAboveEnergy " << iMinEnergy << endl;
	}
}

/*

   calculate significance and upper limits (in event numbers) for each run

*/
void VFluxCalculation::calculateSignificancesAndUpperLimits()
{
	vector < double > IntraUFL;
	vector < double > IntraSigni;
	vector < double > IntraCI_lo_1sigma;
	vector < double > IntraCI_up_1sigma;
	vector < double > IntraCI_lo_3sigma;
	vector < double > IntraCI_up_3sigma;
	
	if( fDebug )
	{
		cout << "calculating significances and upper flux limits" << endl;
		cout << "-----------------------------------------------" << endl;
		cout << "using Li & Ma equation " << fLiMaEqu << endl;
		cout << "using upper flux calculation after ";
		if( fUpperLimitMethod == 0 )
		{
			cout << "Helene";
		}
		else if( fUpperLimitMethod == 3 )
		{
			cout << "Feldman & Cousins";
		}
		else if( fUpperLimitMethod == 4 )
		{
			cout << "Rolke et al";
		}
		cout << ", " << fUpperLimit * 100. << "\% upper limit" << endl;
		cout << "threshold significance is " << fThresholdSignificance << " sigma or less than " << fMinEvents << " events" << endl;
		cout << endl;
	}
	
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		IntraUFL.clear();
		IntraSigni.clear();
		IntraCI_lo_1sigma.clear();
		IntraCI_up_1sigma.clear();
		IntraCI_lo_3sigma.clear();
		IntraCI_up_3sigma.clear();
		
		if( fRunNorm[i] > 0. )
		{
			// calculcate significance
			fRunSigni[i] = VStatistics::calcSignificance( fRunNon[i], fRunNoff[i], fRunNorm[i], fLiMaEqu );
			
			if( i < fIntraRunNon.size() && i < fIntraRunNoff.size() )
			{
				for( unsigned int t = 0; t < fIntraRunNon[i].size(); t++ )
				{
					IntraSigni.push_back( VStatistics::calcSignificance( fIntraRunNon[i][t], fIntraRunNoff[i][t], fRunNorm[i], fLiMaEqu ) );
				}
			}
			else
			{
				IntraSigni.push_back( -99. );
			}
			
			// calculate upper flux if necessary
			if( fRunSigni[i] < fThresholdSignificance || fRunNon[i] < fMinEvents )
			{
				if( fDebug )
				{
					cout << "VFluxCalculation::calculateSignificancesAndUpperLimits: ";
					cout << fRunNon[i] << " " << fRunNoff[i] << " " << fRunNorm[i];
					cout << " " << fUpperLimit << " " << fUpperLimitMethod << endl;
				}
				fRunUFL[i] = VStatistics::calcUpperLimit( fRunNon[i], fRunNoff[i], fRunNorm[i], fUpperLimit, fUpperLimitMethod );
				fRunCI_lo_1sigma[i] = -99.;
				fRunCI_up_1sigma[i] = -99.;
				fRunCI_lo_3sigma[i] = -99.;
				fRunCI_up_3sigma[i] = -99.;
			}
			else
			{
				fRunUFL[i] = -99.;
				// calculate confidence intervals for fluxes
				TRolke i_Rolke;
				i_Rolke.SetCLSigmas( 1. );
				i_Rolke.SetPoissonBkgKnownEff( ( int )fRunNon[i], ( int )fRunNoff[i], 1. / fRunNorm[i], 1. );
				fRunCI_lo_1sigma[i] = i_Rolke.GetLowerLimit();
				fRunCI_up_1sigma[i] = i_Rolke.GetUpperLimit();
				i_Rolke.SetCLSigmas( 3. );
				i_Rolke.SetPoissonBkgKnownEff( ( int )fRunNon[i], ( int )fRunNoff[i], 1. / fRunNorm[i], 1. );
				fRunCI_lo_3sigma[i] = i_Rolke.GetLowerLimit();
				fRunCI_up_3sigma[i] = i_Rolke.GetUpperLimit();
			}
			//Time BINs
			if( fTimebinned && i < fIntraRunNon.size() && i < fIntraRunNoff.size() )
			{
				for( unsigned int t = 0; t < fIntraRunNon[i].size(); t++ )
				{
					if( IntraSigni[t] < fThresholdSignificance || fIntraRunNon[i][t] < fMinEvents )
					{
						IntraUFL.push_back( VStatistics::calcUpperLimit( fIntraRunNon[i][t], fIntraRunNoff[i][t],
											fRunNorm[i], fUpperLimit, fUpperLimitMethod ) );
						IntraCI_lo_1sigma.push_back( -99. );
						IntraCI_up_1sigma.push_back( -99. );
						IntraCI_lo_3sigma.push_back( -99. );
						IntraCI_up_3sigma.push_back( -99. );
					}
					else
					{
						IntraUFL.push_back( -99. );
						// calculate confidence intervals for fluxes
						TRolke i_Rolke;
						i_Rolke.SetCLSigmas( 1. );
						i_Rolke.SetPoissonBkgKnownEff( ( int )fIntraRunNon[i][t], ( int )fIntraRunNoff[i][t], 1. / fRunNorm[i], 1. );
						IntraCI_lo_1sigma.push_back( i_Rolke.GetLowerLimit() );
						IntraCI_up_1sigma.push_back( i_Rolke.GetUpperLimit() );
						i_Rolke.SetCLSigmas( 3. );
						i_Rolke.SetPoissonBkgKnownEff( ( int )fIntraRunNon[i][t], ( int )fIntraRunNoff[i][t], 1. / fRunNorm[i], 1. );
						IntraCI_lo_3sigma.push_back( i_Rolke.GetLowerLimit() );
						IntraCI_up_3sigma.push_back( i_Rolke.GetUpperLimit() );
					}
				}
			}
		}
		else
		{
			fRunSigni[i] = 99.;
			fRunUFL[i] = -99.;
			fRunCI_lo_1sigma[i] = -99.;
			fRunCI_up_1sigma[i] = -99.;
			fRunCI_lo_3sigma[i] = -99.;
			fRunCI_up_3sigma[i] = -99.;
			if( i < fIntraRunNon.size() )
			{
				for( unsigned int t = 0; t < fIntraRunNon[i].size(); t++ )
				{
					IntraSigni.push_back( 99. );
					IntraUFL.push_back( -99. );
					IntraCI_lo_1sigma.push_back( -99. );
					IntraCI_up_1sigma.push_back( -99. );
					IntraCI_lo_3sigma.push_back( -99. );
					IntraCI_up_3sigma.push_back( -99. );
				}
			}
		}
		
		fIntraRunSigni.push_back( IntraSigni );
		fIntraRunUFL.push_back(	IntraUFL );
		fIntraRunCI_lo_1sigma.push_back( IntraCI_lo_1sigma );
		fIntraRunCI_up_1sigma.push_back( IntraCI_up_1sigma );
		fIntraRunCI_lo_3sigma.push_back( IntraCI_lo_3sigma );
		fIntraRunCI_up_3sigma.push_back( IntraCI_up_3sigma );
		
		
	}
}

void VFluxCalculation::getFluxConfidenceInterval( int irun, double& iFlux_low, double& iFlux_up, bool b1Sigma )
{
	iFlux_low = -99.;
	iFlux_up = -99.;
	
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			// return flux values
			if( b1Sigma )
			{
				iFlux_low = fRunFluxCI_lo_1sigma[i];
				iFlux_up = fRunFluxCI_up_1sigma[i];
			}
			else
			{
				iFlux_low = fRunFluxCI_lo_3sigma[i];
				iFlux_up = fRunFluxCI_up_3sigma[i];
			}
			break;
		}
	}
}

/*
 * 'traditional' method allowing for negative flux
 *
 */
void VFluxCalculation::getFlux( int irun, double& iFlux, double& iFluxE, double& iFluxUL )
{
	iFlux = -99.;
	iFluxE = -99.;
	iFluxUL = -99.;
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			// return flux values
			if( fRunUFL[i] < 0. )
			{
				iFlux = fRunFlux[i];
				iFluxE = fRunFluxE[i];
				iFluxUL = -99.;
			}
			else
			{
				iFlux = -99.;
				iFluxE = -99.;
				iFluxUL = fRunFlux[i];
			}
			break;
		}
	}
}

/*
 * bounded flux
 *
 */
void VFluxCalculation::getBoundedFlux( int irun, double& iFlux, double& iFluxE_low, double& iFluxE_up, double& iFluxUL )
{
	iFlux = -99.;
	iFluxE_low = -99.;
	iFluxE_up = -99.;
	iFluxUL = -99.;
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			if( fRunUFL[i] < 0. )
			{
				iFlux = fRunFlux[i];
				iFluxE_low = TMath::Abs( fRunFlux[i] - fRunFluxCI_lo_1sigma[i] );
				iFluxE_up = TMath::Abs( fRunFlux[i] - fRunFluxCI_up_1sigma[i] );
			}
			else
			{
				iFlux = -99.;
				iFluxE_low = -99.;
				iFluxE_up = -99.;
				iFluxUL = fRunFluxCI_up_3sigma[i];
			}
			break;
		}
	}
}


/*

   calculate integral flux for each run and all runs

   use spectral weighted effective areas from getIntegralEffectiveArea()

*/
void VFluxCalculation::calculateIntegralFlux( double iMinEnergy )
{
	fMinEnergy = iMinEnergy;
	
	// get number of events above threshold energy
	if( fMinEnergy > 0. )
	{
		getNumberOfEventsAboveEnergy( fMinEnergy );
	}
	
	// calculate significances and upper limits
	calculateSignificancesAndUpperLimits();
	
	// calculate integrated spectral weighted effective areas
	getIntegralEffectiveArea();
	
	// calculate fluxes and upper flux limits
	calculateFluxes();
}


double VFluxCalculation::getRunTime( int irun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			return fRunTOn[i];
		}
	}
	return -99.;
}

double VFluxCalculation::getRunElevation( int irun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			return 90. - fRunZe[i];
		}
	}
	return -99.;
}


double VFluxCalculation::getSignificance( int irun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			return fRunSigni[i];
		}
	}
	return -99.;
}


double VFluxCalculation::getAlpha( int irun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			return fRunNorm[i];
		}
	}
	return -99.;
}


double VFluxCalculation::getNOff( int irun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			return fRunNoff[i];
		}
	}
	return -99.;
}


double VFluxCalculation::getNOn( int irun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			return fRunNon[i];
		}
	}
	return -99.;
}


double VFluxCalculation::getRate( int irun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			return fRunRate[i];
		}
	}
	return -99.;
}


double VFluxCalculation::getMJD( int irun )
{
	if( irun > 0 )
	{
		for( unsigned int i = 0; i < fRunList.size(); i++ )
		{
			if( irun == fRunList[i] )
			{
				return fRunMJD[i];
			}
		}
	}
	else
	{
		if( fRunMJD.size() > 0 )
		{
			return 0.5 * ( fRunMJD[0] + fRunMJD[fRunMJD.size() - 2] );
		}
		else
		{
			return 0.;
		}
	}
	return 0.;
}


double VFluxCalculation::getRateError( int irun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( irun == fRunList[i] )
		{
			return fRunRateE[i];
		}
	}
	return -99.;
}

/*

   write results (fluxes and upper flux limits per run) into a root file

*/
void VFluxCalculation::writeResults( char* ofile )
{
	if( !ofile )
	{
		return;
	}
	
	TFile fOFILE( ofile, "RECREATE" );
	if( fOFILE.IsZombie() )
	{
		return;
	}
	
	if( fDebug )
	{
		cout << "writing results to " << fOFILE.GetName() << endl;
	}
	
	int iRun;
	double iMJD;
	double iFlux;
	double iFluxE;
	double iSigni;
	double iZe;
	
	TTree t( "fluxes", "flux calculation results" );
	t.Branch( "Run", &iRun, "Run/I" );
	t.Branch( "MJD", &iMJD, "MJD/D" );
	t.Branch( "Flux", &iFlux, "Flux/D" );
	t.Branch( "FluxE", &iFluxE, "FluxE/D" );
	t.Branch( "Signi", &iSigni, "Signi/D" );
	t.Branch( "Ze", &iZe, "Ze/D" );
	
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		iRun = TMath::Nint( fRunList[i] );
		iMJD = fRunMJD[i];
		iFlux = fRunFlux[i];
		iFluxE = fRunFluxE[i];
		iSigni = fRunSigni[i];
		iZe = fRunZe[i];
		t.Fill();
	}
	t.Write();
	
	fOFILE.Close();
}


TCanvas* VFluxCalculation::plotFluxesVSPedvars()
{
	TCanvas* cFPedvars = new TCanvas( "cFPedvars", "fluxes vs pedvars", 400, 10, 1100, 700 );
	cFPedvars->SetGridx( 0 );
	cFPedvars->SetGridy( 0 );
	cFPedvars->Draw();
	
	gFluxPedvars = new TGraphErrors( ( int )fRunMJD.size() - 1 );
	gFluxPedvars->SetTitle( "" );
	gFluxPedvars->SetMarkerStyle( 20 );
	gFluxPedvars->SetMarkerSize( 1. );
	gFluxPedvars->SetLineWidth( 2 );
	
	int z = 0;
	for( unsigned int i = 0; i < fRunMJD.size(); i++ )
	{
		if( fRunMJD[i] > 10 )
		{
			gFluxPedvars->SetPoint( z, fRunPedvars[i], fRunFlux[i] );
			gFluxPedvars->SetPointError( z, 0., fRunFluxE[i] );
			z++;
		}
	}
	char hname[200];
	gFluxPedvars->Draw( "ap" );
	gFluxPedvars->GetHistogram()->SetXTitle( "mean pedvars" );
	if( fMinEnergy < 1. )
	{
		sprintf( hname, "F(E>%d GeV) [cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ) );
	}
	else
	{
		sprintf( hname, "F(E>%.1f TeV) [cm^{-2}s^{-1}]", fMinEnergy );
	}
	gFluxPedvars->GetHistogram()->SetYTitle( hname );
	
	return cFPedvars;
}


TCanvas* VFluxCalculation::plotFluxesVSWobbleOffset()
{
	TCanvas* cFWobbleOffset = new TCanvas( "cFWobbleOffset", "fluxes vs wobble offset", 400, 10, 1100, 700 );
	cFWobbleOffset->SetGridx( 0 );
	cFWobbleOffset->SetGridy( 0 );
	cFWobbleOffset->Draw();
	
	gFluxWobbleOffset = new TGraphErrors( ( int )fRunMJD.size() - 1 );
	gFluxWobbleOffset->SetTitle( "" );
	gFluxWobbleOffset->SetMarkerStyle( 20 );
	gFluxWobbleOffset->SetMarkerSize( 1 );
	gFluxWobbleOffset->SetLineWidth( 2 );
	
	int z = 0;
	for( unsigned int i = 0; i < fRunMJD.size(); i++ )
	{
		if( fRunMJD[i] > 10 )
		{
			gFluxWobbleOffset->SetPoint( z, fRunWobbleOffset[i], fRunFlux[i] );
			gFluxWobbleOffset->SetPointError( z, 0., fRunFluxE[i] );
			z++;
		}
	}
	char hname[200];
	gFluxWobbleOffset->Draw( "ap" );
	gFluxWobbleOffset->GetHistogram()->SetXTitle( "wobble offset [deg]" );
	if( fMinEnergy < 1. )
	{
		sprintf( hname, "F(E>%d GeV) [cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ) );
	}
	else
	{
		sprintf( hname, "F(E>%.1f TeV) [cm^{-2}s^{-1}]", fMinEnergy );
	}
	gFluxWobbleOffset->GetHistogram()->SetYTitle( hname );
	
	return cFWobbleOffset;
}


TCanvas* VFluxCalculation::plotFluxesVSElevation( bool iDraw,
		double iConstantValueLine )
{
	TCanvas* cCanvas_FElevation = 0;
	if( iDraw )
	{
		cCanvas_FElevation = new TCanvas( "cCanvas_FElevation", "fluxes vs elevation", 40, 10, 1100, 700 );
		cCanvas_FElevation->SetGridx( 0 );
		cCanvas_FElevation->SetGridy( 0 );
		cCanvas_FElevation->Draw();
	}
	
	gFluxElevation = new TGraphErrors( ( int )fRunMJD.size() - 1 );
	gFluxElevation->SetTitle( "" );
	gFluxElevation->SetMarkerStyle( 20 );
	gFluxElevation->SetMarkerSize( 1. );
	gFluxElevation->SetLineWidth( 2 );
	
	int z = 0;
	for( unsigned int i = 0; i < fRunMJD.size(); i++ )
	{
		if( fRunMJD[i] > 10 )
		{
			gFluxElevation->SetPoint( z, 90. - fRunZe[i], fRunFlux[i] );
			gFluxElevation->SetPointError( z, 0., fRunFluxE[i] );
			z++;
		}
	}
	char hname[200];
	if( iDraw )
	{
		gFluxElevation->Draw( "ap" );
		gFluxElevation->GetHistogram()->SetXTitle( "elevation [deg]" );
		if( fMinEnergy < 1. )
		{
			sprintf( hname, "F(E>%d GeV) [cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ) );
		}
		else
		{
			sprintf( hname, "F(E>%.1f TeV) [cm^{-2}s^{-1}]", fMinEnergy );
		}
		gFluxElevation->GetHistogram()->SetYTitle( hname );
		
		if( iConstantValueLine > 0. )
		{
			TLine* iL3 = new TLine( gFluxElevation->GetHistogram()->GetXaxis()->GetXmin(), iConstantValueLine,
									gFluxElevation->GetHistogram()->GetXaxis()->GetXmax(), iConstantValueLine );
			iL3->SetLineStyle( 1 );
			iL3->Draw();
		}
	}
	
	return cCanvas_FElevation;
}

/*
 * plot flux vs azimuth
 *
 */
TCanvas* VFluxCalculation::plotFluxesVSAzimuth( bool iDraw,
		double iConstantValueLine )
{
	TCanvas* cCanvas_FAzimuth = 0;
	if( iDraw )
	{
		cCanvas_FAzimuth = new TCanvas( "cCanvas_FAzimuth", "fluxes vs Azimuth", 40, 10, 1100, 700 );
		cCanvas_FAzimuth->SetGridx( 0 );
		cCanvas_FAzimuth->SetGridy( 0 );
		cCanvas_FAzimuth->Draw();
	}
	
	gFluxAzimuth = new TGraphErrors( ( int )fRunMJD.size() - 1 );
	gFluxAzimuth->SetTitle( "" );
	gFluxAzimuth->SetMarkerStyle( 20 );
	gFluxAzimuth->SetMarkerSize( 1. );
	gFluxAzimuth->SetLineWidth( 2 );
	
	int z = 0;
	for( unsigned int i = 0; i < fRunMJD.size(); i++ )
	{
		if( fRunMJD[i] > 10 )
		{
			gFluxAzimuth->SetPoint( z, fRunAz[i], fRunFlux[i] );
			gFluxAzimuth->SetPointError( z, 0., fRunFluxE[i] );
			z++;
		}
	}
	char hname[200];
	if( iDraw )
	{
		gFluxAzimuth->Draw( "ap" );
		gFluxAzimuth->GetHistogram()->SetXTitle( "azimuth [deg]" );
		if( fMinEnergy < 1. )
		{
			sprintf( hname, "F(E>%d GeV) [cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ) );
		}
		else
		{
			sprintf( hname, "F(E>%.1f TeV) [cm^{-2}s^{-1}]", fMinEnergy );
		}
		gFluxAzimuth->GetHistogram()->SetYTitle( hname );
		
		if( iConstantValueLine > 0. )
		{
			TLine* iL3 = new TLine( gFluxAzimuth->GetHistogram()->GetXaxis()->GetXmin(), iConstantValueLine,
									gFluxAzimuth->GetHistogram()->GetXaxis()->GetXmax(), iConstantValueLine );
			iL3->SetLineStyle( 1 );
			iL3->Draw();
		}
	}
	
	return cCanvas_FAzimuth;
}




TGraphErrors* VFluxCalculation::plotFluxesVSMJD( char* iTex, double iMJDOffset, TCanvas* cFMJD, int iMarkerColor, int iMarkerStyle, bool bDrawAxis, double iMinMJD, double iMaxMJD )
{
	char hname[500];
	
	string sFluxMult = "10^{-7}";
	double fluxMult = 1.e7;
	
	if( !cFMJD )
	{
		fCanvasFluxesVSMJD = new TCanvas( "fCanvasFluxesVSMJD", "fluxes vs MJD", 10, 10, 1100, 700 );
		fCanvasFluxesVSMJD->SetGridx( 0 );
		fCanvasFluxesVSMJD->SetGridy( 0 );
		fCanvasFluxesVSMJD->Draw();
		bDrawAxis = true;
		cFMJD = fCanvasFluxesVSMJD;
	}
	else
	{
		fCanvasFluxesVSMJD = cFMJD;
	}
	fCanvasFluxesVSMJD->Clear();
	fCanvasFluxesVSMJD->cd();
	
	TGraphErrors* gFluxMJD = new TGraphErrors( 1 );
	gFluxMJD->SetTitle( "" );
	gFluxMJD->SetMarkerStyle( iMarkerStyle );
	gFluxMJD->SetMarkerColor( iMarkerColor );
	gFluxMJD->SetLineColor( iMarkerColor );
	gFluxMJD->SetMarkerSize( 1. );
	gFluxMJD->SetLineWidth( 2 );
	
	vector< int > iV_Run;
	vector< double > iV_Flux;
	vector< double > iV_FluxE;
	
	double iMinFlux = 1.e90;
	
	double mean_flux = 0;
	double total_time = 0;
	
	int z = 0;
	for( unsigned int i = 0; i < fRunMJD.size(); i++ )
	{
		if( fRunMJD[i] > 10 )
		{
			if( iMinMJD > 0 && fRunMJD[i] < iMinMJD )
			{
				continue;
			}
			if( iMaxMJD > 0 && fRunMJD[i] > iMaxMJD )
			{
				continue;
			}
			
			gFluxMJD->SetPoint( z, fRunMJD[i] - iMJDOffset, fRunFlux[i] );
			gFluxMJD->SetPointError( z, fRunTOn[i] / 86400. / 2., fRunFluxE[i] );
			z++;
			mean_flux += fRunFlux[i] * ( fRunTOn[i] * ( 1. - fRunDeadTime[i] ) );
			total_time += ( fRunTOn[i] * ( 1. - fRunDeadTime[i] ) );
			
			iV_Run.push_back( ( int )fRunList[i] );
			iV_Flux.push_back( fRunFlux[i] );
			iV_FluxE.push_back( fRunFluxE[i] );
			if( fRunFlux[i] - fRunFluxE[i] < iMinFlux )
			{
				iMinFlux = fRunFlux[i] - fRunFluxE[i];
			}
		}
	}
	
	if( fRunNorm.size() > 0 )
	{
		printf( "mean_flux from run by run %.3e \n Total time = %.3e \n total time including dead time %.3e \n", mean_flux / total_time, total_time, fRunTOn[fRunNorm.size() - 1] );
	}
	
	if( bDrawAxis )
	{
		gFluxMJD->Draw( "ap" );
		if( iMinFlux > 0. )
		{
			gFluxMJD->GetHistogram()->SetMinimum( 0. );
		}
	}
	else
	{
		gFluxMJD->Draw( "p" );
	}
	if( iMJDOffset > 0 )
	{
		sprintf( hname, "MJD - %d", ( int )iMJDOffset );
	}
	else
	{
		sprintf( hname, "MJD" );
	}
	gFluxMJD->GetHistogram()->SetXTitle( hname );
	if( fMinEnergy < 1. )
	{
		sprintf( hname, "F(E>%d GeV) [cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ) );
	}
	else
	{
		sprintf( hname, "F(E>%.1f TeV) [cm^{-2}s^{-1}]", fMinEnergy );
	}
	gFluxMJD->GetHistogram()->SetYTitle( hname );
	
	if( iMinFlux < 0. )
	{
		TLine* iL2 = new TLine( gFluxMJD->GetHistogram()->GetXaxis()->GetXmin(), 0.,
								gFluxMJD->GetHistogram()->GetXaxis()->GetXmax(), 0. );
		iL2->SetLineStyle( 2 );
		iL2->Draw();
	}
	
	if( iTex )
	{
		sprintf( hname, "   &[$%s$ cm$^{-2}$s$^{-1}$]", sFluxMult.c_str() );
		writeTexFileForFluxValues( iTex, iV_Run, iV_Flux, iV_FluxE, fluxMult, "run & flux",  "   &[$10^9$ cm$^{-2}$s$^{-1}$]" );
	}
	
	return gFluxMJD;
}

// Plot Time Binned Fluxes

TGraphErrors* VFluxCalculation::plotFluxesInBINs( int run, char* iTex, double iMJDOffset, TCanvas* cFMJD, int iMarkerColor, int iMarkerStyle, bool bDrawAxis )
{
	char hname[500];
	
	string sFluxMult = "10^{-7}";
	double fluxMult = 1.e7;
	
	if( !cFMJD )
	{
		fCanvasFluxesInBINs = new TCanvas( "fCanvasFluxesInBINs", "fluxes vs MJD", 10, 10, 600, 400 );
		fCanvasFluxesInBINs->SetGridx( 0 );
		fCanvasFluxesInBINs->SetGridy( 0 );
		fCanvasFluxesInBINs->Draw();
		bDrawAxis = true;
		cFMJD = fCanvasFluxesInBINs;
	}
	else
	{
		fCanvasFluxesInBINs = cFMJD;
	}
	fCanvasFluxesInBINs->cd();
	
	TGraphErrors* gFluxInBINs = new TGraphErrors();
	gFluxInBINs->SetTitle( "" );
	gFluxInBINs->SetMarkerStyle( iMarkerStyle );
	gFluxInBINs->SetMarkerColor( iMarkerColor );
	gFluxInBINs->SetLineColor( iMarkerColor );
	gFluxInBINs->SetMarkerSize( 1. );
	gFluxInBINs->SetLineWidth( 2 );
	
	vector< int > iV_Run;
	vector< double > iV_Flux;
	vector< double > iV_FluxE;
	
	int z = 0;
	
	unsigned int i_run_min = 0;
	unsigned int i_run_max = fRunMJD.size() - 1;
	
	if( run < 0 )
	{
		cout << "plotFluxesInBINs( int run = -1, char *iTex = 0, double iMJDOffset = 0., TCanvas *c = 0, int iMarkerColor = 1, int iMarkerStyle = 8, bool bDrawAxis = false )" << endl;
		cout << "WARNING: run variable < 0 - all runs are plotted " << endl;
		cout << "run = 0 ... Nruns" << endl;
	}
	else if( run > 0 )
	{
		// get run number
		unsigned iRunIndex = getIndexOfRun( run );
		if( iRunIndex >= fIntraRunFlux.size() )
		{
			cout << "run not found " << run << endl;
			return 0;
		}
		i_run_min = iRunIndex;
		i_run_max = i_run_min + 1;
	}
	
	double mean_flux = 0;
	double total_time = 0;
	
	for( unsigned int i = i_run_min; i < i_run_max; i++ )
	{
	
		//double full_run_duration = fTimeBinDuration[i]*fIntraRunFlux[i].size();
		// fRunMJD is in the middle of the filled region of the bin.
		// Need to know the duration of this region to access the MJD start of the run. Start at first time bin filled
		//--- duration between  first and last bin filled.
		//-------------------------------- time bin filled |x| ; time bin cut | |
		// | | | |x|x|x|x|
		//           ^
		//           |
		//         RunMJD
		// |x|x| | | |x|x|
		//        ^
		//        |
		//      RunMJD
		//-----------------------------------------------------
		
		bool first_time = true;
		int first_filled_bin = 0;
		int last_filled_bin = 0;
		for( unsigned int i_t = 0 ; i_t < fIntraRunTOn[i].size() ; i_t++ )
		{
		
			if( first_time && fIntraRunTOn[i][i_t] > 0 )
			{
				first_filled_bin = i_t;
				first_time = false;
			}
			if( fIntraRunTOn[i][i_t] > 0 )
			{
				last_filled_bin = i_t;
			}
		}
		
		int nb_bin = 1 + last_filled_bin - first_filled_bin;
		double full_run_duration = nb_bin * fTimeBinDuration[i] ; // duration between the first and the last bin filled
		double MJD_start = fRunMJD[i] - ( full_run_duration / 2. ) / 86400.;
		
		if( i < fIntraRunFluxE.size() )
		{
			unsigned int t_c = 0 ; // counting the time bin from first filled
			for( unsigned int t = first_filled_bin; t < fIntraRunFlux[i].size(); t++ )
			{
				if( fRunMJD[i] > 10 )
				{
				
					if( fIntraRunTOn[i][t] > 0 )
					{
						double temps = MJD_start  - iMJDOffset + ( t_c + 0.5 ) * fTimeBinDuration[i] / 86400.; // t_c just here, because we are counting from the first bin filled.
						t_c++;
						gFluxInBINs->SetPoint( z, temps, fIntraRunFlux[i][t] );
						gFluxInBINs->SetPointError( z,  fTimeBinDuration[i] / 86400. / 2., fIntraRunFluxE[i][t] );
						mean_flux += fIntraRunFlux[i][t] * fIntraRunTOn[i][t];
						z++;
						iV_Run.push_back( ( int )fRunList[i] );
						iV_Flux.push_back( fIntraRunFlux[i][t] );
						iV_FluxE.push_back( fIntraRunFluxE[i][t] );
					}
					total_time += fIntraRunTOn[i][t];
				}
			}
		}
	}
	
	printf( "mean_flux from time binned %.3e \n  Total time = %.3e \n", mean_flux / total_time, total_time );
	
	if( bDrawAxis )
	{
		gFluxInBINs->Draw( "ap" );
	}
	else
	{
		gFluxInBINs->Draw( "p" );
	}
	
	if( iMJDOffset > 0 )
	{
		sprintf( hname, "MJD - %d", ( int )iMJDOffset );
	}
	else
	{
		sprintf( hname, "MJD" );
	}
	
	gFluxInBINs->GetHistogram()->SetXTitle( hname );
	
	if( fMinEnergy < 1. )
	{
		if( fMaxEnergy < 1. )
		{
			sprintf( hname, "F(%d GeV < E < %d GeV) [cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ), ( int )( fMaxEnergy * 1.e3 ) );
		}
		else if( fMaxEnergy < MAX_SAFE_MC_ENERGY )
		{
			sprintf( hname, "F(%d GeV < E < %0.1f TeV) [cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ), fMaxEnergy );
		}
		else
		{
			sprintf( hname, "F(E>%d GeV) [cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ) );
		}
	}
	else
	{
		if( fMaxEnergy < MAX_SAFE_MC_ENERGY )
		{
			sprintf( hname, "F(%.1f TeV < E < %0.1f TeV) [cm^{-2}s^{-1}]", fMinEnergy, fMaxEnergy );
		}
		else
		{
			sprintf( hname, "F(E>%.1f TeV) [cm^{-2}s^{-1}]", fMinEnergy );
		}
	}
	
	gFluxInBINs->GetHistogram()->SetYTitle( hname );
	
	TLine* iL2 = new TLine( gFluxInBINs->GetHistogram()->GetXaxis()->GetXmin(), 0., gFluxInBINs->GetHistogram()->GetXaxis()->GetXmax(), 0. );
	iL2->SetLineStyle( 2 );
	iL2->Draw();
	
	if( iTex )
	{
		sprintf( hname, "   &[$%s$ cm$^{-2}$s$^{-1}$]", sFluxMult.c_str() );
		writeTexFileForFluxValues( iTex, iV_Run, iV_Flux, iV_FluxE, fluxMult, "run & flux",  "   &[$10^9$ cm$^{-2}$s$^{-1}$]" );
	}
	
	return gFluxInBINs;
}



/*!
    see how many '-1' runs are there available, combine them
*/
void VFluxCalculation::cleanRunList()
{
	if( fFile.size() < 2 )
	{
		return;
	}
	
	vector< unsigned int > iRemovePos;
	
	for( unsigned int i = 0; i < fRunList.size() - 1; i++ )
	{
		if( fRunList[i] < 0 )
		{
			iRemovePos.push_back( i );
			fRunTOn[fRunTOn.size() - 1] += fRunTOn[i];
			fRunNon[fRunNon.size() - 1] += fRunNon[i];
			fRunNoff[fRunNoff.size() - 1] += fRunNoff[i];
		}
	}
	
	int z = 0;
	for( unsigned int i = 0; i < iRemovePos.size(); i++ )
	{
		fRunList.erase( fRunList.begin() + iRemovePos[i] - z );
		fRunMJD.erase( fRunMJD.begin() + iRemovePos[i] - z );
		fRunTOn.erase( fRunTOn.begin() + iRemovePos[i] - z );
		fRunDeadTime.erase( fRunDeadTime.begin() + iRemovePos[i] - z );
		fRunZe.erase( fRunZe.begin() + iRemovePos[i] - z );
		fRunWobbleOffset.erase( fRunWobbleOffset.begin() + iRemovePos[i] - z );
		fRunPedvars.erase( fRunPedvars.begin() + iRemovePos[i] - z );
		fRunNdiff.erase( fRunNdiff.begin() + iRemovePos[i] - z );
		fRunNdiffE.erase( fRunNdiffE.begin() + iRemovePos[i] - z );
		fRunRate.erase( fRunRate.begin() + iRemovePos[i] - z );
		fRunRateE.erase( fRunRateE.begin() + iRemovePos[i] - z );
		fRunNon.erase( fRunNon.begin() + iRemovePos[i] - z );
		fRunNoff.erase( fRunNoff.begin() + iRemovePos[i] - z );
		fRunNorm.erase( fRunNorm.begin() + iRemovePos[i] - z );
		fRunSigni.erase( fRunSigni.begin() + iRemovePos[i] - z );
		fRunUFL.erase( fRunUFL.begin() + iRemovePos[i] - z );
		fRunCI_lo_1sigma.erase( fRunCI_lo_1sigma.begin() + iRemovePos[i] - z );
		fRunCI_up_1sigma.erase( fRunCI_up_1sigma.begin() + iRemovePos[i] - z );
		fRunCI_lo_3sigma.erase( fRunCI_lo_3sigma.begin() + iRemovePos[i] - z );
		fRunCI_up_3sigma.erase( fRunCI_up_3sigma.begin() + iRemovePos[i] - z );
		fRunFlux.erase( fRunFlux.begin() + iRemovePos[i] - z );
		fRunFluxE.erase( fRunFluxE.begin() + iRemovePos[i] - z );
		fRunFluxCI_lo_1sigma.erase( fRunFluxCI_lo_1sigma.begin() + iRemovePos[i] - z );
		fRunFluxCI_up_1sigma.erase( fRunFluxCI_up_1sigma.begin() + iRemovePos[i] - z );
		fRunFluxCI_lo_3sigma.erase( fRunFluxCI_lo_3sigma.begin() + iRemovePos[i] - z );
		fRunFluxCI_up_3sigma.erase( fRunFluxCI_up_3sigma.begin() + iRemovePos[i] - z );
		fRunFluxConstant.erase( fRunFluxConstant.begin() + iRemovePos[i] - z );
		fRunFluxConstantE.erase( fRunFluxConstantE.begin() + iRemovePos[i] - z );
		fRunEffArea.erase( fRunEffArea.begin() + iRemovePos[i] - z );
		z++;
	}
}


TGraphErrors* VFluxCalculation::plotFluxesVSMJDDaily( char* iTex, double iMJDOffset )
{
	char hname[500];
	
	string sFluxMult = "";
	//    double fluxMult = 1.e7;
	double fluxMult = 1.;
	
	map< int, double > iTDay;
	map< int, double > iFluxDay;
	map< int, double > iFluxDayError;
	
	for( unsigned int i = 0; i < fRunMJD.size(); i++ )
	{
		//double run_duration = (fRunTOn[i] * ( 1. - fRunDeadTime[i] ));
		if( iTDay.find( ( int )fRunMJD[i] ) != iTDay.end() )
		{
			iTDay[( int )fRunMJD[i]] += fRunTOn[i];
			//iTDay[( int )fRunMJD[i]] += run_duration;
			iFluxDay[( int )fRunMJD[i]] += fRunFlux[i] * fRunTOn[i];
			//iFluxDay[( int )fRunMJD[i]] += fRunFlux[i] * run_duration ;
			iFluxDayError[( int )fRunMJD[i]] += fRunFluxE[i] * fRunFluxE[i] * fRunTOn[i] * fRunTOn[i];
			//iFluxDayError[( int )fRunMJD[i]] += fRunFluxE[i] * run_duration * run_duration;
		}
		else
		{
			iTDay[( int )fRunMJD[i]] = fRunTOn[i];
			//iTDay[( int )fRunMJD[i]] = run_duration;
			iFluxDay[( int )fRunMJD[i]] = fRunFlux[i] * fRunTOn[i];
			//iFluxDay[( int )fRunMJD[i]] = fRunFlux[i] * run_duration;
			iFluxDayError[( int )fRunMJD[i]] = fRunFluxE[i] * fRunFluxE[i] * fRunTOn[i] * fRunTOn[i];
			//iFluxDayError[( int )fRunMJD[i]] = fRunFluxE[i] * fRunFluxE[i] * run_duration * run_duration;
		}
	}
	
	TGraphErrors* gFluxMJDDay = new TGraphErrors( ( int )iTDay.size() - 1 );
	
	gFluxMJDDay->SetTitle( "" );
	gFluxMJDDay->SetMarkerStyle( 20 );
	gFluxMJDDay->SetMarkerSize( 1 );
	gFluxMJDDay->SetLineWidth( 2 );
	
	int z = 0;
	typedef map< int, double >::const_iterator CI;
	
	double mean_flux = 0;
	double total_time = 0;
	
	for( CI p = iTDay.begin(); p != iTDay.end(); ++p )
	{
		if( p->first > 10 )
		{
			if( p->second > 0. )
			{
				iFluxDay[p->first] = iFluxDay[p->first] / p->second;
				iFluxDayError[p->first] = sqrt( iFluxDayError[p->first] / p->second / p->second );
				
				// fluxes in 1/[cm2 s ]
				gFluxMJDDay->SetPoint( z, p->first - iMJDOffset, iFluxDay[p->first] * fluxMult );
				gFluxMJDDay->SetPointError( z, 0., iFluxDayError[p->first] * fluxMult );
				mean_flux += iFluxDay[p->first] * fluxMult * iTDay[p->first] ;
				total_time += iTDay[p->first];
				z++;
			}
			else
			{
				gFluxMJDDay->RemovePoint( z );
			}
		}
	}
	
	
	printf( "mean_flux from MJD binned %.3e \n Total time = %.3e \n", mean_flux / total_time, total_time );
	
	
	
	
	TCanvas* cFMJDDay = new TCanvas( "cFMJDDay", "flux per day vs MJD", 610, 10, 600, 400 );
	cFMJDDay->SetGridx( 0 );
	cFMJDDay->SetGridy( 0 );
	cFMJDDay->SetLeftMargin( 0.12 );
	cFMJDDay->Draw();
	
	gFluxMJDDay->Draw( "ap" );
	if( iMJDOffset > 0 )
	{
		sprintf( hname, "MJD - %d", ( int )iMJDOffset );
	}
	else
	{
		sprintf( hname, "MJD" );
	}
	gFluxMJDDay->GetHistogram()->SetXTitle( hname );
	if( fMinEnergy < 1. )
	{
		sprintf( hname, "F(E>%d GeV) [%s cm^{-2}s^{-1}]", ( int )( fMinEnergy * 1.e3 ), sFluxMult.c_str() );
	}
	else
	{
		sprintf( hname, "F(E>%.1f TeV) [%s cm^{-2}s^{-1}]", fMinEnergy, sFluxMult.c_str() );
	}
	gFluxMJDDay->GetHistogram()->SetYTitle( hname );
	gFluxMJDDay->GetHistogram()->GetYaxis()->SetTitleOffset( 1.2 );
	
	if( iTex )
	{
		vector< int > iV_MJD;
		vector< double > iV_Flux;
		vector< double > iV_FluxE;
		for( CI p = iTDay.begin(); p != iTDay.end(); ++p )
		{
			if( p->first > 10 )
			{
				if( p->second > 0. )
				{
					iV_MJD.push_back( p->first );
					iV_Flux.push_back( iFluxDay[p->first] );
					iV_FluxE.push_back( iFluxDayError[p->first] );
				}
			}
		}
		sprintf( hname, "   &[$%s$ m$^{-2}$s$^{-1}$]", sFluxMult.c_str() );
		writeTexFileForFluxValues( iTex, iV_MJD, iV_Flux, iV_FluxE, fluxMult, "MJD & flux", hname );
	}
	
	return gFluxMJDDay;
}


void VFluxCalculation::writeTexFileForFluxValues( string iTex, vector< int > iMJD, vector< double > iFlux, vector< double > iFluxE, double iFac, string iL1, string iL2 )
{
	ofstream is;
	is.open( iTex.c_str() );
	if( !is )
	{
		cout << "error opening " << iTex << endl;
		return;
	}
	cout << "writing tex file: " << iTex << endl;
	cout << endl;
	
	is << "\\documentclass[a4paper]{article}" << endl;
	is << "\\usepackage{longtable}" << endl;
	is << "\\usepackage{lscape}" << endl;
	
	is << "\\begin{document}" << endl;
	
	is << endl;
	is << "\\begin{longtable}{c|c}" << endl;
	is << iL1 << " \\\\" << endl;
	is << iL2 << " \\\\" << endl;
	is << "\\hline" << endl;
	is << "\\hline" << endl;
	for( unsigned int i = 0; i < iMJD.size(); i++ )
	{
		is << iMJD[i] << " & " << setprecision( 3 ) << iFlux[i] * iFac << "$\\pm$" << setprecision( 3 ) << iFluxE[i] * iFac << "\\\\" << endl;
	}
	is << "\\end{longtable}" << endl;
	is << "\\end{document}" << endl;
	is.close();
}


bool VFluxCalculation::readRXTE( string ifile )
{
	if( !fRXTE )
	{
		fRXTE = new VXRayData();
	}
	else
	{
		fRXTE->reset();
	}
	
	// expect at least a few runs in the run list (-1 is one!)
	if( fRunMJD.size() < 2 )
	{
		return false;
	}
	
	double iminMJD = 1.e7;
	double imaxMJD = 0.;
	for( unsigned int i = 0; i < fRunMJD.size(); i++ )
	{
		if( fRunMJD[i] > 0. && fRunMJD[i] < iminMJD )
		{
			iminMJD = fRunMJD[i];
		}
		if( fRunMJD[i] > 0. && fRunMJD[i] > imaxMJD )
		{
			imaxMJD = fRunMJD[i];
		}
	}
	cout << "RUNLIST : " << fRunMJD.size() << "\t" << iminMJD << "\t" << imaxMJD << endl;
	//   return fRXTE->readFile( ifile, "RXTE", getOrbitStart( iminMJD ), getOrbitEnd( imaxMJD ) );
	return fRXTE->readFile( ifile, "RXTE", 0., 0. );
}

bool VFluxCalculation::IsInRunList( int iRun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( fRunList[i] == iRun )
		{
			return true;
		}
	}
	
	return false;
}

unsigned int VFluxCalculation::getIndexOfRun( int iRun )
{
	for( unsigned int i = 0; i < fRunList.size(); i++ )
	{
		if( fRunList[i] == iRun )
		{
			return i;
		}
	}
	
	return 999999;
}

//######################################################################################
//######################################################################################
//######################################################################################
//######################################################################################
//######################################################################################
//######################################################################################

VXRayData::VXRayData()
{
	bDebug = true;
	
	gFluxPhase = 0;
	gFluxMJD = 0;
}


void VXRayData::reset()
{
	if( bDebug )
	{
		cout << "VXRayData::reset()" << endl;
	}
	fMJD.clear();
	fPhase.clear();
	fPhi.clear();
	fFlux.clear();
	fFluxEdown.clear();
	fFluxEup.clear();
	
	gFluxPhase = 0;
	gFluxMJD = 0;
	
	if( bDebug )
	{
		cout << "END: VXRayData::reset()" << endl;
	}
}


bool VXRayData::readFile( string ifile, string tname, double iMJDmin, double iMJDmax )
{
	if( bDebug )
	{
		cout << "VXRayData::readFile " << ifile << "\t" << tname << "\t" << iMJDmin << "\t" << iMJDmax << endl;
	}
	
	reset();
	
	char htitle[200];
	gFluxPhase = new TGraphAsymmErrors();
	sprintf( htitle, "%s_Phase", tname.c_str() );
	gFluxPhase->SetTitle( htitle );
	gFluxPhase->SetMarkerStyle( 8 );
	gFluxMJD = new TGraphAsymmErrors();
	sprintf( htitle, "%s_MJD", tname.c_str() );
	gFluxMJD->SetTitle( htitle );
	gFluxMJD->SetMarkerStyle( 8 );
	
	if( ifile.size() < 1 )
	{
		return false;
	}
	
	TFile f( ifile.c_str() );
	if( f.IsZombie() )
	{
		cout << "error: file with X-ray data not found: " << ifile << endl;
		return false;
	}
	if( iMJDmin > 0 && bDebug )
	{
		cout << tname << "\t" << iMJDmin << "\t" << iMJDmax << endl;
	}
	if( iMJDmin < 1 || iMJDmin > 1.e6 )
	{
		return false;
	}
	if( iMJDmax < 1 || iMJDmax > 1.e6 )
	{
		return false;
	}
	
	float MJD;
	float MJDElow;
	float MJDEup;
	float phase;
	float phi;
	float flux;
	float fluxElow;
	float fluxEup;
	TTree* t = ( TTree* )f.Get( tname.c_str() );
	if( !t )
	{
		cout << "error: tree with X-ray data not found: " << tname << endl;
		return false;
	}
	t->SetBranchAddress( "MJD", &MJD );
	t->SetBranchAddress( "MJDElow", &MJDElow );
	t->SetBranchAddress( "MJDEup", &MJDEup );
	t->SetBranchAddress( "phase", &phase );
	t->SetBranchAddress( "phi", &phi );
	t->SetBranchAddress( "flux", &flux );
	t->SetBranchAddress( "fluxElow", &fluxElow );
	t->SetBranchAddress( "fluxEup", &fluxEup );
	
	int z = 0;
	for( int i = 0; i < t->GetEntries(); i++ )
	{
		t->GetEntry( i );
		
		if( ( iMJDmin > 0. && MJD >= iMJDmin ) || iMJDmin < 0. )
		{
			if( ( iMJDmax > 0. && MJD <= iMJDmax ) || iMJDmax < 0. )
			{
				fMJD.push_back( MJD );
				fPhase.push_back( phase );
				fPhi.push_back( phi );
				fFlux.push_back( flux );
				fFluxEup.push_back( fluxElow );
				fFluxEdown.push_back( fluxEup );
				
				gFluxPhase->SetPoint( z, phase, flux );
				gFluxPhase->SetPointEXlow( z, MJDElow / 26.4960 );
				gFluxPhase->SetPointEXhigh( z, MJDEup / 26.4960 );
				gFluxPhase->SetPointEYlow( z, fluxElow );
				gFluxPhase->SetPointEYhigh( z, fluxEup );
				
				gFluxMJD->SetPoint( z, MJD, flux );
				gFluxMJD->SetPointEXlow( z, MJDElow );
				gFluxMJD->SetPointEXhigh( z, MJDEup );
				gFluxMJD->SetPointEYlow( z, fluxElow );
				gFluxMJD->SetPointEYhigh( z, fluxEup );
				z++;
			}
		}
	}
	f.Close();
	
	if( bDebug && gFluxPhase )
	{
		gFluxPhase->Print();
	}
	
	if( bDebug )
	{
		cout << "END: VXRayData::readFile " << endl;
	}
	
	return true;
}
