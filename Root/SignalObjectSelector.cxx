#include "METTriggerTools/SignalObjectSelector.h"
SignalObjectSelector::SignalObjectSelector(const std::string& name)
  : name(name),
    decorator(name),
    z0sintheta("z0sintheta"),
    d0sig("d0sig"),
    d0("d0"),
    cacc_eleIso("blankEleIso"),
    cacc_eleID("blankEleID"),
    cacc_muID("blankMuID"),
    cacc_muIso("blankMuIso"),
    cacc_gammaIso("blankGammaIso"),
    cacc_gammaID("blankGammaID"),
    cacc_tauID("blankTauID"),
    pileup("pileupJet"),
    passCleaning("passCleaning"),
    doElectrons(false),
    doElectronID(false),
    electronPt(0),
    electronEta(10),
    doElectronIso(false),
    electronIso(""),
    electronZ0sintheta(100),
    electronD0sig(100),
    electronD0(100),
    doMuons(false),
    doMuonID(false),
    muonID(""),
    muonPt(0),
    muonEta(10),
    doMuonIso(false),
    muonZ0sintheta(100),
    muonD0sig(100),
    muonD0(100),
    doJets(false),
    jetPt(0),
    jetEta(10),
    doJetPileup(false),
    doJetBad(false),
    doPhotons(false),
    doPhotonID(false),
    photonPt(0),
    photonEta(10),
    doPhotonIso(false),
    doTaus(false),
    doTauID(false),
    tauPt(0),
    tauEta(10)
{}

SignalObjectSelector::~SignalObjectSelector() {}

void SignalObjectSelector::initialize()
{
}

void SignalObjectSelector::selectElectron(const xAOD::Electron& electron) const
{
  //std::cout << "Using" << std::endl
            //<< " iso : " << electronIso << std::endl
            //<< " ID  : " << electronID << std::endl
            //<< " pt  : " << electronPt << std::endl
            //<< " eta : " << electronEta << std::endl
            //<< electronZ0sintheta << ", " << electronD0sig << ", " << electronD0 << std::endl;
  //static SG::AuxElement::ConstAccessor<char> cacc_ID("ID:"+electronID);
  //static SG::AuxElement::ConstAccessor<char> cacc_iso("iso:"+electronIso);
  float fabsEta = fabs(electron.caloCluster()->eta() );

  decorator(electron) = (doElectrons &&
                        (!doElectronID || cacc_eleID(electron) ) &&
                        electron.pt() * GeV > electronPt &&
                        fabsEta < electronEta && (fabsEta < 1.37 || fabsEta > 1.52) &&
                        (!doElectronIso || cacc_eleIso(electron) ) &&
                        z0sintheta(electron) < electronZ0sintheta &&
                        d0sig(electron) < electronD0sig &&
                        d0(electron) < electronD0);
} 
void SignalObjectSelector::selectMuon(const xAOD::Muon& muon) const
{
  //static SG::AuxElement::ConstAccessor<char> cacc_ID("ID:"+muonID);
  //static SG::AuxElement::ConstAccessor<char> cacc_iso("iso:"+muonIso);
  decorator(muon) = (doMuons &&
                    (!doMuonID || cacc_muID(muon) ) &&
                    muon.pt() * GeV > muonPt &&
                    fabs(muon.eta() ) < muonEta &&
                    (!doMuonIso || cacc_muIso(muon) ) &&
                    z0sintheta(muon) < muonZ0sintheta &&
                    d0sig(muon) < muonD0sig &&
                    d0(muon) < muonD0);
}
void SignalObjectSelector::selectJet(const xAOD::Jet& jet) const
{
  decorator(jet) = (doJets &&
                   (!doJetPileup || !pileup(jet) ) &&
                   (!doJetBad || passCleaning(jet) ) &&
                   jet.pt() * GeV > jetPt &&
                   fabs(jet.eta() ) < jetEta);
}
void SignalObjectSelector::selectPhoton(const xAOD::Photon& photon) const
{
  //static SG::AuxElement::ConstAccessor<char> cacc_ID("ID:"+photonID);
  //static SG::AuxElement::ConstAccessor<char> cacc_iso("iso:"+photonIso);
  float fabsEta = fabs(photon.caloCluster()->eta() );
  decorator(photon) = (doPhotons &&
                      (!doPhotonID || cacc_gammaID(photon) ) &&
                      photon.pt() * GeV > photonPt &&
                      fabsEta < photonEta && (fabsEta < 1.37 || fabsEta > 1.52)&&
                      (!doPhotonIso  || cacc_gammaIso(photon) ) );
}
void SignalObjectSelector::selectTau(const xAOD::TauJet& tau) const
{
  //std::cout << "In select tau with " << tauID << std::endl;
  //std::cout << "pt " << tauPt << std::endl;
  //std::cout << "eta" << tauEta << std::endl;
  //static SG::AuxElement::ConstAccessor<char> cacc_ID("ID:"+tauID);
  float fabsEta = fabs(tau.eta() );
  decorator(tau) = (doTaus &&
                   (!doTauID  || cacc_tauID(tau) ) &&
                   tau.pt() * GeV > tauPt &&
                   fabsEta < tauEta && (fabsEta < 1.37 || fabsEta > 1.52) );
  //std::cout << " done tau" << std::endl;
}
