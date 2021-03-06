#ifndef METTRIGGERTOOLS_GETMETTRIGGEROBJECTS_H
#define METTRIGGERTOOLS_GETMETTRIGGEROBJECTS_H 1

#include "AthenaBaseComps/AthAlgorithm.h"
//#include "GaudiKernel/ToolHandle.h" //included under assumption you'll want to use some tools! Remove if you don't!
#include "AsgTools/AnaToolHandle.h"
#include "METTriggerTools/IMETTriggerAnalysisTool.h"

#include <string>
#include <vector>

class GetMETTriggerObjects: public ::AthAlgorithm { 
 public: 
  GetMETTriggerObjects( const std::string& name, ISvcLocator* pSvcLocator );
  virtual ~GetMETTriggerObjects(); 

  virtual StatusCode  initialize();
  virtual StatusCode  execute();
  virtual StatusCode  finalize();

 private: 
  asg::AnaToolHandle<IMETTriggerAnalysisTool> m_anaTool;
  bool isData;
  std::string m_outMod;
  std::vector<std::string> m_configFiles;
  bool m_fullDebug;
}; 

#endif //> !METTRIGGERTOOLS_GETMETTRIGGEROBJECTS_H
