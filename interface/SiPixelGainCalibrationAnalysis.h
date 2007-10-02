#ifndef CalibTracker__SiPixelGainCalibrationAnalysis_H_
#define CalibTracker__SiPixelGainCalibrationAnalysis_H_
// -*- C++ -*-
//
// Package:    SiPixelGainCalibrationAnalysis
// Class:      SiPixelGainCalibrationAnalysis
// 
/**\class SiPixelGainCalibrationAnalysis SiPixelGainCalibrationAnalysis.cc CalibTracker/SiPixelGainCalibrationAnalysis/src/SiPixelGainCalibrationAnalysis.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Freya Blekman
//         Created:  Mon May  7 14:22:37 CEST 2007
// $Id: SiPixelGainCalibrationAnalysis.h,v 1.13 2007/09/24 10:09:04 friis Exp $
//
//



// system include files
#include <memory>

// user include files
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "PhysicsTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelFedCablingMap.h"
#include "CondFormats/DataRecord/interface/SiPixelFedCablingMapRcd.h"


#include "FWCore/Framework/interface/ESHandle.h"
#include "CalibFormats/SiPixelObjects/interface/SiPixelCalibConfiguration.h"
#include "CalibTracker/SiPixelGainCalibration/interface/PixelROCGainCalibPixel.h"

#include "CondTools/SiPixel/interface/SiPixelGainCalibrationService.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetType.h"
#include "TF1.h"
#include "TH2F.h"

//
// class decleration
//

class SiPixelGainCalibrationAnalysis : public edm::EDAnalyzer {
   public:
      explicit SiPixelGainCalibrationAnalysis(const edm::ParameterSet&);
      ~SiPixelGainCalibrationAnalysis();


   private:
      virtual void beginJob(const edm::EventSetup&) ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;
      bool doFits();

      // ----------member data ---------------------------
      // internal class for storing parameters

      edm::ESHandle<SiPixelCalibConfiguration> calib_; // keeps track of the calibration constants

      std::string recordName_;
      uint32_t eventno_counter_;
      std::string src_;
      std::string instance_;
      
      uint32_t vcal_fitmin_;
      uint32_t vcal_fitmax_;
      uint32_t vcal_fitmax_fixed_;
      std::vector <uint32_t> vcalvalues_ ; // a vector that contains the entire range.... preparation for possibility to have non-constant vcal-step.
      double maximum_ped_; 
      double maximum_gain_;
      double minimum_ped_; 
      double minimum_gain_;
      bool new_configuration_;
      bool save_histos_;
      uint32_t usethisdetid_;
      bool only_one_detid_;
      bool fast_no_plots_; // for use in online filter farm. Ignores all settings, only ped & gain are saved.
      bool do_scurveinstead_;
      // and the containers
      
      
      /// settings useful for debugging odd results
      bool saveAllGainCurvesForGivenPlaquettes_;
      std::vector<std::string> plaquettesToSave_;
      bool saveGainCurvesWithBadChi2_;
      double minimumChi2prob_;
      bool dropLowVcalOutliersForCurvesWithBadChi2_;
      bool useFirstNonZeroBinForFitMin_;

      // tracking geometry
      edm::ESHandle<TrackerGeometry> geom_;
      std::map < uint32_t, TString> detnames_;
      std::map < uint32_t, TH1F*> summaries1D_pedestal_;
      std::map < uint32_t, TH1F*> summaries1D_gain_;
      std::map < uint32_t, TH1F*> summaries1D_chi2_;
      std::map < uint32_t, TH1F*> summaries1D_chi2prob_;
      std::map < uint32_t, TH1F*> summaries1D_plat_;
      std::map < uint32_t, TH2F*> summaries_pedestal_;
      std::map < uint32_t, TH2F*> summaries_gain_;
      std::map < uint32_t, TH2F*> summaries_chi2_;
      std::map < uint32_t, TH2F*> summaries_plat_;
      std::map < uint32_t , uint32_t > detIDmap_;// keeps track of all used detector IDs
      std::map < uint32_t, std::vector < std::pair<uint32_t, uint32_t> > > colrowpairs_;
      std::map < uint32_t, std::vector < PixelROCGainCalibPixel > > calibPixels_;
      
      edm::ParameterSet conf_;
      bool appendMode_;
      bool useonlyonepixel_;
      bool test_;
      TF1 *func;
      SiPixelGainCalibration * SiPixelGainCalibration_;
      SiPixelGainCalibrationService SiPixelGainCalibrationService_; // additional database worker classes
      edm::Service < TFileService >  therootfileservice_; // for saving into root files
      TFileDirectory * errordir_; // directory to store bad gain curves
     
      std::string fitfuncrootformula_;
};

#endif
