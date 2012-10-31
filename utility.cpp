#include "stdafx.h"
#include "Utility.h"
#include "AlembicObject.h"
#include "AlembicLicensing.h"
#include "AlembicArchiveStorage.h"

void logError( const char* msg ) {
#ifdef __EXOCORTEX_CORE_SERVICES_API_H
	Exocortex::essLogError( msg );
#else
	MGlobal::displayError( msg );
#endif
}
void logWarning( const char* msg ) {
#ifdef __EXOCORTEX_CORE_SERVICES_API_H
	Exocortex::essLogWarning( msg );
#else
	MGlobal::displayWarning( msg );
#endif
}
void logInfo( const char* msg ) {
#ifdef __EXOCORTEX_CORE_SERVICES_API_H
	Exocortex::essLogInfo( msg );
#else
	MGlobal::displayInfo( msg );
#endif
}

std::string getIdentifierFromRef(const MObject & in_Ref)
{
   std::string result;
   MString fullName = getFullNameFromRef(in_Ref);
   MStringArray parts;
   fullName.split('|',parts);
   for(unsigned int i=0;i<parts.length();i++)
   {
      result.append("/");
      result.append(parts[i].asChar());
   }
   return result;
}

std::string removeInvalidCharacter(const std::string &str, bool keepSemi)
{
  std::string ret;
  const int len = (int) str.size();
  for (int i = 0; i < len; ++i)
  {
    const char c = str[i];
    if (c == ' ' || c == '_')
      ret.append(1, '_');
    else if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (keepSemi && c == ':'))
      ret.append(1, c);
  }
  return ret;
}

MString removeTrailFromName(MString & name)
{
   MString trail;
   while(name.length() > 0)
   {
      MString end = name.substring(name.length()-1,name.length()-1);
      if(end.asChar()[0] >= '0' && end.asChar()[0] <= '9')
      {
         trail = end + trail;
         name = name.substring(0,name.length()-2);
      }
      else
         break;
   }
   return trail;
}

MString truncateName(const MString & in_Name)
{
   MString name = in_Name;
   MString trail = removeTrailFromName(name);
   if(name.substring(name.length()-3,name.length()-1).toLowerCase() == "xfo")
      name = name.substring(0,name.length()-4);
   else if(name.substring(name.length()-5,name.length()-1).toLowerCase() == "shape")
      name = name.substring(0,name.length()-6);

   // replace the name space
   MStringArray nameParts;
   name.split(':',nameParts);
   name = nameParts[0];
   for(unsigned int i=1;i<nameParts.length();i++)
   {
      name += ".";
      name += nameParts[i];
   }

   return name + trail;
}

MString injectShapeToName(const MString & in_Name)
{
   MString name = in_Name;
   MString trail = removeTrailFromName(name);
   return name + "Shape" + trail;
}

MString getFullNameFromRef(const MObject & in_Ref)
{
   return MFnDagNode(in_Ref).fullPathName();
}

MString getFullNameFromIdentifier(std::string in_Identifier)
{
   if(in_Identifier.length() == 0)
      return MString();
   MString mapped = nameMapGet(in_Identifier.c_str());
   if(mapped.length() != 0)
      return mapped;
   MStringArray parts;
   MString(in_Identifier.c_str()).split('/',parts);
   MString lastPart = parts[parts.length()-1];
   if(lastPart.substring(lastPart.length()-3,lastPart.length()-1).toLowerCase() == "xfo")
      return truncateName(lastPart);
   MString trail = removeTrailFromName(lastPart);
   return lastPart+"Shape"+trail;
}

MObject getRefFromFullName(const MString & in_Path)
{
   MSelectionList sl;
   sl.add(in_Path);
   MDagPath dag;
   sl.getDagPath(0,dag);
   return dag.node();
}

MObject getRefFromIdentifier(std::string in_Identifier)
{
   return getRefFromFullName(getFullNameFromIdentifier(in_Identifier));
}

std::map<std::string,bool> gIsRefAnimatedMap;
bool isRefAnimated(const MObject & in_Ref)
{
   // check the cache
   // TODO: determine how to get the fullname of a MObject
   //std::map<std::string,bool>::iterator it = gIsRefAnimatedMap.find(in_Ref.GetAsText().asChar());
   //if(it!=gIsRefAnimatedMap.end())
   //   return it->second;
   return returnIsRefAnimated(in_Ref,false);
}

bool returnIsRefAnimated(const MObject & in_Ref, bool animated)
{
   // TODO: determine how to get the fullname of a MObject
   //gIsRefAnimatedMap.insert(
   //   std::pair<std::string,bool>(
   //      in_Ref.GetAsText().asChar(),
   //      animated
   //   )
   //);
   return animated;
}

void clearIsRefAnimatedCache()
{
   gIsRefAnimatedMap.clear();
}

std::map<std::string,std::string> gNameMap;
void nameMapAdd(MString identifier, MString name)
{
   std::map<std::string,std::string>::iterator it = gNameMap.find(identifier.asChar());
   if(it == gNameMap.end())
   {
      std::pair<std::string,std::string> pair(identifier.asChar(),name.asChar());
      gNameMap.insert(pair);
   }
}

MString nameMapGet(MString identifier)
{
   std::map<std::string,std::string>::iterator it = gNameMap.find(identifier.asChar());
   if(it == gNameMap.end())
      return "";
   return it->second.c_str();
}

void nameMapClear()
{
   gNameMap.clear();
}

ALEMBIC_TYPE getAlembicTypeFromObject(Abc::IObject object)
{
  ESS_PROFILE_SCOPE("getAlembicTypeFromObject"); 
  const Abc::MetaData &md = object.getMetaData();
  if(AbcG::IXform::matches(md))
    return AT_Xform;
  else if(AbcG::IPolyMesh::matches(md))
    return AT_PolyMesh;
  else if(AbcG::ICurves::matches(md))
    return AT_Curves;
  else if(AbcG::INuPatch::matches(md))
    return AT_NuPatch;
  else if(AbcG::IPoints::matches(md))
    return AT_Points;
  else if(AbcG::ISubD::matches(md))
    return AT_SubD;
  else if(AbcG::ICamera::matches(md))
    return AT_Camera;
  return AT_UNKNOWN;
}

std::string alembicTypeToString(ALEMBIC_TYPE at)
{
  switch(at)
  {
  case AT_Xform:
    return "Xform";
  case AT_PolyMesh:
    return "PolyMesh";
  case AT_Curves:
    return "Curves";
  case AT_NuPatch:
    return "NuPatch";
  case AT_Points:
    return "Points";
  case AT_SubD:
    return "SubD";
  case AT_Camera:
    return "Camera";
  case AT_Group:
    return "Group";
  default:
  case AT_UNKNOWN:
    return "Unknown";
  };
  return "Unknown";
}

MString getTypeFromObject(Abc::IObject object)
{
  return MString( alembicTypeToString( getAlembicTypeFromObject(object) ).c_str() );
}

MMatrix GetGlobalMMatrix(const MObject & in_Ref)
{
   MMatrix result;
   result.setToIdentity();

   MFnDagNode node(in_Ref);
   MDagPathArray paths;
   node.getAllPaths(paths);
   MDagPath path = paths[0];

   MString typeStr = in_Ref.apiTypeStr();
   if(typeStr == "kTransform")
   {
      result = path.inclusiveMatrix();
   }
   else
   {
      for(unsigned int i=0;i<node.parentCount();i++)
      {
         MObject parent = node.parent(i);
         typeStr = parent.apiTypeStr();
         if(typeStr == "kTransform")
         {
            result = GetGlobalMMatrix(parent);
            //break;
         }
      }
   }

   return result;
}

Abc::M44f GetGlobalMatrix(const MObject & in_Ref)
{
   MMatrix matrix = GetGlobalMMatrix(in_Ref);
   Abc::M44f result;
   matrix.get(result.x);
   return result;
}


MSyntax AlembicResolvePathCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-h", "-help");
   syntax.addFlag("-f", "-fileNameArg", MSyntax::kString);
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

MStatus AlembicResolvePathCommand::doIt(const MArgList & args)
{
   ESS_PROFILE_SCOPE("AlembicResolvePathCommand::doIt");
   MStatus status = MS::kSuccess;
   MArgParser argData(syntax(), args, &status);

   if (argData.isFlagSet("help"))
   {
      MGlobal::displayInfo("[ExocortexAlembic]: ExocortexAlembic_resolvePath command:");
      MGlobal::displayInfo("                    -f : provide an unresolved fileName (string)");
      return MS::kSuccess;
   }

   if(!argData.isFlagSet("fileNameArg"))
   {
      // TODO: display dialog
      MGlobal::displayError("[ExocortexAlembic] No fileName specified.");
      return status;
   }

   // get the filename arg
   MString fileName = argData.flagArgumentString("fileNameArg",0);

   setResult(resolvePath(fileName));

   return status;
}


MSyntax AlembicProfileBeginCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-f", "-fileNameArg", MSyntax::kString);
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

#ifdef ESS_PROFILING
std::map<std::string, boost::shared_ptr<Profiler> > nameToProfiler;
#endif

MStatus AlembicProfileBeginCommand::doIt(const MArgList& args) {
   MStatus status = MS::kSuccess;
#ifdef ESS_PROFILING
   MArgParser argData(syntax(), args, &status);

   if(!argData.isFlagSet("fileNameArg"))
   {
      // TODO: display dialog
      MGlobal::displayError("[ExocortexAlembic] No fileName specified.");
      return status;
   }

   // get the filename arg
   MString fileName = argData.flagArgumentString("fileNameArg",0);

   std::string strFileName( fileName.asChar() );

   if( nameToProfiler.find( strFileName ) == nameToProfiler.end() ) {
		boost::shared_ptr<Profiler> profiler( new Profiler( strFileName.c_str() ) );
		nameToProfiler.insert( std::pair<std::string, boost::shared_ptr<Profiler>>( strFileName, profiler ) );
   }
   else {
		nameToProfiler[ strFileName]->resume();
   }
#endif
   return status;
}

MSyntax AlembicProfileEndCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-f", "-fileNameArg", MSyntax::kString);
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

MStatus AlembicProfileEndCommand::doIt(const MArgList& args) {

   MStatus status = MS::kSuccess;
#ifdef ESS_PROFILING
   MArgParser argData(syntax(), args, &status);

   if(!argData.isFlagSet("fileNameArg"))
   {
      // TODO: display dialog
      MGlobal::displayError("[ExocortexAlembic] No fileName specified.");
      return status;
   }

   // get the filename arg
   MString fileName = argData.flagArgumentString("fileNameArg",0);

    std::string strFileName( fileName.asChar() );
   if( nameToProfiler.find( strFileName ) != nameToProfiler.end() ) {
		nameToProfiler[ strFileName ]->stop();
   }
#endif

   return status;

}

MSyntax AlembicProfileStatsCommand::createSyntax()
{
   MSyntax syntax;
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

MStatus AlembicProfileStatsCommand::doIt(const MArgList& args) {

   MStatus status = MS::kSuccess;
   
   ESS_PROFILE_REPORT();

   return status;
}


MSyntax AlembicProfileResetCommand::createSyntax()
{
   MSyntax syntax;
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

MStatus AlembicProfileResetCommand::doIt(const MArgList& args)
{
#ifdef ESS_PROFILING
   nameToProfiler.clear();
#endif
   return MS::kSuccess;
}