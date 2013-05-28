#include "stdafx.h"
#include "Utility.h"
#include "AlembicLicensing.h"
#include "CommonProfiler.h"

using namespace XSI;

/*
SampleInfo getSampleInfo
(
   double iFrame,
   Alembic::AbcCoreAbstract::TimeSamplingPtr iTime,
   size_t numSamps
)
{
   SampleInfo result;
   if (numSamps == 0)
      numSamps = 1;

   std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex = iTime->getFloorIndex(iFrame, numSamps);
}
//*/

void logError( const char* msg ) {
#ifdef __EXOCORTEX_CORE_SERVICES_API_H
	Exocortex::essLogError( msg );
#else
	XSI::Application().LogMessage( msg, XSI::siErrorMsg );
#endif
}
void logWarning( const char* msg ) {
#ifdef __EXOCORTEX_CORE_SERVICES_API_H
	Exocortex::essLogWarning( msg );
#else
	XSI::Application().LogMessage( msg, XSI::siWarningMsg );
#endif
}
void logInfo( const char* msg ) {
#ifdef __EXOCORTEX_CORE_SERVICES_API_H
	Exocortex::essLogInfo( msg );
#else
	XSI::Application().LogMessage( msg, XSI::siInfoMsg );
#endif
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

XSI::CString truncateName(const XSI::CString& in_Name)
{
   CString name = in_Name;
   if(name.GetSubString(name.Length()-3,3).IsEqualNoCase(L"xfo"))
      name = name.GetSubString(0,name.Length()-3);
   return name;
}

XSI::CString stripNamespacePrefix(const XSI::CString& in_Name)
{
   ULONG i=0;
   for(; i<in_Name.Length(); i++){
      if(in_Name.GetAt(i) == ':'){
         break;
      }
   }
   return in_Name.GetSubString(i+1);
}

CString getFullNameFromIdentifier(XSI::CRef importRootNode, std::string in_Identifier, bool bMergedLeaf)
{
   std::vector<std::string> parts;

   boost::split(parts, in_Identifier, boost::is_any_of("/"));

   if(parts.size() > 2){
	   parts.pop_back();
   }

   std::stringstream result;
   result<<"Scene_Root";

   for(int i=1; i<parts.size(); i++){
	   parts[i] = removeXfoSuffix(parts[i]);
       result << ".";
       result << parts[i];
   }

   ESS_LOG_WARNING("path: "<<result.str());

   return result.str().c_str();


   //if(in_Identifier.length() == 0)
   //   return CString();

   //CString mapped = nameMapGet(in_Identifier.c_str());
   //if(!mapped.IsEmpty())
   //   return mapped;

   ////ESS_LOG_WARNING("Identifier: "<<in_Identifier.c_str());

   //CStringArray parts = CString(in_Identifier.c_str()).Split(L"/");

   //LONG count = parts.GetCount();
   //if(bMergedLeaf && parts.GetCount() >= 2){
   //   parts[parts.GetCount()-2] = truncateName(parts[parts.GetCount()-2]);
   //   count--;
   //}


   //CString pathName = importRootNode.GetAsText();

   //if(!importRootNode.IsValid()){
   //   pathName = L"Scene_Root";
   //}

   ////ESS_LOG_WARNING("root: "<<pathName.GetAsciiString());

   //for(int i=0; i<count; i++){
   //   pathName += ".";
   //   pathName += parts[i];
   //}

   //ESS_LOG_WARNING("FullNameFromIdentifier: "<<pathName.GetAsciiString());

   //return pathName;
}

//CRef getRefFromIdentifier(XSI::CRef importRootNode, std::string in_Identifier, bool bMergedLeaf)
//{
//   CRef result;
//   result.Set(getFullNameFromIdentifier(importRootNode,in_Identifier));
//   return result;
//}

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

void updateOperatorInfo( XSI::Operator& op, SampleInfo& sampleInfo, AbcA::TimeSamplingPtr timeSampling, 
						 int nPointsPrimitive, int nPointsCache)
{
	double t0, t1;

	// get time range
	t0 = timeSampling->getStoredTimes()[0]; 
	t1 = timeSampling->getStoredTimes()[ timeSampling->getNumStoredTimes()-1];
	t0 *= (float)CTime().GetFrameRate();
	t1 *= (float)CTime().GetFrameRate();

	int nSamples = (int) timeSampling->getNumStoredTimes();
	double sampleRate = (t1 - t0 + 1.0)/(double)nSamples;

	op.PutParameterValue(L"numSamples",(LONG) nSamples);
	op.PutParameterValue(L"sampleRate", sampleRate);
	op.PutParameterValue(L"StartTime", t0);
	op.PutParameterValue(L"EndTime", t1);
	op.PutParameterValue(L"numPointsPrimitive",(LONG) nPointsPrimitive);
	op.PutParameterValue(L"numPointsCache",(LONG) nPointsCache);
}

CStatus alembicOp_Define( CRef& in_ctxt )
{
   ESS_PROFILE_SCOPE("alembicOp_Define");
   Context ctxt( in_ctxt );
   CustomOperator oCustomOperator;

   Parameter oParam;
   static CRef oPDef;
   Factory oFactory = Application().GetFactory();
   oCustomOperator = ctxt.GetSource();

	oPDef = oFactory.CreateParamDef(L"muted",CValue::siBool,siAnimatable | siPersistable,L"muted",L"muted",1,0,1,0,1);
	oCustomOperator.AddParameter(oPDef,oParam);
	oPDef = oFactory.CreateParamDef(L"time",CValue::siFloat,siAnimatable | siPersistable,L"time",L"time",1,-100000,100000,0,1);
	oCustomOperator.AddParameter(oPDef,oParam);
	oPDef = oFactory.CreateParamDef(L"path",CValue::siString, siPersistable, L"path",L"path",L"",L"",L"",L"",L"");
	oCustomOperator.AddParameter(oPDef,oParam);
	oPDef = oFactory.CreateParamDef(L"multifile",CValue::siBool,siAnimatable | siPersistable,L"multifile",L"multifile",1,0,1,0,1);
	oCustomOperator.AddParameter(oPDef,oParam);
	oPDef = oFactory.CreateParamDef(L"identifier",CValue::siString, siPersistable, L"identifier",L"identifier",L"",L"",L"",L"",L"");
	oCustomOperator.AddParameter(oPDef,oParam);
	oPDef = oFactory.CreateParamDef(L"renderpath",CValue::siString, siPersistable, L"renderpath",L"renderpath",L"",L"",L"",L"",L"");
	oCustomOperator.AddParameter(oPDef,oParam);
	oPDef = oFactory.CreateParamDef(L"renderidentifier",CValue::siString, siPersistable, L"renderidentifier",L"renderidentifier",L"",L"",L"",L"",L"");
	oCustomOperator.AddParameter(oPDef,oParam);

   // Info parameters
   oPDef = oFactory.CreateParamDef( L"startTime", CValue::siDouble, siReadOnly | siPersistable, L"StartTime", L"", 0.0,-std::numeric_limits<float>::max(),std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),std::numeric_limits<float>::max() );
   oCustomOperator.AddParameter(oPDef,oParam);
   oPDef = oFactory.CreateParamDef( L"endTime", CValue::siDouble, siReadOnly | siPersistable, L"EndTime", L"", 0.0,-std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),std::numeric_limits<float>::max() );
   oCustomOperator.AddParameter(oPDef,oParam);
   oPDef = oFactory.CreateParamDef( L"sampleRate", CValue::siDouble, siReadOnly | siPersistable, L"SampleRate", L"", 1.0,-std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),std::numeric_limits<float>::max() );
   oCustomOperator.AddParameter(oPDef,oParam);
   oPDef = oFactory.CreateParamDef( L"numPointsPrimitive", CValue::siInt4, siReadOnly | siPersistable, L"NumPointsGeo", L"", 0, 0,std::numeric_limits<int>::max(), 0,std::numeric_limits<int>::max() );
   oCustomOperator.AddParameter(oPDef,oParam);
   oPDef = oFactory.CreateParamDef( L"numPointsCache", CValue::siInt4, siReadOnly | siPersistable, L"NumPointsCache", L"", 0, 0,std::numeric_limits<int>::max(), 0,std::numeric_limits<int>::max() );
   oCustomOperator.AddParameter(oPDef,oParam);
   oPDef = oFactory.CreateParamDef( L"numSamples", CValue::siInt4, siReadOnly | siPersistable, L"NumSamples", L"", 0, 0,std::numeric_limits<int>::max(), 0,std::numeric_limits<int>::max() );
   oCustomOperator.AddParameter(oPDef,oParam);

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

   oLayout.Clear();
   oLayout.AddTab(L"Main");
   oLayout.AddItem(L"muted",L"Muted");
   oLayout.AddItem(L"time",L"Time");
   oLayout.AddGroup(L"Preview");
   oLayout.AddItem(L"path",L"FilePath");
   oLayout.AddItem(L"multifile",L"Multifile");
   oLayout.AddItem(L"identifier",L"Identifier");
   oLayout.EndGroup();
   oLayout.AddGroup(L"Render");
   oLayout.AddItem(L"renderpath",L"FilePath");
   oLayout.AddItem(L"renderidentifier",L"Identifier");
   oLayout.EndGroup();
   oLayout.AddTab(L"Info");
   oLayout.AddGroup(L"Num Points");
   oLayout.AddItem(L"numPointsPrimitive",L"Primitive").PutAttribute(siUINoSlider, true);
   oLayout.AddItem(L"numPointsCache",L"Cache").PutAttribute(siUINoSlider, true);
   oLayout.EndGroup();
   oLayout.AddGroup(L"Frame Range");
   oLayout.AddRow();
   oLayout.AddItem(L"startTime",L"Start Time").PutAttribute(siUINoSlider, true);
   oLayout.AddItem(L"endTime",L"End Time").PutAttribute(siUINoSlider, true);
   oLayout.EndRow();
   oLayout.EndGroup();
   oLayout.AddGroup(L"Data");
   oLayout.AddRow();
   oLayout.AddItem(L"numSamples",L"Samples").PutAttribute(siUINoSlider, true);
   oLayout.AddItem(L"sampleRate",L"Rate").PutAttribute(siUINoSlider, true);
   oLayout.EndRow();
   oLayout.EndGroup();

   return CStatus::OK;
}

CStatus alembicOp_Init( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CValue udVal = ctxt.GetUserData();
   ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
   
   if(p != NULL){
      return CStatus::OK;
   }

   CValue val = (CValue::siPtrType) new ArchiveInfo;
   ctxt.PutUserData( val ) ;
   
   return CStatus::OK;
}

CStatus alembicOp_Term( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   CValue udVal = ctxt.GetUserData();
   ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
   if(p != NULL)
   {
      if( !p->path.empty() ){
         delRefArchive(p->path);
      }
      delete(p);
   }
   return CStatus::OK;
}

CStatus alembicOp_PathEdit( CRef& in_ctxt, CString& path )
{
	// check if we need t addref the archive
    Context ctxt( in_ctxt );

	CValue udVal = ctxt.GetUserData();
	ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
	if(p->path.empty()){
		p->path = path.GetAsciiString();
		addRefArchive(path);
		CValue val = (CValue::siPtrType) p;
		ctxt.PutUserData( val ) ;
	}
    else{
       if( strcmp(p->path.c_str(), path.GetAsciiString()) != 0 ){
         delRefArchive( XSI::CString(p->path.c_str()) );
		 p->path = path.GetAsciiString();
         addRefArchive(path);
       }
    }

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

       CString path = plugin.GetFilename();
       path = path.GetSubString(0,path.ReverseFindString(CUtils::Slash()));
       path = path.GetSubString(0,path.ReverseFindString(CUtils::Slash()));
#ifdef _WIN32
       path = CUtils::BuildPath(path,L"DSO",L"ExocortexAlembicArnold.dll");
#else
       path = CUtils::BuildPath(path,L"DSO",L"libExocortexAlembicArnold.so");
#endif
       boost::filesystem::path boostPath = path.GetAsciiString();
       if( boost::filesystem::exists( boostPath ) ) {
         result = path;
         break;
       }
#ifdef _WIN32
       path = CUtils::BuildPath(path,L"DSO",L"Arnold3ExocortexAlembic.dll");
#else
       path = CUtils::BuildPath(path,L"DSO",L"libArnold3ExocortexAlembic.so");
#endif
       boostPath = path.GetAsciiString();
       if( boost::filesystem::exists( boostPath ) ) {
         result = path;
         break;
       }
#ifdef _WIN32
       path = CUtils::BuildPath(path,L"DSO",L"Arnold4ExocortexAlembic.dll");
#else
       path = CUtils::BuildPath(path,L"DSO",L"libArnold4ExocortexAlembic.so");
#endif
       boostPath = path.GetAsciiString();
       if( boost::filesystem::exists( boostPath ) ) {
         result = path;
         break;
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
