#include "Utility.h"
#include "AlembicLicensing.h"

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

   std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
   iTime->getFloorIndex(iFrame, numSamps);

   result.floorIndex = floorIndex.first;
   result.ceilIndex = result.floorIndex;

   // check if we have a full license
   if(!HasFullLicense())
   {
      if(result.floorIndex > 75)
      {
         EC_LOG_WARNING("[ExocortexAlembic] Demo Mode: Cannot open sample indices higher than 75.");
         result.floorIndex = 75;
         result.ceilIndex = 75;
         result.alpha = 0.0;
         return result;
      }
   }

   if (fabs(iFrame - floorIndex.second) < 0.0001) {
      result.alpha = 0.0f;
      return result;
   }

   std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
   iTime->getCeilIndex(iFrame, numSamps);

   if (fabs(iFrame - ceilIndex.second) < 0.0001) {
      result.floorIndex = ceilIndex.first;
      result.ceilIndex = result.floorIndex;
      result.alpha = 0.0f;
      return result;
   }

   if (result.floorIndex == ceilIndex.first) {
      result.alpha = 0.0f;
      return result;
   }

   result.ceilIndex = ceilIndex.first;

   result.alpha = (iFrame - floorIndex.second) / (ceilIndex.second - floorIndex.second);

   return result;
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

MString truncateName(const MString & in_Name)
{
   MString name = in_Name;
   // TODO: debug if this is right
   if(name.substring(name.length()-3,name.length()-1).toLowerCase() == "xfo")
      name = name.substring(0,name.length()-3);
   return name;
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
   MString parentXfoName;
   MString objName = truncateName(parts[parts.length()-1]);
   if(objName.toLowerCase() == (parts[parts.length()-1]).toLowerCase())
      return objName;
   if(parts.length() > 2)
      parentXfoName = truncateName(parts[parts.length()-3]) + ".";
   return parentXfoName+objName;
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

MString getTypeFromObject(Alembic::Abc::IObject object)
{
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return "Xform";
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return "PolyMesh";
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return "Curves";
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return "NuPatch";
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return "Points";
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return "SubD";
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return "Camera";
   }
   return "";
}

Alembic::Abc::ICompoundProperty getCompoundFromObject(Alembic::Abc::IObject object)
{
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema();
   }
   return Alembic::Abc::ICompoundProperty();
}

Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::IObject object)
{
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   }
   return Alembic::Abc::TimeSamplingPtr();
}

size_t getNumSamplesFromObject(Alembic::Abc::IObject object)
{
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
   }
   return 0;
}
