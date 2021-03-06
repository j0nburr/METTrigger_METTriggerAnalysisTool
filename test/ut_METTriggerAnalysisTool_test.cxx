    
#include "AsgTools/AnaToolHandle.h"
#include "METTriggerTools/IMETTriggerAnalysisTool.h"

using namespace asg::msgUserCode;

int main() {
  ANA_CHECK_SET_TYPE (int); //makes ANA_CHECK return ints if exiting function
#ifdef ROOTCORE
  StatusCode::enableFailure();
  ANA_CHECK (xAOD::Init ());
#endif

   asg::AnaToolHandle<IMETTriggerAnalysisTool> myTool("METTriggerAnalysisTool/ToolInstanceName");
   ANA_CHECK( myTool.setProperty( "Property", 4.0 ) );
   ANA_CHECK( myTool.setProperty( "EnumProperty", IMETTriggerAnalysisTool::Val2 ) );
   ANA_CHECK( myTool.initialize() );

   //myTool->isSelected(....); put test code here

   return 0; //zero = success
}
