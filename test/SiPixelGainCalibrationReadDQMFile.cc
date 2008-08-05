// -*- C++ -*-
//
// Package:    SiPixelGainCalibrationReadDQMFile
// Class:      SiPixelGainCalibrationReadDQMFile
// 
/**\class SiPixelGainCalibrationReadDQMFile SiPixelGainCalibrationReadDQMFile.cc CalibTracker/SiPixelGainCalibrationReadDQMFile/src/SiPixelGainCalibrationReadDQMFile.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Freya BLEKMAN
//         Created:  Tue Aug  5 16:22:46 CEST 2008
// $Id$
//
//


// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelCalibConfiguration.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelGainCalibration.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelGainCalibrationOffline.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelGainCalibrationForHLT.h"
#include "CondTools/SiPixel/interface/SiPixelGainCalibrationService.h"
#include "CondCore/DBOutputService/interface/PoolDBOutputService.h"


#include "TH2F.h"
//
// class decleration
//

class SiPixelGainCalibrationReadDQMFile : public edm::EDAnalyzer {
   public:
      explicit SiPixelGainCalibrationReadDQMFile(const edm::ParameterSet&);
      ~SiPixelGainCalibrationReadDQMFile();


   private:
      virtual void beginJob(const edm::EventSetup&) ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;
  // functions added by F.B.
  void fillDatabase();
  void getHistograms();
      // ----------member data ---------------------------
  edm::ParameterSet conf_;
  std::map<uint32_t,std::map<std::string, TH2F *> > bookkeeper_; 
  std::string  recordName_;
  bool appendMode_;
  SiPixelGainCalibration *theGainCalibrationDbInput_;
  SiPixelGainCalibrationOffline *theGainCalibrationDbInputOffline_;
  SiPixelGainCalibrationForHLT *theGainCalibrationDbInputHLT_;
  SiPixelGainCalibrationService theGainCalibrationDbInputService_;
  // keep track of lowest and highest vals for range
  float gainlow_;
  float gainhi_;
  float pedlow_;
  float pedhi_;
  std::string rootfilestring_;
  
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//


void SiPixelGainCalibrationReadDQMFile::fillDatabase(){
 // only create when necessary.
  // process the minimum and maximum gain & ped values...


  if(gainlow_>gainhi_){  
    float temp=gainhi_;
    gainhi_=gainlow_;
    gainlow_=temp;
  }
  if(pedlow_>pedhi_){  
    float temp=pedhi_;
    pedhi_=pedlow_;
    pedlow_=temp;
  }
 
  theGainCalibrationDbInput_ = new SiPixelGainCalibration(pedlow_,pedhi_,gainlow_,gainhi_);
  theGainCalibrationDbInputHLT_ = new SiPixelGainCalibrationForHLT(pedlow_,pedhi_,gainlow_,gainhi_);
  theGainCalibrationDbInputOffline_ = new SiPixelGainCalibrationOffline(pedlow_,pedhi_,gainlow_,gainhi_);

  uint32_t nchannels=0;
  uint32_t nmodules=0;
  //  std::cout << "now starting loop on detids" << std::endl;
  uint32_t detid=0;
  for(std::map<uint32_t,std::map<std::string, TH2F *> >::const_iterator idet=bookkeeper_.begin(); idet!= bookkeeper_.end(); ++idet){
    if(detid==idet->first)
      continue;
    detid=idet->first;
    edm::LogInfo("SiPixelGainCalibrationReadDQMFile") << "now creating database object for detid " << detid << std::endl;
    // Get the module sizes.

    int nrows = bookkeeper_[detid]["gain_2d"]->GetNbinsY();
    int ncols = bookkeeper_[detid]["gain_2d"]->GetNbinsX();   

    std::vector<char> theSiPixelGainCalibrationPerPixel;
    std::vector<char> theSiPixelGainCalibrationPerColumn;
    std::vector<char> theSiPixelGainCalibrationGainPerColPedPerPixel;
    
    // Loop over columns and rows of this DetID
    //    std::cout <<" now starting loop over pixels..." << std::endl;
    for(int i=1; i<=ncols; i++) {
      float pedforthiscol=0;
      float gainforthiscol=0;
      int nusedrows=0;
      //      std::cout << "now lookign at col " << i << std::endl;
      for(int j=1; j<=nrows; j++) {
	nchannels++;
	float ped = bookkeeper_[detid]["ped_2d"]->GetBinContent(i,j);
	float gain = bookkeeper_[detid]["gain_2d"]->GetBinContent(i,j);
	
	//	std::cout << "looking at pixel row,col " << j << ","<<i << " gain,ped=" <<gain << "," << ped << std::endl;
	// and fill and convert in the SiPixelGainCalibration object:
	theGainCalibrationDbInput_->setData(ped,gain,theSiPixelGainCalibrationPerPixel);
	theGainCalibrationDbInputOffline_->setDataPedestal(ped, theSiPixelGainCalibrationGainPerColPedPerPixel);
	//	std::cout <<"done with database filling..." << std::endl;

	pedforthiscol+=ped;
	gainforthiscol+=gain;
	nusedrows++;
      }
      if(nusedrows>0){
	pedforthiscol/=nusedrows;
	gainforthiscol/=nusedrows;
      }
      //      std::cout << "filling objects..." << std::endl;
      theGainCalibrationDbInputOffline_->setDataGain(gainforthiscol,nrows,theSiPixelGainCalibrationGainPerColPedPerPixel);
      theGainCalibrationDbInputHLT_->setData(pedforthiscol,gainforthiscol,theSiPixelGainCalibrationPerColumn);
	
    }

    //    std::cout << "setting range..." << std::endl;
    SiPixelGainCalibration::Range range(theSiPixelGainCalibrationPerPixel.begin(),theSiPixelGainCalibrationPerPixel.end());
    SiPixelGainCalibrationForHLT::Range hltrange(theSiPixelGainCalibrationPerColumn.begin(),theSiPixelGainCalibrationPerColumn.end());
    SiPixelGainCalibrationOffline::Range offlinerange(theSiPixelGainCalibrationGainPerColPedPerPixel.begin(),theSiPixelGainCalibrationGainPerColPedPerPixel.end());
    
    //    std::cout <<"putting things in db..." << std::endl;
    // now start creating the various database objects
    if( !theGainCalibrationDbInput_->put(detid,range,ncols) )
      edm::LogError("SiPixelGainCalibrationReadDQMFile")<<"warning: detid already exists for Pixel-level calibration database"<<std::endl;
    if( !theGainCalibrationDbInputOffline_->put(detid,offlinerange,ncols) )
      edm::LogError("SiPixelGainCalibrationReadDQMFile")<<"warning: detid already exists for Offline (gain per col, ped per pixel) calibration database"<<std::endl;
    if(!theGainCalibrationDbInputHLT_->put(detid,hltrange) )
      edm::LogError("SiPixelGainCalibrationReadDQMFile")<<"warning: detid already exists for HLT (pedestal and gain per column) calibration database"<<std::endl;
  }
  
  edm::LogInfo("SiPixelGainCalibrationReadDQMFile") << " ---> PIXEL Modules  " << nmodules  << "\n"
						 << " ---> PIXEL Channels " << nchannels << std::endl;

  edm::LogInfo(" --- writing to DB!");
  edm::Service<cond::service::PoolDBOutputService> mydbservice;
  if(!mydbservice.isAvailable() ){
    edm::LogError("db service unavailable");
    return;
    if( mydbservice->isNewTagRequest(recordName_) ){
      
      mydbservice->createNewIOV<SiPixelGainCalibration>(
							theGainCalibrationDbInput_, 
							mydbservice->beginOfTime(),
							mydbservice->endOfTime(),
							recordName_);
      
      mydbservice->createNewIOV<SiPixelGainCalibrationForHLT>(
							   theGainCalibrationDbInputHLT_,
							   mydbservice->beginOfTime(),
							   mydbservice->endOfTime(),
							   recordName_);
      
      mydbservice->createNewIOV<SiPixelGainCalibrationOffline>(
							       theGainCalibrationDbInputOffline_,
							       mydbservice->beginOfTime(),
							       mydbservice->endOfTime(),
							       recordName_);
      
    } 
    else {
      
      mydbservice->appendSinceTime<SiPixelGainCalibration>(
							   theGainCalibrationDbInput_, 
							   mydbservice->currentTime(),
							   recordName_);
      
      mydbservice->appendSinceTime<SiPixelGainCalibrationForHLT>(
							   theGainCalibrationDbInputHLT_, 
							   mydbservice->currentTime(),
							   recordName_);
      
      mydbservice->appendSinceTime<SiPixelGainCalibrationOffline>(
							   theGainCalibrationDbInputOffline_, 
							   mydbservice->currentTime(),
							   recordName_);
    }
    edm::LogInfo(" --- all OK");
  } 
}

SiPixelGainCalibrationReadDQMFile::SiPixelGainCalibrationReadDQMFile(const edm::ParameterSet& iConfig):
  conf_(iConfig),
  rootfilestring_(conf_.getUntrackedParameter<std::string>("rootfile","inputfile.root")),
  recordName_(conf_.getParameter<std::string>("record")),
  appendMode_(conf_.getUntrackedParameter<bool>("appendMode",true)),
  theGainCalibrationDbInput_(0),
  theGainCalibrationDbInputOffline_(0),
  theGainCalibrationDbInputHLT_(0),
  theGainCalibrationDbInputService_(iConfig),
  gainlow_(10.),gainhi_(0.),pedlow_(255.),pedhi_(0.)
  
  
{
   //now do what ever initialization is needed
  ::putenv("CORAL_AUTH_USER=me");
  ::putenv("CORAL_AUTH_PASSWORD=test");   

}


SiPixelGainCalibrationReadDQMFile::~SiPixelGainCalibrationReadDQMFile()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to for each event  ------------
void
SiPixelGainCalibrationReadDQMFile::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  // empty but should not be called anyway
}


// ------------ method called once each job just before starting event loop  ------------
void 
SiPixelGainCalibrationReadDQMFile::beginJob(const edm::EventSetup&)
{
  //   this is where all the analysis is done
  
}

// ------------ method called once each job just after ending the event loop  ------------
void 
SiPixelGainCalibrationReadDQMFile::endJob() {
  getHistograms();
  fillDatabase();
}
void 
SiPixelGainCalibrationReadDQMFile::getHistograms(){

}
//define this as a plug-in
DEFINE_FWK_MODULE(SiPixelGainCalibrationReadDQMFile);
