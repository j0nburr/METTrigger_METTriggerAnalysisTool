#Skeleton joboption for a simple analysis job

theApp.EvtMax=10                                         #says how many events to run over. Set to -1 for all events

import AthenaPoolCnvSvc.ReadAthenaPool                   #sets up reading of POOL files (e.g. xAODs)
svcMgr.EventSelector.InputCollections=["/data/atlas/atlasdata3/burr/xAOD/testFiles/data15_13TeV.00284285.physics_Main.merge.AOD.r7562_p2521/AOD.07687825._003463.pool.root.1"]   #insert your list of input files here

algseq = CfgMgr.AthSequencer("AthAlgSeq")                #gets the main AthSequencer
algseq += CfgMgr.GetMETTriggerObjects("Getter", IsData = vars().get("isData", True), OutputLevel = DEBUG, ConfigFiles = ["WZCommon.json", "defaultMetConfig.json"] )                                 #adds an instance of your alg to it



##--------------------------------------------------------------------
## This section shows up to set up a HistSvc output stream for outputing histograms and trees
## See https://twiki.cern.ch/twiki/bin/viewauth/AtlasProtected/AthAnalysisBase#How_to_output_trees_and_histogra for more details and examples

#if not hasattr(svcMgr, 'THistSvc'): svcMgr += CfgMgr.THistSvc() #only add the histogram service if not already present (will be the case in this jobo)
#svcMgr.THistSvc.Output += ["MYSTREAM DATAFILE='myfile.root' OPT='RECREATE'"] #add an output root file stream

##--------------------------------------------------------------------


##--------------------------------------------------------------------
## The lines below are an example of how to create an output xAOD
## See https://twiki.cern.ch/twiki/bin/viewauth/AtlasProtected/AthAnalysisBase#How_to_create_an_output_xAOD for more details and examples

from OutputStreamAthenaPool.MultipleStreamManager import MSMgr
xaodStream = MSMgr.NewPoolRootStream( "StreamXAOD", "xAOD.out.root" )

##EXAMPLE OF BASIC ADDITION OF EVENT AND METADATA ITEMS
##AddItem and AddMetaDataItem methods accept either string or list of strings
#xaodStream.AddItem( ["xAOD::JetContainer#*","xAOD::JetAuxContainer#*"] ) #Keeps all JetContainers (and their aux stores)
xaodStream.AddMetaDataItem( ["xAOD::TriggerMenuContainer#*","xAOD::TriggerMenuAuxContainer#*"] )
ToolSvc += CfgMgr.xAODMaker__TriggerMenuMetaDataTool("TriggerMenuMetaDataTool") #MetaDataItems needs their corresponding MetaDataTool
svcMgr.MetaDataSvc.MetaDataTools += [ ToolSvc.TriggerMenuMetaDataTool ] #Add the tool to the MetaDataSvc to ensure it is loaded

##EXAMPLE OF SLIMMING (keeping parts of the aux store)
#xaodStream.AddItem( ["xAOD::ElectronContainer#Electrons","xAOD::ElectronAuxContainer#ElectronsAux.pt.eta.phi"] ) #example of slimming: only keep pt,eta,phi auxdata of electrons

# xaodStream.AddItem(["xAOD::ElectronContainer#CalElectrons", "xAOD::ShallowAuxContainer#CalElectronsAux"])
# xaodStream.AddItem(["xAOD::ElectronContainer#Electrons", "xAOD::ElectronAuxContainer#ElectronsAux"])
# xaodStream.AddItem(["xAOD::MuonContainer#CalMuons", "xAOD::ShallowAuxContainer#CalMuonsAux"])
# xaodStream.AddItem(["xAOD::MuonContainer#Muons", "xAOD::MuonAuxContainer#MuonsAux"])
# xaodStream.AddItem(["xAOD::JetContainer#CalJets", "xAOD::ShallowAuxContainer#CalJetsAux"])
# xaodStream.AddItem(["xAOD::JetContainer#AntiKt4EMTopoJets", "xAOD::JetAuxContainer#AntiKt4EMTopoJetsAux"])
# xaodStream.AddItem(["xAOD::PhotonContainer#CalPhotons", "xAOD::ShallowAuxContainer#CalPhotonsAux"])
# xaodStream.AddItem(["xAOD::PhotonContainer#Photons", "xAOD::PhotonAuxContainer#PhotonsAux"])
# xaodStream.AddItem(["xAOD::TauJetContainer#CalTaus", "xAOD::ShallowAuxContainer#CalTausAux"])
# xaodStream.AddItem(["xAOD::TauJetContainer#TauJets", "xAOD::TauJetAuxContainer#TausJetsAux"])
# xaodStream.AddItem(["xAOD::MissingETContainer#CalEtMissNoMu", "xAOD::MissingETAuxContainer#CalEtMissNoMuAux"])
# xaodStream.AddItem(["xAOD::MissingETContainer#CalEtMiss", "xAOD::MissingETAuxContainer#CalEtMissAux"])

##EXAMPLE OF SKIMMING (keeping specific events)
xaodStream.AddAcceptAlgs( "GetMETTriggerObjects" ) #will only keep events where 'setFilterPassed(false)' has NOT been called in the given algorithm

##--------------------------------------------------------------------


include("AthAnalysisBaseComps/SuppressLogging.py")              #Optional include to suppress as much athena output as possible. Keep at bottom of joboptions so that it doesn't suppress the logging of the things you have configured above
MessageSvc.Format = "% F%50W%S%7W%R%T %0W%M"
MessageSvc.defaultLimit = 9999999999
