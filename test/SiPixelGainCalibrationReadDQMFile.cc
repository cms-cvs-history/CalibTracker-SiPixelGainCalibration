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
// $Id: SiPixelGainCalibrationReadDQMFile.cc,v 1.7 2008/08/18 10:49:27 fblekman Exp $
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

#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/CommonTopologies/interface/PixelTopology.h"

#include "TH2F.h"
#include "TFile.h"
#include "TDirectory.h"
#include "TKey.h"
#include "TString.h"
#include "TList.h"

#include "CalibTracker/SiPixelGainCalibration/test/SiPixelGainCalibrationReadDQMFile.h"
#include "PhysicsTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
//
// class decleration
//


void SiPixelGainCalibrationReadDQMFile::fillDatabase(const edm::EventSetup& iSetup){
 // only create when necessary.
  // process the minimum and maximum gain & ped values...
  std::cout << "now starting db fill!!!" << std::endl;
  
  std::map<uint32_t,std::pair<TString,int> > badresults;
  TFile *histofile = new TFile("temphistofile.root","recreate");
  histofile->cd();
  bool usedmean=false;
  bool isdead=false;
  TH1F *VCAL_endpoint = new TH1F("VCAL_endpoint","value where response = 255 ( x = (255 - ped)/gain )",256,0,256);
  
  size_t ntimes=0;
  std::cout << "now filling record " << record_ << std::endl;
  if(record_!="SiPixelGainCalibrationForHLTRcd" && record_!="SiPixelGainCalibrationOfflineRcd"){
    std::cout << "you passed record " << record_ << ", which I have no idea what to do with!" << std::endl;
    return;
  }
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
  if(gainhi_>gainmax_)
    gainhi_=gainmax_;
  if(pedhi_>pedmax_)
    pedhi_=pedmax_;
  float badpedval=pedlow_-200;
  float badgainval=gainlow_-200;
  std::cout << "now filling db: values: pedlow, hi: " << pedlow_ << ", " << pedhi_ << " and gainlow, hi: " << gainlow_ << ", " << gainhi_ ;
  float meangain= meanGainHist_->GetMean();
  float meanped = meanPedHist_->GetMean();
  std::cout << ", and mean gain " << meangain<< ", ped " << meanped ;
  std::cout << std::endl;
  
  // and fill the dummy histos:
  
  for(size_t icol=0; icol<nmaxcols;++icol){
    for(size_t irow=0; irow<nmaxrows; ++irow){
      defaultGain_->SetBinContent(icol+1,irow+1,meangain);
      defaultPed_->SetBinContent(icol+1,irow+1,meanped);
      defaultChi2_->SetBinContent(icol+1,irow+1,1.0);
    }
  }
  theGainCalibrationDbInputHLT_ = new SiPixelGainCalibrationForHLT(pedlow_*0.999,pedhi_*1.001,gainlow_*0.999,gainhi_*1.001);
  theGainCalibrationDbInputOffline_ = new SiPixelGainCalibrationOffline(pedlow_*0.999,pedhi_*1.001,gainlow_*0.999,gainhi_*1.001);

  
  uint32_t nchannels=0;
  uint32_t nmodules=0;
  std::cout << "now starting loop on detids, there are " << bookkeeper_.size() << " histograms to consider..." << std::endl;
  uint32_t detid=0;
  therootfile_->cd();
  edm::ESHandle<TrackerGeometry> pDD;
  iSetup.get<TrackerDigiGeometryRecord>().get( pDD );     
  edm::LogInfo("SiPixelCondObjOfflineBuilder") <<" There are "<<pDD->dets().size() <<" detectors"<<std::endl;
  
  for(TrackerGeometry::DetContainer::const_iterator it = pDD->dets().begin(); it != pDD->dets().end(); it++){
    detid=0;
    if( dynamic_cast<PixelGeomDetUnit*>((*it))!=0)
      detid=((*it)->geographicalId()).rawId();
    if(detid==0)
      continue;
    ntimes=0;
    //    std::cout << "now creating database object for detid " << detid << " " << bookkeeper_[detid]["gain_2d"] << " " << bookkeeper_[detid]["ped_2d"] << std::endl; //std::cout<< " nrows:" << nrows << " ncols: " << ncols << std::endl;

    // Get the module sizes.
    TString tempchi2string = bookkeeper_[detid]["chi2prob_2d"];
    TH2F *tempchi2 = (TH2F*)therootfile_->Get(tempchi2string);
    if(tempchi2==0)
      tempchi2=defaultChi2_;
    TString tempgainstring = bookkeeper_[detid]["gain_2d"];
    TH2F *tempgain = (TH2F*)therootfile_->Get(tempgainstring);
    if(tempgain==0){
      //      std::cout <<"WARNING, gain histo " << bookkeeper_[detid]["gain_2d"] << " does not exist, using default instead" << std::endl;
      tempgain=defaultGain_;   
    }
    TString temppedstring = bookkeeper_[detid]["ped_2d"];
    TH2F *tempped = (TH2F*) therootfile_->Get(temppedstring);
    if(tempped==0){
      //      std::cout <<"WARNING, ped histo " << bookkeeper_[detid]["ped_2d"] << " for detid " << detid << " does not exist, using default instead" << std::endl;
      std::pair<TString,int> tempval(tempgainstring,0);
      badresults[detid]=tempval;
      tempped=defaultPed_;
    }
    const PixelGeomDetUnit * pixDet  = dynamic_cast<const PixelGeomDetUnit*>((*it));
    const PixelTopology & topol = pixDet->specificTopology();       
    // Get the module sizes.
    size_t nrows = topol.nrows();      // rows in x
    size_t ncols = topol.ncolumns();   // cols in y
    //    int nrows=tempgain->GetNbinsY();
    //    int ncols=tempgain->GetNbinsX();
    //    std::cout << "next histo " << tempgain->GetTitle() << " has nrow,ncol:" << nrows << ","<< ncols << std::endl;
    size_t nrowsrocsplit = theGainCalibrationDbInputHLT_->getNumberOfRowsToAverageOver();
    if(theGainCalibrationDbInputOffline_->getNumberOfRowsToAverageOver()!=nrowsrocsplit)
      throw  cms::Exception("GainCalibration Payload configuration error")
	<< "[SiPixelGainCalibrationAnalysis::fillDatabase] ERROR the SiPixelGainCalibrationOffline and SiPixelGainCalibrationForHLT database payloads have different settings for the number of rows per roc: " << theGainCalibrationDbInputHLT_->getNumberOfRowsToAverageOver() << "(HLT), " << theGainCalibrationDbInputOffline_->getNumberOfRowsToAverageOver() << "(offline)";
    std::vector<char> theSiPixelGainCalibrationPerPixel;
    std::vector<char> theSiPixelGainCalibrationPerColumn;
    std::vector<char> theSiPixelGainCalibrationGainPerColPedPerPixel;
    
    // Loop over columns and rows of this DetID
    float peds[161];
    float gains[161];
    float pedforthiscol[2];
    float gainforthiscol[2];
    int nusedrows[2];
    size_t nemptypixels=0;
    for(size_t icol=1; icol<=ncols; icol++) {
      nusedrows[0]=nusedrows[1]=0;
      pedforthiscol[0]=pedforthiscol[1]=0;
      gainforthiscol[0]=gainforthiscol[1]=0;
      //      std::cout << "now lookign at col " << i << std::endl;
      for(size_t jrow=1; jrow<=nrows; jrow++) {
	size_t iglobalrow=0;
	if(jrow>nrowsrocsplit)
	  iglobalrow=1;
	peds[jrow]=badpedval;
	gains[jrow]=badgainval;
	float ped = tempped->GetBinContent(jrow,icol);
	float gain = tempgain->GetBinContent(jrow,icol);
	float chi2 = tempchi2->GetBinContent(jrow,icol);

	if(ped>pedlow_ && gain>gainlow_ && ped<pedhi_ && gain<gainhi_ && chi2>badchi2_){
	  ntimes++;
	  if(ntimes<=10)
	    std::cout << detid << " " << jrow << " " << icol << " " << ped << " " << ped << " " << chi2 << std::endl;
	  VCAL_endpoint->Fill((255 - ped)/gain);
	  peds[jrow]=ped;
	  gains[jrow]=gain;
	  pedforthiscol[iglobalrow]+=ped;
	  gainforthiscol[iglobalrow]+=gain;
	  nusedrows[iglobalrow]++;
	}
	else{  
	  nemptypixels++;
// 	  if(nemptypixels<0.01*nrows*ncols )
// 	    std::cout << "ped,gain="<< ped << ","<< gain << " row, col " << jrow <<","<< icol << ", detid " << detid <<  std::endl;
	  if(usemeanwhenempty_){
	    if(ntimes<=100)
	      std::cout << "USING DEFAULT MEAN GAIN & PED (" << meangain << ","<< meanped << ")!, observed values are gain,ped : "<< gain << "," << ped <<", chi2 " << chi2  <<  std::endl;
	    peds[jrow]=meanped;
	    gains[jrow]=meangain;
	    std::pair<TString,int> tempval(tempgainstring,1);
	    badresults[detid]=tempval;
	  }
	  else{
	    if(ntimes<=100)
	      std::cout << tempgainstring << " bad/dead Pixel, observed values are gain,ped : "<< gain << "," << ped <<", chi2 " << chi2 << std::endl;
	    std::pair<TString,int> tempval(tempgainstring,2);
	    badresults[detid]=tempval;
	    // if everything else fails: set the gain & ped now to dead
	    peds[jrow]=badpedval;
	    gains[jrow]=badgainval;
	  }

	}
      }// now collected all info, actually do the filling
      
      for(size_t jrow=1; jrow<=nrows; jrow++) {
	nchannels++;
	size_t iglobalrow=0;
	if(jrow>nrowsrocsplit)
	  iglobalrow=1;
	float ped=peds[jrow];
	float gain=gains[jrow]; 
	if(ped<pedlow_ || gain<gainlow_)
	  theGainCalibrationDbInputOffline_->setDeadPixel(theSiPixelGainCalibrationGainPerColPedPerPixel);
	else if(ped>pedhi_ || gain>gainhi_)
	  theGainCalibrationDbInputOffline_->setDeadPixel(theSiPixelGainCalibrationGainPerColPedPerPixel);
	else
	  theGainCalibrationDbInputOffline_->setDataPedestal(ped, theSiPixelGainCalibrationGainPerColPedPerPixel);

	
	if(jrow%nrowsrocsplit==0){
	  //	  std::cout << "now in col " << icol << " " << jrow << " " << iglobalrow << std::endl;
	  if(nusedrows[iglobalrow]>0){
	    pedforthiscol[iglobalrow]/=(float)nusedrows[iglobalrow];
	    gainforthiscol[iglobalrow]/=(float)nusedrows[iglobalrow];
	   
	  } 
	  if(gainforthiscol[iglobalrow]>gainlow_ && gainforthiscol[iglobalrow]<gainhi_ && pedforthiscol[iglobalrow]>pedlow_ && pedforthiscol[iglobalrow]<pedhi_ ){// good 
	    //	    std::cout << "setting ped & col aves: " << pedforthiscol[iglobalrow] << " " <<  gainforthiscol[iglobalrow]<< std::endl;
	  }
	  else{
	    if(usemeanwhenempty_){
	      //	      std::cout << "setting ped & col aves: " << pedforthiscol[iglobalrow] << " " <<  gainforthiscol[iglobalrow]<< std::endl;
	      pedforthiscol[iglobalrow]=meanped;
	      gainforthiscol[iglobalrow]=meangain;
	      std::pair<TString,int> tempval(tempgainstring,3);
	      badresults[detid]=tempval;
	    } 
	    else{ //make dead
	      if(ntimes<=100)
		std::cout << tempgainstring << "dead Column, observed values are gain,ped : "<< gainforthiscol[iglobalrow] << "," << pedforthiscol[iglobalrow] << std::endl;
	      pedforthiscol[iglobalrow]=badpedval;
	      gainforthiscol[iglobalrow]=badgainval;
	      std::pair<TString,int> tempval(tempgainstring,4);
	      badresults[detid]=tempval;
	    }
	  }
	  
	  if(gainforthiscol[iglobalrow]>gainlow_ && gainforthiscol[iglobalrow]<gainhi_ && pedforthiscol[iglobalrow]>pedlow_ && pedforthiscol[iglobalrow]<pedhi_ ){
	    theGainCalibrationDbInputOffline_->setDataGain(gainforthiscol[iglobalrow],nrowsrocsplit,theSiPixelGainCalibrationGainPerColPedPerPixel);
	    theGainCalibrationDbInputHLT_->setData(pedforthiscol[iglobalrow],gainforthiscol[iglobalrow],theSiPixelGainCalibrationPerColumn);
	  }
	  else{
	    //	    std::cout << pedforthiscol[iglobalrow] << " " << gainforthiscol[iglobalrow] << std::endl;
	    theGainCalibrationDbInputOffline_->setDeadColumn(nrowsrocsplit,theSiPixelGainCalibrationGainPerColPedPerPixel);
	    theGainCalibrationDbInputHLT_->setDeadColumn(nrowsrocsplit,theSiPixelGainCalibrationPerColumn);
	  }
	}
      }
    }

  
    //    std::cout << "setting range..." << std::endl;
    SiPixelGainCalibration::Range range(theSiPixelGainCalibrationPerPixel.begin(),theSiPixelGainCalibrationPerPixel.end());
    SiPixelGainCalibrationForHLT::Range hltrange(theSiPixelGainCalibrationPerColumn.begin(),theSiPixelGainCalibrationPerColumn.end());
    SiPixelGainCalibrationOffline::Range offlinerange(theSiPixelGainCalibrationGainPerColPedPerPixel.begin(),theSiPixelGainCalibrationGainPerColPedPerPixel.end());
    
    //    std::cout <<"putting things in db..." << std::endl;
    // now start creating the various database objects
    if( !theGainCalibrationDbInputOffline_->put(detid,offlinerange,ncols) )
      edm::LogError("SiPixelGainCalibrationAnalysis")<<"warning: detid already exists for Offline (gain per col, ped per pixel) calibration database"<<std::endl;
    if(!theGainCalibrationDbInputHLT_->put(detid,hltrange, ncols) )
      edm::LogError("SiPixelGainCalibrationAnalysis")<<"warning: detid already exists for HLT (pedestal and gain per column) calibration database"<<std::endl;
  }
  // now printing out summary:
  size_t nempty=0;
  size_t ndefault=0;
  size_t ndead=0;
  size_t ncoldefault=0;
  size_t ncoldead=0;
  for(std::map<uint32_t,std::pair<TString,int> >::const_iterator ibad= badresults.begin(); ibad!=badresults.end(); ++ibad){
    uint32_t detid = ibad->first;
    if(badresults[detid].second==0){
      //      std::cout << " used pixel mean value";
      nempty++;
    }
    else if(badresults[detid].second==1){
      //      std::cout << " used pixel mean value";
      ndefault++;
    }
    else if(badresults[detid].second==2){
      std::cout << badresults[detid].first  ;
      std::cout << " has one or more dead pixels";
      std::cout << std::endl;
      ndead++;
    }
    else if(badresults[detid].second==3){
      //      std::cout << " used column mean value";
      ncoldefault++;
    }
    else if(badresults[detid].second==4){
    std::cout << badresults[detid].first  ;
      std::cout << " has one or more dead columns";
      ncoldead++;
    std::cout << std::endl;
    }
  }
  std::cout << nempty << " modules were empty and now have pixels filled with default values." << std::endl;
  std::cout << ndefault << " modules have pixels filled with default values." << std::endl;
  std::cout << ndead << " modules have pixels flagged as dead." << std::endl;
  std::cout << ncoldefault << " modules have columns filled with default values." << std::endl;
  std::cout << ncoldead << " modules have columns filled with dead values." << std::endl;
  std::cout << " ---> PIXEL Modules  " << nmodules  << "\n"
						 << " ---> PIXEL Channels " << nchannels << std::endl;

  std::cout << " --- writing to DB!" << std::endl;
  edm::Service<cond::service::PoolDBOutputService> mydbservice;
  if(!mydbservice.isAvailable() ){
    edm::LogError("db service unavailable");
    return;
  }
  else{
    if(record_=="SiPixelGainCalibrationForHLTRcd"){
      std::cout << "now doing SiPixelGainCalibrationForHLTRcd payload..." << std::endl;
      if( mydbservice->isNewTagRequest(record_) ){
	mydbservice->createNewIOV<SiPixelGainCalibrationForHLT>(
								theGainCalibrationDbInputHLT_,
								mydbservice->beginOfTime(),
								mydbservice->endOfTime(),
								"SiPixelGainCalibrationForHLTRcd");
      }
      else{

      mydbservice->appendSinceTime<SiPixelGainCalibrationForHLT>(
								 theGainCalibrationDbInputHLT_, 
								 mydbservice->currentTime(),
								 "SiPixelGainCalibrationForHLTRcd");
      
      }
    }
    else if (record_=="SiPixelGainCalibrationOfflineRcd"){
      std::cout << "now doing SiPixelGainCalibrationOfflineRcd payload..." << std::endl; 
      if( mydbservice->isNewTagRequest(record_) ){
	mydbservice->createNewIOV<SiPixelGainCalibrationOffline>(
								 theGainCalibrationDbInputOffline_,
								 mydbservice->beginOfTime(),
								 mydbservice->endOfTime(),
								 "SiPixelGainCalibrationOfflineRcd");
      }
      else{
	mydbservice->appendSinceTime<SiPixelGainCalibrationOffline>(
								    theGainCalibrationDbInputOffline_, 
								    mydbservice->currentTime(),
								    "SiPixelGainCalibrationOfflineRcd");
	
      }
    }
    edm::LogInfo(" --- all OK");
  } 
  histofile->Write();
  histofile->Close();
}

SiPixelGainCalibrationReadDQMFile::SiPixelGainCalibrationReadDQMFile(const edm::ParameterSet& iConfig):
  conf_(iConfig),
  rootfilestring_(conf_.getUntrackedParameter<std::string>("inputrootfile","inputfile.root")),
  appendMode_(conf_.getUntrackedParameter<bool>("appendMode",true)),
  theGainCalibrationDbInput_(0),
  theGainCalibrationDbInputOffline_(0),
  theGainCalibrationDbInputHLT_(0),
  theGainCalibrationDbInputService_(iConfig),
  usemeanwhenempty_(conf_.getUntrackedParameter<bool>("useMeanWhenEmpty",false)),
  record_(conf_.getUntrackedParameter<std::string>("record","SiPixelGainCalibrationOfflineRcd")),
  gainlow_(10.),gainhi_(0.),pedlow_(255.),pedhi_(-256),gainmax_(20),pedmax_(200),badchi2_(conf_.getUntrackedParameter<double>("badChi2Prob",0.01)),nmaxcols(10*52),nmaxrows(160)
  
  
{
   //now do what ever initialization is needed
  ::putenv("CORAL_AUTH_USER=me");
  ::putenv("CORAL_AUTH_PASSWORD=test");   
  meanGainHist_= new TH1F("meanGainHist","mean Gain Hist",500,0,gainmax_);
  meanPedHist_ = new TH1F("meanPedHist", "mean Ped Hist", 512,-200,pedmax_);
  defaultGain_=new TH2F("defaultGain","default gain, contains mean",nmaxcols,0,nmaxcols,nmaxrows,0,nmaxrows);// using dummy (largest) module size
  defaultPed_=new TH2F("defaultPed","default pedestal, contains mean",nmaxcols,0,nmaxcols,nmaxrows,0,nmaxrows);// using dummy (largest) module size
  defaultChi2_=new TH2F("defaultChi2","default chi2 probability, contains '1'",nmaxcols,0,nmaxcols,nmaxrows,0,nmaxrows);// using dummy (largest) module size
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
  getHistograms();
  fillDatabase(iSetup);
  // empty but should not be called anyway
 
}

void SiPixelGainCalibrationReadDQMFile::beginRun(const edm::EventSetup& iSetup){
 
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
}
void 
SiPixelGainCalibrationReadDQMFile::getHistograms(){

  
  std::cout <<"now parsing file " << rootfilestring_ << std::endl;
  therootfile_ = new TFile(rootfilestring_.c_str());
  therootfile_->cd();
  TDirectory *dir = therootfile_->GetDirectory("DQMData");
  TList *list = dir->GetListOfKeys();
  std::cout << list->GetEntries() << std::endl;

  TString comparestring = "Module";

  std::vector<TString> keylist;
  std::vector<TString> hist2list;
  std::vector<TString> dirlist;
  std::vector<TString> notdonelist;
  std::vector<int> nsubdirs;
  int ikey=0;

  for(ikey=0;ikey<list->GetEntries();  ikey++){
    TKey *thekey = (TKey*)list->At(ikey);
    if(thekey==0)
      continue;
    TString keyname=thekey->GetName();
    TString keytype=thekey->GetClassName();
    //    if(keyname=="EventInfo")
    //      continue;
    //    std::cout <<  keytype << " " << keyname << std::endl;
    if(keytype=="TDirectoryFile"){
      TString dirname=dir->GetPath();
      dirname+="/";
      dirname+=keyname;
      //      std::cout << dirname << std::endl;
      dir=therootfile_->GetDirectory(dirname);
  
      list=dir->GetListOfKeys();
      if(dirname.Contains(comparestring)){
	dirlist.push_back(dirname);
	//	std::cout << dirname << std::endl;
      }
      else{
	notdonelist.push_back(dirname);
	nsubdirs.push_back(-1);
      }
    }
  }
  size_t nempty=0;
  while(nempty!=notdonelist.size()){
    for(size_t idir=0; idir<notdonelist.size(); ++idir){
      if(nsubdirs[idir]==0)
	continue;
      //      std::cout << "now examining " << notdonelist[idir]<< " " << nsubdirs[idir] <<  std::endl;
      dir = therootfile_->GetDirectory(notdonelist[idir]); 
      //      std::cout << dir->GetName() << std::endl;
      list= dir->GetListOfKeys(); 
      //      std::cout << list->GetEntries() << std::endl;
      int ndirectories=0;
      for(ikey=0;ikey<list->GetEntries();  ikey++){
	TKey *thekey = (TKey*)list->At(ikey);
	if(thekey==0)
	  continue;
	TString keyname=thekey->GetName();
	TString keytype=thekey->GetClassName();
	//	std::cout << keyname << " " << keytype << std::endl;
	if(keytype=="TDirectoryFile"){
	  TString dirname=dir->GetPath();
	  dirname+="/";
	  dirname+=keyname;
	  //	  std::cout << dirname << std::endl;
	  ndirectories++;
	  if(dirname.Contains(comparestring)){
	    //	    std::cout << dirname << std::endl;
	    dirlist.push_back(dirname);
	  }
	  else{ 
	    notdonelist.push_back(dirname);
	    nsubdirs.push_back(-1);
	  }
	}
      }
      nsubdirs[idir]=ndirectories;
      // std::cout << "now done examining " << notdonelist[idir]<< " " << nsubdirs[idir] <<  std::endl;
    }
    nempty=0;
    for(size_t i=0; i<nsubdirs.size(); i++){
      if(nsubdirs[i]!=-1)
	nempty++;
    }
  }
  //  std::cout << "\n done!" << std::endl;
  
  for(size_t idir=0; idir<dirlist.size() ; ++idir){
    //    std::cout << "good dir "  << dirlist[idir] << std::endl;
    
    dir = therootfile_->GetDirectory(dirlist[idir]);
    list = dir->GetListOfKeys();
    for(ikey=0;ikey<list->GetEntries();  ikey++){
      TKey *thekey = (TKey*)list->At(ikey);
      if(thekey==0)
	continue;
      TString keyname=thekey->GetName();
      TString keytype=thekey->GetClassName();
      //	std::cout << keyname << " " << keytype << std::endl;
      if(keytype=="TH2F" && (keyname.Contains("Gain2d")||keyname.Contains("Pedestal2d")||keyname.Contains("GainChi2Prob2d"))){
	TString detidstring = keyname;
	detidstring.Remove(0,detidstring.Sizeof()-10);
	
	uint32_t detid = atoi(detidstring.Data());
	  
	if(keyname.Contains("GainChi2Prob2d")){

	  TString tempstr =dirlist[idir];
	  tempstr+="/";
	  tempstr+=keyname;
	  TString replacestring = rootfilestring_;
	  replacestring+=":";
	  tempstr.ReplaceAll(replacestring,"");
	  //	  std::cout << tempstr << std::endl;
	  bookkeeper_[detid]["chi2prob_2d"]=tempstr;
	}
	else if(keyname.Contains("Gain2d")){
	  
	  //	  std::cout << dirlist[idir] << std::endl;
	  std::map<std::string,TString> tempmap;
	  TString tempstr =dirlist[idir];
	  tempstr+="/";
	  tempstr+=keyname;
	  TString replacestring = rootfilestring_;
	  replacestring+=":";
	  tempstr.ReplaceAll(replacestring,"");
	  //	  std::cout << tempstr << std::endl;
	  bookkeeper_[detid]["gain_2d"]=tempstr;
	  TH2F *temphisto = (TH2F*)therootfile_->Get(bookkeeper_[detid]["gain_2d"]);
	  //std::cout << detidstring << " " << keyname << " " << detid << " " << bookkeeper_[detid]["gain_2d"] << std::endl ;

	  for(int xbin=1; xbin<=temphisto->GetNbinsX(); ++xbin){
	    for(int ybin=1; ybin<=temphisto->GetNbinsY(); ++ybin){
	      float val = temphisto->GetBinContent(xbin,ybin);
	      if(val<=0.0001)
		continue;
	      if(gainlow_>val)
		gainlow_=val;
	      if(gainhi_<val)
		gainhi_=val;
	      meanGainHist_->Fill(val);
	    }
	  }
	}
	if(keyname.Contains("Pedestal2d")){
	  //	  std::cout << dirlist[idir] << std::endl;
	  std::map<std::string,TString> tempmap;
	  
	  TString tempstr =dirlist[idir];
	  tempstr+="/";
	  tempstr+=keyname;
	  //	  std::cout << tempstr << std::endl;
	  TString replacestring = rootfilestring_;
	  replacestring+=":";
	  tempstr.ReplaceAll(replacestring,"");
	  bookkeeper_[detid]["ped_2d"]=tempstr;
	  TH2F *temphisto = (TH2F*)therootfile_->Get(bookkeeper_[detid]["ped_2d"]);
	  //std::cout << detidstring << " " << keyname << " " << detid << " " << bookkeeper_[detid]["ped_2d"] << std::endl ;

	  for(int xbin=1; xbin<=temphisto->GetNbinsX(); ++xbin){
	    for(int ybin=1; ybin<=temphisto->GetNbinsY(); ++ybin){
	      float val = temphisto->GetBinContent(xbin,ybin);
	      if(val>pedmax_)
		continue;
	      if(pedlow_>val)
		pedlow_=val;
	      if(pedhi_<val)
		pedhi_=val;
	      meanPedHist_->Fill(temphisto->GetBinContent(xbin,ybin));
	      
	    }
	  }
	}
	//   	std::cout << keyname << " " << keytype << std::endl;
      }
    }
  }
}
//define this as a plug-in
