// VDB_PixelDataReader reading pixel wise data from DB
//

#ifndef VDB_PixelDataReader_H
#define VDB_PixelDataReader_H

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <TH1F.h>
#include <TSQLResult.h>
#include <TSQLRow.h>
#include <TSQLServer.h>
#include <TTree.h>

#include "VDB_Connection.h"
#include "VSkyCoordinatesUtilities.h"

using namespace std;

class VDB_PixelData
{
  private:


  public:

    string  fDataType;
    float   fTimeBinWidth_s;                    // typical interval of readout [s]
    vector< float > fMJD;                       // one entry per time bin
    vector< float > fsec_of_day;                // one entry per time bin
    vector< float > fData;                      // one entry per time bin

    VDB_PixelData( string iDataType = "" );
   ~VDB_PixelData() {};
    void         print();
};

//////////////

class VDB_PixelDataReader
{
    private:

    bool  fDebug;
    bool  fDBStatus;
    
    vector< float > fDummyReturnVector;

    vector< string > fPixelDataType;
    vector< vector< vector< VDB_PixelData* > > > fPixelData;   // array [ndatatype][ntel][npixel]
    vector< vector< TH1F* > >  fPixelData_histogram;

    void fillDataRow( unsigned int iDataType, string iTimeStamp, int iTel, int iPix, float iData );
    vector< unsigned int > getDeadChannelList( unsigned int iDataType, unsigned int iTel, int iMJD, float iTime, float i_min, float i_max );
    vector< float > getDataVector( unsigned int iDataType, unsigned int iTel, int iMJD, float iTime );
    TH1F*           getDataHistogram( unsigned int iDataType, unsigned int iTel, int iMJD, float iTime );
    float           getValue( unsigned int iDataType, unsigned int iTel, unsigned int iChannel, int iMJD, float iTime );

    public:

    VDB_PixelDataReader( vector< unsigned int > nPixel_per_telescope );    
   ~VDB_PixelDataReader() {};
    bool   getDBStatus() { return fDBStatus; }
    vector< unsigned int > getL1_DeadChannelList( unsigned int iTel, int iMJD, float iTime, float i_min, float i_max ) 
                                                  { return getDeadChannelList( 0, iTel, iMJD, iTime, i_min, i_max ); }
    vector< unsigned int > getHV_DeadChannelList( unsigned int iTel, int iMJD, float iTime, float i_min, float i_max ) 
                                                  { return getDeadChannelList( 1, iTel, iMJD, iTime, i_min, i_max ); }
    vector< unsigned int > getCurrents_DeadChannelList( unsigned int iTel, int iMJD, float iTime, float i_min, float i_max ) 
                                                  { return getDeadChannelList( 2, iTel, iMJD, iTime, i_min, i_max ); }
    TH1F*           getL1Histogram( unsigned int iTel, int iMJD, float iTime ) { return getDataHistogram( 0, iTel, iMJD, iTime ); }
    TH1F*           getHVHistogram( unsigned int iTel, int iMJD, float iTime ) { return getDataHistogram( 1, iTel, iMJD, iTime ); }
    TH1F*           getCurrentsHistogram( unsigned int iTel, int iMJD, float iTime ) { return getDataHistogram( 2, iTel, iMJD, iTime ); }
    float           getL1Rate( unsigned int iTel, unsigned int iChannel, int iMJD, float iTime ) { return getValue( 0, iTel, iChannel, iMJD, iTime ); }
    float           getHV( unsigned int iTel, unsigned int iChannel, int iMJD, float iTime ) { return getValue( 1, iTel, iChannel, iMJD, iTime ); }
    float           getCurrent( unsigned int iTel, unsigned int iChannel, int iMJD, float iTime ) { return getValue( 2, iTel, iChannel, iMJD, iTime ); }
    vector< float > getL1Rates( unsigned int iTel, int iMJD, float iTime ) { return getDataVector( 0, iTel, iMJD, iTime ); }
    vector< float > getHV( unsigned int iTel, int iMJD, float iTime ) { return getDataVector( 1, iTel, iMJD, iTime ); }
    vector< float > getCurrents( unsigned int iTel, int iMJD, float iTime ) { return getDataVector( 2, iTel, iMJD, iTime ); }
    void   print();
    void   print( string iDatatype, unsigned int iTelID, unsigned int iPixel );
    bool   readFromDB( string DBServer, unsigned int runNumber );
    void   setDebug( bool iB = false ) { fDebug = iB; }
    bool   writeDataTree( unsigned int iTel );
};



#endif
