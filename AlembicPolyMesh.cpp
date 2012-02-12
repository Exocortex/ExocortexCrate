#include "AlembicPolyMesh.h"

#include <maya/MFnMesh.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicPolyMesh::AlembicPolyMesh(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   MString name = node.name();
   mObject = Alembic::AbcGeom::OPolyMesh(GetParentObject(),name.asChar(),GetJob()->GetAnimatedTs());

   mSchema = mObject.getSchema();
}

AlembicPolyMesh::~AlembicPolyMesh()
{
   mObject.reset();
   mSchema.reset();
}

MStatus AlembicPolyMesh::Save(double time)
{
   // access the geometry
   MFnMesh node(GetRef());

   // TODO: implement storage of metadata
   // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

   // prepare the bounding box
   Alembic::Abc::Box3d bbox;

   // access the points
   MFloatPointArray points;
   node.getPoints(points);

   // face indices

   mPosVec.resize(points.length());
   for(unsigned int i=0;i<points.length();i++)
   {
      mPosVec[i].x = points[i].x;
      mPosVec[i].y = points[i].y;
      mPosVec[i].z = points[i].z;
      bbox.extendBy(mPosVec[i]);
   }

   // store the positions to the samples
   mSample.setPositions(Alembic::Abc::P3fArraySample(&mPosVec.front(),mPosVec.size()));
   mSample.setSelfBounds(bbox);

   // check if we are doing pure pointcache
   if(GetJob()->GetOption(L"exportPurePointCache").asInt() > 0)
   {
      if(mNumSamples == 0)
      {
         // store a dummy empty topology
         mFaceCountVec.push_back(0);
         mFaceIndicesVec.push_back(0);
         Alembic::Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),mFaceCountVec.size());
         Alembic::Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),mFaceIndicesVec.size());
         mSample.setFaceCounts(faceCountSample);
         mSample.setFaceIndices(faceIndicesSample);
      }
      mSchema.set(mSample);
      mNumSamples++;
      return MStatus::kSuccess;
   }

   bool dynamicTopology = GetJob()->GetOption(L"exportDynamicTopology").asInt() > 0;
   if(mNumSamples == 0 || dynamicTopology)
   {
      MIntArray counts,indices;
      node.getVertices(counts,indices);

      mFaceCountVec.resize(counts.length());
      mFaceIndicesVec.resize(indices.length());
      unsigned int offset = 0;
      for(unsigned int i=0;i<counts.length();i++)
      {
         mFaceCountVec[i] = counts[i];
         for(unsigned int j=0;j<(unsigned int)counts[i];j++)
            mFaceIndicesVec[offset+counts[i]-j-1] = indices[offset+j];
         offset += counts[i];
      }

      Alembic::Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),mFaceCountVec.size());
      Alembic::Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),mFaceIndicesVec.size());
      mSample.setFaceCounts(faceCountSample);
      mSample.setFaceIndices(faceIndicesSample);


      // TODO: UVS

      // TODO: check for facesets
   }

   // now do the normals
   // let's check if we have user normals
   unsigned int normalCount = 0;
   unsigned int normalIndexCount = 0;
   if(GetJob()->GetOption(L"exportNormals").asInt() > 0)
   {
      MFloatVectorArray normals;
      node.getNormals(normals);
      normalCount = normals.length();
      mNormalVec.resize(normalCount);
      memcpy(&mNormalVec[0],&normals[0],sizeof(float) * normalCount * 3);

      // now let's sort the normals 
      if(GetJob()->GetOption(L"indexedNormals").asInt() > 0) {
         std::map<SortableV3f,size_t> normalMap;
         std::map<SortableV3f,size_t>::const_iterator it;
         unsigned int sortedNormalCount = 0;
         std::vector<Alembic::Abc::V3f> sortedNormalVec;
         mNormalIndexVec.resize(mNormalVec.size());
         sortedNormalVec.resize(mNormalVec.size());

         // loop over all normals
         for(size_t i=0;i<mNormalVec.size();i++)
         {
            it = normalMap.find(mNormalVec[i]);
            if(it != normalMap.end())
               mNormalIndexVec[normalIndexCount++] = (uint32_t)it->second;
            else
            {
               mNormalIndexVec[normalIndexCount++] = (uint32_t)sortedNormalCount;
               normalMap.insert(std::pair<Alembic::Abc::V3f,size_t>(mNormalVec[i],(uint32_t)sortedNormalCount));
               sortedNormalVec[sortedNormalCount++] = mNormalVec[i];
            }
         }

         // use indexed normals if they use less space
         if(sortedNormalCount * sizeof(Alembic::Abc::V3f) + 
            normalIndexCount * sizeof(uint32_t) < 
            sizeof(Alembic::Abc::V3f) * mNormalVec.size())
         {
            mNormalVec = sortedNormalVec;
            normalCount = sortedNormalCount;
         }
         else
         {
            normalIndexCount = 0;
            mNormalIndexVec.clear();
         }
         sortedNormalCount = 0;
         sortedNormalVec.clear();
      }

      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
      if(mNormalVec.size() > 0 && normalCount > 0)
      {
         normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&mNormalVec.front(),normalCount));
         if(normalIndexCount > 0)
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&mNormalIndexVec.front(),normalIndexCount));
         mSample.setNormals(normalSample);
      }
   }

   // save the sample
   mSchema.set(mSample);
   mNumSamples++;

   return MStatus::kSuccess;
}

AlembicPolyMeshNode::~AlembicPolyMeshNode()
{
   mSchema.reset();
   delRefArchive(mFileName);
}

MObject AlembicPolyMeshNode::mTimeAttr;
MObject AlembicPolyMeshNode::mFileNameAttr;
MObject AlembicPolyMeshNode::mIdentifierAttr;
MObject AlembicPolyMeshNode::mOutGeometryAttr;

MStatus AlembicPolyMeshNode::initialize()
{
   MStatus status;

   MFnUnitAttribute uAttr;
   MFnTypedAttribute tAttr;
   MFnNumericAttribute nAttr;
   MFnGenericAttribute gAttr;
   MFnStringData emptyStringData;
   MObject emptyStringObject = emptyStringData.create("");

   // input time
   mTimeAttr = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0);
   status = uAttr.setStorable(true);
   status = addAttribute(mTimeAttr);

   // input file name
   mFileNameAttr = tAttr.create("fileName", "fn", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setUsedAsFilename(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mFileNameAttr);

   // input identifier
   mIdentifierAttr = tAttr.create("identifier", "it", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mIdentifierAttr);

   // output mesh
   /*
   mOutGeometryAttr = nAttr.create("focalLength", "fl", MFnNumericData::kDouble, 30.0);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = addAttribute(mOutGeometryAttr);
   */


   // create a mapping
   //status = attributeAffects(mTimeAttr, mOutGeometryAttr);
   //status = attributeAffects(mFileNameAttr, mOutGeometryAttr);
   //status = attributeAffects(mIdentifierAttr, mOutGeometryAttr);

   return status;
}

MStatus AlembicPolyMeshNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   MStatus status;

   // update the frame number to be imported
   double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
   MString & fileName = dataBlock.inputValue(mFileNameAttr).asString();
   MString & identifier = dataBlock.inputValue(mIdentifierAttr).asString();

   // check if we have the file
   if(fileName != mFileName || identifier != mIdentifier)
   {
      mSchema.reset();
      if(fileName != mFileName)
      {
         delRefArchive(mFileName);
         mFileName = fileName;
         addRefArchive(mFileName);
      }
      mIdentifier = identifier;

      // get the object from the archive
      Alembic::Abc::IObject iObj = getObjectFromArchive(mFileName,identifier);
      if(!iObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      Alembic::AbcGeom::IPolyMesh obj(iObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a Camera.");
         return MStatus::kFailure;
      }
      mSchema = obj.getSchema();
   }

   if(!mSchema.valid())
      return MStatus::kFailure;

   // get the sample
   SampleInfo sampleInfo = getSampleInfo(
      inputTime,
      mSchema.getTimeSampling(),
      mSchema.getNumSamples()
   );

   // access the camera values
   Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
   mSchema.get(sample,sampleInfo.floorIndex);

   // blend the sample if we are between frames
   if(sampleInfo.alpha != 0.0)
   {
      mSchema.get(sample,sampleInfo.ceilIndex);
   }

   // output all channels
   //dataBlock.outputValue(mOutGeometryAttr);

   return status;
}
