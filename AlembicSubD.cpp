#include "AlembicSubD.h"
#include <maya/MFnSubdData.h>
#include "MetaData.h"
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MFnSubDNames.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicSubD::AlembicSubD(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   MString name = truncateName(node.name());
   mObject = Alembic::AbcGeom::OSubD(GetParentObject(),name.asChar(),GetJob()->GetAnimatedTs());

   mSchema = mObject.getSchema();
}

AlembicSubD::~AlembicSubD()
{
   mObject.reset();
   mSchema.reset();
}

MStatus AlembicSubD::Save(double time)
{
   // access the geometry
   MFnSubd node(GetRef());

   // save the metadata
   SaveMetaData(this);

   // prepare the bounding box
   Alembic::Abc::Box3d bbox;

   // access the points
   mPosVec.resize(node.vertexCount());
   MPointArray positions;
   node.vertexBaseMeshGet(positions);
   for(unsigned int i=0;i<node.vertexCount();i++)
   {
      mPosVec[i].x = (float)positions[i].x;
      mPosVec[i].y = (float)positions[i].y;
      mPosVec[i].z = (float)positions[i].z;
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
      unsigned int polyCount = node.polygonCount();

      mFaceCountVec.reserve(polyCount);
      mFaceIndicesVec.reserve(polyCount * 4);
      mSampleLookup.reserve(polyCount * 4);
      unsigned int offset = 0;
      for(unsigned int i=0;i<polyCount;i++)
      {
         MUint64Array indices;
         unsigned int count = node.polygonVertexCount(MFnSubdNames::baseFaceIdFromIndex(i));
         node.polygonVertices(MFnSubdNames::baseFaceIdFromIndex(i),indices);
         mFaceCountVec.push_back(count);
         for(unsigned int j=0;j<(unsigned int)count;j++)
         {
            mSampleLookup.push_back(offset+count-j-1);
            mFaceIndicesVec.push_back(node.vertexBaseIndexFromVertexId(indices[count-j-1]));
         }
         offset += count;
      }

      Alembic::Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),mFaceCountVec.size());
      Alembic::Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),mFaceIndicesVec.size());
      mSample.setFaceCounts(faceCountSample);
      mSample.setFaceIndices(faceIndicesSample);

      // check if we need to export uvs
      if(GetJob()->GetOption(L"exportUVs").asInt() > 0)
      {
         if(node.polygonHasVertexUVs(MFnSubdNames::baseFaceIdFromIndex(0)))
         {
            unsigned int uvCount = (unsigned int)mSampleLookup.size();
            mUvVec.resize(uvCount);
            unsigned int offset = 0;
            for(unsigned int i=0;i<polyCount;i++)
            {
               MDoubleArray uValues;
               MDoubleArray vValues;
               unsigned int count = node.polygonVertexCount(MFnSubdNames::baseFaceIdFromIndex(i));
               node.polygonGetVertexUVs(MFnSubdNames::baseFaceIdFromIndex(i),uValues,vValues);
               for(unsigned j=0;j<count;j++)
               {
                  mUvVec[mSampleLookup[offset]].x = (float)uValues[j];
                  mUvVec[mSampleLookup[offset]].y = (float)vValues[j];
                  offset++;
               }
            }

            // now let's sort the uvs 
            unsigned int uvIndexCount = 0;
            if(GetJob()->GetOption(L"indexedUVs").asInt() > 0) {
               std::map<SortableV2f,size_t> uvMap;
               std::map<SortableV2f,size_t>::const_iterator it;
               unsigned int sortedUVCount = 0;
               std::vector<Alembic::Abc::V2f> sortedUVVec;
               mUvIndexVec.resize(mUvVec.size());
               sortedUVVec.resize(mUvVec.size());

               // loop over all uvs
               for(size_t i=0;i<mUvVec.size();i++)
               {
                  it = uvMap.find(mUvVec[i]);
                  if(it != uvMap.end())
                     mUvIndexVec[uvIndexCount++] = (uint32_t)it->second;
                  else
                  {
                     mUvIndexVec[uvIndexCount++] = (uint32_t)sortedUVCount;
                     uvMap.insert(std::pair<Alembic::Abc::V2f,size_t>(mUvVec[i],(uint32_t)sortedUVCount));
                     sortedUVVec[sortedUVCount++] = mUvVec[i];
                  }
               }

               // use indexed uvs if they use less space
               if(sortedUVCount * sizeof(Alembic::Abc::V2f) + 
                  uvIndexCount * sizeof(uint32_t) < 
                  sizeof(Alembic::Abc::V2f) * mUvVec.size())
               {
                  mUvVec = sortedUVVec;
                  uvCount = sortedUVCount;
               }
               else
               {
                  uvIndexCount = 0;
                  mUvIndexVec.clear();
               }
               sortedUVCount = 0;
               sortedUVVec.clear();
            }

            Alembic::AbcGeom::OV2fGeomParam::Sample uvSample(Alembic::Abc::V2fArraySample(&mUvVec.front(),uvCount),Alembic::AbcGeom::kFacevaryingScope);
            if(mUvIndexVec.size() > 0 && uvIndexCount > 0)
               uvSample.setIndices(Alembic::Abc::UInt32ArraySample(&mUvIndexVec.front(),uvIndexCount));
            mSample.setUVs(uvSample);
         }
      }

      // loop for facesets
      std::size_t attrCount = node.attributeCount();
      for (unsigned int i = 0; i < attrCount; ++i)
      {
         MObject attr = node.attribute(i);
         MFnAttribute mfnAttr(attr);
         MPlug plug = node.findPlug(attr, true);

         // if it is not readable, then bail without any more checking
         if (!mfnAttr.isReadable() || plug.isNull())
            continue;

         MString propName = plug.partialName(0, 0, 0, 0, 0, 1);
         std::string propStr = propName.asChar();
         if (propStr.substr(0, 8) == "FACESET_")
         {
            MStatus status;
            MFnIntArrayData arr(plug.asMObject(), &status);
            if (status != MS::kSuccess)
                continue;

            std::string faceSetName = propStr.substr(8);
            std::size_t numData = arr.length();
            std::vector<Alembic::Util::int32_t> faceVals(numData);
            for (unsigned int j = 0; j < numData; ++j)
                faceVals[j] = arr[j];

            Alembic::AbcGeom::OFaceSet faceSet = mSchema.createFaceSet(faceSetName);
            Alembic::AbcGeom::OFaceSetSchema::Sample faceSetSample;
            faceSetSample.setFaces(Alembic::Abc::Int32ArraySample(faceVals));
            faceSet.getSchema().set(faceSetSample);
         }
      }
   }

   // store the display resolution
   MObject attr = node.attribute("dispResolution");
   MFnAttribute mfnAttr(attr);
   MPlug plug = node.findPlug(attr, true);
   mSample.setFaceVaryingInterpolateBoundary(plug.asInt());

   // save the sample
   mSchema.set(mSample);
   mNumSamples++;

   return MStatus::kSuccess;
}

void AlembicSubDNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicSubDNode::~AlembicSubDNode()
{
   PreDestruction();
}

MObject AlembicSubDNode::mTimeAttr;
MObject AlembicSubDNode::mFileNameAttr;
MObject AlembicSubDNode::mIdentifierAttr;
MObject AlembicSubDNode::mUvsAttr;
MObject AlembicSubDNode::mOutGeometryAttr;
MObject AlembicSubDNode::mOutDispResolutionAttr;

MStatus AlembicSubDNode::initialize()
{
   MStatus status;

   MFnUnitAttribute uAttr;
   MFnTypedAttribute tAttr;
   MFnNumericAttribute nAttr;
   MFnGenericAttribute gAttr;
   MFnStringData emptyStringData;
   MObject emptyStringObject = emptyStringData.create("");

   // input time
   mTimeAttr = uAttr.create("inTime", "tm", MFnUnitAttribute::kTime, 0.0);
   status = uAttr.setStorable(true);
   status = uAttr.setKeyable(true);
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

   // input uvs
   mUvsAttr = nAttr.create("uvs", "uv", MFnNumericData::kBoolean, 1.0);
   status = tAttr.setStorable(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mUvsAttr);

   // output subd
   mOutGeometryAttr = tAttr.create("outSubdiv", "os", MFnData::kSubdSurface);
   status = tAttr.setStorable(false);
   status = tAttr.setWritable(false);
   status = tAttr.setKeyable(false);
   status = tAttr.setHidden(false);
   status = addAttribute(mOutGeometryAttr);

   // output disp resolution
   mOutDispResolutionAttr = nAttr.create("dispResolution", "dr", MFnNumericData::kInt, 1.0);
   status = nAttr.setStorable(false);
   status = nAttr.setWritable(false);
   status = nAttr.setKeyable(false);
   status = nAttr.setHidden(false);
   status = addAttribute(mOutDispResolutionAttr);

   // create a mapping
   status = attributeAffects(mTimeAttr, mOutGeometryAttr);
   status = attributeAffects(mFileNameAttr, mOutGeometryAttr);
   status = attributeAffects(mIdentifierAttr, mOutGeometryAttr);
   status = attributeAffects(mUvsAttr, mOutGeometryAttr);
   status = attributeAffects(mTimeAttr, mOutDispResolutionAttr);
   status = attributeAffects(mFileNameAttr, mOutDispResolutionAttr);
   status = attributeAffects(mIdentifierAttr, mOutDispResolutionAttr);
   return status;
}

MStatus AlembicSubDNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   MStatus status;

   // update the frame number to be imported
   double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
   MString & fileName = dataBlock.inputValue(mFileNameAttr).asString();
   MString & identifier = dataBlock.inputValue(mIdentifierAttr).asString();
   bool importUvs = dataBlock.inputValue(mUvsAttr).asBool();

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
      Alembic::AbcGeom::ISubD obj(iObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a Camera.");
         return MStatus::kFailure;
      }
      mSchema = obj.getSchema();
      mSubDData = MObject::kNullObj;
   }

   if(!mSchema.valid())
      return MStatus::kFailure;

   // get the sample
   SampleInfo sampleInfo = getSampleInfo(
      inputTime,
      mSchema.getTimeSampling(),
      mSchema.getNumSamples()
   );

   // check if we have to do this at all
   if(!mSubDData.isNull() && mLastSampleInfo.floorIndex == sampleInfo.floorIndex && mLastSampleInfo.ceilIndex == sampleInfo.ceilIndex)
      return MStatus::kSuccess;

   mLastSampleInfo = sampleInfo;

   // access the camera values
   Alembic::AbcGeom::ISubDSchema::Sample sample;
   Alembic::AbcGeom::ISubDSchema::Sample sample2;
   mSchema.get(sample,sampleInfo.floorIndex);
   if(sampleInfo.alpha != 0.0)
      mSchema.get(sample2,sampleInfo.ceilIndex);

   // create the output subd
   if(mSubDData.isNull())
   {
      MFnSubdData subdDataFn;
      mSubDData = subdDataFn.create();
   }

   Alembic::Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   Alembic::Abc::Int32ArraySamplePtr sampleCounts = sample.getFaceCounts();
   Alembic::Abc::Int32ArraySamplePtr sampleIndices = sample.getFaceIndices();

   MPointArray points;
   if(samplePos->size() > 0)
   {
      points.setLength((unsigned int)samplePos->size());
      bool done = false;
      if(sampleInfo.alpha != 0.0)
      {
         Alembic::Abc::P3fArraySamplePtr samplePos2 = sample2.getPositions();
         if(points.length() == (unsigned int)samplePos2->size())
         {
            float blend = (float)sampleInfo.alpha;
            float iblend = 1.0f - blend;
            for(unsigned int i=0;i<points.length();i++)
            {
               points[i].x = samplePos->get()[i].x * iblend + samplePos2->get()[i].x * blend;
               points[i].y = samplePos->get()[i].y * iblend + samplePos2->get()[i].y * blend;
               points[i].z = samplePos->get()[i].z * iblend + samplePos2->get()[i].z * blend;
            }
            done = true;
         }
      }

      if(!done)
      {
         for(unsigned int i=0;i<points.length();i++)
         {
            points[i].x = samplePos->get()[i].x;
            points[i].y = samplePos->get()[i].y;
            points[i].z = samplePos->get()[i].z;
         }
      }
   }

   // check if we already have the right polygons
   if(mSubD.vertexCount() != points.length() || 
      mSubD.polygonCount() != (unsigned int)sampleCounts->size())
   {
      MIntArray counts;
      MIntArray indices;
      counts.setLength((unsigned int)sampleCounts->size());
      indices.setLength((unsigned int)sampleIndices->size());
      mSampleLookup.resize(indices.length());

      unsigned int offset = 0;
      for(unsigned int i=0;i<counts.length();i++)
      {
         counts[i] = sampleCounts->get()[i];
         for(int j=0;j<counts[i];j++)
         {
            MString count,index;
            count.set((double)counts[i]);
            index.set((double)sampleIndices->get()[offset+j]);
            mSampleLookup[offset+counts[i]-j-1] = offset+j;
            indices[offset+j] = sampleIndices->get()[offset+counts[i]-j-1];
         }
         offset += counts[i];
      }

      // create a subd either with or without uvs
      mSubD.createBaseMesh(false,points.length(),counts.length(),points,counts,indices,mSubDData);
      //mSubD.updateSubdSurface();

      // check if we need to import uvs
      if(importUvs)
      {
         Alembic::AbcGeom::IV2fGeomParam uvsParam = mSchema.getUVsParam();
         if(uvsParam.valid())
         {
            if(uvsParam.getNumSamples() > 0)
            {
               sampleInfo = getSampleInfo(
                  inputTime,
                  uvsParam.getTimeSampling(),
                  uvsParam.getNumSamples()
               );

               Alembic::Abc::V2fArraySamplePtr sampleUvs = uvsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
               if(sampleUvs->size() == (size_t)indices.length())
               {
                  unsigned int offset = 0;
                  MUint64Array indices;
                  for(unsigned int i=0;i<mSubD.polygonCount();i++)
                  {
                     unsigned int count = mSubD.polygonVertexCount(MFnSubdNames::baseFaceIdFromIndex(i));
                     MDoubleArray uValues,vValues;
                     uValues.setLength(count);
                     vValues.setLength(count);
                     for(unsigned int j=0;j<count;j++)
                     {
                        uValues[count-j-1] = sampleUvs->get()[offset].x;
                        vValues[count-j-1] = sampleUvs->get()[offset].y;
                        offset++;
                     }
                     mSubD.polygonSetVertexUVs(MFnSubdNames::baseFaceIdFromIndex(i),uValues,vValues);
                     mSubD.polygonSetUseUVs(MFnSubdNames::baseFaceIdFromIndex(i),true);
                  }
               }
            }
         }
      }
   }
   else if(mSubD.vertexCount() == points.length())
      mSubD.vertexBaseMeshSet(points);

   // output all channels
   dataBlock.outputValue(mOutGeometryAttr).set(mSubDData);
   dataBlock.outputValue(mOutDispResolutionAttr).set((int)sample.getFaceVaryingInterpolateBoundary());

   return MStatus::kSuccess;
}
