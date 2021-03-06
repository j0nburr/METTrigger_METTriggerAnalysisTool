// METTriggerTools includes
#include "METTriggerTools/METTriggerAnalysisTool.h"

// Athena includes
#include "PathResolver/PathResolver.h"

// EDM includes
#include "xAODBase/IParticleHelpers.h"
#include "xAODTracking/TrackParticlexAODHelpers.h"
#include "xAODMissingET/MissingETAssociationMap.h"
#include "xAODEventInfo/EventInfo.h"
#include "xAODTrigMissingET/TrigMissingETContainer.h"

#include "TrigDecisionInterface/ITrigDecisionTool.h"

// Tool interface includes
#include "JetCalibTools/IJetCalibrationTool.h"
#include "JetInterface/IJetSelector.h"
#include "JetInterface/IJetUpdateJvt.h"

#include "MuonSelectorTools/IMuonSelectionTool.h"
#include "MuonMomentumCorrections/IMuonCalibrationAndSmearingTool.h"
#include "ElectronPhotonSelectorTools/IAsgElectronLikelihoodTool.h"
#include "ElectronPhotonSelectorTools/IAsgPhotonIsEMSelector.h"
#include "ElectronPhotonFourMomentumCorrection/IEgammaCalibrationAndSmearingTool.h"
#include "TauAnalysisTools/ITauSelectionTool.h"
#include "IsolationSelection/IIsolationSelectionTool.h"

#include "METInterface/IMETMaker.h"

#include "TrigConfInterfaces/ITrigConfigTool.h"

#include "AssociationUtils/IOverlapRemovalTool.h"
//#include "AssociationUtils/OverlapRemovalInit.h"

METTriggerAnalysisTool::METTriggerAnalysisTool( const std::string& name ) 
  : asg::AsgTool( name ),
    m_configured(false),
    m_configuredMET(false),
    m_toolsInitDone(false),
    m_metObjectSelector("met"),
    cacc_met("met"),
    dec_z0sintheta("z0sintheta"),
    dec_d0sig("d0sig"),
    dec_d0("d0"),
    GeV(0.001),
    m_orOutput("passOR")
    //m_muonIDDecorator("ID:Medium"),
    //m_electronIDDecorator("ID:MediumLH"),
    //m_photonIDDecorator("ID:Tight"),
    //m_tauIDDecorator("ID:Medium"),
{
  declareProperty( "IsData",       m_isData = true     );
  declareProperty( "METObjectKey", m_metObjectKey = "met");
  declareProperty( "InputElectronContainer", m_electronContainerName = "Electrons");
  declareProperty( "InputMuonContainer", m_muonContainerName = "Muons");
  declareProperty( "InputPhotonContainer", m_photonContainerName = "Photons");
  declareProperty( "InputTauContainer", m_tauContainerName = "TauJets");
  declareProperty( "JetType", m_jetInputType = xAOD::JetInput::EMTopo);
  declareProperty( "ConfigFiles", m_configFiles = std::vector<std::string>({"defaultMetConfig.json"}) );
  declareProperty( "ORDoTaus", m_ORDoTaus = true);
  declareProperty( "ORDoPhotons", m_ORDoPhotons = true);
  declareProperty( "JetJVTCut", m_JVTCut = 0.64);
  declareProperty( "JetPileupEtaCut", m_pileupEtaCut = 2.4);
  declareProperty( "JetPileupPtCut", m_pileupPtCut = 50);
  declareProperty( "SubToolOutputLevel", m_subToolOutputLevel = -1);
  declareProperty( "TrigDecTool", m_trigDec.handle() );
}

StatusCode METTriggerAnalysisTool::configure()
{
  ATH_MSG_INFO( "Configuring" );
  
  m_configured = true;
  
  for (std::string configFile : m_configFiles) {
    configFile = PathResolverFindCalibFile(configFile);
    ATH_CHECK( readConfigFile(configFile) );
  }

  if (!m_configuredMET) {
    ATH_MSG_ERROR( "No MET configuration provided" );
    return StatusCode::FAILURE;
  }
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::initialize() {
  ATH_MSG_INFO ("Initializing " << name() << "...");
  //
  //Make use of the property values to configure the tool
  //Tools should be designed so that no method other than setProperty is called before initialize
  //

  if (!m_configured) ATH_CHECK( configure() );
  
  m_jetPrefix = "AntiKt4" + xAOD::JetInput::typeName(xAOD::JetInput::Type(m_jetInputType) );
  
  ATH_CHECK( toolsInit() );
  
  cacc_met = SG::AuxElement::ConstAccessor<char>("met");

  ATH_MSG_INFO( "Done METTriggerAnalysisTool::initialize()" );
  
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::getElectrons(xAOD::ElectronContainer*& copy,
                                                xAOD::ShallowAuxContainer*& copyAux,
                                                const bool recordSG,
                                                const std::string& key) const
{
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::getElectrons" );
  std::string inputKey = (key == "" ? m_electronContainerName : key);
  const xAOD::ElectronContainer* constElectrons(0);
  ATH_CHECK( evtStore()->retrieve(constElectrons, inputKey) );
  std::pair<xAOD::ElectronContainer*, xAOD::ShallowAuxContainer*> shallowCopy = xAOD::shallowCopyContainer(*constElectrons);
  copy = shallowCopy.first;
  copyAux = shallowCopy.second;
  bool setLinks = xAOD::setOriginalObjectLink(*constElectrons, *copy);
  if (!setLinks) {
    ATH_MSG_WARNING( "Failed to set original object link on electrons copy" );
  }
  if (recordSG) {
    ATH_CHECK( evtStore()->record(copy, name() + "_" + inputKey) );
    ATH_CHECK( evtStore()->record(copyAux, name() + "_" + inputKey + "Aux") );
  }

  const xAOD::Vertex* pv = getPrimaryVertex();
  for (xAOD::Electron* ele : *copy) {
    ATH_CHECK( fillElectron(*ele, pv) );
  }
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::getMuons(xAOD::MuonContainer*& copy,
                                            xAOD::ShallowAuxContainer*& copyAux,
                                            const bool recordSG,
                                            const std::string& key) const
{
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::getMuons" );
  std::string inputKey = (key == "" ? m_muonContainerName : key);
  const xAOD::MuonContainer* constMuons(0);
  ATH_CHECK( evtStore()->retrieve(constMuons, inputKey) );
  std::pair<xAOD::MuonContainer*, xAOD::ShallowAuxContainer*> shallowCopy = xAOD::shallowCopyContainer(*constMuons);
  copy = shallowCopy.first;
  copyAux = shallowCopy.second;
  bool setLinks = xAOD::setOriginalObjectLink(*constMuons, *copy);
  if (!setLinks) {
    ATH_MSG_WARNING( "Failed to set original object link on muons copy" );
  }
  if (recordSG) {
    ATH_CHECK( evtStore()->record(copy, name() + "_" + inputKey) );
    ATH_CHECK( evtStore()->record(copyAux, name() + "_" + inputKey + "Aux") );
  }

  const xAOD::Vertex* pv = getPrimaryVertex();
  for (xAOD::Muon* muon : *copy) {
    ATH_CHECK( fillMuon(*muon, pv) );
  }
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::getPhotons(xAOD::PhotonContainer*& copy,
                                              xAOD::ShallowAuxContainer*& copyAux,
                                              const bool recordSG,
                                              const std::string& key) const
{
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::getPhotons" );
  std::string inputKey = (key == "" ? m_photonContainerName : key);
  const xAOD::PhotonContainer* constPhotons(0);
  ATH_CHECK( evtStore()->retrieve(constPhotons, inputKey) );
  std::pair<xAOD::PhotonContainer*, xAOD::ShallowAuxContainer*> shallowCopy = xAOD::shallowCopyContainer(*constPhotons);
  copy = shallowCopy.first;
  copyAux = shallowCopy.second;
  bool setLinks = xAOD::setOriginalObjectLink(*constPhotons, *copy);
  if (!setLinks) {
    ATH_MSG_WARNING( "Failed to set original object link on photons copy" );
  }
  if (recordSG) {
    ATH_CHECK( evtStore()->record(copy, name() + "_" + inputKey) );
    ATH_CHECK( evtStore()->record(copyAux, name() + "_" + inputKey + "Aux") );
  }

  for (xAOD::Photon* gamma : *copy) {
    ATH_CHECK (fillPhoton(*gamma) );
  }
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::getJets(xAOD::JetContainer*& copy,
                                           xAOD::ShallowAuxContainer*& copyAux,
                                           const bool recordSG,
                                           const std::string& key) const
{
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::getJets" );
  std::string inputKey = (key == "" ? m_jetPrefix + "Jets" : key);
  const xAOD::JetContainer* constJets(0);
  ATH_CHECK( evtStore()->retrieve(constJets, inputKey) );
  std::pair<xAOD::JetContainer*, xAOD::ShallowAuxContainer*> shallowCopy = xAOD::shallowCopyContainer(*constJets);
  copy = shallowCopy.first;
  copyAux = shallowCopy.second;
  bool setLinks = xAOD::setOriginalObjectLink(*constJets, *copy);
  if (!setLinks) {
    ATH_MSG_WARNING( "Failed to set original object link on jets copy" );
  }

  if (recordSG) {
    ATH_CHECK( evtStore()->record(copy, name() + "_" + inputKey) );
    ATH_CHECK( evtStore()->record(copyAux, name() + "_" +inputKey + "Aux") );
  }

  for (xAOD::Jet* jet : *copy) {
    ATH_CHECK(fillJet(*jet) );
  } //TODO make decorators settable
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::getTaus(xAOD::TauJetContainer*& copy,
                                           xAOD::ShallowAuxContainer*& copyAux,
                                           const bool recordSG,
                                           const std::string& key) const
{
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::getTaus" );
  std::string inputKey = (key == "" ? m_tauContainerName : key);
  const xAOD::TauJetContainer* constTauJets(0);
  ATH_CHECK( evtStore()->retrieve(constTauJets, inputKey) );
  std::pair<xAOD::TauJetContainer*, xAOD::ShallowAuxContainer*> shallowCopy = xAOD::shallowCopyContainer(*constTauJets);
  copy = shallowCopy.first;
  copyAux = shallowCopy.second;
  bool setLinks = xAOD::setOriginalObjectLink(*constTauJets, *copy);
  if (!setLinks) {
    ATH_MSG_WARNING( "Failed to set original object link on taus copy" );
  }
  if (recordSG) {
    ATH_CHECK( evtStore()->record(copy, name() + "_" + inputKey) );
    ATH_CHECK( evtStore()->record(copyAux, name() + "_" + inputKey + "Aux") );
  }

  for (xAOD::TauJet* tau : *copy) {
    ATH_CHECK(fillTau(*tau) );
  }
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::getMET(xAOD::MissingETContainer& met,
                                          const xAOD::JetContainer* jets,
                                          const xAOD::ElectronContainer* electrons,
                                          const xAOD::MuonContainer* muons,
                                          const xAOD::PhotonContainer* photons,
                                          const xAOD::TauJetContainer* taus,
                                          bool doTST, bool doJVTCut) const
{
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::getMET" );

  

  const xAOD::MissingETContainer *metCore;
  ATH_CHECK( evtStore()->retrieve(metCore, "MET_Core_" + m_jetPrefix) );

  const xAOD::MissingETAssociationMap* metMap;
  ATH_CHECK( evtStore()->retrieve(metMap, "METAssoc_" + m_jetPrefix) );
  metMap->resetObjSelectionFlags();

  if (electrons) {
    ATH_MSG_VERBOSE( "Build electron MET" );
    ConstDataVector<xAOD::ElectronContainer> metElectrons(SG::VIEW_ELEMENTS);
    for (const xAOD::Electron* ele : *electrons) {
      if (cacc_met(*ele) ) metElectrons.push_back(ele);
    }
    ATH_CHECK( m_metMaker->rebuildMET("RefEle", xAOD::Type::Electron, &met, metElectrons.asDataVector(), metMap) );
  }

  if (muons) {
    ATH_MSG_VERBOSE( "Build muon MET" );
    ConstDataVector<xAOD::MuonContainer> metMuons(SG::VIEW_ELEMENTS);
    for (const xAOD::Muon* muon : *muons) {
      if (cacc_met(*muon) ) metMuons.push_back(muon);
    }
    ATH_CHECK( m_metMaker->rebuildMET(m_muonMetTerm, xAOD::Type::Muon, &met, metMuons.asDataVector(), metMap) );
  }

  if (photons) {
    ATH_MSG_VERBOSE( "Build photon MET" );
    ConstDataVector<xAOD::PhotonContainer> metPhotons(SG::VIEW_ELEMENTS);
    for (const xAOD::Photon* photon : *photons) {
      if (cacc_met(*photon) ) metPhotons.push_back(photon);
    }
    ATH_CHECK( m_metMaker->rebuildMET(m_photonMetTerm, xAOD::Type::Photon, &met, metPhotons.asDataVector(), metMap) );
  }

  if (taus) {
    ATH_MSG_VERBOSE( "Build tau MET" );
    ConstDataVector<xAOD::TauJetContainer> metTaus(SG::VIEW_ELEMENTS);
    for (const xAOD::TauJet* tau : *taus) {
      if (cacc_met(*tau) ) metTaus.push_back(tau);
    }
    ATH_CHECK( m_metMaker->rebuildMET(m_tauMetTerm, xAOD::Type::Tau, &met, metTaus.asDataVector(), metMap) );
  }

  if (!jets) {
    ATH_MSG_ERROR( "Invalid jet container specified for MET rebuilding" );
    return StatusCode::FAILURE;
  }

  
  ATH_MSG_VERBOSE( "Build jet/soft MET" );
  ATH_CHECK( m_metMaker->rebuildJetMET( m_jetMetTerm, "SoftClus", "PVSoftTrk", &met, jets, metCore, metMap, doJVTCut) );
  
  ATH_CHECK( m_metMaker->buildMETSum("FinalTrk", &met, MissingETBase::Source::Track) );
  ATH_CHECK( m_metMaker->buildMETSum("FinalClus", &met, MissingETBase::Source::EMTopo) );
  

  ATH_MSG_VERBOSE( "Done building MET" );

  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::removeOverlaps(const xAOD::ElectronContainer* electrons,
                                                  const xAOD::MuonContainer* muons,
                                                  const xAOD::JetContainer* jets,
                                                  const xAOD::PhotonContainer* photons,
                                                  const xAOD::TauJetContainer* taus) const
{
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::removeOverlaps" );
  //for (auto& toolBox : m_orTools) {
    //ATH_CHECK( toolBox.masterTool->removeOverlaps(electrons, muons, jets, taus, photons) );
  //}
  for (auto& toolPair : m_orTools) {
    ATH_CHECK( toolPair.second->removeOverlaps(electrons, muons, jets, taus, photons) );
    if (msg().level() < MSG::INFO) {
      ATH_MSG_DEBUG( "Objects remaining after overlap removal with " << toolPair.first );
      if (electrons) {
        int nElePreOR = 0;
        int nElePostOR = 0;
        for (const auto iele : *electrons) {
          if (iele->auxdata<char>(toolPair.first) ) ++nElePreOR;
          if (iele->auxdata<char>(toolPair.first+":"+m_orOutput) ) ++nElePostOR;
        }
        ATH_MSG_DEBUG( "Electrons: " << nElePostOR << " of " << nElePreOR );
      }
      if (muons) {
        int nMuPreOR = 0;
        int nMuPostOR = 0;
        for (const auto imu : *muons) {
          if (imu->auxdata<char>(toolPair.first) ) ++nMuPreOR;
          if (imu->auxdata<char>(toolPair.first+":"+m_orOutput) ) ++nMuPostOR;
        }
        ATH_MSG_DEBUG( "Muons: " << nMuPostOR << " of " << nMuPreOR );
      }
      if (jets) {
        int nJetPreOR = 0;
        int nJetPostOR = 0;
        for (const auto ijet : *jets) {
          if (ijet->auxdata<char>(toolPair.first) ) ++nJetPreOR;
          if (ijet->auxdata<char>(toolPair.first+":"+m_orOutput) ) ++nJetPostOR;
        }
        ATH_MSG_DEBUG( "Jets: " << nJetPostOR << " of " << nJetPreOR );
      }
      if (photons) {
        int nGammaPreOR = 0;
        int nGammaPostOR = 0;
        for (const auto igamma : *photons) {
          if (igamma->auxdata<char>(toolPair.first) ) ++nGammaPreOR;
          if (igamma->auxdata<char>(toolPair.first+":"+m_orOutput) ) ++nGammaPostOR;
        }
        ATH_MSG_DEBUG( "Photons: " << nGammaPostOR << " of " << nGammaPreOR );
      }
      if (taus) {
        int nTauPreOR = 0;
        int nTauPostOR = 0;
        for (const auto itau : *taus) {
          if (itau->auxdata<char>(toolPair.first) ) ++nTauPreOR;
          if (itau->auxdata<char>(toolPair.first+":"+m_orOutput) ) ++nTauPostOR;
        }
        ATH_MSG_DEBUG( "Taus: " << nTauPostOR << " of " << nTauPreOR );
      }
    }
  }
  return StatusCode::SUCCESS;
}


const xAOD::Vertex* METTriggerAnalysisTool::getPrimaryVertex() const
{
  const xAOD::VertexContainer* vertices(0);
  if ( (evtStore()->retrieve(vertices, "PrimaryVertices") ).isSuccess() ) {
    for (const auto& vx : *vertices ) {
      if (vx->vertexType() == xAOD::VxType::PriVtx) {
        ATH_MSG_DEBUG( "Primary vertex found with z = " << vx->z() );
        return vx;
      }
    }
  }
  ATH_MSG_DEBUG( "No primary vertex found!" );
  return 0;
}

StatusCode METTriggerAnalysisTool::fillElectron(xAOD::Electron& ele, const xAOD::Vertex* priVtx) const
{
  // Calibrate the electrons
  if (m_egammaCalibrationTool->applyCorrection(ele) != CP::CorrectionCode::Ok) {
    ATH_MSG_ERROR("Failed to apply correction to electron");
  }
  //m_electronIDDecorator(ele) = m_electronSelectionTool->accept(ele);
  for (const auto& pair : m_electronSelectionTools) pair.first(ele) = pair.second->accept(ele);
  for (const auto& pair : m_electronIsolationTools) pair.first(ele) = pair.second->accept(ele);
  //Root::TAccept isoAccept = m_isolationSelectionTool->accept(ele);
  //for (unsigned int i = 0; i < isoAccept.getNCuts(); ++i) {
    //m_isoEleDecorators.at(i)(ele) = isoAccept.getCutResult(i);
  //}
  const xAOD::EventInfo* evtInfo(0);
  ATH_CHECK( evtStore()->retrieve(evtInfo, "EventInfo") );
  double pvtx_z = priVtx ? priVtx->z() : 0;
  const xAOD::TrackParticle* track = ele.trackParticle();
  if (track) {
    dec_z0sintheta(ele) = (track->vz() - pvtx_z) * TMath::Sin(ele.p4().Theta() );
    dec_d0sig(ele) = xAOD::TrackingHelpers::d0significance(track, evtInfo->beamPosSigmaX(), evtInfo->beamPosSigmaY(), evtInfo->beamPosSigmaXY() );
    dec_d0(ele) = track->d0();
  }
  else {
    dec_z0sintheta(ele) = 1e8;
    dec_d0sig(ele) = 1e8;
    dec_d0(ele) = 1e8;
  }
  ATH_MSG_DEBUG("About to select for met");
  m_metObjectSelector.selectElectron(ele);
  ATH_MSG_DEBUG("About to select for signals");
  for (const auto& sel : m_objSelectors) sel.selectElectron(ele);
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::fillPhoton(xAOD::Photon& photon) const
{
  uint16_t author = photon.author();
  if ((author & xAOD::EgammaParameters::AuthorPhoton || author & xAOD::EgammaParameters::AuthorAmbiguous) ) {
    if (m_egammaCalibrationTool->applyCorrection(photon) != CP::CorrectionCode::Ok) {
      ATH_MSG_ERROR( "Failed to apply correction to photon" );
    }
    //m_photonIDDecorator(photon) = m_photonSelectionTool->accept(photon);
    for (const auto& pair : m_photonSelectionTools) pair.first(photon) = pair.second->accept(photon);
    for (const auto& pair : m_photonIsolationTools) pair.first(photon) = pair.second->accept(photon);
    //Root::TAccept isoAccept = m_isolationSelectionTool->accept(photon);
    //for (unsigned int i = 0; i < isoAccept.getNCuts(); ++i) {
      //m_isoPhotonDecorators.at(i)(photon) = isoAccept.getCutResult(i);
    //}
    m_metObjectSelector.selectPhoton(photon);
    for (const auto& sel : m_objSelectors) sel.selectPhoton(photon);
  }
  else {
    //m_photonIDDecorator(photon) = false;
    for (const auto& pair : m_photonSelectionTools) pair.first(photon) = false;
    for (const auto& pair : m_photonIsolationTools) pair.first(photon) = false;
    //for (auto& dec : m_isoPhotonDecorators) dec(photon) = false;
    m_metObjectSelector.decorator(photon) = false;
    for (const auto& sel : m_objSelectors) sel.decorator(photon) = false;
  }
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::fillMuon(xAOD::Muon& muon, const xAOD::Vertex* priVtx) const
{
  const xAOD::TrackParticle* track = 0;
  if (muon.muonType() == xAOD::Muon::SiliconAssociatedForwardMuon) {
    track = muon.trackParticle(xAOD::Muon::ExtrapolatedMuonSpectrometerTrackParticle);
  }
  else track = muon.primaryTrackParticle();
  if (track) {
    if (m_muonCalibrationTool->applyCorrection(muon) != CP::CorrectionCode::Ok) {
      ATH_MSG_ERROR( "Failed to apply correction to muon");
    }
    //m_muonIDDecorator(muon) = m_muonSelectionTool->accept(muon);
    for (const auto& pair : m_muonSelectionTools) pair.first(muon) = pair.second->accept(muon);
    for (const auto& pair : m_muonIsolationTools) pair.first(muon) = pair.second->accept(muon);
    //Root::TAccept isoAccept = m_isolationSelectionTool->accept(muon);
    //for (unsigned int i = 0; i < isoAccept.getNCuts(); ++i) {
      //m_isoMuDecorators.at(i)(muon) = isoAccept.getCutResult(i);
    //}
    const xAOD::EventInfo* evtInfo(0);
    ATH_CHECK( evtStore()->retrieve(evtInfo, "EventInfo") );
    double pvtx_z = priVtx ? priVtx->z() : 0;
    dec_z0sintheta(muon) = (track->z0() + track->vz() - pvtx_z) * TMath::Sin(muon.p4().Theta() );
    dec_d0sig(muon) = xAOD::TrackingHelpers::d0significance(track, evtInfo->beamPosSigmaX(), evtInfo->beamPosSigmaY(), evtInfo->beamPosSigmaXY() );
    dec_d0(muon) = track->d0();
    m_metObjectSelector.selectMuon(muon);
    for (const auto& sel : m_objSelectors) sel.selectMuon(muon);
  }
  else {
    //m_muonIDDecorator(muon) = false;
    for (const auto& pair : m_muonSelectionTools) pair.first(muon) = false;
    for (const auto& pair : m_muonIsolationTools) pair.first(muon) = false;
    //for (const auto& dec : m_isoMuDecorators) dec(muon) = false;
    m_metObjectSelector.decorator(muon) = false;
    for (const auto& sel : m_objSelectors) sel.decorator(muon) = false;
  }
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool:: fillJet(xAOD::Jet& jet) const
{
  ATH_CHECK( m_jetCalibrationTool->applyCalibration(jet) );
  static SG::AuxElement::Decorator<float> dec_Jvt("Jvt");
  float newJvt;
  dec_Jvt(jet) = newJvt = m_jvtTool->updateJvt(jet);
  static SG::AuxElement::Decorator<char> dec_passCleaning("passCleaning");
  dec_passCleaning(jet) = m_jetCleaningTool->keep(jet);
  static SG::AuxElement::Decorator<char> dec_pileup("pileupJet");
  
  dec_pileup(jet) = (newJvt < m_JVTCut &&
                     fabs(jet.eta() ) < m_pileupEtaCut &&
                     jet.pt() * GeV < m_pileupPtCut);
  for (const auto& sel : m_objSelectors) sel.selectJet(jet);
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::fillTau(xAOD::TauJet& tau) const
{
  //std::cout << &tau << std::endl;
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::fillTau" );
  for (const auto& pair : m_tauSelectionTools) {
    pair.first(tau) = pair.second->accept(tau);
  }
  //m_tauIDDecorator(tau) = m_tauSelectionTool->accept(tau);
  ATH_MSG_DEBUG("About to run selectTau");
  m_metObjectSelector.selectTau(tau);
  for (const auto& sel : m_objSelectors) sel.selectTau(tau);
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::finalize()
{
  ATH_MSG_INFO( "Finalising " << name() );
#if 0
  for (auto& pair : m_electronIsolationTools) {ATH_MSG_INFO(pair.second->name() ); pair.second.handle()->release() ;} 
  m_electronIsolationTools.erase(m_electronIsolationTools.begin() );
  for (auto& pair : m_muonIsolationTools) {ATH_MSG_INFO(pair.second->name() ); pair.second.handle()->release() ;} 
  for (auto& pair : m_photonIsolationTools) {ATH_MSG_INFO(pair.second->name() ); pair.second.handle()->release() ;} 
//#if 0
  while( m_electronIsolationTools.size() != 0) {
    ATH_MSG_INFO( m_electronIsolationTools.begin()->second->name() );
    m_electronIsolationTools.erase(m_electronIsolationTools.begin() );
  }
  while( m_muonIsolationTools.size() != 0) {
    ATH_MSG_INFO( m_muonIsolationTools.begin()->second->name() );
    m_muonIsolationTools.erase(m_electronIsolationTools.begin() );
  }
  while( m_photonIsolationTools.size() != 0) {
    ATH_MSG_INFO( m_photonIsolationTools.begin()->second->name() );
    m_photonIsolationTools.erase(m_electronIsolationTools.begin() );
  }
#endif
  ATH_CHECK( asg::AsgTool::finalize() );
  return StatusCode::SUCCESS;
}


