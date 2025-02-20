/*! \class VImageBaseAnalyzer

    basic routines for integration, tzero calculation, double pass integration, etc
    used by VImageAnalyzer, VCalibrator, VDST

 */

#include "VImageBaseAnalyzer.h"

/*
 * special channels are e.g. L2 channels or channels disabled by the user
 *
 */
bool VImageBaseAnalyzer::setSpecialChannels()
{
	if( getDebugFlag() )
	{
		cout << "VImageBaseAnalyzer::setSpecialChannels " << getEventNumber() << endl;
	}
	
	// set masked (FADC trig) channels
	if( getFADCstopTrig().size() > 0 )
	{
		for( unsigned int i = 0; i < getMasked().size(); i++ )
		{
			for( unsigned int t = 0; t < getFADCstopTrig().size(); t++ )
			{
				if( i == getFADCstopTrig()[t] )
				{
					if( t == 0 )
					{
						getMasked()[i] = 1;
					}
					else if( t == 1 )
					{
						getMasked()[i] = 2;
					}
					else if( t == 2 )
					{
						getMasked()[i] = 3;
					}
					else if( t == 3 )
					{
						getMasked()[i] = 4;
					}
				}
			}
		}
	}
	
	if( fReader->isMC() )
	{
		fRunPar->fL2TimeCorrect = false;
	}
	
	///////////////////////////////////////////////////////////////////////////////////
	// set channel status
	//
	// Note: dead channel settings overwrite all pre-existing settings
	for( unsigned int i = 0; i < getChannelStatus().size(); i++ )
	{
		if( getAnaData()->getSpecialChannel() )
		{
			if( getChannelStatus()[i] == 1 )
			{
				getChannelStatus()[i] = getAnaData()->getSpecialChannel()->getChannelStatus( i );
			}
		}
	}
	///////////////////////////////////////////////////////////////////////////////////
	
	return true;
}

/*

   calculate integrated charges for all pixels

   (to be used for calibration only)

*/
void VImageBaseAnalyzer::calcSums( int iFirst, int iLast, bool iMakingPeds, bool iLowGainOnly, unsigned int iTraceIntegrationMethod )
{
	if( getDebugFlag() )
	{
		cout << "VImageBaseAnalyzer::calcSums() " << iFirst << "\t" << iLast << endl;
	}
	int sw_original = iLast - iFirst;
	
	// for DST source file, ignore everything and just get the sums
	if( getRunParameter()->frunmode != 1 && ( fReader->getDataFormatNum() == 4 || fReader->getDataFormatNum() == 6 ) )
	{
		setSums( fReader->getSums() );
		return;
	}
	
	// check integration range
	if( iFirst < 0 )
	{
		if( getDebugFlag() )
		{
			cout << "void VImageBaseAnalyzer::calcSums warning: set summation start to 0 " << endl;
		}
		iFirst = 0;
	}
	if( iLast > ( int )getNSamples() )
	{
		if( getDebugFlag() )
		{
			cout << "void VImageBaseAnalyzer::calcSums warning: set summation last to " << getNSamples() << endl;
		}
		iLast = ( int )getNSamples();
	}
	
	// calculates the sum over a sample range for all channels
	setSums( 0. );
	unsigned int nhits = fReader->getNumChannelsHit();
	// exclude photodiode from this
	if( nhits > getDead( false ).size() )
	{
		nhits = getDead( false ).size();
	}
	
	for( unsigned int i = 0; i < nhits; i++ )
	{
		unsigned int i_channelHitID = 0;
		try
		{
			i_channelHitID = fReader->getHitID( i );
			// for low gain pedestal calibration: ignore high gain channels
			if( iLowGainOnly && i_channelHitID < getHiLo().size() && !getHiLo()[i_channelHitID] )
			{
				continue;
			}
			
			if( i_channelHitID < getHiLo().size() && i_channelHitID < getDead( getHiLo()[i_channelHitID] ).size()
					&& !getDead( i_channelHitID, getHiLo()[i_channelHitID] )
					&& iLast > iFirst )
			{
				fReader->selectHitChan( i );
				initializeTrace( iMakingPeds, i_channelHitID, i, iTraceIntegrationMethod );
				
				// calculate sums
				setSums( i_channelHitID,
						 fTraceHandler->getTraceSum( iFirst,
													 iLast,
													 iMakingPeds,
													 9999,
													 true,
													 getSearchWindowLast() )
						 * getLowGainSumCorrection( sw_original, iLast - iFirst, getHiLo()[i_channelHitID] ) );
				setTraceAverageTime( i_channelHitID, fTraceHandler->getTraceIntegrationFirst() );
			}
		}
		catch( ... )
		{
			if( getDebugLevel() == 0 )
			{
				cout << "VImageBaseAnalyzer::calcSums(), index out of range (fReader->getHitID) ";
				cout << i << ", i_channelHitID " << i_channelHitID << endl;
				cout << "\t nhits: " << nhits << endl;
				cout << "\t (Telescope " << getTelID() + 1 << ", event " << getEventNumber() << ")" << endl;
				setDebugLevel( 1 );
			}
			continue;
		}
	}
}


/*

   calculate trace timing parameters

   (used for time calibration)

*/
void VImageBaseAnalyzer::calcTZeros( int fFirst, int fLast )
{
	if( getDebugFlag() )
	{
		cout << "VImageBaseAnalyzer::calcTZeros() " << fFirst << "\t" << fLast << endl;
	}
	// for DST source file, ignore everything and just get the sums and tzeros
	if( fReader->getDataFormatNum() == 4 || fReader->getDataFormatNum() == 6 )
	{
		setPulseTiming( fReader->getTracePulseTiming(), true );
		setPulseTiming( fReader->getTracePulseTiming(), false );
		return;
	}
	// check integration range
	if( fFirst < 0 )
	{
		if( getDebugFlag() )
		{
			cout << "void VImageBaseAnalyzer::calcTZeros warning: set start to 0 " << endl;
		}
		fFirst = 0;
	}
	if( fLast > ( int )getNSamples() )
	{
		if( getDebugFlag() )
		{
			cout << "void VImageBaseAnalyzer::calcTZeros warning: set last to " << getNSamples() << endl;
		}
		fLast = ( int )getNSamples();
	}
	// calculates the rising edge time for all channels
	setPulseTiming( 0., true );
	
	unsigned int nhits = fReader->getNumChannelsHit();
	// exclude photodiode from this
	if( nhits > getDead( false ).size() )
	{
		nhits = getDead( false ).size();
	}
	
	for( unsigned int i = 0; i < nhits; i++ )
	{
		unsigned int i_channelHitID = 0;
		try
		{
			i_channelHitID = fReader->getHitID( i );
			
			if( i_channelHitID < getHiLo().size() && i_channelHitID < getDead( getHiLo()[i_channelHitID] ).size() && !getDead( i_channelHitID, getHiLo()[i_channelHitID] ) )
			{
				if( getDebugFlag() )
				{
					cout << "\t VImageBaseAnalyzer::calcTZeros() channel ID: " << i_channelHitID << endl;
				}
				fReader->selectHitChan( i );
				fTraceHandler->setTrace( fReader, getNSamples(), getPeds( getHiLo()[i_channelHitID] )[i_channelHitID],
										 getPedrms( getHiLo()[i_channelHitID] )[i_channelHitID], i_channelHitID, i,
										 getLowGainMultiplier_Trace()*getHiLo()[i_channelHitID] );
										 
				fTraceHandler->setTraceIntegrationmethod( getTraceIntegrationMethod() );
				setPulseTiming( i_channelHitID, fTraceHandler->getPulseTiming( fFirst, fLast, 0, getNSamples() ), true );
			}
		}
		catch( ... )
		{
			if( getDebugLevel() == 0 )
			{
				cout << "VImageBaseAnalyzer::calcTZeros, index out of range " << i;
				cout << "(Telescope " << getTelID() + 1 << ", event " << getEventNumber() << ")" << endl;
				setDebugLevel( 1 );
			}
			continue;
		}
	}
}

/*
 *
 * calculate correction for crate jitter from L2 channels fed into each crate
 *
 * (this is very VERITAS specific)
 *
 */
void VImageBaseAnalyzer::FADCStopCorrect()
{
	if( fDebug )
	{
		cout << "VImageBaseAnalyzer::FADCStopCorrect()" << endl;
	}
	
	// don't do this if there is no L2 trigger
	if( !getReader()->hasLocalTrigger( getTelID() ) )
	{
		return;
	}
	
	// don't do this if there are no FADCs
	if( !getReader()->hasFADCTrace() )
	{
		return;
	}
	
	// do the FADC stop correction only if all channels are set
	if( getFADCstopTrig().size() == 0 )
	{
		return;
	}
	unsigned int i_channelHitID = 0;
	setFADCStopOffsets( 0. );
	
	// special treatment of channels with i_channelHitID beyond camera channels (e.g. 499)
	unsigned int iPedFADCTrigChan = 1000000;
	for( unsigned int t = 0; t < getFADCstopTrig().size(); t++ )
	{
		if( getFADCstopTrig()[t] < getNChannels() )
		{
			iPedFADCTrigChan = getFADCstopTrig()[t];
		}
	}
	
	///////////////////////////////////////////////////////////////////////////////////////
	// now get channel jitter
	
	for( unsigned int t = 0; t < getFADCstopTrig().size(); t++ )
	{
		//  calculate TZero for first crate trigger signal
		pair< bool, uint32_t > i_hitIndexPair = fReader->getChannelHitIndex( ( unsigned int )getFADCstopTrig()[t] );
		if( !i_hitIndexPair.first )
		{
			continue;
		}
		unsigned int i = ( unsigned int )i_hitIndexPair.second;
		double crateTZero = 0.;
		double offset = 0.;
		try
		{
			if( i < fReader->getMaxChannels() )
			{
				i_channelHitID = fReader->getHitID( i );
				if( i_channelHitID < getZeroSuppressed().size() && getZeroSuppressed()[i_channelHitID] )
				{
					continue;
				}
				
				fReader->selectHitChan( ( uint32_t )i );
				if( i_channelHitID < getPeds().size() )
				{
					fTraceHandler->setTrace( fReader, getNSamples(), getPeds()[i_channelHitID], getPedrms()[i_channelHitID], i_channelHitID, i, 0. );
				}
				// take pedestal from another FADC trig channel
				else if( iPedFADCTrigChan < getPeds().size() && !getZeroSuppressed()[iPedFADCTrigChan] )
				{
					fTraceHandler->setTrace( fReader, getNSamples(), getPeds()[iPedFADCTrigChan], getPedrms()[iPedFADCTrigChan], i_channelHitID, i, 0. );
					i_channelHitID = fReader->getHitID( i );
				}
				else
				{
					continue;
				}
				fTraceHandler->setTraceIntegrationmethod( getTraceIntegrationMethod() );
				// calculate t0
				if( fTraceHandler->getTraceSum( 0, getNSamples(), false ) > 300 )
				{
					crateTZero = fTraceHandler->getFADCTiming( 0, getNSamples() )[getRunParameter()->fpulsetiming_tzero_index];
					if( i_channelHitID < getHiLo().size() && getHiLo()[i_channelHitID] )
					{
						crateTZero = 0.;
					}
				}
				getFADCstopTZero()[t] = crateTZero;
				// calculate offsets
				if( t > 0 )
				{
					if( getFADCstopTZero()[0] > 0 &&  crateTZero > 0 )
					{
						offset = getFADCstopTZero()[0] - crateTZero;
					}
					else
					{
						offset = 0.;
					}
					
					if( fDebug && fabs( offset ) > 5 )
					{
						cout << "VImageBaseAnalyzer::FADCStopCorrect() warning: crate jitter offset > 5 samples in Event ";
						cout << fEventNumber << ", Tel " << fTelID + 1 << ", TZero[0] " << getFADCstopTZero()[0] << ", TZero[" << t << "] " << crateTZero << endl;
					}
					unsigned int iC_start = 0;
					unsigned int iC_stop =  0;
					// crate 2
					if( t == 1 )
					{
						iC_start = 130;
						iC_stop =  250;
					}
					// crate 3
					else if( t == 2 )
					{
						iC_start = 250;
						iC_stop =  380;
					}
					// crate 4
					else if( t == 3 )
					{
						iC_start = 380;
						iC_stop =  getTZeros().size();
					}
					// apply offset
					for( unsigned int j = iC_start; j < iC_stop; j++ )
					{
						if( j != i )
						{
							setPulseTimingCorrection( j, offset );
						}
						getFADCStopOffsets()[j] = offset;
					}
				}
			}
		}
		catch( ... )
		{
			if( getDebugLevel() == 0 )
			{
				cout << "VImageBaseAnalyzer::FADCStopCorrect(), index out of range warning (fReader->getHitID) ";
				cout << "channel " << i << ", hit ID " << i_channelHitID << endl;
				cout << "zero suppression " << getZeroSuppressed()[i_channelHitID] << endl;
				cout << " (Telescope " << getTelID() + 1 << ", event " << getEventNumber() << ")";
				cout << endl;
				setDebugLevel( 1 );
			}
		}
	}
}


/*

   this is doing the same as calcTZerosSums, calcTZeros (makes program about 15% faster that way)

   used in VDisplay for event movies, not in general analysis

*/
void VImageBaseAnalyzer::calcTCorrectedSums( int iFirst, int iLast )
{

	int sw_original = iLast - iFirst;
	// for DST source file, ignore everything and just get the sums and tzeros
	if( fReader->getDataFormatNum() == 4 || fReader->getDataFormatNum() == 6 )
	{
		setSums( fReader->getSums() );
		setPulseTiming( fReader->getTracePulseTiming(), true );
		setPulseTiming( fReader->getTracePulseTiming(), false );
		setTraceMax( fReader->getTraceMax() );
		setTraceRawMax( fReader->getTraceRawMax() );
		return;
	}
	
	// calculates the sum over a sample range, corrected for time offsets, for all channels
	setSums( 0. );
	unsigned int nhits = fReader->getNumChannelsHit();
	// exclude photodiode from this
	if( nhits > getDead( false ).size() )
	{
		nhits = getDead( false ).size();
	}
	
	for( unsigned int i = 0; i < nhits; i++ )
	{
	
		unsigned int i_channelHitID = 0;
		try
		{
			i_channelHitID = fReader->getHitID( i );
			
			if( i_channelHitID < getHiLo().size() && i_channelHitID < getDead( getHiLo()[i_channelHitID] ).size() && !getDead( i_channelHitID, getHiLo()[i_channelHitID] ) )
			{
				fReader->selectHitChan( i );
				fTraceHandler->setTrace( fReader, getNSamples(), getPeds( getHiLo()[i_channelHitID] )[i_channelHitID],
										 getPedrms( getHiLo()[i_channelHitID] )[i_channelHitID], i_channelHitID, i,
										 getLowGainMultiplier_Trace()*getHiLo()[i_channelHitID] );
										 
				fTraceHandler->setTraceIntegrationmethod( getTraceIntegrationMethod() );
				int offset = 0;
				if( !getRunParameter()->fFixWindowStart )
				{
					if( getTOffsets()[i_channelHitID] > 0 )
					{
						offset = ( int )getTOffsets()[i_channelHitID];
					}
					if( getTOffsets()[i_channelHitID] < 0 )
					{
						offset = ( int )( getTOffsets()[i_channelHitID] - 1. );
					}
				}
				int corrfirst = iFirst + offset;
				if( corrfirst < 0 )
				{
					corrfirst = 0;
				}
				if( corrfirst > ( int )getNSamples() )
				{
					corrfirst = ( int )getNSamples();
				}
				int corrlast = iLast + offset;
				if( corrlast < 0 )
				{
					corrlast = 0;
				}
				if( corrlast > ( int )getNSamples() )
				{
					corrlast = ( int )getNSamples();
				}
				
				setTCorrectedSumFirst( i_channelHitID, corrfirst );
				setTCorrectedSumLast( i_channelHitID, corrlast );
				setCurrentSummationWindow( i_channelHitID, corrfirst, corrlast, false );
				setSums( i_channelHitID, fTraceHandler->getTraceSum( corrfirst, corrlast, fRaw )* getLowGainSumCorrection( sw_original, iLast - iFirst, getHiLo()[i_channelHitID] ) );  // nb sw_original is small->most likely correction will be 1
				
			}
		}
		catch( ... )
		{
			cout << "VImageBaseAnalyzer::calcTCorrectedSums(), index out of range (fReader->getHitID) ";
			cout << i << "(Telescope " << getTelID() + 1 << ", event " << getEventNumber() << ")" << endl;
			continue;
		}
	}
}


/*

     calculate sums and timing parameters of FADC traces


     this function is called from VImageAnalyzer::doAnalysis()

     (used for calibration, DP pass1 integration, NN cleaning, and tzero calculation)

*/
void VImageBaseAnalyzer::calcTZerosSums( int iFirstSum, int iLastSum, unsigned int iTraceIntegrationMethod )
{
	if( getDebugFlag() )
	{
		cout << "VImageBaseAnalyzer::calcTZerosSums() \t" << iFirstSum << "\t" << iLastSum << endl;
	}
	
	/////////////////////////////////////////////////////////////////////////////////
	// DST source file,
	// ignore everything and just get the sums and tzeros from data trees
	if( !fReader->hasFADCTrace() || !getRunParameter()->doFADCAnalysis() )
	{
		if( getDebugFlag() )
		{
			cout << "VImageBaseAnalyzer::calcTZerosSums() reading sums from DST/QADC file" << endl;
		}
		setSums( fReader->getSums( getNChannels() ) );
		setSums2( fReader->getSums( getNChannels() ) );
		setPulseTiming( fReader->getTracePulseTiming( getNChannels() ), true );
		setPulseTiming( fReader->getTracePulseTiming( getNChannels() ), false );
		setTraceMax( fReader->getTraceMax( getNChannels() ) );
		setTraceRawMax( fReader->getTraceRawMax( getNChannels() ) );
		setTraceAverageTime( 0. );
		return;
	}
	
	/////////////////////////////////////////////////////////////////////////////////
	// FAD trace analysis
	
	// reset the data vectors
	if( getTraceFit() > -1 )
	{
		setTraceRiseTime( 0. );
		setTraceFallTime( 0. );
		setTraceChi2( 0. );
	}
	setPulseTiming( 0., true );    // corrected   times
	setPulseTiming( 0., false );   // uncorrected times
	setTraceN255( 0 );
	setTraceAverageTime( 0. );
	setSums( 0. );
	setSums2( 0. );
	
	/////////////////////////////////////////////////////////////////////////////////////////////
	// calculates the sum over a sample range, corrected for time offsets, for all channels
	/////////////////////////////////////////////////////////////////////////////////////////////
	
	unsigned int nhits = fReader->getNumChannelsHit();
	// exclude photodiode from this (which would be channel Nchannel + 1 )
	if( nhits > getDead( false ).size() )
	{
		nhits = getDead( false ).size();
	}
	unsigned int ndead_size = getDead().size();
	
	double i_tempTraceMax = 0;
	unsigned int i_tempN255 = 0;
	int corrfirst = 0;
	int corrlast = 0;
	int corrlast_sw2 = 0;
	
	//////////////////////////////////////////////////////////////////
	// loop over all channels (hits)
	//////////////////////////////////////////////////////////////////
	for( unsigned int i = 0; i < nhits; i++ )
	{
		unsigned int i_channelHitID = 0;
		try
		{
			i_channelHitID = fReader->getHitID( i );
		}
		catch( ... )
		{
			if( getDebugLevel() == 0 )
			{
				cout << "VImageBaseAnalyzer::calcTZerosSums, index out of range " << i;
				cout << "(Telescope " << getTelID() + 1 << ", event " << getEventNumber() << ")" << endl;
				setDebugLevel( 0 );
			}
			continue;
		}
		/////////////////////////////////////////////////////////////////
		// calculate tzero and sums for good channels only
		/////////////////////////////////////////////////////////////////
		if( i_channelHitID < ndead_size && !getDead( i_channelHitID, getHiLo()[i_channelHitID] ) )
		{
			// initialize trace handler
			fTraceHandler->setTrace( fReader, getNSamples(),
									 getPeds( getHiLo()[i_channelHitID] )[i_channelHitID], getPedrms( getHiLo()[i_channelHitID] )[i_channelHitID],
									 i_channelHitID, i, getLowGainMultiplier_Trace()*getHiLo()[i_channelHitID] );
			fTraceHandler->setTraceIntegrationmethod( iTraceIntegrationMethod );
			
			// get time offsets (from laser/flasher calibration): make sure that trace integration starts at the same time in all channels
			int offset = 0;
			if( !getRunParameter()->fFixWindowStart )
			{
				if( getTOffsets()[i_channelHitID] > 0 )
				{
					offset = ( int )getTOffsets()[i_channelHitID];
				}
				if( getTOffsets()[i_channelHitID] < 0 )
				{
					offset = ( int )getTOffsets()[i_channelHitID] - 1;
				}
			}
			
			///////////////////////////////////////////
			// determine summation window parameters
			// start and end of summation window
			corrfirst = getFADCTraceIntegrationPosition( iFirstSum + offset );
			corrlast  = getFADCTraceIntegrationPosition( iLastSum + offset );
			// calculate timing parameters (raw and corrected; tzero correction happens later)
			if( getSumWindowStart_at_T0() )
			{
				setPulseTiming( i_channelHitID, fTraceHandler->getPulseTiming( 0, getNSamples(), 0, getNSamples() ), true );
			}
			else
			{
				setPulseTiming( i_channelHitID, fTraceHandler->getPulseTiming( corrfirst, corrlast, 0, getNSamples() ), true );
			}
			// shift the summation window if necessary
			if( getSumWindowShift() != 0 && !getRunParameter()->fFixWindowStart )
			{
				corrfirst = getFADCTraceIntegrationPosition( iFirstSum + ( int )getTOffsets()[i_channelHitID] + getSumWindowShift() );
				corrlast  = getFADCTraceIntegrationPosition( corrfirst + ( iLastSum - iFirstSum ) );
			}
			// use T0 as start of integraction window:
			if( getSumWindowStart_at_T0() )
			{
				corrfirst = getFADCTraceIntegrationPosition( getPulseTiming()[getRunParameter()->fpulsetiming_tzero_index][i_channelHitID] + getSumWindowShift() );
				corrlast  = getFADCTraceIntegrationPosition( corrfirst + ( iLastSum - iFirstSum ) );
			}
			// set current summation window (might change from pixel to pixel and event to event
			setCurrentSummationWindow( i_channelHitID, corrfirst, corrlast, false );
			
			///////////////////////////////////////////
			// integrate trace
			setSums( i_channelHitID, fTraceHandler->getTraceSum( corrfirst, corrlast, fRaw ) * getLowGainSumCorrection( iLastSum - iFirstSum, corrlast - corrfirst, getHiLo()[i_channelHitID] ) );
			
			// fill parameters characterizing the trace
			setTCorrectedSumFirst( i_channelHitID, fTraceHandler->getTraceIntegrationFirst() );
			setTCorrectedSumLast( i_channelHitID, fTraceHandler->getTraceIntegrationLast() );
			setTraceAverageTime( i_channelHitID, fTraceHandler->getTraceAverageTime() );
			i_tempTraceMax = fTraceHandler->getTraceMax( i_tempN255, getLowGainMultiplier_Trace() );
			setTraceMax( i_channelHitID, i_tempTraceMax );
			setTraceRawMax( i_channelHitID, i_tempTraceMax + getPeds( getHiLo()[i_channelHitID] )[i_channelHitID] );
			setTraceN255( i_channelHitID, i_tempN255 );
			if( getFillMeanTraces() )
			{
				setTrace( i_channelHitID, fTraceHandler->getTrace(), getHiLo()[i_channelHitID], getPeds( getHiLo()[i_channelHitID] )[i_channelHitID] );
			}
			/////////////////
			// no doublepass: fill summation windows and pass2 sum
			if( !getRunParameter()->fDoublePass )
			{
				// don't fill pulse timing in first pass of double pass (saves time)
				if( getFillPulseSum() )
				{
					getAnaData()->fillPulseSum( i_channelHitID, getSums()[i_channelHitID], getHiLo()[i_channelHitID] );
				}
				corrlast_sw2 = getFADCTraceIntegrationPosition( corrfirst + ( int )getSumWindow_2() );
				if( corrlast_sw2 != corrlast )
				{
					setSums2( i_channelHitID, fTraceHandler->getTraceSum( corrfirst, corrlast_sw2, fRaw )* getLowGainSumCorrection( fRunPar->fsumwindow_2[fTelID], corrlast - corrfirst, getHiLo()[i_channelHitID] ) );
				}
				else
				{
					setSums2( i_channelHitID, getSums()[i_channelHitID] );
				}
				setCurrentSummationWindow( i_channelHitID, corrfirst, corrlast, true );
			}
			//////////////////////////////////////////////////
			
			// get results for trace fitting
			// rise and fall time are 10/90% values
			if( getTraceFit() > -1 )
			{
				setTraceChi2( i_channelHitID, fTraceHandler->getTraceChi2() );
				setTraceFallTime( i_channelHitID, fTraceHandler->getTraceFallTime( getPeds( getHiLo()[i_channelHitID] )[i_channelHitID], 0.9, 0.1 ) );
				setTraceRiseTime( i_channelHitID, fTraceHandler->getTraceRiseTime( getPeds( getHiLo()[i_channelHitID] )[i_channelHitID], 0.1, 0.9 ) );
				setTraceFallTimeParameter( i_channelHitID, fTraceHandler->getTraceFallTime() );
				setTraceRiseTimeParameter( i_channelHitID, fTraceHandler->getTraceRiseTime() );
				setTraceNorm( i_channelHitID, fTraceHandler->getTraceNorm() );
			}
		}
		/////////////////////////////////////////////////////////////////
		// calculate size of FADC stop channel
		// (used for DQM)
		// (note that this does not catch FADC stop channels in channel 499)
		else
		{
			for( unsigned int c = 0; c < getFADCstopTrig().size(); c++ )
			{
				if( i_channelHitID == getFADCstopTrig()[c] )
				{
					fReader->selectHitChan( ( uint32_t )i );
					if( fReader->has16Bit() )
					{
						fTraceHandler->setTrace( fReader->getSamplesVec16Bit(), getPeds( getHiLo()[i_channelHitID] )[i_channelHitID],
												 getPedrms( getHiLo()[i_channelHitID] )[i_channelHitID], i_channelHitID,
												 getLowGainMultiplier_Trace()*getHiLo()[i_channelHitID] );
					}
					else
					{
						fTraceHandler->setTrace( fReader->getSamplesVec(), getPeds( getHiLo()[i_channelHitID] )[i_channelHitID],
												 getPedrms( getHiLo()[i_channelHitID] )[i_channelHitID], i_channelHitID,
												 getLowGainMultiplier_Trace()*getHiLo()[i_channelHitID] );
					}
					fTraceHandler->setTraceIntegrationmethod( iTraceIntegrationMethod );
					getFADCstopSums()[c] = fTraceHandler->getTraceSum( 0, getNSamples(), fRaw );
				}
			}
		}
	}
	// fill tzero vector with uncorrected times
	setPulseTiming( getPulseTiming( true ), false );
}

/*!

    apply relative gain corretion

*/
void VImageBaseAnalyzer::gainCorrect()
{
	if( fDebug )
	{
		cout << "void VImageBaseAnalyzer::gainCorrect()" << endl;
	}
	
	// do not gain correct if traces where not calculated by evndisp (using e.g. sim_telarray analysis results)
	if( !getRunParameter()->doFADCAnalysis() )
	{
		return;
	}
	
	// loop over all channels
	unsigned int nc = getSums().size();
	for( unsigned int i = 0; i < nc; i++ )
	{
		// apply gain correction for HIGHQE channels
		// (e.g. V5 with 6 highqe pixels installed in T3)
		double iHIGHQE = getHIGHQE_gainfactor( i );
		if( iHIGHQE <= 0. )
		{
			iHIGHQE = 1.;
		}
		// correct gains
		if( getGains()[i] > 0  && !getDead( i, getHiLo()[i] ) )
		{
			setSums( i, getSums()[i] / ( getGains()[i] * iHIGHQE ) );
		}
		else
		{
			setSums( i, 0. );
		}
		if( getGains()[i] > 0  && !getDead( i, getHiLo()[i] ) )
		{
			setSums2( i, getSums2()[i] / ( getGains()[i] * iHIGHQE ) );
		}
		else
		{
			setSums2( i, 0. );
		}
	}
}


/*!

  If you add an additional code value, please add the corresponding text
  in void VEvndispData::setDeadChannelText()

  dead channel coding (see also VEvndispData::setDeadChannelText())
  - outside pedestal range (1)
  - small absolute pedvars (2)
  - small relative pedvars (3)
  - large relative pedvars (4)
  - outside gain range (5)
  - small/large gain variations (6)
  - large gain deviations (7)
  - large time offset (8)
  - FADC stop signal (9)
  - masked (10)
  - user set (11)
  - MC dead (12)
  - gain multiplier (13)

*/
void VImageBaseAnalyzer::findDeadChans( bool iLowGain, bool iFirst )
{
	if( fDebug )
	{
		cout << "VImageBaseAnalyzer::findDeadChans( bool iLowGain ) T" << getTelID() + 1 << endl;
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// do not set anything dead for PE-mode
	if( getRunParameter()->fsourcetype == 6 )
	{
		return;
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// DST: read dead channel list from DST file
	if( getRunParameter()->fsourcetype == 7 || getRunParameter()->fsourcetype == 4 )
	{
		for( unsigned int i = 0; i < getNChannels(); i++ )
		{
			if( getReader()->getDead()[i] > 0 )
			{
				setDead( i, TMath::Nint( log( ( float )getReader()->getDead()[i] ) / log( 2. ) ), iLowGain );
			}
			else
			{
				setDead( i, 0, iLowGain );
			}
		}
		// getFullAnaVec()[i]: -1: dead channel, 0: channel does not exist, 1 channel o.k.
		if( getNChannels() >= getDetectorGeometry()->getAnaPixel().size() )
		{
			for( unsigned int i = 0; i < getNChannels(); i++ )
			{
				if( getDetectorGeometry()->getAnaPixel()[i] < 1 )
				{
					setDead( i, 12, iLowGain );
				}
			}
		}
		return;
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// for plotting ignore dead channels (no pedestal information available)
	if( getRunParameter()->fPlotRaw )
	{
		return;
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// find dead channels from calibration and event data
	
	// check if dead channel configuration is time dependent
	if( !iFirst && !usePedestalsInTimeSlices( iLowGain ) )
	{
		return;
	}
	
	// reset dead channel vector
	setDead( false, iLowGain );
	
	// get mean and rms of pedvar
	double i_meanPedVar = 0.;
	double i_meanPedVarRMS = 0.;
	getmeanPedvars( i_meanPedVar, i_meanPedVarRMS, iLowGain, getSumWindow() );
	
	for( unsigned int i = 0; i < getNChannels(); i++ )
	{
		// FADC stop channels (don't set any other reasons for channels to be dead)
		for( unsigned int t = 0; t < getFADCstopTrig().size(); t++ )
		{
			if( getFADCstopTrig()[t] == i )
			{
				setDead( i, 9, iLowGain );
				continue;
			}
		}
		// time dependent dead channels finder (pedestals in time slices)
		if( usePedestalsInTimeSlices( iLowGain ) )
		{
			// reset bits for ped settings
			setDead( i, 1, iLowGain, false, true );
			setDead( i, 2, iLowGain, false, true );
			setDead( i, 3, iLowGain, false, true );
			setDead( i, 4, iLowGain, false, true );
			// check values
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainPedestals() )
					 ->testPedestals( i, getPeds( iLowGain )[i] ), iLowGain );
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainPedestals() )
					 ->testPedestalVariations( i, getPedvars( iLowGain, getSumWindow() )[i] ), iLowGain );
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainPedestals() )
					 ->testPedestalVariationsMinOut( i, getPedvars( iLowGain, getSumWindow() )[i], i_meanPedVar, i_meanPedVarRMS ), iLowGain );
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainPedestals() )
					 ->testPedestalVariationsMaxOut( i, getPedvars( iLowGain, getSumWindow() )[i], i_meanPedVar, i_meanPedVarRMS ), iLowGain );
		}
		// same data for whole run
		else
		{
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainPedestals() )
					 ->testPedestals( i, getPeds( iLowGain )[i] ), iLowGain );
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainPedestals() )
					 ->testPedestalVariations( i, getPedvars( iLowGain )[i] ), iLowGain );
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainPedestals() )
					 ->testPedestalVariationsMinOut( i, getPedvars( iLowGain )[i], i_meanPedVar, i_meanPedVarRMS ), iLowGain );
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainPedestals() )
					 ->testPedestalVariationsMaxOut( i, getPedvars( iLowGain )[i], i_meanPedVar, i_meanPedVarRMS ), iLowGain );
		}
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// time independent values
		// gain/toff/low gain
		if( ( !getRunParameter()->fNoCalibNoPb && !iLowGain && getRunParameter()->fGainFileNumber[getTelID()] > 0
				&& !( getRunParameter()->fNextDayGainHack && getGains( iLowGain )[i] == 1.0 ) )
				|| ( iLowGain && getRunParameter()->fGainLowGainFileNumber[getTelID()] > 0 ) )
		{
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainGains() )
					 ->testGains( i, getGains( iLowGain )[i] ), iLowGain );
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainGains() )
					 ->testGainVariations( i, getGainvars( iLowGain && getLowGainGains() )[i] ), iLowGain );
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainGains() )
					 ->testGainDev( i, getGains( iLowGain )[i], getGainvars( iLowGain && getLowGainGains() )[i], getGains_DefaultValue( iLowGain )[i] ), iLowGain );
		}
		if( ( !getRunParameter()->fNoCalibNoPb && !iLowGain && getRunParameter()->fTOffFileNumber[getTelID()] > 0 )
				|| ( iLowGain && getRunParameter()->fTOffLowGainFileNumber[getTelID()] > 0 ) )
		{
			setDead( i, getDeadChannelFinder( iLowGain && getLowGainTOff() )
					 ->testTimeOffsets( i, getTOffsets( iLowGain )[i] ), iLowGain );
		}
		
		// test pixel status (from pix file in calibration directory)
		if( getChannelStatus()[i] <= 0 )
		{
			setDead( i, 11, iLowGain );
		}
		/////////////////////////////////////////////////////
		// set channels dead from .cfg file
		if( fReader->isMC() )
		{
			// getFullAnaVec()[i]: -1: dead channel, 0: channel does not exist, 1 channel o.k.
			if( fReader->getDataFormatNum() == 1 && getNChannels() >= fReader->getFullAnaVec().size() )
			{
				if( fReader->getFullAnaVec()[i] == 0 )
				{
					setDead( i, 12, iLowGain );
				}
				if( !fRunPar->fMCnoDead && fReader->getFullAnaVec()[i] == -1 )
				{
					setDead( i, 12, iLowGain );
				}
			}
			else if( getNChannels() >= getDetectorGeometry()->getAnaPixel().size() )
			{
				if( getDetectorGeometry()->getAnaPixel()[i] < 1 )
				{
					setDead( i, 12, iLowGain );
				}
			}
		}
	}
	/////////////////////////////////////////////////////
	// read DB pixel values and check values
	if( getDBPixelDataReader() && getDBPixelDataReader()->getDBStatus() )
	{
		// check L1 rates
		vector< unsigned int > iL1Rates_dead = getDBPixelDataReader()->getL1_DeadChannelList( getTelID(), getEventMJD(), getEventTime(),
											   getDeadChannelFinder( iLowGain && getLowGainTOff() )->getDeadChannelDefinition_L1Rates_min(),
											   getDeadChannelFinder( iLowGain && getLowGainTOff() )->getDeadChannelDefinition_L1Rates_max() );
		// do not allow L1 rates to kill a significant part of the camera
		if( iL1Rates_dead.size() < getNChannels() / 2 )
		{
			for( unsigned int i = 0; i < iL1Rates_dead.size(); i++ )
			{
				if( iL1Rates_dead[i] < getNChannels() )
				{
					setDead( iL1Rates_dead[i], 13, iLowGain );
				}
			}
		}
		// check measured HVs
		vector< unsigned int > iHV_dead = getDBPixelDataReader()->getHV_DeadChannelList( getTelID(), getEventMJD(), getEventTime(),
										  getDeadChannelFinder( iLowGain && getLowGainTOff() )->getDeadChannelDefinition_HVrms_min(),
										  getDeadChannelFinder( iLowGain && getLowGainTOff() )->getDeadChannelDefinition_HVrms_max() );
		// do not allow HV to kill a significant part of the camera
		if( iHV_dead.size() < getNChannels() / 2 )
		{
			for( unsigned int i = 0; i < iHV_dead.size(); i++ )
			{
				if( iHV_dead[i] < getNChannels() )
				{
					setDead( iHV_dead[i], 14, iLowGain );
				}
			}
		}
	}
	/////////////////////////////////////////////////////
	
	// check number of dead channels, print warning for more than 30 dead channels
	unsigned int n_dead = 0;
	for( unsigned int i = 0; i < getDead( iLowGain ).size(); i++ )
	{
		if( getDead( iLowGain )[i] )
		{
			if( fDebug )
			{
				cout << "\t dead channel " << i << "\t" << getDead( iLowGain )[i] << endl;
			}
			n_dead++;
		}
	}
	
	if( fDebug )
	{
		printDeadChannels( iLowGain );
	}
	
	setNDead( n_dead, iLowGain );
	
	if( iFirst )
	{
		// print warning if more than 30 channels are dead
		if( getNDead( iLowGain ) > 30 )
		{
			cout << "WARNING: number of dead";
			if( !iLowGain )
			{
				cout << " high gain ";
			}
			else
			{
				cout << " low gain ";
			}
			cout << "channels on telescope " << getTelID() + 1 << " exceeds 30: " << n_dead << endl;
		}
		// print info when less than 30 channels are dead
		else
		{
			cout << "Number of dead";
			if( !iLowGain )
			{
				cout << " high gain ";
			}
			else
			{
				cout << " low gain ";
			}
			cout << "channels on telescope " << getTelID() + 1 << " (at first event):\t" << n_dead << endl;
		}
		//dump list of dead channels
		if( fRunPar->fprintdeadpixelinfo )
		{
			printDeadChannels( iLowGain, true );
		}
		// do not allow to run eventdisplay with all channels dead in the low channels one of the telescopes
		// (GM) why low-gain channels?? Why not all?
		//        if( iLowGain && getNDead( iLowGain ) == getNChannels() )
		if( getNDead( iLowGain ) == getNChannels() )
		{
			cout << "Error: Number of dead channel is comparable to total number of channels" << endl;
			cout << "Exiting..." << endl;
			printDeadChannels( iLowGain );
			exit( EXIT_FAILURE );
		}
	}
	// set random dead channels in camera (unless it's a GrISU run, in which case
	// setting dead here might be double-dipping.  Require fMCNdead > 4 because
	// there will be 4 dead pixels at minimum in a real camera due to the L2
	// triggers used to correct timing jitter on the FADC crates.
	if( iFirst && fRunPar->fMCNdead > 4 && fRunPar->fsourcetype != 4 && fRunPar->fsourcetype != 6 )
	{
		int iN = 0;
		unsigned int iPix = 0;
		int iNstep = 0;
		cout << "Telescope " << getTelID() + 1 << ": setting randomly channels dead (seed " << getAnaData()->getRandomDeadChannelSeed() << ")" << endl;
		do
		{
			iPix = getAnaData()->getRandomDeadChannel();
			// check if this one is alread dead
			if( !( getDead( iLowGain ).at( iPix ) ) )
			{
				setDead( iPix, 10, iLowGain );
				iN++;
			}
			iNstep++;
			// avoid too large loop
			if( iNstep > 10000 )
			{
				cout << "VImageBaseAnalyzer::findDeadChans: too many dead "
					 << "channels, break setting of random dead "
					 << "channels at " << iN << endl;
				break;
			}
		}
		while( iN < fRunPar->fMCNdead - 4 );
	}
}


/*
 *
 * apply timing correction determined from flasher/laser runs
 */
void VImageBaseAnalyzer::timingCorrect()
{
	// apply timing correction to all pulse timing parameters
	const unsigned int nc = getTZeros().size();
	if( nc == getTOffsets().size() )
	{
		for( unsigned int i = 0; i < nc; i++ )
		{
			if( !getDead()[i] )
			{
				setPulseTimingCorrection( i, -1. * getTOffsets()[i] );
			}
		}
	}
}

/*
 * fill a vector with all zero-suppressed channels
 *
 * return number of zero suppressed channels
 */
unsigned int VImageBaseAnalyzer::fillZeroSuppressed()
{
	setZeroSuppressed( false );
	
	int z = 0;
	for( unsigned int i = 0; i < getZeroSuppressed().size(); i++ )
	{
		if( getReader()->isZeroSuppressed( i ) )
		{
			setZeroSuppressed( i, true );
			z++;
		}
	}
	
	return z;
}

unsigned int VImageBaseAnalyzer::fillHiLo()
{
	// reset all channels
	setHiLo( false );
	int z = 0;
	unsigned int chID = 0;
	unsigned int nchannel = getReader()->getNumChannelsHit();
	unsigned int nhilo = getHiLo().size();
	
	for( unsigned int i = 0; i < nchannel; i++ )
	{
		if( getReader()->getHiLo( i ) != 0 )
		{
			getReader()->selectHitChan( i );
			try
			{
				chID = fReader->getHitID( i );
			}
			catch( ... )
			{
				if( getDebugLevel() == 0 )
				{
					cout << "VImageBaseAnalyzer::fillHiLo(), index out of range (fReader->getHitID) " << i;
					cout << "(Telescope " << getTelID() + 1 << ", event " << getEventNumber() << ")" << endl;
					// flag this event
					getAnalysisTelescopeEventStatus()[getTelID()] = 1;
					setDebugLevel( 1 );
				}
				continue;
			}
			if( chID >= nhilo )
			{
				continue;
			}
			setHiLo( chID, true );
			if( !getDead( true )[chID] && !( fReader->getMaxChannels() == 500 && chID == 499 ) )
			{
				z++;
			}
		}
	}
	return z;
}

int VImageBaseAnalyzer::fillSaturatedChannels()
{
	int z = 0;
	for( unsigned int i = 0; i < getTraceN255().size(); i++ )
	{
		if( getTraceN255()[i] > 0 )
		{
			z++;
		}
	}
	return z;
}

/*
 * write list of dead channels into a tree
 *
 * note: this is probably called at the end of the run
 *       the tree will be filled with the dead channel
 *       configuration of the last event
 *       (but the dead channel configuration is time dependent)
 *
 */
TTree* VImageBaseAnalyzer::makeDeadChannelTree()
{
	char htitle[200];
	sprintf( htitle, "channel state (Telescope %d)", getTelID() + 1 );
	TTree* itc = new TTree( "tchannel", htitle );
	UShort_t istat = 0;
	UShort_t istatLow = 0;
	itc->Branch( "state", &istat, "state/s" );
	itc->Branch( "stateLow", &istatLow, "stateLow/s" );
	
	for( unsigned int i = 0; i < getDead().size(); i++ )
	{
		istat = ( UShort_t )getDead()[i];
		if( i < getDead( true ).size() )
		{
			istatLow = ( UShort_t )getDead( true )[i];
		}
		itc->Fill();
	}
	return itc;
}


/*

   FADC integration: second pass for double pass method

   use time gradient of first image to determine window placement

*/
void VImageBaseAnalyzer::calcSecondTZerosSums()
{
	if( fDebug )
	{
		cout << "VImageBaseAnalyzer::calcSecondTZerosSums()" << endl;
	}
	// print lots of output for trace debugging
	bool fDebugTrace = false;
	
	// get number of channels
	unsigned int nhits = fReader->getNumChannelsHit();
	// exclude photodiode from number of channels (would be for VTS channel 500)
	if( nhits > getDead( false ).size() )
	{
		nhits = getDead( false ).size();
	}
	
	// set integration window
	unsigned int iSumWindow = getSumWindow();
	setTCorrectedSumFirst( getSumFirst() );
	// set integration window
	// (depending on the measured integrated charge in first pass)
	// (not implemented yet)
	for( unsigned int i = 0; i < nhits; i++ )
	{
		unsigned int i_channelHitID = 0;
		try
		{
			i_channelHitID = fReader->getHitID( i );
			if( i_channelHitID < getHiLo().size() && i_channelHitID < getDead( getHiLo()[i_channelHitID] ).size() && !getDead( i_channelHitID, getHiLo()[i_channelHitID] ) )
			{
				// -- dynamic integration window is not implemented - function returns fixed window ---
				if( getRunParameter()->fDynamicIntegrationWindow )
				{
					iSumWindow = getDynamicSummationWindow( i_channelHitID );
				}
				int corrlast = getFADCTraceIntegrationPosition( getSumFirst() + iSumWindow );
				setTCorrectedSumLast( i_channelHitID, getFADCTraceIntegrationPosition( getSumFirst() + iSumWindow ) );
				setCurrentSummationWindow( i_channelHitID, getSumFirst(), corrlast, false );
				setCurrentSummationWindow( i_channelHitID, getSumFirst(),
										   getFADCTraceIntegrationPosition( getSumFirst() + ( int )getSumWindow_2() ), true );
			}
		}
		catch( ... )
		{
			if( getDebugLevel() == 0 )
			{
				cout << "VAnalyzer::calcSecondTZerosSums(), dynamical summation window, index out of range (fReader->getHitID) ";
				cout << i << "(Telescope " << getTelID() + 1 << ", event " << getEventNumber() << ")" << endl;
				setDebugLevel( 0 );
			}
			continue;
		}
	}
	
	// (re)initialize arrays
	setSums( 0. );
	setSums2( 0. );
	setImage( false );
	setBorder( false );
	setBrightNonImage( false );
	setImageBorderNeighbour( false );
	
	float xpmt = 0.;
	float ypmt = 0.;
	float xpos = 0;
	float xtime = 0.;
	int corrfirst = 0;
	
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// loop over all hit channel
	///////////////////////////////////////////////////////////////////////////////////////////////////
	for( unsigned int i = 0; i < nhits; i++ )
	{
		unsigned int i_channelHitID = 0;
		try
		{
			i_channelHitID = fReader->getHitID( i );
			// dead channels are excluded
			if( i_channelHitID < getHiLo().size() && i_channelHitID < getDead( getHiLo()[i_channelHitID] ).size() && !getDead( i_channelHitID, getHiLo()[i_channelHitID] ) )
			{
				// trace handler
				fTraceHandler->setTrace( fReader, getNSamples(), getPeds( getHiLo()[i_channelHitID] )[i_channelHitID],
										 getPedrms( getHiLo()[i_channelHitID] )[i_channelHitID], i_channelHitID, i,
										 getLowGainMultiplier_Trace( )*getHiLo()[i_channelHitID] );
				fTraceHandler->setTraceIntegrationmethod( getTraceIntegrationMethod() );
				
				//////////////////////////////////////////////////////////////////////////////////////////////////
				// calculate start of integration window (window 1 of pass 2)
				//
				// double pass method: use time gradient along the long-axis of the image to calculate expected T0
				//////////////////////////////////////////////////////////////////////////////////////////////////
				// position of the PMT in camera coordinates
				xpmt = getDetectorGeo()->getX()[i_channelHitID] - getImageParameters()->cen_x;
				ypmt = getDetectorGeo()->getY()[i_channelHitID] - getImageParameters()->cen_y;
				// position along the major axis of the image
				xpos = xpmt * getImageParameters()->cosphi + ypmt * getImageParameters()->sinphi;
				// fit time along the major axis of the image (hardwired check!)
				if( abs( getImageParameters()->tgrad_x ) < 200 )
				{
					xtime = getImageParameters()->tgrad_x * xpos + getImageParameters()->tint_x;
				}
				else
				{
					xtime = getSumFirst();
				}
				// fit times are corrected for TOffsets and FADCStopOffsets.
				// undo this to get integration window in FADC trace.
				corrfirst = ( int )( xtime + getTOffsets()[i_channelHitID] - getFADCStopOffsets()[i_channelHitID] + getSumWindowShift() );
				if( fDebugTrace )
				{
					cout << "2ndFITcorrfirst " << getTelID() + 1 << ", CH " << i_channelHitID;
					cout << " : fit " << xtime << ", toff " << getTOffsets()[i_channelHitID];
					cout << ", sumwindowshift " << getSumWindowShift() << " corrfirst " << corrfirst << endl;
				}
				// no success in determining start of integration window
				// (usually happens when first pass of image cleaning returned no image/border pixels)
				if( corrfirst < getSumFirst() )
				{
					// take average tzero per telescope and pixel
					//                   corrfirst = (int)(getAverageTZeros()[i_channelHitID]-0.5 + getTOffsets()[i_channelHitID] - getFADCStopOffsets()[i_channelHitID]);
					// take average tzero per telescope (more stable than previous statement)
					corrfirst = ( int )( getMeanAverageTZero() - 0.5 + getTOffsets()[i_channelHitID]
										 - getFADCStopOffsets()[i_channelHitID] + getSumWindowShift() );
					if( fDebugTrace )
					{
						cout << "2ndIgnoreFit " << getTelID() + 1 << ", CH " << i_channelHitID;
						cout << " : mean " << getMeanAverageTZero() << ", corrfirst " << corrfirst << endl;
					}
				}
				///////////////////
				// low gain channel have different timing
				// (not for CTA-DSTs)
				//
				if( getHiLo()[i_channelHitID] && getRunParameter()->fsourcetype != 7 )
				{
					// integrate low-gain pulse only if prediction window start is before the end of the readout window
					if( corrfirst < ( int )getNSamples() )
					{
						// get new tzero for sumwindow starting at corrfirst to the end of the window
						// assume that high and low gain timing is not more than 5 samples off
						if( getSumWindowMaxTimeDifferenceLGtoHG() > -998. )
						{
							corrfirst += getSumWindowMaxTimeDifferenceLGtoHG();
						}
						else
						{
							corrfirst = 0;
						}
						try
						{
							float iT0 = fTraceHandler->getPulseTiming( corrfirst, getNSamples(), corrfirst, getNSamples() ).at( getRunParameter()->fpulsetiming_tzero_index );
							if( fTraceHandler->getPulseTimingStatus() )
							{
								if( corrfirst - iT0 < getSumWindowMaxTimedifferenceToDoublePassPosition() )
								{
									corrfirst = TMath::Nint( iT0 ) + getSumWindowShift();
								}
							}
							else
							{
								corrfirst = getNSamples();
							}
						}
						catch( const std::out_of_range& oor )
						{
							cout << "VImageBaseAnalyzer::calcSecondTZerosSums: out of Range error: " << oor.what() << endl;
						}
						if( corrfirst < 0 )
						{
							corrfirst = 0;
						}
					}
				}
				////////////////////
				// high gain channel
				// integration start might be before sample 0 -> set to average t0
				else if( corrfirst < 0 )
				{
					unsigned int isw = getDynamicSummationWindow( i_channelHitID );
					if( -1 * corrfirst > ( int )isw )
					{
						isw = 0;
					}
					else
					{
						isw += corrfirst;
					}
					// take average tzero per telescope and pixel
					//                      corrfirst = (int)(getAverageTZeros()[i_channelHitID]-0.5 + getTOffsets()[i_channelHitID] - getFADCStopOffsets()[i_channelHitID]);
					// take average tzero per telescope (more stable than previous statement)
					corrfirst = getFADCTraceIntegrationPosition(
									getMeanAverageTZero() - 0.5 + getTOffsets()[i_channelHitID] - getFADCStopOffsets()[i_channelHitID] + getSumWindowShift() );
					setCurrentSummationWindow( i_channelHitID, corrfirst, isw, false );
				}
				// integration start is at beyond last sample: set to last sample
				corrfirst = getFADCTraceIntegrationPosition( corrfirst );
				// set start of integration window (sum window 1)
				setTCorrectedSumFirst( i_channelHitID, corrfirst );
				
				//////////////////////////////////////////////////////////////////////////////////
				// calculate end of integration window
				// (depends on user settings and length of readout window)
				//////////////////////////////////////////////////////////////////////////////////
				//                int corrlast = getFADCTraceIntegrationPosition( corrfirst + getCurrentSumWindow()[i_channelHitID] );
				int corrlast = getFADCTraceIntegrationPosition( corrfirst + getDynamicSummationWindow( i_channelHitID ) );
				setTCorrectedSumLast( i_channelHitID, corrlast );
				// set current summation window
				setCurrentSummationWindow( i_channelHitID, corrfirst, corrlast, false );
				
				///////////////////////////////////
				// calculate pulse sums
				///////////////////////////////////
				// sum for first summation window
				setSums( i_channelHitID, fTraceHandler->getTraceSum( corrfirst, corrlast, fRaw ) * getLowGainSumCorrection( fRunPar->fsumwindow_1[fTelID], corrlast - corrfirst, getHiLo()[i_channelHitID] ) );
				if( getFillPulseSum() )
				{
					getAnaData()->fillPulseSum( i_channelHitID, getSums()[i_channelHitID], getHiLo()[i_channelHitID] );
				}
				// sum for second summation window
				if( getRunParameter()->fFixWindowStart_sumwindow2 )
				{
					corrfirst = getFADCTraceIntegrationPosition( getSumFirst() + getTOffsets()[i_channelHitID] - getFADCStopOffsets()[i_channelHitID] );
					corrlast  = getFADCTraceIntegrationPosition( getSumFirst() + ( int )getSumWindow_2() );
					setSums2( i_channelHitID, fTraceHandler->getTraceSum( getSumFirst(), corrlast, fRaw ) * getLowGainSumCorrection( fRunPar->fsumwindow_2[fTelID], corrlast - getSumFirst(), getHiLo()[i_channelHitID] ) );
				}
				corrlast = getFADCTraceIntegrationPosition( corrfirst + ( int )getSumWindow_2() );
				setSums2( i_channelHitID, fTraceHandler->getTraceSum( corrfirst, corrlast, fRaw ) * getLowGainSumCorrection( fRunPar->fsumwindow_2[fTelID], corrlast - corrfirst, getHiLo()[i_channelHitID] ) );
				setCurrentSummationWindow( i_channelHitID, corrfirst, corrlast, true );
				
			}
		}
		catch( ... )
		{
			if( getDebugLevel() == 0 )
			{
				cout << "VAnalyzer::calcSecondTZerosSums(), index out of range (fReader->getHitID) ";
				cout << i << "(Telescope " << getTelID() + 1 << ", event " << getEventNumber() << ")" << endl;
				setDebugLevel( 0 );
			}
			continue;
		}
	}
}

/*

    determine length of integration window depending on pulse size of first integration pass

    NOT IMPLEMENTED YET

    (function returns unchanged summations windows)

*/
unsigned int VImageBaseAnalyzer::getDynamicSummationWindow( unsigned int i_channelHitID )
{
	// low gain: return larger integration window (from size2)
	if( getHiLo()[i_channelHitID] )
	{
		return getSumWindow_2();
	}
	// for integrated pulses below a certain threshold: return smallest dynamical window
	if( getSums()[i_channelHitID] < 0 )
	{
		return getSumWindow();
	}
	
	// calculate integration window
	// (not implemented yet)
	
	return getSumWindow();
}

/*

   make sure that integration window position is insided the readout window

*/
int VImageBaseAnalyzer::getFADCTraceIntegrationPosition( int iPos )
{
	if( iPos < 0 )
	{
		return 0;
	}
	if( iPos > ( int )getNSamples() )
	{
		return ( int )getNSamples();
	}
	
	return iPos;
}

/*
 *
 * set trace and parameters for trace integration methods
 *
 */
void VImageBaseAnalyzer::initializeTrace( bool iMakingPeds, unsigned int i_channelHitID, unsigned int i, unsigned int iTraceIntegrationMethod )
{
	double i_LG = getLowGainMultiplier_Trace() * getHiLo()[i_channelHitID];
	
	// sum for pedestal analysis
	// (set all low-gain sums to
	// zero)
	if( iMakingPeds )
	{
		i_LG = 0.;
	}
	
	// set trace
	fTraceHandler->setTrace( fReader,
							 getNSamples(),
							 getPeds( getHiLo()[i_channelHitID] )[i_channelHitID],
							 getPedrms( getHiLo()[i_channelHitID] )[i_channelHitID],
							 i_channelHitID,
							 i,
							 i_LG );
	// make sure that trace integration is set (important for pedestal calculations in QADC runs)
	if( iTraceIntegrationMethod < 9999 )
	{
		fTraceHandler->setTraceIntegrationmethod( iTraceIntegrationMethod );
	}
	else if( iTraceIntegrationMethod == 9999 && getTraceIntegrationMethod() != 0 )
	{
		fTraceHandler->setTraceIntegrationmethod( getTraceIntegrationMethod() );
	}
	// default trace integration method
	else
	{
		fTraceHandler->setTraceIntegrationmethod( 1 );
	}
	
}

