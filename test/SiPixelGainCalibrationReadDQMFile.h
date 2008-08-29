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
// $Id: SiPixelGainCalibrationReadDQMFile.h,v 1.4 2008/08/15 09:40:24 fblekman Exp $
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
  void fillDatabase(const edm::EventSetup& iSetup);
  void getHistograms();
      // ----------member data ---------------------------
  edm::ParameterSet conf_;
  std::map<uint32_t,std::map<std::string,TString> > bookkeeper_; 
  bool appendMode_;
  SiPixelGainCalibration *theGainCalibrationDbInput_;
  SiPixelGainCalibrationOffline *theGainCalibrationDbInputOffline_;
  SiPixelGainCalibrationForHLT *theGainCalibrationDbInputHLT_;
  SiPixelGainCalibrationService theGainCalibrationDbInputService_;
  TH2F *defaultGain_;
  TH2F *defaultPed_;
  std::string record_;
  bool invertgain_;
  // keep track of lowest and highest vals for range
  float gainlow_;
  float gainhi_;
  float pedlow_;
  float pedhi_;
  double gainsum_;
  double pedsum_;
  double ntimesped_;
  double ntimesgain_;
  bool usemeanwhenempty_;
  TFile *therootfile_;
  std::string rootfilestring_;
  
};
