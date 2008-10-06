import FWCore.ParameterSet.Config as cms

process = cms.Process("test")
process.load("CondTools.SiPixel.SiPixelGainCalibrationService_cfi")
process.load("Configuration.StandardSequences.Geometry_cff")
process.load("Geometry.TrackerGeometryBuilder.trackerGeometry_cfi")
process.load("Geometry.TrackerGeometryBuilder.idealForDigiTrackerGeometry_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.GlobalTag.globaltag = "STARTUP_V4::All"

process.readfileOffline = cms.EDFilter("SiPixelGainCalibrationReadDQMFile",
    inputrootfile = cms.untracked.string('DQM_GainCalibration_Run54974.root'),
    record = cms.untracked.string('SiPixelGainCalibrationOfflineRcd'),
    useMeanWhenEmpty = cms.untracked.bool(True)                                     
)

process.readfileHLT = cms.EDFilter("SiPixelGainCalibrationReadDQMFile",
    inputrootfile = cms.untracked.string('DQM_GainCalibration_Run54974.root'),
    record = cms.untracked.string('SiPixelGainCalibrationForHLTRcd'),
    useMeanWhenEmpty = cms.untracked.bool(True)                      
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)
process.source = cms.Source("EmptyIOVSource",
    lastValue = cms.uint64(1),
    timetype = cms.string('runnumber'),
    firstValue = cms.uint64(1),
    interval = cms.uint64(1)
)

process.PoolDBOutputService = cms.Service("PoolDBOutputService",
    BlobStreamerName = cms.untracked.string('TBufferBlobStreamingService'),
    DBParameters = cms.PSet(
        messageLevel = cms.untracked.int32(10),
        authenticationPath = cms.untracked.string('.')
#        authenticationPath = cms.untracked.string('/afs/cern.ch/cms/DB/conddb')
    ),
    toPut = cms.VPSet(
        cms.PSet(
            record = cms.string('SiPixelGainCalibrationForHLTRcd'),
            tag = cms.string('GainCalib_TESTHLT2')
        ), 
        cms.PSet(
            record = cms.string('SiPixelGainCalibrationOfflineRcd'),
            tag = cms.string('GainCalib_TESTOFFLINE2')
        )
    ),
    connect = cms.string('sqlite_file:prova_rawdata.db')
#    connect = cms.string('oracle://cms_orcoff_int2r/CMS_COND_21X_PIXEL')
)

process.p = cms.Path(process.readfileHLT*process.readfileOffline)


