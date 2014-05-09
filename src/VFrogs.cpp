/*********************************************
 *
 * EVENDISPLAY - FROGS INTERFACE Ver. 0.0
 *
 *********************************************/

/*! /class VFrogs

\brief  Called from VEventLoop. Opens templates and does minimization.

\author S Vincent, G Hughes.

*/

#include "VFrogs.h"
#include "frogs.h"

#define FROGSDEBUG 0

VFrogs::VFrogs()
{

	fFrogParameters = new VFrogParameters();
	frogsRecID = getRunParameter()->ffrogsRecID;
    templatelistname = getRunParameter()->ffrogstemplatelist ;
	
	fInitialized = false;
	
}

VFrogs::~VFrogs()
{

}

//================================================================
//================================================================
void VFrogs::doFrogsStuff( int eventNumber )
{

	int i, j;
	
	// only at first call in the analysis run: initialize data class, set trees
	if( !fInitialized )
	{
		initAnalysis();
		fInitialized = true;
		readTableFrogs();
		fStartEnergyLoop = 0;
	}
	
	initFrogEvent();
	
	int adc = 2;
	double inEnergy = getFrogsStartEnergy( eventNumber );
	
	// Get Anasum Event Number for this Run
	///int AnasumNumber = getFrogsAnasumNumber(eventNumber,fRunPar->frunnumber);
	
	//Store data from the GrISU analysis to a FROGS structure
	//In this example the function frogs_convert_from_grisu takes
	//arguments corresponding the the GrISU analysis. An equivalent
	//function should be written for any other analysis package to
	//make the FROGS analysis usable.
	////if ( inEnergy != FROGS_BAD_NUMBER && AnasumNumber != FROGS_BAD_NUMBER ) {
	if( inEnergy != FROGS_BAD_NUMBER )
	{
	
		struct frogs_imgtmplt_in d;
		d = frogs_convert_from_ed( eventNumber, adc, inEnergy );
		
		//Print out the data contained in the FROGS structure frogs_imgtmplt_in
		//This is useful when developing a frogs_convert_from_XXXX function
		
		if( FROGSDEBUG )
		{
			frogs_print_raw_event( d );
		}
		
		//Call the FROGS analysis
		struct frogs_imgtmplt_out output;
		//output = frogs_img_tmplt( &d );
		char templatelistnamecstr[FROGS_FILE_NAME_MAX_LENGTH] ;
        int maxchar     = FROGS_FILE_NAME_MAX_LENGTH - 1 ;
        
        // 'formatbuff' is so we only put the first "FROGS_FILE_NAME_MAX_LENGTH-1" characters of the templatelistname string into the char array 'templatelistnamecstr'
        char formatbuff[20] ;
        sprintf( formatbuff, "%%.%ds", maxchar ) ; 
        sprintf( templatelistnamecstr, formatbuff, templatelistname.c_str() ) ;
        
		output = frogs_img_tmplt( &d, templatelistnamecstr );
		
		frogsEventID     = output.event_id;
		frogsGSLConStat  = output.gsl_convergence_status;
		frogsNB_iter     = output.nb_iter;
		frogsNImages     = output.nb_images;
		frogsXS          = output.cvrgpt.xs;
		frogsXSerr       = output.cvrgpterr.xs;
		frogsYS          = output.cvrgpt.ys;
		frogsYSerr       = output.cvrgpterr.ys;
		frogsXP          = output.cvrgpt.xp;
		frogsXPerr       = output.cvrgpterr.xp;
		frogsYP          = output.cvrgpt.yp;
		frogsYPerr       = output.cvrgpterr.yp;
		frogsXPGC	     = transformShowerPosition( 90. - fData->getShowerParameters()->fTelElevation[0], fData->getShowerParameters()->fTelAzimuth[0], output.cvrgpt.xp, output.cvrgpt.yp, 0 );
		frogsYPGC	     = transformShowerPosition( 90. - fData->getShowerParameters()->fTelElevation[0], fData->getShowerParameters()->fTelAzimuth[0], output.cvrgpt.xp, output.cvrgpt.yp, 1 );
		frogsEnergy      = output.cvrgpt.log10e;
		frogsEnergyerr   = output.cvrgpterr.log10e;
		frogsLambda      = output.cvrgpt.lambda;
		frogsLambdaerr   = output.cvrgpterr.lambda;
		frogsGoodnessImg = output.goodness_img;
		frogsNpixImg     = output.npix_img;
		frogsGoodnessBkg = output.goodness_bkg;
		frogsNpixBkg     = output.npix_bkg;
		
		for( j = 0; j < 4; j++ )
		{
			frogsTelGoodnessImg[j] = output.tel_goodnessImg[j];
			frogsTelGoodnessBkg[j] = output.tel_goodnessBkg[j];
		}
		
		for( j = 0; j < 4; j++ )
		{
			for( i = 0; i < 500; i++ )
			{
				if( j == 0 )
				{
					frogsTemplateMu0[i] = output.tmplt_tubes[j][i];
				}
				if( j == 1 )
				{
					frogsTemplateMu1[i] = output.tmplt_tubes[j][i];
				}
				if( j == 2 )
				{
					frogsTemplateMu2[i] = output.tmplt_tubes[j][i];
				}
				if( j == 3 )
				{
					frogsTemplateMu3[i] = output.tmplt_tubes[j][i];
				}
			}
		}
		
		frogsXPStart     = getShowerParameters()->fShowerXcore_SC[frogsRecID];
		frogsYPStart     = getShowerParameters()->fShowerYcore_SC[frogsRecID];
		frogsXPED        = getShowerParameters()->fShowerXcore[frogsRecID];
		frogsYPED        = getShowerParameters()->fShowerYcore[frogsRecID];
		frogsXSStart     = fData->getShowerParameters()->fShower_Xoffset[frogsRecID]; //TEMP GH
		//frogsXSStart     = getShowerParameters()->fShower_Xoffset[frogsRecID];
		frogsYSStart     = -1.0 * fData->getShowerParameters()->fShower_Yoffset[frogsRecID];
		
		getFrogParameters()->frogsEventID = getFrogsEventID();
		getFrogParameters()->frogsGSLConStat = getFrogsGSLConStat();
		getFrogParameters()->frogsNB_iter = getFrogsNB_iter();
		getFrogParameters()->frogsNImages = getFrogsNImages();
		getFrogParameters()->frogsXS = getFrogsXS();
		getFrogParameters()->frogsXSerr = getFrogsXSerr();
		getFrogParameters()->frogsYS = getFrogsYS();
		getFrogParameters()->frogsYSerr = getFrogsYSerr();
		getFrogParameters()->frogsXP = getFrogsXP();
		getFrogParameters()->frogsXPerr = getFrogsXPerr();
		getFrogParameters()->frogsYP = getFrogsYP();
		getFrogParameters()->frogsXPGC = getFrogsXPGC();
		getFrogParameters()->frogsYPGC = getFrogsYPGC();
		getFrogParameters()->frogsYPerr = getFrogsYPerr();
		getFrogParameters()->frogsEnergy = getFrogsEnergy();
		getFrogParameters()->frogsEnergyerr = getFrogsEnergyerr();
		getFrogParameters()->frogsLambda = getFrogsLambda();
		getFrogParameters()->frogsLambdaerr = getFrogsLambdaerr();
		getFrogParameters()->frogsGoodnessImg = getFrogsGoodnessImg();
		getFrogParameters()->frogsNpixImg = getFrogsNpixImg();
		getFrogParameters()->frogsGoodnessBkg = getFrogsGoodnessBkg();
		getFrogParameters()->frogsNpixBkg = getFrogsNpixBkg();
		
		getFrogParameters()->frogsXPStart = getFrogsXPStart();
		getFrogParameters()->frogsYPStart = getFrogsYPStart();
		getFrogParameters()->frogsXPED = getFrogsXPED();
		getFrogParameters()->frogsYPED = getFrogsYPED();
		getFrogParameters()->frogsXSStart = getFrogsXSStart();
		getFrogParameters()->frogsYSStart = getFrogsYSStart();
		
		getFrogParameters()->frogsTelGoodnessImg0 = getFrogsTelGoodnessImg( 0 );
		getFrogParameters()->frogsTelGoodnessImg1 = getFrogsTelGoodnessImg( 1 );
		getFrogParameters()->frogsTelGoodnessImg2 = getFrogsTelGoodnessImg( 2 );
		getFrogParameters()->frogsTelGoodnessImg3 = getFrogsTelGoodnessImg( 3 );
		getFrogParameters()->frogsTelGoodnessBkg0 = getFrogsTelGoodnessBkg( 0 );
		getFrogParameters()->frogsTelGoodnessBkg1 = getFrogsTelGoodnessBkg( 1 );
		getFrogParameters()->frogsTelGoodnessBkg2 = getFrogsTelGoodnessBkg( 2 );
		getFrogParameters()->frogsTelGoodnessBkg3 = getFrogsTelGoodnessBkg( 3 );
		
		getFrogParameters()->getTree()->Fill();
		
		valarray<double> a0( frogsTemplateMu0, 500 );
		valarray<double> a1( frogsTemplateMu1, 500 );
		valarray<double> a2( frogsTemplateMu2, 500 );
		valarray<double> a3( frogsTemplateMu3, 500 );
		for( j = 0; j < 4; j++ )
		{
			setTelID( j );
			if( j == 0 )
			{
				fData->setTemplateMu( a0 );
			}
			if( j == 1 )
			{
				fData->setTemplateMu( a1 );
			}
			if( j == 2 )
			{
				fData->setTemplateMu( a2 );
			}
			if( j == 3 )
			{
				fData->setTemplateMu( a3 );
			}
		}
		
		//Print out the results of the image template analysis
		//The values returned by frogs_img_tmplt really are to be stored in
		//structures used in the analysis package in which FROGS is being sed.
		if( FROGSDEBUG )
		{
			frogs_print_param_spc_point( output );
		}
		
	}
	//return FROGS_OK;
	return;
}
//================================================================
//================================================================
void VFrogs::initFrogEvent()
{
}
//================================================================
//================================================================
void VFrogs::initAnalysis()
{
	initOutput();
	initFrogTree();
}
//================================================================
//================================================================
void VFrogs::initOutput()
{
	if( fDebug )
	{
		cout << "void VFrogs::initOuput()" << endl;
	}
	// check if root outputfile exist
	if( fOutputfile != 0 )
	{
		printf( "FROGPUT: Frogs Output File Exists\n" );
		return;
	}
	// otherwise create it
	if( fRunPar->foutputfileName != "-1" )
	{
		printf( "FROGPUT: Frogs foutputfileName = -1 : Attempt to create file\n" );
		char i_textTitle[300];
		sprintf( i_textTitle, "VERSION %d", getRunParameter()->getEVNDISP_TREE_VERSION() );
		if( getRunParameter()->fShortTree )
		{
			sprintf( i_textTitle, "%s (short tree)", i_textTitle );
		}
		fOutputfile = new TFile( fRunPar->foutputfileName.c_str(), "RECREATE", i_textTitle );
	}
	
	if( FROGSDEBUG )
	{
		printf( "FROGPUT Are we initOutput??\n" );
	}
	
}
//================================================================
//================================================================
void VFrogs::initFrogTree()
{

	if( FROGSDEBUG )
	{
		printf( "FROGPUT Check in initTree\n" );
	}
	
	char i_text[300];
	char i_textTitle[300];
	sprintf( i_text, "frogspars" );
	// tree versioning numbers used in mscw_energy
	sprintf( i_textTitle, "FROGPUT: Frogs Parameters (VERSION %d)\n", getRunParameter()->getEVNDISP_TREE_VERSION() );
	if( getRunParameter()->fShortTree )
	{
		sprintf( i_textTitle, "%s (short tree)", i_textTitle );
	}
	fFrogParameters->initTree( i_text, i_textTitle );
	
}
//================================================================
//================================================================
void VFrogs::readTableFrogs()
{

	int eventNumber;
	double Erec;
	
	string fmscwFrogsFile = getRunParameter()->ffrogsmscwfile;
	cout << "FROGS readTableFrogs " << getRunParameter()->ffrogsmscwfile.c_str() << endl;
	
	TFile* mscwFrogsFile = new TFile( fmscwFrogsFile.c_str() , "READ" );
	
	if( mscwFrogsFile->IsZombie() )
	{
		cout << "VFrogs::readTableFrogs error: File " << fmscwFrogsFile.c_str() << " does not exist!" << endl;
		exit( -1 );
	}
	
	TTree* mscwTreeFrogs = ( TTree* )mscwFrogsFile->Get( "data" );
	mscwTreeFrogs->SetBranchAddress( "eventNumber", &eventNumber );
	mscwTreeFrogs->SetBranchAddress( "Erec", &Erec );
	
	for( int i = 0 ; i < mscwTreeFrogs->GetEntries() ; i++ )
	{
		mscwTreeFrogs->GetEntry( i );
		fTableRunNumber.push_back( eventNumber );
		fTableEnergy.push_back( Erec );
	}
	
	mscwFrogsFile->Close();
	
	cout << "Finished Reading: " << fTableRunNumber.size() << " with size " << fTableEnergy.size() << endl;
	
}
//================================================================
//================================================================
double VFrogs::getFrogsStartEnergy( int eventNumber )
{

	int mscwTotal = fTableRunNumber.size();
	
	for( int i = fStartEnergyLoop ; i < mscwTotal ; i++ )
	{
		if( eventNumber == fTableRunNumber[i] )
		{
			fStartEnergyLoop = i - 1;
			return fTableEnergy[i];
		}
		else if( eventNumber < fTableRunNumber[i] )
		{
			return FROGS_BAD_NUMBER;
		}
	}
	return FROGS_BAD_NUMBER;
	
}
//================================================================
//================================================================
int VFrogs::getFrogsAnasumNumber( int eventNumber, int runNumber )
{

	int AnasumTotal = fAnasumRunNumber.size();
	
	for( int i = 0 ; i < AnasumTotal ; i++ )
	{
		if( runNumber == fAnasumRunNumber[i] && eventNumber == fAnasumEventNumber[i] )
		{
			return eventNumber;
		}
	}
	
	return FROGS_BAD_NUMBER;
	
}
//================================================================
//================================================================
float VFrogs::transformTelescopePosition( int iTel, float i_ze, float i_az, int axis )
{
	// transform telescope positions from ground into shower coordinates
	float i_xrot, i_yrot, i_zrot;
	float i_xcos = 0.;
	float i_ycos = 0.;
	
	// calculate direction cosine
	i_xcos = sin( i_ze / TMath::RadToDeg() ) * sin( ( i_az - 180. ) / TMath::RadToDeg() );
	i_ycos = sin( i_ze / TMath::RadToDeg() ) * cos( ( i_az - 180. ) / TMath::RadToDeg() );
	
	setTelID( iTel );
	// call to GrIsu routine
	tel_impact( i_xcos, i_ycos, getDetectorGeo()->getTelXpos()[iTel], getDetectorGeo()->getTelYpos()[iTel], getDetectorGeo()->getTelZpos()[iTel], &i_xrot, &i_yrot, &i_zrot, false );
	
	if( axis == 0 )
	{
		return i_xrot;
	}
	else if( axis == 1 )
	{
		return i_yrot;
	}
	else
	{
		return FROGS_BAD_NUMBER;
	}
}
//================================================================
//================================================================
float VFrogs::transformShowerPosition( float i_ze, float i_az, float xcore, float ycore, int axis )
{
	// transform coordinates from shower coord system to ground coord
	// see also void VArrayAnalyzer::transformTelescopePosition in VArrayAnalyzer.cpp
	// and void VGrIsuAnalyzer::tel_impact in VGrIsuAnalyzer.cpp
	float i_xrot, i_yrot, i_zrot;
	float i_xcos = 0.;
	float i_ycos = 0.;
	
	// calculate direction cosine
	i_xcos = sin( i_ze / TMath::RadToDeg() ) * sin( ( i_az - 180. ) / TMath::RadToDeg() );
	i_ycos = sin( i_ze / TMath::RadToDeg() ) * cos( ( i_az - 180. ) / TMath::RadToDeg() );
	
	// call to GrIsu routine
	// the parameter true/false sets the transformation matrix
	// false: ground coordinate system -> shower coord system
	// true: the inverse matrix is ON. shower coord -> ground coordinate
	tel_impact( i_xcos, i_ycos, xcore, ycore, 0., &i_xrot, &i_yrot, &i_zrot, true );
	
	if( axis == 0 )
	{
		return i_xrot;
	}
	else if( axis == 1 )
	{
		return i_yrot;
	}
	else
	{
		return FROGS_BAD_NUMBER;
	}
}
//================================================================
//================================================================
float VFrogs::transformPosition( float i_ze, float i_az, float x, float y, float z, int axis, bool bInv )
{
	// transform coordinates from shower coord system to ground coord
	// see also void VArrayAnalyzer::transformTelescopePosition in VArrayAnalyzer.cpp
	// and void VGrIsuAnalyzer::tel_impact in VGrIsuAnalyzer.cpp
	float i_xrot, i_yrot, i_zrot;
	float i_xcos = 0.;
	float i_ycos = 0.;
	
	// calculate direction cosine
	i_xcos = sin( i_ze / TMath::RadToDeg() ) * sin( ( i_az - 180. ) / TMath::RadToDeg() );
	i_ycos = sin( i_ze / TMath::RadToDeg() ) * cos( ( i_az - 180. ) / TMath::RadToDeg() );
	
	// call to GrIsu routine
	// the parameter true/false sets the transformation matrix
	// false: ground coordinate system -> shower coord system
	// true: the inverse matrix is ON. shower coord -> ground coordinate
	tel_impact( i_xcos, i_ycos, x, y, z, &i_xrot, &i_yrot, &i_zrot, bInv );
	
	if( axis == 0 )
	{
		return i_xrot;
	}
	else if( axis == 1 )
	{
		return i_yrot;
	}
	else if( axis == 2 )
	{
		return i_zrot;
	}
	else
	{
		return FROGS_BAD_NUMBER;
	}
}
//================================================================
//================================================================
void VFrogs::terminate()
{

	getFrogParameters()->getTree()->Write();
	
}

void VFrogs::finishFrogs( TFile* f )
{

	// Open outfile again to copy frogs tree to mscw file.
	
	// reopen mscw file
	string fmscwFrogsFile = getRunParameter()->ffrogsmscwfile;
	TFile* mscwFrogsFile = new TFile( fmscwFrogsFile.c_str() , "UPDATE" );
	if( mscwFrogsFile->IsZombie() )
	{
		cout << "Finish Frogs:" << endl;
		cout << "VFrogs::readTableFrogs error: File " << fmscwFrogsFile.c_str() << " does not exist!" << endl;
		exit( -1 );
	}
	
	// Clone tree to mscw file checking it opened
	if( f->IsZombie() )
	{
		cout << "error: finish Frogs f file problem: "  << endl;
	}
	else
	{
		( ( TTree* )f->Get( "frogspars" ) )->CloneTree()->Write();
	}
	
	// Close the files
	mscwFrogsFile->Close();
	
	return;
	
}
//================================================================
//================================================================
struct frogs_imgtmplt_in VFrogs::frogs_convert_from_ed( int eventNumber, int adc_type, double inEnergy )
{
	/* The frogs_convert_from_grisu function is called with
	   arguments containing all the data necessary to the image template
	   analysis as per the structures used in grisu analysis. It can serve
	   as an example for the interfacing of the template analysis with other
	   analysis packages. It returns the data necessary to the template
	   analysis in a structure that is appropriate.  */
	
	struct frogs_imgtmplt_in rtn;
	
	//Tracked elevation from telescope 0
	rtn.elevation = fData->getShowerParameters()->fTelElevation[0];
	rtn.azimuth   = fData->getShowerParameters()->fTelAzimuth[0];
	rtn.event_id  = eventNumber;
	
	//Telescopes
	rtn.ntel = fData->getNTel(); //Number of telescopes
	
	rtn.scope = new struct frogs_telescope [rtn.ntel];
	rtn.nb_live_pix_total = 0; //Total number or pixels in use
	for( int tel = 0; tel < rtn.ntel; tel++ )
	{
		initializeDataReader();
		setTelID( tel );
		
		//Telescope position in the shower coordinate coordinate system used in the reconstruction
		//rtn.scope[tel].xfield =
		//transformTelescopePosition( tel, 90. - fData->getShowerParameters()->fTelElevation[0], fData->getShowerParameters()->fTelAzimuth[0], 0 );
		//rtn.scope[tel].yfield =
		//transformTelescopePosition( tel, 90. - fData->getShowerParameters()->fTelElevation[0], fData->getShowerParameters()->fTelAzimuth[0], 1 );
		rtn.scope[tel].xfield =
			transformPosition( 90. - fData->getShowerParameters()->fTelElevation[0], fData->getShowerParameters()->fTelAzimuth[0], getDetectorGeo()->getTelXpos()[tel], getDetectorGeo()->getTelYpos()[tel], getDetectorGeo()->getTelZpos()[tel], 0, false );
		rtn.scope[tel].yfield =
			transformPosition( 90. - fData->getShowerParameters()->fTelElevation[0], fData->getShowerParameters()->fTelAzimuth[0], getDetectorGeo()->getTelXpos()[tel], getDetectorGeo()->getTelYpos()[tel], getDetectorGeo()->getTelZpos()[tel], 1, false );
		rtn.scope[tel].zfield =
			transformPosition( 90. - fData->getShowerParameters()->fTelElevation[0], fData->getShowerParameters()->fTelAzimuth[0], getDetectorGeo()->getTelXpos()[tel], getDetectorGeo()->getTelYpos()[tel], getDetectorGeo()->getTelZpos()[tel], 2, false );
			
			
		if( FROGSDEBUG )
			printf( "TelSC %d | %.2f %.2f | %.2f %.2f | %.2f %.2f | %.2f %.2f | %.2f %.2f \n", tel,
					fData->getShowerParameters()->fShowerZe[frogsRecID],
					fData->getShowerParameters()->fShowerAz[frogsRecID],
					getDetectorGeo()->getTelXpos()[tel] - fData->getShowerParameters()->fShowerXcore[frogsRecID],
					getDetectorGeo()->getTelYpos()[tel] - fData->getShowerParameters()->fShowerYcore[frogsRecID],
					getDetectorGeo()->getTelXpos()[tel],
					getDetectorGeo()->getTelYpos()[tel],
					rtn.scope[tel].xfield,
					rtn.scope[tel].yfield,
					( 180.0 / 3.14159265 )*atan2( rtn.scope[tel].yfield, rtn.scope[tel].xfield ),
					( 180.0 / 3.14159265 )*atan2( getDetectorGeo()->getTelYpos()[tel], getDetectorGeo()->getTelXpos()[tel] ) );
					
		//Telescope effective collection area
		//Number of pixels
		rtn.scope[tel].npix = fData->getDetectorGeo()->getNChannels()[tel];
		
		//Set the dimension of the pixel parameter arrays
		rtn.scope[tel].xcam = new float [rtn.scope[tel].npix];
		rtn.scope[tel].ycam = new float [rtn.scope[tel].npix];
		rtn.scope[tel].q = new float [rtn.scope[tel].npix];
		rtn.scope[tel].ped = new float [rtn.scope[tel].npix];
		rtn.scope[tel].exnoise = new float [rtn.scope[tel].npix];
		rtn.scope[tel].pixinuse = new int [rtn.scope[tel].npix];
		rtn.scope[tel].telpixarea = new float [rtn.scope[tel].npix];
		rtn.scope[tel].pixradius = new float [rtn.scope[tel].npix]; //(SV)
		float foclen = 1000.0 * fData->getDetectorGeo()->getFocalLength()[tel]; //(SV) Focal length in mm
		
		//Initialize the number of live pixel in the telescope
		rtn.scope[tel].nb_live_pix = 0;
		
		//Loop on the pixels
		for( int pix = 0; pix < rtn.scope[tel].npix; pix++ )
		{
			//Pixel coordinates
			rtn.scope[tel].xcam[pix] = fData->getDetectorGeo()->getX()[pix];
			rtn.scope[tel].ycam[pix] = fData->getDetectorGeo()->getY()[pix];
			
			//Excess noise
			rtn.scope[tel].exnoise[pix] = extra_noise;
			//Pixel dead or alive
			rtn.scope[tel].pixinuse[pix] = FROGS_OK;
			//(GH) modify to remove LG pixels
			//if(fData->getDead()[pix]!=0)
			//if(fData->getDead()[pix]!=0 || fData->getData()->getHiLo()[pix]==1 )
			if( fData->getDead( fData->getData()->getHiLo()[pix] )[pix] != 0 )//(SV)
			{
				rtn.scope[tel].pixinuse[pix] = FROGS_NOTOK;
			}
			//Increment the number of live pixels
			if( rtn.scope[tel].pixinuse[pix] == FROGS_OK )
			{
				rtn.scope[tel].nb_live_pix++;
			}
			//Pixel effective collecting area in square degrees
			float tmppixarea = fData->getDetectorGeo()->getTubeRadius_MM( tel )[pix] * FROGS_DEG_PER_RAD / foclen; //(SV)
			tmppixarea = FROGS_PI * tmppixarea * tmppixarea;
			rtn.scope[tel].telpixarea[pix] = telarea * tmppixarea * cone_eff;
			//Pixel radius in degree
			rtn.scope[tel].pixradius[pix] = fData->getDetectorGeo()->getTubeRadius_MM( tel )[pix] * FROGS_DEG_PER_RAD / foclen; //(SV)
			
			//Initialize the pixel signal and pedestal width to zero
			rtn.scope[tel].q[pix] = 0;
			rtn.scope[tel].ped[pix] = 0;
			//Set them to their values in p.e. if the d.c./p.e. factor is non zero
			if( dc2pe != 0 )
			{
				rtn.scope[tel].q[pix] = fData->getData()->getSums2()[pix] / dc2pe;
				//rtn.scope[tel].ped[pix]=
				//fData->getData()->getPedvars( fData->getData()->getHiLo()[pix], fData->getCurrentSumWindow_2()[pix] )[pix]*fData->getData()->getLowGainMultiplier()[pix]*frogs_pedwidth_correction/dc2pe;
				
				//rtn.scope[tel].q[pix]=fData->getData()->getSums()[pix]/dc2pe;
				if( fData->getData()->getHiLo()[pix] == 1 )
				{
					//rtn.scope[tel].q[pix]=fData->getData()->getSums()[pix]*fData->getData()->getLowGainMultiplier()[pix]/dc2pe;
					//rtn.scope[tel].q[pix]=fData->getData()->getSums()[pix]/dc2pe; //(SV): getLowGainMultiplier removed
					//rtn.scope[tel].ped[pix]=fData->getData()->getPedvars(true,18)[pix]*frogs_pedwidth_correction/dc2pe;
					rtn.scope[tel].ped[pix] =
						fData->getData()->getPedvars( fData->getData()->getHiLo()[pix], fData->getCurrentSumWindow_2()[pix] )[pix] / dc2pe;
				}
				else
				{
					//rtn.scope[tel].q[pix]=fData->getData()->getSums()[pix]/dc2pe;
					//rtn.scope[tel].ped[pix]=fData->getData()->getPedvars(false,18)[pix]*frogs_pedwidth_correction/dc2pe;
					//rtn.scope[tel].ped[pix]=fData->getData()->getPedvars()[pix]*frogs_pedwidth_correction/dc2pe;
					rtn.scope[tel].ped[pix] =
						fData->getData()->getPedvars( fData->getData()->getHiLo()[pix], fData->getCurrentSumWindow_2()[pix] )[pix] / dc2pe;
				}
			}
		}
		//Total number of live pixels in the array
		rtn.nb_live_pix_total = rtn.nb_live_pix_total + rtn.scope[tel].nb_live_pix;
	}
	
	//Optimization starting point todo y -> -y ??
	rtn.startpt.xs = 1.0 * fData->getShowerParameters()->fShower_Xoffset[frogsRecID]; //(SV) starting points set to ED parameters
	rtn.startpt.ys = -1.0 * fData->getShowerParameters()->fShower_Yoffset[frogsRecID]; //(SV) starting points set to ED parameters
	
	//rtn.startpt.xs=1.0*fData->getShowerParameters()->MCTel_Xoff; //(SV) starting points set to MC parameters
	//rtn.startpt.ys=-1.0*fData->getShowerParameters()->MCTel_Yoff; //(SV) starting points set to MC parameters
	
	rtn.startpt.xp = fData->getShowerParameters()->fShowerXcore_SC[frogsRecID]; //(SV) starting points set to ED parameters
	rtn.startpt.yp = fData->getShowerParameters()->fShowerYcore_SC[frogsRecID]; //(SV) starting points set to ED parameters
	
	//rtn.startpt.xp=fData->getShowerParameters()->MCxcore_SC; //(SV) starting points set to MC parameters
	//rtn.startpt.yp=fData->getShowerParameters()->MCycore_SC; //(SV) starting points set to MC parameters
	
	//rtn.startpt.xp=fData->getShowerParameters()->MCxcore; //(SV) starting points set to MC parameters (Ground Coord)
	//rtn.startpt.yp=fData->getShowerParameters()->MCycore; //(SV) starting points set to MC parameters (Ground Coord)
	
	if( FROGSDEBUG )
	{
		cout << " eventNumber  " << eventNumber << endl;
		cout << " ED: ShowerSC " << fData->getShowerParameters()->fShowerXcore_SC[frogsRecID] << " " << fData->getShowerParameters()->fShowerYcore_SC[frogsRecID] << endl;
		cout << " ED: Shower   " << fData->getShowerParameters()->fShowerXcore[frogsRecID]    << " " << fData->getShowerParameters()->fShowerYcore[frogsRecID] << endl;
		cout << " MC: ShowerSC " << fData->getShowerParameters()->MCxcore_SC                  << " " << fData->getShowerParameters()->MCycore_SC << " " << fData->getShowerParameters()->MCzcore_SC << endl;
		cout << " MC: Shower   " << fData->getShowerParameters()->MCxcore                     << " " << fData->getShowerParameters()->MCycore    << " " << fData->getShowerParameters()->MCzcore << endl;
	}
	
	rtn.startpt.lambda = 1.0; //(SV) We use a fixed value by lack of information.
	
	rtn.startpt.log10e = inEnergy; //(SV) inEnergy from ED
	if( rtn.startpt.log10e > 0.0 )
	{
		rtn.startpt.log10e = log10( rtn.startpt.log10e );
	}
	else
	{
		rtn.startpt.log10e = FROGS_BAD_NUMBER;
	}
	
	//rtn.startpt.log10e = log10(fData->getShowerParameters()->MCenergy); //(SV) starting points set to MC parameters (inEnergy from MC)
	
	//Decides if the event is worth analysing.
	rtn.worthy_event = FROGS_OK;
	//Log(0.06)=-1.2; Log(0.1)=-1; Log(0.15)=-0.824; Log(0.2)=-0.699; Log(0.25)=-0.602
	//Log(0.3)=-0.523; Log(0.35)=-0.456; Log(0.4)=-0.398; Log(30)=1.477
	//Energy large enough?
	if( rtn.startpt.log10e < -1.20 )
	{
		rtn.worthy_event = FROGS_NOTOK;
	}
	//Energy small enough?
	//if(rtn.startpt.log10e>1.0)   rtn.worthy_event=FROGS_NOTOK;
	if( rtn.startpt.log10e > 1.470 )
	{
		rtn.worthy_event = FROGS_NOTOK;
	}
	//Distance of the impact point small enough?
	if( sqrt( rtn.startpt.xp * rtn.startpt.xp + rtn.startpt.yp * rtn.startpt.yp ) > 450.0 )
	{
		rtn.worthy_event = FROGS_NOTOK;
	}
	//Count the number of telescopes with more than 300dc in their image
	int ngoodimages = 0;
	for( int tel = 0; tel < rtn.ntel; tel++ )
	{
		setTelID( tel );
		if( fData->getImageParameters()->size > 200.0 )
		{
			ngoodimages = ngoodimages + 1;
		}
	}
	//Require the number of telescopes with more than 300dc to be at least 3
	if( ngoodimages < 3 )
	{
		rtn.worthy_event = FROGS_NOTOK;
	}
	//Require the image to be fully contained into the camera
	//if (fData->getImageParameters()->loss>0.) rtn.worthy_event=FROGS_NOTOK;
	
	if( rtn.worthy_event == FROGS_OK )
	{
		return rtn;
	}
	
#ifdef DIFF_EVOLUTION
	//=======================================
	//    diff. evolution algorithm
	//=======================================
	//Decides if the event is worth analysing
	rtn.worthy_event = FROGS_NOTOK;
	if( fabs( rtn.startpt.log10e-FROGS_BAD_NUMBER ) < 1E-8 )
	{
	
		//Distance of the impact point small enough?
		double dummy = sqrt( rtn.startpt.xp * rtn.startpt.xp + rtn.startpt.yp * rtn.startpt.yp );
		
		//Count the number of telescopes with more than 200dc in their image
		ngoodimages = 0;
		for( int tel = 0; tel < rtn.ntel; tel++ )
		{
			setTelID( tel );
			if( fData->getImageParameters()->size > 200.0 )
			{
				ngoodimages = ngoodimages + 1;
			}
		}
		
		//Require the number of telescopes with more than 200dc to be at least 2
		if( ngoodimages > 1 && dummy < 200.0 )
		{
			rtn.worthy_event = FROGS_OK;
		}
	}
	//=======================================
	//    diff. evolution algorithm
	//=======================================
#endif //DIFF_EVOLUTION
	
	return rtn;
	
}
