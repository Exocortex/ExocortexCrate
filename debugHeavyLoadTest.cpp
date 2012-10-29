#include "stdafx.h"

using namespace XSI; 

SICALLBACK XSILoadPlugin_2( PluginRegistrar& in_reg ) {
	in_reg.PutAuthor(L"Exocortex Technologies, Inc.");
	in_reg.PutName(L"HeavyLoadTest");
	in_reg.PutVersion(1,0);

	in_reg.RegisterCommand(L"exocortex_run_test",L"exocortex_run_test");
	in_reg.RegisterOperator(L"exocortex_nop");

	in_reg.RegisterMenu(siMenuMainFileProjectID,L"exocortex_RunTest",false,false);

	return CStatus::OK;
}

SICALLBACK exocortex_run_test_Init( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	Command oCmd;
	oCmd = ctxt.GetSource();
	oCmd.PutDescription(L"");
	oCmd.EnableReturnValue(true);
	return CStatus::OK;
}


int CreateSubTreeWithNopCubes( CRef& parentNode, CString& nodeName, CString& path, CString& identifier, int subLevels, int childrenPerLevel, int operatorsPerNode )
{
	X3DObject parentX3DObject(parentNode);
	X3DObject meshObj;

	parentX3DObject.AddGeometry(L"Cube",L"MeshSurface",nodeName,meshObj);
	CRef newNode = meshObj.GetRef();

	CRef realTarget = meshObj.GetActivePrimitive().GetRef();

	for( int i = 0; i < operatorsPerNode; i ++ ) {
		CustomOperator op = Application().GetFactory().CreateObject("exocortex_nop");
		op.AddOutputPort(realTarget);
		op.AddInputPort(realTarget);

		siConstructionMode consMode = siConstructionModeModeling;
		op.Connect(consMode);

		op.PutParameterValue(L"path",path);
		op.PutParameterValue(L"identifier",identifier);
	}

	int numNodes = 1;

	if( subLevels > 0 ) {
		for( int i = 0; i < childrenPerLevel; i ++ ) {
			std::stringstream s;
			s << "node_level" << subLevels << "_child" << i;
			CString childNodeName( s.str().c_str() );
			numNodes += CreateSubTreeWithNopCubes( newNode, childNodeName, path, identifier, subLevels - 1, childrenPerLevel, operatorsPerNode );
		}
	}

	return numNodes ;
}

SICALLBACK exocortex_run_test_Execute( CRef& in_ctxt )
{
	time_t start, end;
	time (&start);

	CRef sceneRoot = Application().GetActiveSceneRoot().GetRef();

   CString name = CString( L"test_root" );
   CString path = CString( L"c:\fakepath.abc" );
   CString identifier = CString( L"/path/to/my/object" );
	int numNodes = CreateSubTreeWithNopCubes( sceneRoot,
		name, path, identifier,
		4, 5, 2 );
	
	time (&end);
	double dif = difftime (end,start);

	std::stringstream s;
	s << "new nodes:" << numNodes << " total seconds:" << dif << " new nodes/sec:" << ( numNodes / dif );
			
	// Ben's results on a fast PC are with Softimage 2013 SP1
	// new nodes:781 total seconds:7 new nodes/sec:111.571 (operatorsPerNode = 1)
	// new nodes:781 total seconds:10 new nodes/sec:78.1 (operatorsPerNode = 2)
	Application().LogMessage( CString( s.str().c_str() ), XSI::siWarningMsg );
	return CStatus::OK;
}

SICALLBACK exocortex_RunTest_Init( CRef& in_ctxt )
{
	Context ctxt( in_ctxt );
	Menu oMenu;
	oMenu = ctxt.GetSource();
	MenuItem oNewItem;
	oMenu.AddCommandItem(L"Exocortex Run Test",L"exocortex_run_test",oNewItem);
	return CStatus::OK;
}

SICALLBACK exocortex_nop_Define( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CustomOperator oCustomOperator;

   Factory oFactory = Application().GetFactory();
   oCustomOperator = ctxt.GetSource();

	CRef oPDef0 = oFactory.CreateParamDef(L"muted",CValue::siBool,siAnimatable | siPersistable,L"muted",L"muted",0,0,1,0,1);
	CRef  oPDef1 = oFactory.CreateParamDef(L"time",CValue::siFloat,siAnimatable | siPersistable,L"time",L"time",1,-100000,100000,0,1);
	CRef   oPDef2 = oFactory.CreateParamDef(L"path",CValue::siString,siReadOnly | siPersistable,L"path",L"path",L"",L"",L"",L"",L"");
	CRef   oPDef3 = oFactory.CreateParamDef(L"identifier",CValue::siString,siReadOnly | siPersistable,L"identifier",L"identifier",L"",L"",L"",L"",L"");
	CRef   oPDef4 = oFactory.CreateParamDef(L"renderpath",CValue::siString,siReadOnly | siPersistable,L"renderpath",L"renderpath",L"",L"",L"",L"",L"");
	CRef   oPDef5 = oFactory.CreateParamDef(L"renderidentifier",CValue::siString,siReadOnly | siPersistable,L"renderidentifier",L"renderidentifier",L"",L"",L"",L"",L"");

   Parameter oParam;

   oCustomOperator.AddParameter(oPDef0,oParam);
   oCustomOperator.AddParameter(oPDef1,oParam);
   oCustomOperator.AddParameter(oPDef2,oParam);
   oCustomOperator.AddParameter(oPDef3,oParam);
   oCustomOperator.AddParameter(oPDef4,oParam);
   oCustomOperator.AddParameter(oPDef5,oParam);

   oCustomOperator.PutAlwaysEvaluate(false);
   oCustomOperator.PutDebug(0);

   return CStatus::OK;
}

SICALLBACK exocortex_nop_DefineLayout( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   PPGLayout oLayout;
   //PPGItem oItem;
   oLayout = ctxt.GetSource();
   oLayout.AddItem(L"muted",L"Muted");
   oLayout.AddItem(L"time",L"Time");
   oLayout.AddGroup(L"Preview");
   oLayout.AddItem(L"path",L"FilePath");
   oLayout.AddItem(L"identifier",L"Identifier");
   oLayout.EndGroup();
   oLayout.AddGroup(L"Render");
   oLayout.AddItem(L"renderpath",L"FilePath");
   oLayout.AddItem(L"renderidentifier",L"Identifier");
   oLayout.EndGroup();
   return CStatus::OK;
}


SICALLBACK exocortex_nop_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );
	// doing nothing at all
   return CStatus::OK;
}

SICALLBACK  exocortex_nop_Term( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CustomOperator op(ctxt.GetSource());
   return CStatus::OK;
}
