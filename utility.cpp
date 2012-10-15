#include "Utility.h"
#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_model.h>
#include <xsi_operator.h>
#include <xsi_primitive.h>
#include <xsi_kinematicstate.h>
#include <xsi_kinematics.h>
#include <xsi_comapihandler.h>
#include <xsi_inputport.h>
#include <xsi_outputport.h>
#include <xsi_context.h>
#include <xsi_operatorcontext.h>
#include <xsi_customoperator.h>
#include <xsi_factory.h>
#include <xsi_parameter.h>
#include <xsi_ppglayout.h>
#include <xsi_ppgitem.h>
#include <xsi_shapekey.h>
#include <xsi_plugin.h>
#include <xsi_utils.h>
#include "AlembicLicensing.h"
#include "CommonProfiler.h"
using namespace XSI;


void logError( const char* msg ) {
	XSI::Application().LogMessage( msg, XSI::siErrorMsg );
}
void logWarning( const char* msg ) {
	XSI::Application().LogMessage( msg, XSI::siWarningMsg );
}
void logInfo( const char* msg ) {
	XSI::Application().LogMessage( msg, XSI::siInfoMsg );
}

std::string getIdentifierFromRef(CRef in_Ref, bool includeHierarchy)
{
   std::string result;
   CRef ref = in_Ref;
   bool has3DObj = false;
   while(ref.IsValid())
   {
      Model model(ref);
      X3DObject obj(ref);
      ProjectItem item(ref);
      if(model.IsValid())
      {
         if(model.GetFullName() == Application().GetActiveSceneRoot().GetFullName())
            break;
         result = std::string("/")+ std::string(model.GetName().GetAsciiString()) + std::string("Xfo") + result;
         ref = model.GetModel().GetRef();
      }
      else if(obj.IsValid())
      {
         if(!has3DObj)
         {
            result = std::string("/")+ std::string(obj.GetName().GetAsciiString()) + result;
            has3DObj = true;
         }
         result = std::string("/")+ std::string(obj.GetName().GetAsciiString()) + std::string("Xfo") + result;
         if(includeHierarchy)
            ref = obj.GetParent3DObject().GetRef();
         else
            ref = obj.GetModel().GetRef();
      }
      else if(item.IsValid())
      {
         ref = item.GetParent3DObject().GetRef();
      }
   }
   return result;
}

int getNodeDepthFromRef(XSI::CRef in_Ref)
{
   int nDepth = 0;
   CRef ref = in_Ref;
   bool has3DObj = false;
   while(ref.IsValid())
   {
      Model model(ref);
      X3DObject obj(ref);
      ProjectItem item(ref);
      if(model.IsValid())
      {
         if(model.GetFullName() == Application().GetActiveSceneRoot().GetFullName())
            break;
         ref = model.GetModel().GetRef();
		 nDepth++;
      }
      else if(obj.IsValid())
      {
         if(!has3DObj)
         {
            has3DObj = true;
         }
         ref = obj.GetParent3DObject().GetRef();
		 nDepth++;
      }
      else if(item.IsValid())
      {
         ref = item.GetParent3DObject().GetRef();
		 nDepth++;
      }
   }
   return nDepth;
}

XSI::CString truncateName(const XSI::CString & in_Name)
{
   CString name = in_Name;
   if(name.GetSubString(name.Length()-3,3).IsEqualNoCase(L"xfo"))
      name = name.GetSubString(0,name.Length()-3);
   return name;
}

CString getFullNameFromIdentifier(std::string in_Identifier)
{
   if(in_Identifier.length() == 0)
      return CString();
   CString mapped = nameMapGet(in_Identifier.c_str());
   if(!mapped.IsEmpty())
      return mapped;
   CStringArray parts = CString(in_Identifier.c_str()).Split(L"/");
   CString modelName = L"Scene_Root";
   CString objName = truncateName(parts[parts.GetCount()-1]);
   if(!objName.IsEqualNoCase(parts[parts.GetCount()-1]))
      return objName;
   if(parts.GetCount() > 2)
      modelName = truncateName(parts[parts.GetCount()-3]);
   return modelName+L"."+objName;
}

CRef getRefFromIdentifier(std::string in_Identifier)
{
   CRef result;
   result.Set(getFullNameFromIdentifier(in_Identifier));
   return result;
}

CRefArray getOperators( CRef in_Ref)
{
   Primitive primitive(in_Ref);
   CComAPIHandler comPrimitive( primitive.GetRef() );
   CComAPIHandler constructionHistory = comPrimitive.GetProperty( L"ConstructionHistory" );
   CValue valOperatorCollection;
   CValueArray args(3);
   args[1] = CValue( L"Operators" );
   constructionHistory.Call( L"Filter", valOperatorCollection, args );
   CComAPIHandler comOpColl = valOperatorCollection;
   CValue cnt = comOpColl.GetProperty( L"Count" );
   CRefArray ops( (LONG)cnt );
   for ( LONG i=0; i<(LONG)cnt; i++ )
   {
      CValue outOp;
      CValueArray itemsArgs;
      itemsArgs.Add( i );
      comOpColl.Invoke(L"Item", CComAPIHandler::PropertyGet, outOp, itemsArgs);
      ops[i] = outOp;
   }
   return ops;
}

std::map<std::string,bool> gIsRefAnimatedMap;
bool isRefAnimated(const CRef & in_Ref, bool xformCache, bool globalSpace)
{
   // check the cache
   std::map<std::string,bool>::iterator it = gIsRefAnimatedMap.find(in_Ref.GetAsText().GetAsciiString());
   if(it!=gIsRefAnimatedMap.end())
      return it->second;

   // convert to all types
   X3DObject x3d(in_Ref);
   Property prop(in_Ref);
   Primitive prim(in_Ref);
   KinematicState kineState(in_Ref);
   Operator op(in_Ref);
   ShapeKey shapeKey(in_Ref);

   if(x3d.IsValid())
   {
      // exclude the scene root model
      if(x3d.GetFullName() == Application().GetActiveSceneRoot().GetFullName())
         return returnIsRefAnimated(in_Ref,false);
      // check both kinematic states
      if(isRefAnimated(x3d.GetKinematics().GetLocal().GetRef()))
         return returnIsRefAnimated(in_Ref,true);
      if(isRefAnimated(x3d.GetKinematics().GetGlobal().GetRef()))
         return returnIsRefAnimated(in_Ref,true);
      // check all constraints
      CRefArray constraints = x3d.GetKinematics().GetConstraints();
      for(LONG i=0;i<constraints.GetCount();i++)
      {
         if(isRefAnimated(constraints[i]))
            return returnIsRefAnimated(in_Ref,true);
      }
      // check the parent
      if(isRefAnimated(x3d.GetParent()) && !xformCache)
         return returnIsRefAnimated(in_Ref,true);
   }
   else if(shapeKey.IsValid())
      return returnIsRefAnimated(in_Ref,true);
   else if(prop.IsValid())
      return returnIsRefAnimated(in_Ref,prop.IsAnimated());
   else if(prim.IsValid())
   {
      // check if we are in global space mode and there's animation on the
      // kinematics
      if(globalSpace)
      {
         if(isRefAnimated(prim.GetParent3DObject().GetRef()))
            return returnIsRefAnimated(in_Ref,true);
      }

      // first check if there are any envelopes
      if(prim.GetParent3DObject().GetEnvelopes().GetCount() > 0)
         return returnIsRefAnimated(in_Ref,true);
      // check if we have ice trees
      if(prim.GetICETrees().GetCount() > 0)
         return returnIsRefAnimated(in_Ref,true);
      // loop all ops on the construction history
      CRefArray ops = getOperators(prim.GetRef());
      for(LONG i=0;i<ops.GetCount();i++)
      {
         if(isRefAnimated(ops[i]))
            return returnIsRefAnimated(in_Ref,true);
      }
      if(prim.IsAnimated())
         return returnIsRefAnimated(in_Ref,true);

      // check all parameters on the primitive
      CParameterRefArray parameters = prim.GetParameters();
      for(LONG i=0;i<parameters.GetCount();i++)
      {
         Parameter param(parameters[i]);
         if(param.IsAnimated())
            return returnIsRefAnimated(in_Ref,true);
         if(param.GetSource().IsValid())
            return returnIsRefAnimated(in_Ref,true);
      }
   }
   else if(kineState.IsValid())
   {
      // myself is animated?
      if(kineState.IsAnimated())
         return returnIsRefAnimated(in_Ref,true);
      CRef retargetOpRef;
      retargetOpRef.Set(kineState.GetFullName()+L".RetargetGlobalOp");
      if(retargetOpRef.IsValid())
         return returnIsRefAnimated(in_Ref,true);
   }
   else if(op.IsValid())
   {
      // check myself
      if(op.IsAnimated())
         return returnIsRefAnimated(in_Ref,true);

      // check several custom types
      if(op.GetType().IsEqualNoCase(L"shapejitterop"))
         return returnIsRefAnimated(in_Ref,true);
      if(op.GetType().IsEqualNoCase(L"wave"))
         return returnIsRefAnimated(in_Ref,true);
      if(op.GetType().IsEqualNoCase(L"qstretch"))
         return returnIsRefAnimated(in_Ref,true);

      CRefArray outputPorts = op.GetOutputPorts();
      CRefArray inputPorts = op.GetInputPorts();
      for(LONG i=0;i<inputPorts.GetCount();i++)
      {
         InputPort port(inputPorts[i]);
         if(!port.IsConnected())
            continue;
         CRef target = port.GetTarget();
         bool isOutput = false;
         // ensure this target is not an output
         for(LONG j=0;j<outputPorts.GetCount();j++)
         {
            OutputPort output(outputPorts[j]);
            if(output.GetTarget().GetAsText().IsEqualNoCase(target.GetAsText()))
            {
               isOutput = true;
               break;
            }
         }
         if(!isOutput)
         {
            // first check for kinematics
            KinematicState opKineState(target);
            if(opKineState.IsValid())
            {
               if(isRefAnimated(opKineState.GetParent3DObject().GetRef()))
                  return returnIsRefAnimated(in_Ref,true);
            }
            else if(isRefAnimated(target))
               return returnIsRefAnimated(in_Ref,true);
         }
      }
   }
   return returnIsRefAnimated(in_Ref,false);
}

bool returnIsRefAnimated(const XSI::CRef & in_Ref, bool animated)
{
   gIsRefAnimatedMap.insert(
      std::pair<std::string,bool>(
         in_Ref.GetAsText().GetAsciiString(),
         animated
      )
   );
   return animated;
}

void clearIsRefAnimatedCache()
{
   gIsRefAnimatedMap.clear();
}

std::map<std::string,std::string> gNameMap;
void nameMapAdd(CString identifier, CString name)
{
   std::map<std::string,std::string>::iterator it = gNameMap.find(identifier.GetAsciiString());
   if(it == gNameMap.end())
   {
      std::pair<std::string,std::string> pair(identifier.GetAsciiString(),name.GetAsciiString());
      gNameMap.insert(pair);
   }
}

CString nameMapGet(CString identifier)
{
   std::map<std::string,std::string>::iterator it = gNameMap.find(identifier.GetAsciiString());
   if(it == gNameMap.end())
      return L"";
   return it->second.c_str();
}

void nameMapClear()
{
   gNameMap.clear();
}

CStatus alembicOp_Define( CRef& in_ctxt )
{
   ESS_PROFILE_SCOPE("alembicOp_Define");
   Context ctxt( in_ctxt );
   CustomOperator oCustomOperator;

   Parameter oParam;
   static CRef oPDef0;
   static CRef oPDef1;
   static CRef oPDef2;
   static CRef oPDef3;
   static CRef oPDef4;
   static CRef oPDef5;
   static CRef oPDef6;
	static bool s_initialized = false;

   Factory oFactory = Application().GetFactory();
   oCustomOperator = ctxt.GetSource();

   if( ! s_initialized ) {
	   oPDef0 = oFactory.CreateParamDef(L"muted",CValue::siBool,siAnimatable | siPersistable,L"muted",L"muted",0,0,1,0,1);
	   oPDef1 = oFactory.CreateParamDef(L"time",CValue::siFloat,siAnimatable | siPersistable,L"time",L"time",1,-100000,100000,0,1);
	   oPDef2 = oFactory.CreateParamDef(L"path",CValue::siString,siReadOnly | siPersistable,L"path",L"path",L"",L"",L"",L"",L"");
	   oPDef3 = oFactory.CreateParamDef(L"identifier",CValue::siString,siReadOnly | siPersistable,L"identifier",L"identifier",L"",L"",L"",L"",L"");
	   oPDef4 = oFactory.CreateParamDef(L"renderpath",CValue::siString,siReadOnly | siPersistable,L"renderpath",L"renderpath",L"",L"",L"",L"",L"");
	   oPDef5 = oFactory.CreateParamDef(L"renderidentifier",CValue::siString,siReadOnly | siPersistable,L"renderidentifier",L"renderidentifier",L"",L"",L"",L"",L"");
   }

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

CStatus alembicOp_DefineLayout( CRef& in_ctxt )
{
   ESS_PROFILE_SCOPE("alembicOp_DefineLayout");
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

CStatus alembicOp_Term( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CustomOperator op(ctxt.GetSource());
   delRefArchive(op.GetParameterValue(L"path").GetAsText());
   return CStatus::OK;
}

int gHasStandinSupport = -1;
bool hasStandinSupport()
{
   if(gHasStandinSupport < 0)
   {
      gHasStandinSupport = 0;
      CRefArray plugins = Application().GetPlugins();
      for(LONG i=0;i<plugins.GetCount();i++)
      {
         Plugin plugin(plugins[i]);
         if(plugin.GetName().GetSubString(0,6).IsEqualNoCase(L"arnold"))
         {
            gHasStandinSupport = 1;
            break;
         }
      }
   }
   return gHasStandinSupport > 0;
}
#pragma disable( warning: 4996 )

CString gDSOPath;

CString getDSOPath()
{
   // check if we have a cached result
   if(!gDSOPath.IsEmpty())
      return gDSOPath;

   CString result;
   // first check the environment variables
   if(getenv("ArnoldAlembicDSO") != NULL)
   {
      std::string env = getenv("ArnoldAlembicDSO");
      if(!env.empty())
         result = env.c_str();
   }

   if(result.IsEmpty())
   {
      CRefArray plugins = Application().GetPlugins();
      for(LONG i=0;i<plugins.GetCount();i++)
      {
         Plugin plugin(plugins[i]);
         if(plugin.GetName().IsEqualNoCase(L"ExocortexAlembicSoftimage"))
         {
            CString path = plugin.GetFilename();
            path = path.GetSubString(0,path.ReverseFindString(CUtils::Slash()));
            path = path.GetSubString(0,path.ReverseFindString(CUtils::Slash()));
#ifdef _WIN32
            path = CUtils::BuildPath(path,L"DSO",L"ExocortexAlembicArnold.dll");
#else
            path = CUtils::BuildPath(path,L"DSO",L"libExocortexAlembicArnold.so");
#endif
            result = path;
         }
      }
   }

   // validate the path exists
   if(!result.IsEmpty())
   {
      FILE * dsoFile = fopen(result.GetAsciiString(),"r");
      if(dsoFile == NULL)
      {
         Application().LogMessage(L"[ExocortexAlembic] Arnold DSO path '"+result+L"' does not exist.",siErrorMsg);
         result.Clear();
      }
      else
         fclose(dsoFile);
   }

   // store the cached result
   gDSOPath = result;
   return result;
}