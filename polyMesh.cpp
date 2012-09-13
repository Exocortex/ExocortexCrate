#include "polyMesh.h"

struct __indices
{
  AtArray *faceIndices;
  AtArray *indices;
};

template<typename AbcGeom>    // AbcGeom is either of type Alembic::AbcGeom::IPolyMesh or Alembic::AbcGeom::ISubD
static bool usingDynamicTopology(AbcGeom &typedObject)
{
  Alembic::Abc::IInt32ArrayProperty faceIndicesProp = Alembic::Abc::IInt32ArrayProperty(typedObject.getSchema(),".faceIndices");
  if (faceIndicesProp.valid())
    return !faceIndicesProp.isConstant();
  return false;
}

// common function for PolyMesh and SubD!
static bool faceCount(AtNode *shapeNode, __indices &ind, Alembic::Abc::Int32ArraySamplePtr &abcFaceCounts, Alembic::Abc::Int32ArraySamplePtr &abcFaceIndices)
{
  if(abcFaceCounts->get()[0] == 0)
    return false;

  AtArray * faceCounts = AiArrayAllocate((AtInt)abcFaceCounts->size(),   1, AI_TYPE_UINT);
  ind.faceIndices      = AiArrayAllocate((AtInt)abcFaceIndices->size(),  1, AI_TYPE_UINT);
  ind.indices          = AiArrayAllocate((AtInt)(abcFaceIndices->size()),1, AI_TYPE_UINT);
  AtUInt offset = 0;
  for(AtULong i=0; i<faceCounts->nelements; ++i)
  {
    const int _FaceCounts = abcFaceCounts->get()[i];
    AiArraySetUInt(faceCounts, i, _FaceCounts);
    for(AtLong j=0; j < _FaceCounts; ++j)
    {
      const int offFaceCounts = offset + _FaceCounts - (j+1);
      AiArraySetUInt(ind.faceIndices,offset+j, abcFaceIndices->get()[offFaceCounts]);
      AiArraySetUInt(ind.indices,    offset+j, offFaceCounts);
    }
    offset += _FaceCounts;
  }
  AiNodeSetArray(shapeNode, "nsides", faceCounts);
  AiNodeSetArray(shapeNode, "vidxs", ind.faceIndices);
  AiNodeSetBool(shapeNode, "smoothing", true);
  return true;
}

static void subSetUVParams(AtNode *shapeNode, Alembic::Abc::IFloatArrayProperty &prop)   // because this part is template independent!
{
  Alembic::Abc::FloatArraySamplePtr ptr = prop.getValue(0);
  if(ptr->size() > 1)
  {
    const bool uWrap = ptr->get()[0] != 0.0f;
    const bool vWrap = ptr->get()[1] != 0.0f;

    // fill the arnold array
    AtArray * uvOptions = AiArrayAllocate(2,1,AI_TYPE_BOOLEAN);
    AiArraySetBool(uvOptions,0,uWrap);
    AiArraySetBool(uvOptions,1,vWrap);

    // create a second identical array to avoid problem when deleted
    AtArray * uvOptions2 = AiArrayAllocate(2,1,AI_TYPE_BOOLEAN);
    AiArraySetBool(uvOptions2,0,uWrap);
    AiArraySetBool(uvOptions2,1,vWrap);

    // we need to define this two times, once for a named and once
    // for an unnamed texture projection.
    AiNodeDeclare(shapeNode, "Texture_Projection_wrap", "constant ARRAY BOOL");
    AiNodeDeclare(shapeNode, "_wrap", "constant ARRAY BOOL");
    AiNodeSetArray(shapeNode, "Texture_Projection_wrap", uvOptions);
    AiNodeSetArray(shapeNode, "_wrap", uvOptions2);
  }

  if( ptr->size() > 2 )
  {
    bool subdsmooth = ptr->get()[2] != 0.0f;
    if( subdsmooth )
      AiNodeSetStr(shapeNode, "subdiv_uv_smoothing", "pin_borders");
    else
      AiNodeSetStr(shapeNode, "subdiv_uv_smoothing", "linear");
  }
}

template<typename AbcGeom>    // AbcGeom is either of type Alembic::AbcGeom::IPolyMesh or Alembic::AbcGeom::ISubD
static bool setUVParams(AbcGeom &typedObject, SampleInfo &sampleInfo, AtNode *shapeNode, AtArray *uvsIdx, AtArray *faceIndices, Alembic::AbcGeom::IV2fGeomParam &uvParam)
{
  AiNodeSetArray(shapeNode, "uvlist", removeUvsDuplicate(uvParam, sampleInfo, uvsIdx, faceIndices));
  AiNodeSetArray(shapeNode, "uvidxs", uvsIdx);

  if(typedObject.getSchema().getPropertyHeader( ".uvOptions" ) != NULL)
  {
    Alembic::Abc::IFloatArrayProperty prop = Alembic::Abc::IFloatArrayProperty( typedObject.getSchema(), ".uvOptions" );
    if(prop.getNumSamples() > 0)
      subSetUVParams(shapeNode, prop);
  }
  return true;
}

template<typename SCHEMA>
static void postShaderProcess(SCHEMA &schema, nodeData &nodata, userData *ud, Alembic::Abc::Int32ArraySamplePtr &abcFaceCounts)
{
  if(ud->gProcShaders == NULL || nodata.shaders != NULL)
    return;

  nodata.shaders = ud->gProcShaders;
  if(nodata.shaders->nelements > 1)
  {
    // check if we have facesets on this node
    std::vector<std::string> faceSetNames;
    schema.getFaceSetNames(faceSetNames);
    if(faceSetNames.size() > 0)
    {
      // allocate the shader index array
      const size_t shaderIndexCount = abcFaceCounts->size();
      nodata.shaderIndices = AiArrayAllocate((AtUInt32)shaderIndexCount,1,AI_TYPE_BYTE);
      for(size_t i=0; i < shaderIndexCount; ++i)
        AiArraySetByte(nodata.shaderIndices,(AtUInt32)i,0);

      const size_t faceSetNamesSize = faceSetNames.size();
      for(size_t i=0; i < faceSetNamesSize; ++i)
      {
        Alembic::AbcGeom::IFaceSetSchema faceSet = schema.getFaceSet(faceSetNames[i]).getSchema();
        Alembic::AbcGeom::IFaceSetSchema::Sample faceSetSample = faceSet.getValue();
        Alembic::Abc::Int32ArraySamplePtr faces = faceSetSample.getFaces();
        for(size_t j=0; j < faces->size(); ++j)
        {
          if((size_t)faces->get()[j] < shaderIndexCount && i+1 < ud->gProcShaders->nelements)
            AiArraySetByte(nodata.shaderIndices,(AtUInt32)faces->get()[j],(AtByte)(i+1));
        }
      }
    }
  }
}

AtNode *createPolyMeshNode(nodeData &nodata, userData * ud, std::vector<float> &samples, int i)
{
  Alembic::AbcGeom::IPolyMesh typedObject(nodata.object,Alembic::Abc::kWrapExisting);
  size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : samples.size();
  Alembic::AbcGeom::IN3fGeomParam normalParam = typedObject.getSchema().getNormalsParam();
  if(!normalParam.valid())
  {
     AiMsgError("[ExocortexAlembicArnold] Mesh '%s' does not contain normals. Aborting.", nodata.object.getFullName().c_str());
     return NULL;
  }

  shiftedProcessing(nodata, ud);

  AtNode *shapeNode = AiNode("polymesh");
  nodata.createdShifted = false;
  nodata.isPolyMeshNode = true;

  // create arrays to hold the data
  AtArray * pos = NULL;
  AtArray * nor = NULL;
  AtArray * nsIdx = NULL;

  // check if we have dynamic topology
  bool dynamicTopology = usingDynamicTopology(typedObject);

  // loop over all samples
  AtULong posOffset = 0;
  AtULong norOffset = 0;
  size_t firstSampleCount = 0;
  Alembic::Abc::Int32ArraySamplePtr abcFaceCounts;
  Alembic::Abc::Int32ArraySamplePtr abcFaceIndices;

  for(size_t sampleIndex = 0; sampleIndex < minNumSamples; sampleIndex++)
  {
    SampleInfo sampleInfo = getSampleInfo(
      samples[sampleIndex],
      typedObject.getSchema().getTimeSampling(),
      typedObject.getSchema().getNumSamples()
    );

    // get the floor sample
    Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
    typedObject.getSchema().get(sample,sampleInfo.floorIndex);

    // take care of the topology
    __indices ind;
    if(sampleIndex == 0)
    {
      abcFaceCounts = sample.getFaceCounts();
      abcFaceIndices = sample.getFaceIndices();
      if (!faceCount(shapeNode, ind, abcFaceCounts, abcFaceIndices))
        return NULL;

      // copy the indices a second time because they can be overwritten in UVs
      {
        const int nbElem = ind.indices->nelements;
        nsIdx = AiArrayAllocate(nbElem, 1, AI_TYPE_UINT);
        for (int ii = 0; ii < nbElem; ++ii)
          AiArraySetUInt(nsIdx, ii, AiArrayGetUInt(ind.indices, ii));
      }

      // check if we have UVs in the alembic file
      Alembic::AbcGeom::IV2fGeomParam uvParam = typedObject.getSchema().getUVsParam();
      if(uvParam.valid())
        setUVParams(typedObject, sampleInfo, shapeNode, ind.indices, ind.faceIndices, uvParam);

      // check if we have a bindpose in the alembic file
      if ( typedObject.getSchema().getPropertyHeader( ".bindpose" ) != NULL )
      {
        Alembic::Abc::IV3fArrayProperty prop = Alembic::Abc::IV3fArrayProperty( typedObject.getSchema(), ".bindpose" );
        if(prop.valid())
        {
          Alembic::Abc::V3fArraySamplePtr abcBindPose = prop.getValue(sampleInfo.floorIndex);
          AiNodeDeclare(shapeNode, "Pref", "varying POINT");

          AtArray * bindPose = AiArrayAllocate((AtInt)abcBindPose->size(), 1, AI_TYPE_POINT);
          AtPoint pnt;
          for(AtULong i=0;i<abcBindPose->size();i++)
          {
            pnt.x = abcBindPose->get()[i].x;
            pnt.y = abcBindPose->get()[i].y;
            pnt.z = abcBindPose->get()[i].z;
            AiArraySetPnt(bindPose, i, pnt);
          }
          AiNodeSetArray(shapeNode, "Pref", bindPose);
        }
      }
    }

    // access the positions
    Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();
    Alembic::Abc::V3fArraySamplePtr abcVel = sample.getVelocities();
    if(pos == NULL)
    {
      firstSampleCount = sample.getFaceIndices()->size();
      pos = AiArrayAllocate((AtInt)(abcPos->size() * 3),(AtInt)minNumSamples,AI_TYPE_FLOAT);
    }

    if(dynamicTopology)
    {
      SampleInfo sampleInfoFirst = getSampleInfo(
         samples[0],
         typedObject.getSchema().getTimeSampling(),
         typedObject.getSchema().getNumSamples()
      );
      typedObject.getSchema().get(sample,sampleInfoFirst.floorIndex);
      abcPos = sample.getPositions();

      sampleInfoFirst.alpha += double(sampleInfo.floorIndex) - double(sampleInfoFirst.floorIndex);
      sampleInfo = sampleInfoFirst;
    }

    // access the normals
    Alembic::Abc::N3fArraySamplePtr abcNor = normalParam.getExpandedValue(sampleInfo.floorIndex).getVals();

    // if we have to interpolate
    if(sampleInfo.alpha <= sampleTolerance)
    {
      for(size_t i=0;i<abcPos->size();i++)
      {
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
      }
      if(abcNor != NULL)
        nor = removeNormalsDuplicate(abcNor, sampleInfo, nsIdx, ind.faceIndices);
    }
    else
    {
      Alembic::AbcGeom::IPolyMeshSchema::Sample sample2;
      typedObject.getSchema().get(sample2,sampleInfo.ceilIndex);
      Alembic::Abc::P3fArraySamplePtr abcPos2 = sample2.getPositions();
      float alpha = (float)sampleInfo.alpha;
      float ialpha = 1.0f - alpha;
      if(!dynamicTopology)
      {
        for(size_t i=0;i<abcPos->size();i++)
        {
          AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x * ialpha + abcPos2->get()[i].x * alpha);
          AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y * ialpha + abcPos2->get()[i].y * alpha);
          AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z * ialpha + abcPos2->get()[i].z * alpha);
        }
      }
      else if(abcVel)
      {
        if(abcVel->size() == abcPos->size())
        {
          float timeAlpha = (float)(typedObject.getSchema().getTimeSampling()->getSampleTime(sampleInfo.ceilIndex) - 
                            typedObject.getSchema().getTimeSampling()->getSampleTime(sampleInfo.floorIndex)) * alpha;
          for(size_t i=0;i<abcPos->size();i++)
          {
            AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x + timeAlpha * abcVel->get()[i].x);
            AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y + timeAlpha * abcVel->get()[i].y);
            AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z + timeAlpha * abcVel->get()[i].z);
          }
        }
        else
        {
          for(size_t i=0;i<abcPos->size();i++)
          {
            AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
            AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
            AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
          }
        }
      }
      else
      {
        for(size_t i=0;i<abcPos->size();i++)
        {
          AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
          AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
          AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
        }
      }

      if(abcNor != NULL)
      {
        if(!dynamicTopology)
        {
          Alembic::Abc::N3fArraySamplePtr abcNor2 = normalParam.getExpandedValue(sampleInfo.ceilIndex).getVals();
          nor = removeNormalsDuplicateDynTopology(abcNor, abcNor2, alpha, sampleInfo, nsIdx, ind.faceIndices);
        }
        else
        {
          nor = removeNormalsDuplicate(abcNor, sampleInfo, nsIdx, ind.faceIndices);
        }
      }
    }
  }

  AiNodeSetArray(shapeNode, "vlist", pos);
  AiNodeSetArray(shapeNode, "nlist", nor);
  AiNodeSetArray(shapeNode, "nidxs", nsIdx);

  postShaderProcess(typedObject.getSchema(), nodata, ud, abcFaceCounts);
  return shapeNode;
}

AtNode *createSubDNode(nodeData &nodata, userData * ud, std::vector<float> &samples, int i)
{
  Alembic::AbcGeom::ISubD typedObject(nodata.object, Alembic::Abc::kWrapExisting);
  size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : samples.size();

  shiftedProcessing(nodata, ud);

  AtNode *shapeNode = AiNode("polymesh");
  nodata.createdShifted = false;
  nodata.isPolyMeshNode = true;

  // create arrays to hold the data
  AtArray * pos = NULL;

  // check if we have dynamic topology
  bool dynamicTopology = usingDynamicTopology(typedObject);

  // loop over all samples
  size_t firstSampleCount = 0;
  AtULong posOffset = 0;
  Alembic::Abc::Int32ArraySamplePtr abcFaceCounts;

  for(size_t sampleIndex = 0; sampleIndex < minNumSamples; sampleIndex++)
  {
    SampleInfo sampleInfo = getSampleInfo(
      samples[sampleIndex],
      typedObject.getSchema().getTimeSampling(),
      typedObject.getSchema().getNumSamples()
    );

    // get the floor sample
    Alembic::AbcGeom::ISubDSchema::Sample sample;
    typedObject.getSchema().get(sample,sampleInfo.floorIndex);

    // take care of the topology
    if(sampleIndex == 0)
    {
      __indices ind;
      Alembic::Abc::Int32ArraySamplePtr abcFaceIndices = sample.getFaceIndices();
      if (!faceCount(shapeNode, ind, abcFaceCounts, abcFaceIndices))
        return NULL;

      // subdiv settings
      AiNodeSetStr(shapeNode, "subdiv_type", "catclark");
      AiNodeSetInt(shapeNode, "subdiv_iterations", (AtInt)sample.getFaceVaryingInterpolateBoundary());
      AiNodeSetFlt(shapeNode, "subdiv_pixel_error", 0.0f);

      // check if we have UVs in the alembic file
      Alembic::AbcGeom::IV2fGeomParam uvParam = typedObject.getSchema().getUVsParam();
      if(uvParam.valid())
        setUVParams(typedObject, sampleInfo, shapeNode, ind.indices, ind.faceIndices, uvParam);
      else
        AiArrayDestroy(ind.indices); // we don't need the uvindices
    }

    // access the positions
    Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();
    if(pos == NULL)
    {
      pos = AiArrayAllocate((AtInt)(abcPos->size() * 3),(AtInt)minNumSamples,AI_TYPE_FLOAT);
      firstSampleCount = sample.getFaceIndices()->size();
    }

     // if the count has changed, let's move back to the first sample
     if(dynamicTopology)
     {
        SampleInfo sampleInfoFirst = getSampleInfo(
           samples[0],
           typedObject.getSchema().getTimeSampling(),
           typedObject.getSchema().getNumSamples()
        );
        typedObject.getSchema().get(sample,sampleInfoFirst.floorIndex);
        abcPos = sample.getPositions();

        sampleInfoFirst.alpha += double(sampleInfo.floorIndex) - double(sampleInfoFirst.floorIndex);
        sampleInfo = sampleInfoFirst;
     }

     // if we have to interpolate
     if(sampleInfo.alpha <= sampleTolerance)
     {
        for(size_t i=0;i<abcPos->size();i++)
        {
           AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
           AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
           AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
        }
     }
     else
     {
        Alembic::AbcGeom::ISubDSchema::Sample sample2;
        typedObject.getSchema().get(sample2,sampleInfo.ceilIndex);
        Alembic::Abc::P3fArraySamplePtr abcPos2 = sample2.getPositions();
        float alpha = (float)sampleInfo.alpha;
        float ialpha = 1.0f - alpha;
        if(!dynamicTopology)
        {
           for(size_t i=0;i<abcPos->size();i++)
           {
              AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x * ialpha + abcPos2->get()[i].x * alpha);
              AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y * ialpha + abcPos2->get()[i].y * alpha);
              AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z * ialpha + abcPos2->get()[i].z * alpha);
           }
        }
        else if(typedObject.getSchema().getPropertyHeader( ".velocities" ) != NULL)
        {
           Alembic::Abc::IV3fArrayProperty velocitiesProp = Alembic::Abc::IV3fArrayProperty( typedObject.getSchema(), ".velocities" );
           SampleInfo velSampleInfo = getSampleInfo(
              samples[sampleIndex],
              velocitiesProp.getTimeSampling(),
              velocitiesProp.getNumSamples()
           );

           Alembic::Abc::V3fArraySamplePtr abcVel = velocitiesProp.getValue(velSampleInfo.floorIndex);
           if(abcVel->size() == abcPos->size())
           {
              for(size_t i=0;i<abcPos->size();i++)
              {
                 AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x + alpha * abcVel->get()[i].x);
                 AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y + alpha * abcVel->get()[i].y);
                 AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z + alpha * abcVel->get()[i].z);
              }
           }
           else
           {
              for(size_t i=0;i<abcPos->size();i++)
              {
                 AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
                 AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
                 AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
              }
           }
        }
        else
        {
           for(size_t i=0;i<abcPos->size();i++)
           {
              AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
              AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
              AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
           }
        }
     }
  }
  AiNodeSetArray(shapeNode, "vlist", pos);

  postShaderProcess(typedObject.getSchema(), nodata, ud, abcFaceCounts);
  return shapeNode;
}


