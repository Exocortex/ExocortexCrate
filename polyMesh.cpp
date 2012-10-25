#include "stdafx.h"
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

  AtArray *faceCounts = AiArrayAllocate((AtInt)abcFaceCounts->size(),  1, AI_TYPE_UINT);
  ind.faceIndices     = AiArrayAllocate((AtInt)abcFaceIndices->size(), 1, AI_TYPE_UINT);
  ind.indices         = AiArrayAllocate((AtInt)abcFaceIndices->size(), 1, AI_TYPE_UINT);
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

static void plainPositionCopy(AtArray *pos, Alembic::Abc::P3fArraySamplePtr &abcPos, AtULong &posOffset)
{
  const int nbPos = (int) abcPos->size();
  for(size_t i=0; i < nbPos; ++i)
  {
    AtPoint pt;
    pt.x = abcPos->get()[i].x;
    pt.y = abcPos->get()[i].y;
    pt.z = abcPos->get()[i].z;
    AiArraySetPnt(pos, posOffset++, pt);
  }
}

// IF pos == NULL, it needs to be done before calling that function
template<typename SCHEMA, typename SCHEMA_SAMPLE>
static bool hadToInterpolatePositions(SCHEMA &schema, SCHEMA_SAMPLE &sample, AtArray *pos, AtULong &posOffset, std::vector<float> &samples, SampleInfo &sampleInfo, bool dynamicTopology)
{
  Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();

  if(dynamicTopology)
  {
    SampleInfo sampleInfoFirst = getSampleInfo
    (
      samples[0],
      schema.getTimeSampling(),
      schema.getNumSamples()
    );
    schema.get(sample,sampleInfoFirst.floorIndex);
    abcPos = sample.getPositions();

    sampleInfoFirst.alpha += double(sampleInfo.floorIndex) - double(sampleInfoFirst.floorIndex);
    sampleInfo = sampleInfoFirst;
  }

  // if we have to interpolate
  if(sampleInfo.alpha <= sampleTolerance) // NOT!
  {
    plainPositionCopy(pos, abcPos, posOffset);
    return false;
  }
  else
  {
    SCHEMA_SAMPLE sample2;
    schema.get(sample2,sampleInfo.ceilIndex);
    Alembic::Abc::P3fArraySamplePtr abcPos2 = sample2.getPositions();
    const float alpha = (float)sampleInfo.alpha;
    const float ialpha = 1.0f - alpha;

    if(!dynamicTopology)
    {
      for(size_t i=0;i<abcPos->size();i++)
      {
        AtPoint pt;
        pt.x = abcPos->get()[i].x * ialpha + abcPos2->get()[i].x * alpha;
        pt.y = abcPos->get()[i].y * ialpha + abcPos2->get()[i].y * alpha;
        pt.z = abcPos->get()[i].z * ialpha + abcPos2->get()[i].z * alpha;
        AiArraySetPnt(pos, posOffset++, pt);
      }
    }
    else
    {
      Alembic::Abc::V3fArraySamplePtr abcVel = sample.getVelocities();
      if(abcVel && abcVel->size() == abcPos->size())
      {
		const float timeAlpha = getTimeOffsetFromSchema( schema, sampleInfo );

        for(size_t i=0;i<abcPos->size();i++)
        {
          AtPoint pt;
          pt.x = abcPos->get()[i].x + timeAlpha * abcVel->get()[i].x;
          pt.y = abcPos->get()[i].y + timeAlpha * abcVel->get()[i].y;
          pt.z = abcPos->get()[i].z + timeAlpha * abcVel->get()[i].z;
          AiArraySetPnt(pos, posOffset++, pt);
        }
      }
      else
        plainPositionCopy(pos, abcPos, posOffset);
    }
  }
  return true;
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

  __indices ind;
  for(size_t sampleIndex = 0; sampleIndex < minNumSamples; ++sampleIndex)
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

	  if( typedObject.getSchema().getPropertyHeader( ".faceVaryingInterpolateBoundary" ) != NULL ) {
	 		Abc::IInt32Property faceVaryingInterpolateBoundary = Abc::IInt32Property( typedObject.getSchema(), ".faceVaryingInterpolateBoundary" );
			Abc::int32_t subDLevel;
			faceVaryingInterpolateBoundary.get( subDLevel, 0 );
			AiNodeSetStr(shapeNode, "subdiv_type", "catclark");
			AiNodeSetInt(shapeNode, "subdiv_iterations", (AtInt)subDLevel);
			AiNodeSetFlt(shapeNode, "subdiv_pixel_error", 0.0f);
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

          AtArray *bindPose = AiArrayAllocate((AtInt)abcBindPose->size(), 1, AI_TYPE_POINT);
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
    Alembic::Abc::N3fArraySamplePtr abcNor = normalParam.getExpandedValue(sampleInfo.floorIndex).getVals();
    if(pos == NULL)
    {
      Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();
      firstSampleCount = sample.getFaceIndices()->size();
      pos = AiArrayAllocate((AtInt)abcPos->size(), (AtInt)minNumSamples, AI_TYPE_POINT);
    }

    const bool interpolated = hadToInterpolatePositions(typedObject.getSchema(), sample, pos, posOffset, samples, sampleInfo, dynamicTopology);
    if (abcNor != NULL)
    {
      if (nor == NULL)
        nor = AiArrayAllocate((AtInt)abcNor->size(), (AtInt)minNumSamples, AI_TYPE_VECTOR);

      if (!interpolated || dynamicTopology)
        removeNormalsDuplicate(nor, norOffset, abcNor, sampleInfo, nsIdx, ind.faceIndices);
      else
      {
        Alembic::Abc::N3fArraySamplePtr abcNor2 = normalParam.getExpandedValue(sampleInfo.ceilIndex).getVals();
        removeNormalsDuplicateDynTopology(nor, norOffset, abcNor, abcNor2, (float)sampleInfo.alpha, sampleInfo, nsIdx, ind.faceIndices);
      }
    }
  }

  AiNodeSetArray(shapeNode, "vlist", pos);
  if (nor != NULL)
  {
    AiNodeSetArray(shapeNode, "nlist", nor);
    AiNodeSetArray(shapeNode, "nidxs", nsIdx);
  }
  else
    AiArrayDestroy(nsIdx);    // no normals... no need for nsIdx!

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

  for(size_t sampleIndex = 0; sampleIndex < minNumSamples; ++sampleIndex)
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
      abcFaceCounts = sample.getFaceCounts();
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
    if(pos == NULL)
    {
      Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();
      pos = AiArrayAllocate((AtInt)abcPos->size(), (AtInt)minNumSamples, AI_TYPE_POINT);
      firstSampleCount = sample.getFaceIndices()->size();
    }
    hadToInterpolatePositions(typedObject.getSchema(), sample, pos, posOffset, samples, sampleInfo, dynamicTopology);
  }
  AiNodeSetArray(shapeNode, "vlist", pos);

  postShaderProcess(typedObject.getSchema(), nodata, ud, abcFaceCounts);
  return shapeNode;
}


