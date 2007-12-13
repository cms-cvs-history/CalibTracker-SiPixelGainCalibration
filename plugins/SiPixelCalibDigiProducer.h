#ifndef CALIBTRACKER_SIPIXELCALIBDIGIPRODUCER_H
#define CALIBTRACKER_SIPIXELCALIBDIGIPRODUCER_H

//
// Package:    SiPixelCalibDigiProducer
// Class:      SiPixelCalibDigiProducer
// 
/**\class SiPixelCalibDigiProducer SiPixelCalibDigiProducer.cc CalibTracker/SiPixelGainCalibration/src/SiPixelCalibDigiProducer.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Freya Blekman
//         Created:  Wed Oct 31 15:28:52 CET 2007
// $Id: SiPixelCalibDigiProducer.h,v 1.3 2007/12/12 12:29:20 fblekman Exp $
//
//

// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "FWCore/ServiceRegistry/interface/Service.h"

#include "FWCore/ParameterSet/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/Common/interface/DetSetVector.h"

#include "DataFormats/SiPixelDigi/interface/SiPixelCalibDigifwd.h"
#include "DataFormats/SiPixelDigi/interface/SiPixelCalibDigi.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"

#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "CalibFormats/SiPixelObjects/interface/SiPixelCalibConfiguration.h"
#include "CondFormats/DataRecord/interface/SiPixelCalibConfigurationRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelFedCablingMapRcd.h"

#include <map>
#include <vector>
#include <iostream>
//
// class decleration
//

class SiPixelCalibDigiProducer : public edm::EDProducer {
   public:
      explicit SiPixelCalibDigiProducer(const edm::ParameterSet& iConfig);
      ~SiPixelCalibDigiProducer();

   private:

      virtual void beginJob(const edm::EventSetup&);
      virtual void produce(edm::Event& iEvent, const edm::EventSetup& iSetup);
      virtual bool store();
      virtual void setPattern();
      virtual void fill(edm::Event &iEvent, const edm::EventSetup& iSetup);
      virtual void fillPixel(uint32_t detid, short row, short col,short ipoint, short adc);
      virtual bool checkPixel(uint32_t detid, short row, short col);
      virtual void clear();
      virtual void endJob() ;
      
      // ----------member data ---------------------------
      edm::InputTag src_;
      uint32_t iEventCounter_;
      bool ignore_non_pattern_;
      bool control_pattern_size_;
      edm::ParameterSet conf_;
      std::string label_;
      std::string instance_;
      
      edm::ESHandle<SiPixelCalibConfiguration> calib_; // keeps track of the calibration constants
      edm::ESHandle<TrackerGeometry> theGeometry_; // the tracker geometry
      edm::ESHandle<SiPixelFedCablingMap> theCablingMap_;

      // worker variables
      std::map<uint32_t,std::vector<SiPixelCalibDigi> > intermediate_data_; // data container, copied over into the event every pattern_repeat_ events
      std::map<uint32_t,std::vector<std::pair<short, short> > > detPixelMap_;// map to keep track of which pixels are filled where in intermediate_data_
      uint32_t pattern_repeat_; // keeps track of when the pattern should change
      std::map<uint32_t,uint32_t> detid_to_fedid_; // keeps track in which fed each detid is present.

      std::vector<std::pair<short,short> > currentpattern_;// keeps track of which pattern we are at
      std::pair<short,short> currentpair_;//worker class to keep track of pairs
};


#endif
