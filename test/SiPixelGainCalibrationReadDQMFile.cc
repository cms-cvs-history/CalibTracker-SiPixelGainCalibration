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
// $Id: SiPixelGainCalibrationReadDQMFile.cc,v 1.1 2008/08/05 14:56:05 fblekman Exp $
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
#include "TFile.h"
#include "TDirectory.h"
#include "TKey.h"
#include "TString.h"
#include "TList.h"

#include "CalibTracker/SiPixelGainCalibration/test/SiPixelGainCalibrationReadDQMFile.h"

//
// class decleration
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
  std::cout << "now filling db: values: pedlow, hi: " << pedlow_ << "," << pedhi_ << " and gainlow, hi: " << gainlow_ << "," << gainhi_ << ", and mean gain " << gainsum_/ntimes_ << ", ped " << pedsum_/ntimes_ <<  std::endl;
  
  theGainCalibrationDbInput_ = new SiPixelGainCalibration(pedlow_,pedhi_,gainlow_,gainhi_);
  theGainCalibrationDbInputHLT_ = new SiPixelGainCalibrationForHLT(pedlow_,pedhi_,gainlow_,gainhi_);
  theGainCalibrationDbInputOffline_ = new SiPixelGainCalibrationOffline(pedlow_,pedhi_,gainlow_,gainhi_);

  uint32_t nchannels=0;
  uint32_t nmodules=0;
  std::cout << "now starting loop on detids" << std::endl;
  uint32_t detid=0;
  therootfile_->cd();
  for(std::map<uint32_t,std::map<std::string, TString> >::const_iterator idet=bookkeeper_.begin(); idet!= bookkeeper_.end(); ++idet){
    
    //    std::cout << "now creating database object for detid " << detid << " " << bookkeeper_[detid]["gain_2d"] << " " << bookkeeper_[detid]["ped_2d"] << std::endl; //std::cout<< " nrows:" << nrows << " ncols: " << ncols << std::endl;
    detid=idet->first;
    //    edm::LogInfo("SiPixelGainCalibrationReadDQMFile") 
    // Get the module sizes.
    TString tempgainstring = bookkeeper_[detid]["gain_2d"];
    TH2F *tempgain = (TH2F*)therootfile_->Get(tempgainstring);
    if(tempgain==0){
      std::cout <<"ERROR, gain histo " << bookkeeper_[detid]["gain_2d"] << " does not exist" << std::endl;
      continue;
    }
    TString temppedstring = bookkeeper_[detid]["ped_2d"];
    TH2F *tempped = (TH2F*) therootfile_->Get(temppedstring);
    if(tempped==0){
      std::cout <<"ERROR, ped histo " << bookkeeper_[detid]["ped_2d"] << " does not exist" << std::endl;
      continue;
    }
    int nrows=tempgain->GetNbinsX();
    int ncols=tempgain->GetNbinsY();
    std::vector<char> theSiPixelGainCalibrationPerPixel;
    std::vector<char> theSiPixelGainCalibrationPerColumn;
    std::vector<char> theSiPixelGainCalibrationGainPerColPedPerPixel;
    
    // Loop over columns and rows of this DetID
    for(int i=1; i<=ncols; i++) {
      float pedforthiscol=0;
      float gainforthiscol=0;
      int nusedrows=0;
      //      std::cout << "now lookign at col " << i << std::endl;
      for(int j=1; j<=nrows; j++) {
	nchannels++;
	float ped = tempped->GetBinContent(i,j);
	float gain = tempgain->GetBinContent(i,j);
	
	if(usemeanwhenempty_ && ped<0.00001 && gain<0.00001){
	  ped=pedsum_/ntimes_;
	  gain=gainsum_/ntimes_;
	}
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
    
    std::cout <<"putting things in db..." << std::endl;
    // now start creating the various database objects
    if( !theGainCalibrationDbInput_->put(detid,range,ncols) )
      edm::LogError("SiPixelGainCalibrationReadDQMFile")<<"warning: detid already exists for Pixel-level calibration database"<<std::endl;
    if( !theGainCalibrationDbInputOffline_->put(detid,offlinerange,ncols) )
      edm::LogError("SiPixelGainCalibrationReadDQMFile")<<"warning: detid already exists for Offline (gain per col, ped per pixel) calibration database"<<std::endl;
    if(!theGainCalibrationDbInputHLT_->put(detid,hltrange) )
      edm::LogError("SiPixelGainCalibrationReadDQMFile")<<"warning: detid already exists for HLT (pedestal and gain per column) calibration database"<<std::endl;
  }
  
  //  edm::LogInfo("SiPixelGainCalibrationReadDQMFile") << " ---> PIXEL Modules  " << nmodules  << "\n"
  //						 << " ---> PIXEL Channels " << nchannels << std::endl;

 //  edm::LogInfo(" --- writing to DB!");
  edm::Service<cond::service::PoolDBOutputService> mydbservice;
  if(!mydbservice.isAvailable() ){
    edm::LogError("db service unavailable");
    return;
  }
  else{
    if(mydbservice->isNewTagRequest("SiPixelGainCalibrationOfflineRcd") ){
      
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
    
    if(mydbservice->isNewTagRequest("SiPixelGainCalibrationForHLTRcd") ){
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
    edm::LogInfo(" --- all OK");
  } 
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
  gainlow_(10.),gainhi_(0.),pedlow_(255.),pedhi_(0.),gainsum_(0.),pedsum_(0.),ntimes_(0)
  
  
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
    if(keyname=="EventInfo")
      continue;
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
  std::cout << "\n done!" << std::endl;
  
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
      if(keytype=="TH1F" && keyname.Contains("Gain1d")){
	TH1F *thehist = (TH1F*) dir->Get(keyname);
	gainsum_+=thehist->GetMean();
	ntimes_++;
      }
      if(keytype=="TH1F" && keyname.Contains("Pedestal1d")){
	TH1F *thehist = (TH1F*) dir->Get(keyname);
	pedsum_+=thehist->GetMean();
      }
      if(keytype=="TH2F" && (keyname.Contains("Gain2d")||keyname.Contains("Pedestal2d"))){
	TString detidstring = keyname;
	detidstring.Remove(0,detidstring.Sizeof()-10);
	
	uint32_t detid = atoi(detidstring.Data());
	  
	if(keyname.Contains("Gain2d")){
	  
	  std::cout << dirlist[idir] << std::endl;
	  std::map<std::string,TString> tempmap;
	  TString tempstr =dirlist[idir];
	  tempstr+="/";
	  tempstr+=keyname;
	  TString replacestring = rootfilestring_;
	  replacestring+=":";
	  tempstr.ReplaceAll(replacestring,"");
	  std::cout << tempstr << std::endl;
	  bookkeeper_[detid]["gain_2d"]=tempstr;
	  TH2F *temphisto = (TH2F*)therootfile_->Get(bookkeeper_[detid]["gain_2d"]);
	  //std::cout << detidstring << " " << keyname << " " << detid << " " << bookkeeper_[detid]["gain_2d"] << std::endl ;
	  if(temphisto->GetMinimum()<gainlow_)
	    gainlow_=temphisto->GetMinimum();
	  if(gainhi_<temphisto->GetMaximum())
	    gainhi_=temphisto->GetMaximum();
	}
	if(keyname.Contains("Pedestal2d")){
	  std::cout << dirlist[idir] << std::endl;
	  std::map<std::string,TString> tempmap;
	  
	  TString tempstr =dirlist[idir];
	  tempstr+="/";
	  tempstr+=keyname;
	  std::cout << tempstr << std::endl;
	  TString replacestring = rootfilestring_;
	  replacestring+=":";
	  tempstr.ReplaceAll(replacestring,"");
	  bookkeeper_[detid]["ped_2d"]=tempstr;
	  TH2F *temphisto = (TH2F*)therootfile_->Get(bookkeeper_[detid]["ped_2d"]);
	  //std::cout << detidstring << " " << keyname << " " << detid << " " << bookkeeper_[detid]["ped_2d"] << std::endl ;
	  if(pedlow_> temphisto->GetMinimum())
	    pedlow_=temphisto->GetMinimum();
	  if(pedhi_<temphisto->GetMaximum())
	    pedhi_=temphisto->GetMaximum();

	}
	//   	std::cout << keyname << " " << keytype << std::endl;
      }
    }
  }
}
//define this as a plug-in
