import FWCore.ParameterSet.Config as cms

process = cms.Process("test")
process.load("CondTools.SiPixel.SiPixelGainCalibrationService_cfi")


process.readfileOffline = cms.EDFilter("SiPixelGainCalibrationReadDQMFile",
    inputrootfile = cms.untracked.string('DQM_Gain_Run54974.root'),
    record = cms.untracked.string('SiPixelGainCalibrationOfflineRcd'),
    useMeanWhenEmpty = cms.untracked.bool(True)                                     
)

process.readfileHLT = cms.EDFilter("SiPixelGainCalibrationReadDQMFile",
    inputrootfile = cms.untracked.string('DQM_Gain_Run54974.root'),
    record = cms.untracked.string('SiPixelGainCalibrationForHLTRcd'),
    useMeanWhenEmpty = cms.untracked.bool(True)                      
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)
process.source = cms.Source("EmptyIOVSource",
    lastRun = cms.untracked.uint32(1),
    timetype = cms.string('runnumber'),
    firstRun = cms.untracked.uint32(1),
    interval = cms.uint32(1)
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
            tag = cms.string('GainCalib_TESTHLT')
        ), 
        cms.PSet(
            record = cms.string('SiPixelGainCalibrationOfflineRcd'),
            tag = cms.string('GainCalib_TESTOFFLINE')
        )
    ),
    connect = cms.string('sqlite_file:prova_rawdata.db')
#    connect = cms.string('oracle://cms_orcoff_int2r/CMS_COND_21X_PIXEL')
)

process.p = cms.Path(process.readfileHLT*process.readfileOffline)


