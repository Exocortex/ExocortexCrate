#include "AlembicPolyMesh.h"
#include <maya/MFnMeshData.h>
#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>
#include "MetaData.h"
#include <sstream>
#include "CommonMeshUtilities.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicPolyMesh::AlembicPolyMesh(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   //MString name = GetUniqueName(truncateName(node.name()));
   MString name = GetUniqueName(node.name());
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
   MDagPath path;
   node.getPath(path);

   // save the metadata
   Alembic::AbcGeom::OPolyMeshSchema::Sample mSample;
   std::vector<Alembic::Abc::V3f> mPosVec;
   std::vector<Alembic::AbcGeom::OV2fGeomParam> mUvParams;

   SaveMetaData(this);

   // prepare the bounding box
   Alembic::Abc::Box3d bbox;

   // access the points
   MFloatPointArray points;
   node.getPoints(points);

   // check if we have the global cache option
   bool globalCache = GetJob()->GetOption(L"exportInGlobalSpace").asInt() > 0;
   Alembic::Abc::M44f globalXfo;
   if(globalCache)
      globalXfo = GetGlobalMatrix(GetRef());

   // ensure to keep the same topology if dynamic topology is disabled
   bool dynamicTopology = GetJob()->GetOption(L"exportDynamicTopology").asInt() > 0;
   if(!dynamicTopology && mNumSamples > 0)
   {
      if(mPointCountLastFrame  != (size_t)points.length())
      {
         MString fullName = MFnDagNode(GetRef()).fullPathName();
		 EC_LOG_ERROR( "Object '" << fullName.asChar() << "' contains dynamic topology (original vertex count " << mPointCountLastFrame  << " and new vertex count " << points.length() << "). Not exporting sample.");
         return MStatus::kFailure;
      }
   }
   mPointCountLastFrame = points.length();

   mPosVec.resize(points.length());
   for(unsigned int i=0;i<points.length();i++)
   {
      mPosVec[i].x = points[i].x;
      mPosVec[i].y = points[i].y;
      mPosVec[i].z = points[i].z;
      if(globalCache)
         globalXfo.multVecMatrix(mPosVec[i],mPosVec[i]);
      bbox.extendBy(mPosVec[i]);
   }

   // store the positions to the samples
   mSample.setPositions(Alembic::Abc::P3fArraySample(&mPosVec.front(),mPosVec.size()));
   mSample.setSelfBounds(bbox);

   // check if we are doing pure pointcache
   std::vector<Alembic::Abc::int32_t> mFaceCountVec;
   std::vector<Alembic::Abc::int32_t> mFaceIndicesVec;
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

   std::vector<std::vector<Alembic::Abc::V2f> > mUvVec;
   std::vector<std::vector<Alembic::Abc::uint32_t> > mUvIndexVec;
   if(mNumSamples == 0 || dynamicTopology)
   {
      MIntArray counts,indices;
      node.getVertices(counts,indices);

      mFaceCountVec.resize(counts.length());
      mFaceIndicesVec.resize(indices.length());
      mSampleLookup.resize(indices.length());
      unsigned int offset = 0;
      for(unsigned int i=0, k = 0; i<counts.length(); ++i)
      {
         const unsigned int cnt = (mFaceCountVec[i] = counts[i]);
         for(unsigned int j=0; j<cnt; ++j, ++k)
         {
            const int offPos = offset + cnt - (j+1);
            mSampleLookup[offPos] = offset+j;
            mFaceIndicesVec[offPos] = indices[offset+j];
         }
         offset += cnt;
      }

      Alembic::Abc::Int32ArraySample faceCountSample(mFaceCountVec);
      Alembic::Abc::Int32ArraySample faceIndicesSample(mFaceIndicesVec);
      mSample.setFaceCounts(faceCountSample);
      mSample.setFaceIndices(faceIndicesSample);

      // check if we need to export uvs
      if(GetJob()->GetOption(L"exportUVs").asInt() > 0)
      {
         MStatus status;
         MStringArray uvSetNames;
         node.getUVSetNames(uvSetNames);

         if(mNumSamples == 0)
         {
            std::vector<std::string> cUvSetNames;
            for(unsigned int uvSetIndex = 0; uvSetIndex < uvSetNames.length(); uvSetIndex++)
               cUvSetNames.push_back(uvSetNames[uvSetIndex].asChar());
            Alembic::Abc::OStringArrayProperty uvSetNamesProperty = Alembic::Abc::OStringArrayProperty(
               GetCompound(), ".uvSetNames", GetCompound().getMetaData(), GetJob()->GetAnimatedTs() );
            Alembic::Abc::StringArraySample uvSetNamesSample(&cUvSetNames.front(),cUvSetNames.size());
            uvSetNamesProperty.set(uvSetNamesSample);
         }

         int actualUvSetIndex = 0;
         for(unsigned int uvSetIndex = 0; uvSetIndex < uvSetNames.length(); uvSetIndex++)
         {
            const MString &uvSetName = uvSetNames[uvSetIndex];
            if (uvSetName != MString(""))
            {
               MFloatArray uValues, vValues;
               status = node.getUVs(uValues, vValues, &uvSetName);
               if ( uValues.length() == vValues.length() )
               {
                  MIntArray uvCounts, uvIds;
                  status = node.getAssignedUVs(uvCounts, uvIds, &uvSetName);
                  unsigned int uvCount = (unsigned int)mSampleLookup.size();
                  if(uvIds.length() == uvCount)
                  {
                     mUvVec.push_back(std::vector<Alembic::Abc::V2f>());
                     std::vector<Alembic::Abc::V2f> &uvVecIndexed = mUvVec.back();
                     mUvIndexVec.push_back(std::vector<Alembic::Abc::uint32_t>());
                     std::vector<Alembic::Abc::uint32_t> &uvIndexVec = mUvIndexVec.back();

                     uvVecIndexed.resize(uValues.length());
                     for (int i = 0; i < uvVecIndexed.size(); ++i)
                     {
                       Alembic::Abc::V2f &curUV = uvVecIndexed[i];
                       curUV.x = uValues[i];
                       curUV.y = vValues[i];
                     }

                     uvIndexVec.resize(uvIds.length());
                     for (int i = 0; i < uvIndexVec.size(); ++i)
                       uvIndexVec[mSampleLookup[i]] = uvIds[i];

                     Alembic::AbcGeom::OV2fGeomParam::Sample uvSample(Alembic::Abc::V2fArraySample(uvVecIndexed),Alembic::AbcGeom::kFacevaryingScope);
                     if(uvIndexVec.size() > 0)
                        uvSample.setIndices(Alembic::Abc::UInt32ArraySample(uvIndexVec));

                     if(actualUvSetIndex == 0)
                     {
                        mSample.setUVs(uvSample);
                     }
                     else
                     {
                        if(mNumSamples == 0)
                        {
                           MString storedUvSetName;
                           storedUvSetName.set((double)actualUvSetIndex);
                           storedUvSetName = MString("uv") + storedUvSetName;
                           mUvParams.push_back(Alembic::AbcGeom::OV2fGeomParam( mSchema, storedUvSetName.asChar(), uvIndexVec.size() > 0, Alembic::AbcGeom::kFacevaryingScope, 1, mSchema.getTimeSampling()));
                        }
                        mUvParams.back().set(uvSample);
                     }
                     ++actualUvSetIndex;
                  }
               }
            }
         }
      }

      if(GetJob()->GetOption(L"exportFaceSets").asInt() > 0)
      {
        // loop for facesets
        std::map<std::string,unsigned int> setNameMap;
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

              // ensure the faceSetName is unique
              std::string faceSetName = propStr.substr(8);
              int suffixId = 1;
              std::string suffix;
              while(setNameMap.find(faceSetName+suffix) != setNameMap.end()) {
                 std::stringstream out;
                 out << suffixId;
                 suffix = out.str();
                 suffixId++;
              }
              faceSetName += suffix;
              setNameMap.insert(std::pair<std::string, unsigned int>(faceSetName, (unsigned int)setNameMap.size()));

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

        // more face sets, based on the material assignments
        MObjectArray sets, comps;
        unsigned int instanceNumber = path.instanceNumber();
        node.getConnectedSetsAndMembers( instanceNumber, sets, comps, 1 );
        for ( unsigned int i = 0; i < sets.length() ; i++ )
        {
           MFnSet setFn ( sets[i] );
           MItMeshPolygon tempFaceIt ( path, comps[i] ); 
           std::vector<Alembic::Util::int32_t> faceVals(tempFaceIt.count());
           unsigned int j=0;
           for ( ;!tempFaceIt.isDone() ; tempFaceIt.next() )
              faceVals[j++] = tempFaceIt.index();

           // ensure the faceSetName is unique
           std::string faceSetName = setFn.name().asChar();
           int suffixId = 1;
           std::string suffix;
           while(setNameMap.find(faceSetName+suffix) != setNameMap.end()) {
              std::stringstream out;
              out << suffixId;
              suffix = out.str();
              suffixId++;
           }
           faceSetName += suffix;
           setNameMap.insert(std::pair<std::string, unsigned int>(faceSetName, (unsigned int)setNameMap.size()));

           Alembic::AbcGeom::OFaceSet faceSet = mSchema.createFaceSet(faceSetName);
           Alembic::AbcGeom::OFaceSetSchema::Sample faceSetSample;
           faceSetSample.setFaces(Alembic::Abc::Int32ArraySample(faceVals));
           faceSet.getSchema().set(faceSetSample);
        }
      }
   }

   // now do the normals
   // let's check if we have user normals
   int normalCount = 0;
   std::vector<Alembic::Abc::N3f> indexedNormalsValues;
   std::vector<unsigned int> indexedNormalsIndices;
   if(GetJob()->GetOption(L"exportNormals").asInt() > 0)
   {
     {
       MFloatVectorArray normalsArray;
       node.getNormals(normalsArray);

       indexedNormalsValues.resize(normalsArray.length());
       for (int i = 0; i < indexedNormalsValues.size(); ++i)
       {
         const MFloatVector &nOut = normalsArray[i];
         Alembic::Abc::N3f  &nIn  = indexedNormalsValues[i];

         nIn.x = nOut.x;
         nIn.y = nOut.y;
         nIn.z = nOut.z;
       }
     }

     {
       MIntArray normPerFaceArray;
       MIntArray normalIDsArray;
       node.getNormalIds(normPerFaceArray, normalIDsArray);

       indexedNormalsIndices.resize(normalIDsArray.length());
       for (int i = 0; i < indexedNormalsIndices.size(); ++i)
         indexedNormalsIndices[mSampleLookup[i]] = normalIDsArray[i];
     }

     Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
     normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
     normalSample.setVals(Alembic::Abc::N3fArraySample(indexedNormalsValues));
     normalSample.setIndices(Alembic::Abc::UInt32ArraySample(indexedNormalsIndices));
     mSample.setNormals(normalSample);
   }

   // save the sample
   mSchema.set(mSample);
   mNumSamples++;
   return MStatus::kSuccess;
}

void AlembicPolyMeshNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicPolyMeshNode::~AlembicPolyMeshNode()
{
   PreDestruction();
}

MObject AlembicPolyMeshNode::mTimeAttr;
MObject AlembicPolyMeshNode::mFileNameAttr;
MObject AlembicPolyMeshNode::mIdentifierAttr;
MObject AlembicPolyMeshNode::mNormalsAttr;
MObject AlembicPolyMeshNode::mUvsAttr;
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
   mIdentifierAttr = tAttr.create("identifier", "if", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mIdentifierAttr);

   // input normals
   mNormalsAttr = nAttr.create("normals", "nm", MFnNumericData::kBoolean, 1.0);
   status = tAttr.setStorable(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mNormalsAttr);

   // input normals
   mUvsAttr = nAttr.create("uvs", "uv", MFnNumericData::kBoolean, 1.0);
   status = tAttr.setStorable(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mUvsAttr);

   // output mesh
   mOutGeometryAttr = tAttr.create("outMesh", "om", MFnData::kMesh);
   status = tAttr.setStorable(false);
   status = tAttr.setWritable(false);
   status = tAttr.setKeyable(false);
   status = tAttr.setHidden(false);
   status = addAttribute(mOutGeometryAttr);

   // create a mapping
   status = attributeAffects(mTimeAttr, mOutGeometryAttr);
   status = attributeAffects(mFileNameAttr, mOutGeometryAttr);
   status = attributeAffects(mIdentifierAttr, mOutGeometryAttr);
   status = attributeAffects(mNormalsAttr, mOutGeometryAttr);
   status = attributeAffects(mUvsAttr, mOutGeometryAttr);

   return status;
}

MStatus AlembicPolyMeshNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   ESS_PROFILE_SCOPE("AlembicPolyMeshNode::compute");
   MStatus status;

   // update the frame number to be imported
   double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
   MString & fileName = dataBlock.inputValue(mFileNameAttr).asString();
   MString & identifier = dataBlock.inputValue(mIdentifierAttr).asString();
   bool importNormals = dataBlock.inputValue(mNormalsAttr).asBool();
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
      mObj = getObjectFromArchive(mFileName,identifier);
      if(!mObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      Alembic::AbcGeom::IPolyMesh obj(mObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a PolyMesh.");
         return MStatus::kFailure;
      }
      mSchema = obj.getSchema();
      mMeshData = MObject::kNullObj;
   }

   if(!mSchema.valid())
      return MStatus::kFailure;

   // get the sample
   SampleInfo sampleInfo = getSampleInfo(
      inputTime,
      mSchema.getTimeSampling(),
      mSchema.getNumSamples()
   );

   bool isTopologyDynamic = false;
   if( mObj.valid() ) {
	   isTopologyDynamic = isAlembicMeshTopoDynamic( &mObj );
	   //ESS_LOG_WARNING( "(A) isTopologyDynamic = " << isTopologyDynamic );
   }

   // check if we have to do this at all
   if(!mMeshData.isNull() && 
	   mLastSampleInfo.floorIndex == sampleInfo.floorIndex && 
	   mLastSampleInfo.ceilIndex == sampleInfo.ceilIndex && ! isTopologyDynamic ) {
      //ESS_LOG_WARNING( "not doing this at all." );
      return MStatus::kSuccess;
   }

   mLastSampleInfo = sampleInfo;

   // access the camera values
   Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
   Alembic::AbcGeom::IPolyMeshSchema::Sample sample2;
   mSchema.get(sample,sampleInfo.floorIndex);
   if(sampleInfo.alpha != 0.0)
      mSchema.get(sample2,sampleInfo.ceilIndex);

   // create the output mesh
   if(mMeshData.isNull())
   {
      MFnMeshData meshDataFn;
      mMeshData = meshDataFn.create();
   }

   Alembic::Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   Alembic::Abc::V3fArraySamplePtr sampleVel = sample.getVelocities();
      
   Alembic::Abc::Int32ArraySamplePtr sampleCounts = sample.getFaceCounts();
   Alembic::Abc::Int32ArraySamplePtr sampleIndices = sample.getFaceIndices();

   // ensure that we are not running on a purepoint cache mesh
   if(sampleCounts->get()[0] == 0)
      return MStatus::kFailure;


   MFloatPointArray points;
   if(samplePos->size() > 0)
   {
      points.setLength((unsigned int)samplePos->size());
      bool done = false;
      //ESS_LOG_WARNING( "sampleInfo.alpha: " << sampleInfo.alpha );
      if(sampleInfo.alpha != 0.0)
      {
         Alembic::Abc::P3fArraySamplePtr samplePos2 = sample2.getPositions();
		 if( isTopologyDynamic ) {
			 if( sampleVel != NULL ) {
					float timeAlpha = getTimeOffsetFromSchema( mSchema, sampleInfo );
				   //ESS_LOG_WARNING( "timeAlpha: " << timeAlpha );
				   if( sampleVel->size() == samplePos->size() ) {
					   for(unsigned int i=0;i< sampleVel->size();i++)
					   {
						   points[i].x = samplePos->get()[i].x + timeAlpha * sampleVel->get()[i].x;
						   points[i].y = samplePos->get()[i].y + timeAlpha * sampleVel->get()[i].y;
						   points[i].z = samplePos->get()[i].z + timeAlpha * sampleVel->get()[i].z;
						}
					   done = true;
				   }
			 }
		 }
         else if(points.length() == (unsigned int)samplePos2->size() )
         {
            //ESS_LOG_WARNING( "blending vertex positions (1-2) A." );
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
   if(mMesh.numVertices() != points.length() || 
      mMesh.numPolygons() != (unsigned int)sampleCounts->size() || 
      mMesh.numFaceVertices() != (unsigned int)sampleIndices->size() ||
	  isTopologyDynamic
	  )
   {
	  //ESS_LOG_WARNING( "Updating face topology." );

      MIntArray counts;
      MIntArray indices;
      counts.setLength((unsigned int)sampleCounts->size());
      indices.setLength((unsigned int)sampleIndices->size());
      mSampleLookup.resize(indices.length());
      mNormalFaces.setLength(indices.length());
      mNormalVertices.setLength(indices.length());

      unsigned int offset = 0;
      for(unsigned int i=0;i<counts.length();i++)
      {
         counts[i] = sampleCounts->get()[i];
         for(int j=0;j<counts[i];j++)
         {
            //MString count,index;
            //count.set((double)counts[i]);
            //index.set((double)sampleIndices->get()[offset+j]);
            mSampleLookup[offset+counts[i]-j-1] = offset+j;
            indices[offset+j] = sampleIndices->get()[offset+counts[i]-j-1];

            mNormalFaces[offset+j] = i;
            mNormalVertices[offset+j] = indices[offset+j];
         }
         offset += counts[i];
      }

      // create a mesh either with or without uvs
      mMesh.create(points.length(),counts.length(),points,counts,indices,mMeshData);
      mMesh.updateSurface();
      if(mMesh.numFaceVertices() != indices.length()){
		  EC_LOG_ERROR("Error: mesh topology has changed. Cannot import UVs or normals.");
		  return MStatus::kFailure;
		  //importUvs = false;
		  //importNormals = false;
      }

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

               // check if we have uvSetNames
               MStringArray uvSetNames;
               if ( mSchema.getPropertyHeader( ".uvSetNames" ) != NULL )
               {
                  Alembic::Abc::IStringArrayProperty uvSetNamesProp = Alembic::Abc::IStringArrayProperty( mSchema, ".uvSetNames" );
                  Alembic::Abc::StringArraySamplePtr ptr = uvSetNamesProp.getValue(0);
                  for(size_t i=0;i<ptr->size();i++)
                     uvSetNames.append(ptr->get()[i].c_str());
               }

			   if(uvSetNames.length() <= 0){
                  uvSetNames.append("uvset1");
			   }
			   //else{
			   //   //rename the first uv set to the "map1" since maya doesn't seem allow the default uv set to be renamed
      //            uvSetNames[0] = "map1";
			   //}
          
			   //delete all uvsets other than the default, which is named "map1"   
			   MStringArray existingUVSets;
               mMesh.getUVSetNames(existingUVSets);
               for(unsigned int i=0;i<existingUVSets.length();i++)
               {
			      if(existingUVSets[i] == "map1") continue;
                  status = mMesh.deleteUVSet(existingUVSets[i]);
				  if ( status != MS::kSuccess ){
					  EC_LOG_ERROR("mMesh.deleteUVSet(\""<<existingUVSets[i]<<"\") failed: "<<status.errorString().asChar());
				  }
               }
			   //status = mMesh.clearUVs(&uvSetNames[0]);
			   //if ( status != MS::kSuccess ) cout << "mMesh.clearUVs(\"map1\") failed: "<<status.errorString().asChar()<< endl;
			   


			   for(unsigned int uvSetIndex = 0; uvSetIndex < uvSetNames.length(); uvSetIndex++)
			   {
                  status = mMesh.createUVSetDataMesh( uvSetNames[uvSetIndex] );
				  if( status != MS::kSuccess ){
					  EC_LOG_ERROR("mMesh.createUVSet(\""<<uvSetNames[uvSetIndex]<<"\") failed: "<<status.errorString().asChar());
				  }
			   }
              


			   for(unsigned int uvSetIndex =0; uvSetIndex < uvSetNames.length(); uvSetIndex++)
			   {
				  std::vector<Imath::V2f> uvValuesFloor, uvValuesCeil;
				  std::vector<AbcA::uint32_t> uvIndicesFloor, uvIndicesCeil;
					bool uvFloor = false, uvCeil = false;
				  if(uvSetIndex == 0) {
					uvFloor = getIndexAndValues( sampleIndices, uvsParam, sampleInfo.floorIndex,
					   uvValuesFloor, uvIndicesFloor );
					uvCeil = getIndexAndValues( sampleIndices, uvsParam, sampleInfo.ceilIndex,
					   uvValuesCeil, uvIndicesCeil );
				  }
                  else
                  {
                     MString storedUvSetName;
                     storedUvSetName.set((double)uvSetIndex);
                     storedUvSetName = MString("uv") + storedUvSetName;
                     if(mSchema.getPropertyHeader( storedUvSetName.asChar() ) == NULL)
                        continue;
                     Alembic::AbcGeom::IV2fGeomParam uvParamExtended = Alembic::AbcGeom::IV2fGeomParam( mSchema, storedUvSetName.asChar() );
					 uvFloor = getIndexAndValues( sampleIndices, uvParamExtended, sampleInfo.floorIndex,
					   uvValuesFloor, uvIndicesFloor );
					 uvCeil = getIndexAndValues( sampleIndices, uvParamExtended, sampleInfo.ceilIndex,
					   uvValuesCeil, uvIndicesCeil );
                  }

                  if(uvIndicesFloor.size() == (size_t)indices.length())
                  {
                     MFloatArray uValues((unsigned int)uvValuesFloor.size()), vValues((unsigned int)uvValuesFloor.size());
                      if((sampleInfo.alpha != 0.0)&& uvCeil && uvIndicesFloor.size() == uvIndicesCeil.size() && ! isTopologyDynamic )
					  {
						 float blend = (float)sampleInfo.alpha;
						 float iblend = 1.0f - blend;
						 for(unsigned int i=0;i<uvValuesFloor.size();i++)
						 {
							uValues[i] = uvValuesFloor[i].x * iblend + uvValuesCeil[i].x * blend;
							vValues[i] = uvValuesFloor[i].y * iblend + uvValuesCeil[i].y * blend;
						 }	                 
				   }
				   else {
						 for(unsigned int i=0;i<uvValuesFloor.size();i++)
						 {
							uValues[i] = uvValuesFloor[i].x;
							vValues[i] = uvValuesFloor[i].y;
						 }
					}

                     MIntArray uvIndices( (unsigned int) uvIndicesFloor.size() );
                     for(unsigned int i=0;i<uvIndicesFloor.size();i++)
                     {
                        uvIndices[i] = uvIndicesFloor[mSampleLookup[i]];
                     }
					 MIntArray uvCounts( mMesh.numPolygons() );
                     for(int f=0;f<mMesh.numPolygons();f++){
                         uvCounts[f] = mMesh.polygonVertexCount(f);
					 }

					 status = mMesh.setCurrentUVSetName(uvSetNames[uvSetIndex]);
					 /*
						-- this tends to error out but it actually works. Ben.
					 if( status != MS::kSuccess ){
						 EC_LOG_ERROR("mMesh.setCurrentUVSetName(\""<<uvSetNames[uvSetIndex]<<"\") failed: "<<status.errorString().asChar());
					 }*/
                     status = mMesh.setUVs(uValues, vValues, &uvSetNames[uvSetIndex]);
					 if( status != MS::kSuccess ){
						 EC_LOG_ERROR("mMesh.setUVs(\""<<uvSetNames[uvSetIndex]<<"\") failed: "<<status.errorString().asChar());
					 }
					 status = mMesh.assignUVs(uvCounts, uvIndices, &uvSetNames[uvSetIndex]);
					 if ( status != MS::kSuccess ){
						 EC_LOG_ERROR("mMesh.assignUVs(\""<<uvSetNames[uvSetIndex]<<"\") failed: "<<status.errorString().asChar());
					 }
                  }
               }

            }
         }
      }
   }
   else if(mMesh.numVertices() == points.length())
      mMesh.setPoints(points);

   // import the normals
   if(importNormals) 
   {
      Alembic::AbcGeom::IN3fGeomParam normalsParam = mSchema.getNormalsParam();
      if(normalsParam.valid())
      {
         if(normalsParam.getNumSamples() > 0)
         {
            sampleInfo = getSampleInfo(
               inputTime,
               normalsParam.getTimeSampling(),
               normalsParam.getNumSamples()
            );

			 std::vector<Imath::V3f> normalValuesFloor, normalValuesCeil;
			 std::vector<AbcA::uint32_t> normalIndicesFloor, normalIndicesCeil;
			bool normalFloor = false, normalCeil = false;
		  
			normalFloor = getIndexAndValues( sampleIndices, normalsParam, sampleInfo.floorIndex,
			   normalValuesFloor, normalIndicesFloor );
			normalCeil = getIndexAndValues( sampleIndices, normalsParam, sampleInfo.floorIndex,
			   normalValuesCeil, normalIndicesCeil );

            if(normalIndicesFloor.size() == mSampleLookup.size())
            {
               MVectorArray normals((unsigned int)normalValuesFloor.size());

               bool done = false;
               if((sampleInfo.alpha != 0.0)&& normalCeil && normalIndicesFloor.size() == normalIndicesCeil.size() && ! isTopologyDynamic )
                  {
                     //ESS_LOG_WARNING( "blending vertex normals (1-2) A." );
					 float blend = (float)sampleInfo.alpha;
                     float iblend = 1.0f - blend;
                     MVector normal;
                     for(unsigned int i=0;i<normalValuesFloor.size();i++)
                     {
                        normal.x = normalValuesFloor[i].x * iblend + normalValuesCeil[i].x * blend;
                        normal.y = normalValuesFloor[i].y * iblend + normalValuesCeil[i].y * blend;
                        normal.z = normalValuesFloor[i].z * iblend + normalValuesCeil[i].z * blend;
                        normals[i] = normal.normal();
                     }
                 
               }
			   else {
                  for(unsigned int i=0;i<normalValuesFloor.size();i++)
                  {
                     normals[i].x = normalValuesFloor[i].x;
                     normals[i].y = normalValuesFloor[i].y;
                     normals[i].z = normalValuesFloor[i].z;
                  }
               }

			    MIntArray normalIndices( (unsigned int) normalIndicesFloor.size() );
				for( int i = 0; i < normalIndicesFloor.size(); i ++) {
					normalIndices[i] = normalIndicesFloor[ mSampleLookup[i] ];
				}

				MVectorArray normalExpanded((unsigned int)normalIndicesFloor.size());
				for( int i = 0; i < normalExpanded.length(); i ++ ) {
					normalExpanded[i] = normals[ normalIndicesFloor[ mSampleLookup[i] ] ];
				}

				MIntArray faceList((unsigned int)normalIndicesFloor.size());
	            MIntArray vertexList((unsigned int)normalIndicesFloor.size());

				 MIntArray normalCounts( normalIndicesFloor.size() );
			  int numFaces = mMesh.numPolygons();
				int nIndex = 0;
				for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
				{
					MIntArray polyVerts;
					mMesh.getPolygonVertices(faceIndex, polyVerts);
					int numVertices = polyVerts.length();
					for (int v = numVertices - 1; v >= 0; v--, ++nIndex)
					{
						faceList[nIndex] = faceIndex;
						vertexList[nIndex] = polyVerts[v];
					}
				}

				status = mMesh.setFaceVertexNormals( normalExpanded, faceList, vertexList );
				if ( status != MS::kSuccess ){
						 EC_LOG_ERROR("mMesh.setFaceVertexNormals - failed: "<<status.errorString().asChar());
				}
			}
         }
      }
   }

   // output all channels
   dataBlock.outputValue(mOutGeometryAttr).set(mMeshData);

   return MStatus::kSuccess;
}

void AlembicPolyMeshDeformNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicPolyMeshDeformNode::~AlembicPolyMeshDeformNode()
{
   PreDestruction();
}

MObject AlembicPolyMeshDeformNode::mTimeAttr;
MObject AlembicPolyMeshDeformNode::mFileNameAttr;
MObject AlembicPolyMeshDeformNode::mIdentifierAttr;

MStatus AlembicPolyMeshDeformNode::initialize()
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
   mIdentifierAttr = tAttr.create("identifier", "if", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mIdentifierAttr);

   // create a mapping
   status = attributeAffects(mTimeAttr, outputGeom);
   status = attributeAffects(mFileNameAttr, outputGeom);
   status = attributeAffects(mIdentifierAttr, outputGeom);

   return status;
}

MStatus AlembicPolyMeshDeformNode::deform(MDataBlock & dataBlock, MItGeometry & iter, const MMatrix & localToWorld, unsigned int geomIndex)
{
   MStatus status;

   // get the envelope data
   float env = dataBlock.inputValue( envelope ).asFloat();
   if(env == 0.0f) // deformer turned off
      return MStatus::kSuccess;

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
      mObj = getObjectFromArchive(mFileName,identifier);
      if(!mObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      Alembic::AbcGeom::IPolyMesh obj(mObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a PolyMesh.");
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

   bool isTopologyDynamic = false;
   if( mObj.valid() ) {
	   isTopologyDynamic = isAlembicMeshTopoDynamic( &mObj );
	   //ESS_LOG_WARNING( "isTopologyDynamic = " << isTopologyDynamic );
   }

   mLastSampleInfo = sampleInfo;

   // access the camera values
   Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
   Alembic::AbcGeom::IPolyMeshSchema::Sample sample2;
   mSchema.get(sample,sampleInfo.floorIndex);
   if(sampleInfo.alpha != 0.0)
      mSchema.get(sample2,sampleInfo.ceilIndex);

   Alembic::Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   Alembic::Abc::P3fArraySamplePtr samplePos2;
   if(sampleInfo.alpha != 0.0)
      samplePos2 = sample2.getPositions();

   // iteration should not be necessary. the iteration is only 
   // required if the same mesh is attached to the same deformer
   // several times
   float blend = (float)sampleInfo.alpha;
   float iblend = 1.0f - blend;
   for(iter.reset(); !iter.isDone(); iter.next())
   {
      float weight = weightValue(dataBlock,geomIndex,iter.index()) * env;
      if(weight == 0.0f)
         continue;
      float iweight = 1.0f - weight;
      if(iter.index() >= samplePos->size())
         continue;
      bool done = false;
      //MFloatPoint pt = iter.position();
      MPoint pt = iter.position();

      MFloatPoint abcPt;
      if(sampleInfo.alpha != 0.0)
      {
         if(samplePos2->size() == samplePos->size() && ! isTopologyDynamic )
         {
			//ESS_LOG_WARNING( "blending vertex positions (1-2) B." );
            pt.x = iweight * pt.x + weight * (samplePos->get()[iter.index()].x * iblend + samplePos2->get()[iter.index()].x * blend);
            pt.y = iweight * pt.y + weight * (samplePos->get()[iter.index()].y * iblend + samplePos2->get()[iter.index()].y * blend);
            pt.z = iweight * pt.z + weight * (samplePos->get()[iter.index()].z * iblend + samplePos2->get()[iter.index()].z * blend);
            done = true;
         }
      }
      if(!done)
      {
         pt.x = iweight * pt.x + weight * samplePos->get()[iter.index()].x;
         pt.y = iweight * pt.y + weight * samplePos->get()[iter.index()].y;
         pt.z = iweight * pt.z + weight * samplePos->get()[iter.index()].z;
      }
      iter.setPosition(pt);
   }

   return MStatus::kSuccess;
}

MSyntax AlembicCreateFaceSetsCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-h", "-help");
   syntax.addFlag("-f", "-fileNameArg", MSyntax::kString);
   syntax.addFlag("-i", "-identifierArg", MSyntax::kString);
   syntax.addFlag("-o", "-objectArg", MSyntax::kString);
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

MStatus AlembicCreateFaceSetsCommand::doIt(const MArgList & args)
{
   MStatus status = MS::kSuccess;
   MArgParser argData(syntax(), args, &status);

   if (argData.isFlagSet("help"))
   {
      MGlobal::displayInfo("[ExocortexAlembic]: ExocortexAlembic_createFaceSets command:");
      MGlobal::displayInfo("                    -f : provide an unresolved fileName (string)");
      MGlobal::displayInfo("                    -i : provide an identifier inside the file");
      MGlobal::displayInfo("                    -o : provide an object to create the meta data on");
      return MS::kSuccess;
   }

   if(!argData.isFlagSet("objectArg"))
   {
      MGlobal::displayError("[ExocortexAlembic] No objectArg specified.");
      return MStatus::kFailure;
   }
   if(!argData.isFlagSet("fileNameArg"))
   {
      MGlobal::displayError("[ExocortexAlembic] No fileNameArg specified.");
      return MStatus::kFailure;
   }
   if(!argData.isFlagSet("identifierArg"))
   {
      MGlobal::displayError("[ExocortexAlembic] No identifierArg specified.");
      return MStatus::kFailure;
   }
   MString objectPath = argData.flagArgumentString("objectArg",0);
   MObject nodeObject = getRefFromFullName(objectPath);
   if(nodeObject.isNull())
   {
      MGlobal::displayError("[ExocortexAlembic] Invalid objectArg specified.");
      return MStatus::kFailure;
   }
   MFnDagNode node(nodeObject);

   MString fileName = argData.flagArgumentString("fileNameArg",0);
   if(fileName.length() == 0)
   {
      MGlobal::displayError("[ExocortexAlembic] No valid fileNameArg specified.");
      return MStatus::kFailure;
   }
   fileName = resolvePath(fileName);
   MString identifier = argData.flagArgumentString("identifierArg",0);
   if(identifier.length() == 0)
   {
      MGlobal::displayError("[ExocortexAlembic] No valid identifierArg specified.");
      return MStatus::kFailure;
   }

   addRefArchive(fileName);

   Alembic::Abc::IObject object = getObjectFromArchive(fileName,identifier);
   if(!object.valid())
   {
      MGlobal::displayError("[ExocortexAlembic] No valid fileNameArg or identifierArg specified.");
      return MStatus::kFailure;
   }

   // check the type of object
   Alembic::AbcGeom::IPolyMesh mesh;
   Alembic::AbcGeom::ISubD subd;
   if(Alembic::AbcGeom::IPolyMesh::matches(object.getMetaData()))
      mesh = Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting);
   else if(Alembic::AbcGeom::ISubD::matches(object.getMetaData()))
      subd = Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting);
   else
   {
      MGlobal::displayError("[ExocortexAlembic] Specified identifer doesn't refer to a PolyMesh or a SubD object.");
      return MStatus::kFailure;
   }

   std::vector<std::string> faceSetNames;
   if(mesh.valid())
      mesh.getSchema().getFaceSetNames(faceSetNames);
   else
      subd.getSchema().getFaceSetNames(faceSetNames);

   MFnTypedAttribute tAttr;
   for(size_t i=0;i<faceSetNames.size();i++)
   {
      // access the face set
      Alembic::AbcGeom::IFaceSetSchema faceSet;
      if(mesh.valid())
         faceSet = mesh.getSchema().getFaceSet(faceSetNames[i]).getSchema();
      else
         faceSet = subd.getSchema().getFaceSet(faceSetNames[i]).getSchema();
      Alembic::AbcGeom::IFaceSetSchema::Sample faceSetSample = faceSet.getValue();

      // create the int data
      MFnIntArrayData fnData;
      MIntArray arr((int *)faceSetSample.getFaces()->getData(), static_cast<unsigned int>(faceSetSample.getFaces()->size()));
      MObject attrObj = fnData.create(arr);

      // check if we need to create the attribute
      MString attributeName = "FACESET_";
      attributeName += removeInvalidCharacter(faceSetNames[i]).c_str();
      MObject attribute = node.attribute(attributeName, &status);
      if(!status || attribute.isNull())
      {
         attribute = tAttr.create(attributeName, attributeName, MFnData::kIntArray, attrObj, &status);
         if (!status) { MGlobal::displayError(status.errorString()); break; }
         status = tAttr.setStorable(true);
         if (!status) { MGlobal::displayError(status.errorString()); break; }
         status = tAttr.setKeyable(false);
         if (!status) { MGlobal::displayError(status.errorString()); break; }
         status = node.addAttribute(attribute);
         if (!status) { MGlobal::displayError(status.errorString()); break; }
      }
      else
      {
         MPlug attributePlug(nodeObject, attribute);
         attributePlug.setMObject(attrObj);
      }
   }

   object.reset();
   mesh.reset();
   delRefArchive(fileName);

   return status;
}
