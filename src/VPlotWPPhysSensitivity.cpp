/*! \class VPlotWPPhysSensitivity

*/

#include "VPlotWPPhysSensitivity.h"

VPlotWPPhysSensitivity::VPlotWPPhysSensitivity()
{
   fIRF = 0;
}


void VPlotWPPhysSensitivity::addAnalysis( string iAnalysis, int iColor, int iLineStyle )
{
   fAnalysis.push_back( iAnalysis );
   fAnalysisColor.push_back( iColor );
   fAnalysisLineStyle.push_back( iLineStyle );
}

void VPlotWPPhysSensitivity::addCameraOffset( double iCameraOffset_deg )
{
   fCameraOffset_deg.push_back( iCameraOffset_deg );
}

void VPlotWPPhysSensitivity::addObservationTime( double iObsTime )
{
   fObservationTime_H.push_back( iObsTime );
}

void VPlotWPPhysSensitivity::addSubArray( string iArray )
{
   fSubArray.push_back( iArray );
}

bool VPlotWPPhysSensitivity::initialize()
{
   char hname[200];
   fSensitivityFile.clear();
   fLegend.clear();
   fPlottingColor.clear();
   fPlottingLineStyle.clear();

   for( unsigned int a = 0; a < fSubArray.size(); a++ )
   {
      for( unsigned int t = 0; t < fObservationTime_H.size(); t++ )
      {
	 for( unsigned int i = 0; i < fAnalysis.size(); i++ )
	 {
	    ostringstream iTemp;
	    if( fAnalysis[i] == "DESY" )
	    {
	       sprintf( hname, "%.1fh", fObservationTime_H[t] );
	       iTemp << "DESY_20111123/DESY." << fSubArray[a] << "." << hname << ".root";
	    }
	    else if( fAnalysis[i] == "ISDC" )
	    {
	       sprintf( hname, "%.1f", fObservationTime_H[t] );
	       iTemp << "ISDC/ISDC_2000m_KonradB_optimal_"  << fSubArray[a] << "_" << hname;
	       iTemp << "h_20deg_20110615.root";
	    }
	    else if( fAnalysis[i] == "IFAE" )
	    {
	       if( fObservationTime_H[t] > 1. ) sprintf( hname, "%d", (int)fObservationTime_H[t] );
	       else                             sprintf( hname, "%.1f", fObservationTime_H[t] );
	       iTemp << "IFAEPerformanceBCDEINANB_Nov2011/Subarray" << fSubArray[a];
	       iTemp << "_IFAE_" << hname << "hours_20111109.root";
	    }
	    else if( fAnalysis[i] == "IFAE_OFFAXIS" )
	    {
	       if( fObservationTime_H[t] > 1. ) sprintf( hname, "%d", (int)fObservationTime_H[t] );
	       else                             sprintf( hname, "%.1f", fObservationTime_H[t] );
	       iTemp << "IFAEOffaxisPerformanceBEI_Nov2011/Subarray" << fSubArray[a];
	       iTemp << "_IFAE_" << hname << "hours_20111121_offaxis.root";
            }
	    else if( fAnalysis[i] == "HD_KB" || fAnalysis[i] == "MPIK" )
	    {
	       if( fObservationTime_H[t] > 1. ) sprintf( hname, "%d", (int)fObservationTime_H[t] );
	       else                             sprintf( hname, "%.1f", fObservationTime_H[t] );
	       iTemp << "data_KB/kb_" << fSubArray[a];
	       iTemp << "_" << hname << "h_20deg_v3.root";
            }
	    else if( fAnalysis[i] == "ParisMVA" )
	    {
	       if( fObservationTime_H[t] > 1. ) sprintf( hname, "%d", (int)fObservationTime_H[t] );
	       else                             sprintf( hname, "%.1f", fObservationTime_H[t] );
	       iTemp << "ParisMVA/Subarray" << fSubArray[a];
	       iTemp << "_ParisMVA_" << hname << "hours.root";
            }
	    else 
	    {
	        cout << "VPlotWPPhysSensitivity::initialize() warning: unknown analysis: " << fAnalysis[i] << endl;
	        continue;
            }
	    if( fCameraOffset_deg.size() == 0 )
	    {
	       fSensitivityFile.push_back( iTemp.str() );
	       sprintf( hname, "%s (%s, %.1f h)", fSubArray[a].c_str(), fAnalysis[i].c_str(), fObservationTime_H[t] );
	       fLegend.push_back( hname );
	       fPlottingColor.push_back( fAnalysisColor[i] );
	       if( fAnalysisLineStyle[i] > 0 ) fPlottingLineStyle.push_back( fAnalysisLineStyle[i] );
	       else                            fPlottingLineStyle.push_back( t+1 );
	       fIRFCameraOffset_deg.push_back( 0. );
            }
	    else
	    {
	       for( unsigned int c = 0; c < fCameraOffset_deg.size(); c++ )
	       {
		  fSensitivityFile.push_back( iTemp.str() );
		  if( fCameraOffset_deg.size() == 1 )
		  {
		     sprintf( hname, "%s (%s, %.1f h)", fSubArray[a].c_str(), fAnalysis[i].c_str(), fObservationTime_H[t] );
                  }
		  else
		  {
	             sprintf( hname, "%s (%s, %.1f h, %.1f deg)", fSubArray[a].c_str(), fAnalysis[i].c_str(), fObservationTime_H[t], fCameraOffset_deg[c] );
                  }
		  fLegend.push_back( hname );
		  fPlottingColor.push_back( fAnalysisColor[i] );
		  if( fAnalysisLineStyle[i] > 0 ) fPlottingLineStyle.push_back( fAnalysisLineStyle[i] );
		  else                            fPlottingLineStyle.push_back( t+1 );
		  fIRFCameraOffset_deg.push_back( fCameraOffset_deg[c] );
               }
            }
         }
      }
   }

// print data sets
   for( unsigned int i = 0; i < fSensitivityFile.size(); i++ )
   {
      cout << fSensitivityFile[i] << "\t" << fPlottingColor[i] << "\t" << fPlottingLineStyle[i] << "(" << fLegend[i] << ")" << endl;
   }

   return true;
}
  
bool VPlotWPPhysSensitivity::plotIRF()
{
    fIRF = new VPlotInstrumentResponseFunction();

    fIRF->setCanvasSize( 400, 400 );
    fIRF->setPlottingAxis( "energy_Lin", "X", true, 0.01, 200, "energy [TeV]" );
    fIRF->setPlottingAxis( "effarea_Lin", "X", true, 50., 5.e7 );
    fIRF->setPlottingAxis( "energyresolution_Lin", "X", false, 0., 0.7 );

    for( unsigned int i = 0; i < fSensitivityFile.size(); i++ )
    {
       fIRF->addInstrumentResponseData( fSensitivityFile[i], 20., fIRFCameraOffset_deg[i], 0, 2.4, 200, "A_MC", 
                                        fPlottingColor[i], fPlottingLineStyle[i], 21, 0.5 );
    }

    TCanvas *c = 0;

    c = fIRF->plotEffectiveArea();
    plotLegend( c, true );
    c = fIRF->plotAngularResolution();
    plotLegend( c, false );
    c = fIRF->plotAngularResolution( "energy", "80" );
    plotLegend( c, false );
    c = fIRF->plotEnergyResolution( 0.5 );
    plotLegend( c, false );

    return true;
}

bool VPlotWPPhysSensitivity::plotLegend( TCanvas *c, bool iDown )
{
   if( !c ) return false;
   c->cd();

   double x = 0.2+0.35;
   double y = 0.65;
   if( iDown ) y -= 0.5;
   TLegend *iL = new TLegend( x, y, x+0.30, y+0.22 );
   iL->SetFillColor( 0 );

   for( unsigned int i = 0; i < fLegend.size(); i++ )
   {
      TGraph *g = new TGraph( 1 );
      g->SetLineColor( fPlottingColor[i] );
      g->SetLineStyle( fPlottingLineStyle[i] );
      g->SetMarkerStyle( 1 );
      iL->AddEntry( g, fLegend[i].c_str(), "l" );
   }
   iL->Draw();
   return true; 
}

bool VPlotWPPhysSensitivity::plotSensitivity()
{
   string iCrabFile = "$EVNDISPDATA/AstroData/TeV_data/EnergySpectrum_literatureValues_CrabNebula.dat";
   unsigned int iCrabID = 6;

   TCanvas *cSens = 0;
   TCanvas *cBck = 0;
   for( unsigned int i = 0; i < fSensitivityFile.size(); i++ )
   {
      VSensitivityCalculator *a = new VSensitivityCalculator();
      a->setMonteCarloParametersCTA_MC( fSensitivityFile[i], fIRFCameraOffset_deg[i], iCrabFile, iCrabID );
      a->setEnergyRange_Lin( 0.01, 200. );
      a->setPlotCanvasSize( 900, 600 );
      a->setPlottingStyle( fPlottingColor[i], fPlottingLineStyle[i], 2., 1 );
      TCanvas *c_temp = a->plotDifferentialSensitivityvsEnergyFromCrabSpectrum( cSens, "CTA-PHYS", fPlottingColor[i], "ENERGY", 0.2, 0.01 );
      if( c_temp ) cSens = c_temp;
      if( i == 0 ) c_temp = a->plotSignalBackgroundRates( cBck, true );   // plot protons and electrons
      else         c_temp = a->plotSignalBackgroundRates( cBck, false );
      if( c_temp ) cBck = c_temp;
   }
   if( cSens ) plotLegend( cSens, false );
   if( cBck )  plotLegend( cBck, false );

   return true;
}
