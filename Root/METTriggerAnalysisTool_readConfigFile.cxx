#include "METTriggerTools/METTriggerAnalysisTool.h"
//#include "METTriggerTools/jsonParser.h"
#include <fstream>

StatusCode METTriggerAnalysisTool::checkProperties(JSONNode& node, const std::string& context)
{
  if (node.subNodes.size() != 0) {
    ATH_MSG_ERROR( "Error. Unexpected configuration(s) provided to " << context << ":" ); 
    for (const auto& pair : node.subNodes) ATH_MSG_ERROR(pair.first);
    return StatusCode::FAILURE;
  }
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::readConfigFile(const std::string& configFile) {
  std::ifstream confStream(configFile);
  JSONParser parser(confStream, msg().level() < MSG::INFO);
  JSONNode& rootNode = parser.rootNode();
  rootNode.recursivePrint();
  std::string type;
  if (!rootNode.get("type", type) ) {
    ATH_MSG_ERROR( "No 'type' node in config file " << configFile );
    return StatusCode::FAILURE;
  }
  if (type == "met") {
    ATH_CHECK(configureMET(rootNode) );
    m_configuredMET = true;
  }
  else {
    JSONNode node;
    if (!rootNode.get("objDefs", node) ) {
      ATH_MSG_ERROR( "No 'objDefs' node in config file " << configFile );
      return StatusCode::FAILURE;
    }
    std::vector<JSONNode>& objDefs = node.vectorSubNodes;
    for (JSONNode& objDef : objDefs) {
      std::string decorator;
      if (!objDef.get("decorator", decorator) ) {
        ATH_MSG_ERROR( "No decorator provided for 'objDef' node in config file " << configFile );
        return StatusCode::FAILURE;
      }
      m_objSelectors.push_back(SignalObjectSelector(decorator) );
      ATH_CHECK( configureSignalObjects(objDef, m_objSelectors.back() ) );
    }
  } 
  ATH_MSG_DEBUG( "Done config on " << configFile );
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::configureMET(JSONNode& configNode)
{
  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::configureMET" );
  JSONNode node;
  if (!configNode.get("objDefs", node) ) {
    ATH_MSG_ERROR( "No 'objDefs' node in met node" );
    return StatusCode::FAILURE;
  }
  if (node.vectorSubNodes.size() != 1) {
    ATH_MSG_ERROR( "Invalid number of objDefs supplied for met: " << node.vectorSubNodes.size() << ". For now only 1 is supported" );
    return StatusCode::FAILURE;
  }
  JSONNode& objDefNode = node.vectorSubNodes.front();
  if (!objDefNode.get("decorator", m_metObjectKey) ) {
    ATH_MSG_ERROR( "No decorator provided for 'objDef' node in met node" );
    return StatusCode::FAILURE;
  }
  m_metObjectSelector = SignalObjectSelector(m_metObjectKey);

  ATH_CHECK( configureSignalObjects(objDefNode, m_metObjectSelector) );

  if (m_metObjectSelector.doJets) ATH_MSG_WARNING( "Jet configuration provided for MET - this will have no effect!");
  return StatusCode::SUCCESS;
}

StatusCode METTriggerAnalysisTool::configureSignalObjects(JSONNode& objDefNode, SignalObjectSelector& sel) {

  ATH_MSG_DEBUG( "In METTriggerAnalysisTool::configureSignalObjects" );

  JSONNode node;
  if (objDefNode.get("electron", node) ) {
    sel.doElectrons = true;
    std::string ID;
    if (node.get("ID", ID) ) {
      sel.doElectronID = true;
      sel.electronID = ID;
      sel.cacc_eleID = SG::AuxElement::ConstAccessor<char>("ID:"+ID);
      m_requestedElectronIDs.insert(ID);
    }
    node.get("pt", sel.electronPt);
    node.get("eta", sel.electronEta);
    std::string iso;
    if (node.get("iso", iso) ) {
      ATH_MSG_DEBUG("Setting ele iso: " << iso );
      sel.doElectronIso = true;
      sel.electronIso = iso;
      sel.cacc_eleIso = SG::AuxElement::ConstAccessor<char>("iso:"+iso);
      m_requestedElectronIsos.insert(iso);
    }
    node.get("z0sintheta", sel.electronZ0sintheta);
    node.get("d0sig", sel.electronD0sig);
    node.get("d0", sel.electronD0);
    ATH_CHECK( checkProperties(node, "electrons") );
  }
  if (objDefNode.get("muon", node) ) {
    sel.doMuons = true;
    std::string ID;
    if (node.get("ID", ID) ) {
      sel.doMuonID = true;
      sel.muonID = ID;
      sel.cacc_muID = SG::AuxElement::ConstAccessor<char>("ID:"+ID);
      m_requestedMuonIDs.insert(ID);
    }
    node.get("pt", sel.muonPt);
    node.get("eta", sel.muonEta);
    std::string iso;
    if (node.get("iso", iso) ) {
      ATH_MSG_DEBUG("Setting mu iso: " << iso );
      sel.doMuonIso = true;
      sel.muonIso = iso;
      sel.cacc_muIso = SG::AuxElement::ConstAccessor<char>("iso:"+iso);
      m_requestedMuonIsos.insert(iso);
    }
    node.get("z0sintheta", sel.muonZ0sintheta);
    node.get("d0sig", sel.muonD0sig);
    node.get("d0", sel.muonD0);
    ATH_CHECK( checkProperties(node, "muons") );
  }
  if (objDefNode.get("jet", node) ) {
    sel.doJets = true;
    node.get("pt", sel.jetPt);
    node.get("eta", sel.jetEta);
    node.get("doPileup", sel.doJetPileup);
    node.get("doBad", sel.doJetBad);
    ATH_CHECK( checkProperties(node, "jets") );
  }
  if (objDefNode.get("photon", node) ) {
    sel.doPhotons = true;
    std::string ID;
    if (node.get("ID", ID) ) {
      sel.doPhotonID = true;
      sel.photonID = ID;
      sel.cacc_gammaID = SG::AuxElement::ConstAccessor<char>("ID:"+ID);
      m_requestedPhotonIDs.insert(ID);
    }
    node.get("pt", sel.photonPt);
    node.get("eta", sel.photonEta);
    std::string iso;
    if (node.get("iso", iso) ) {
      ATH_MSG_DEBUG("Setting photon iso: " << iso );
      sel.doPhotonIso = true;
      sel.photonIso = iso;
      sel.cacc_gammaIso = SG::AuxElement::ConstAccessor<char>("iso:"+iso);
      m_requestedPhotonIsos.insert(iso);
    }
    ATH_CHECK( checkProperties(node, "photons") );
  }
  if (objDefNode.get("tau", node) ) {
    sel.doTaus = true;
    std::string ID;
    if (node.get("ID", ID) ) {
      sel.doTauID = true;
      sel.tauID = ID;
      sel.cacc_tauID = SG::AuxElement::ConstAccessor<char>("ID:"+ID);
      m_requestedTauIDs.insert(ID);
    }
    node.get("pt", sel.tauPt);
    node.get("eta", sel.tauEta);
    ATH_CHECK( checkProperties(node,"taus") );
  }
  ATH_CHECK( checkProperties(objDefNode, "object configurations") );
  return StatusCode::SUCCESS;
}

