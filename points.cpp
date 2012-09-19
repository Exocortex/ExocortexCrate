#include "points.h"

AtNode *createPointsNode(nodeData &nodata, userData * ud, std::vector<float> &samples, int i)
{
  Alembic::AbcGeom::IPoints typedObject(nodata.object,Alembic::Abc::kWrapExisting);
  size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1 ? typedObject.getSchema().getNumSamples() : samples.size();

  // create the arnold node
  AtNode *shapeNode = AiNode("points");

  // create arrays to hold the data
  AtArray *pos = NULL;
  nodata.shifted = false;

  // loop over all samples
  AtULong posOffset = 0;
  for(size_t sampleIndex = 0; sampleIndex < minNumSamples; ++sampleIndex)
  {
    SampleInfo sampleInfo = getSampleInfo
    (
      samples[sampleIndex],
      typedObject.getSchema().getTimeSampling(),
      typedObject.getSchema().getNumSamples()
    );

    // get the floor sample
    Alembic::AbcGeom::IPointsSchema::Sample sample;
    typedObject.getSchema().get(sample,sampleInfo.floorIndex);

    // access the points
    Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();

    // take care of the topology
    if(sampleIndex == 0)
    {
      // hard coded mode
      if(!ud->gPointsMode.empty())
        AiNodeSetStr(shapeNode, "mode", ud->gPointsMode.c_str());

      // check if we have a radius
      Alembic::AbcGeom::IFloatGeomParam widthParam = typedObject.getSchema().getWidthsParam();
      if(widthParam.valid())
      {
        Alembic::Abc::FloatArraySamplePtr abcRadius = widthParam.getExpandedValue(sampleInfo.floorIndex).getVals();
        AtArray * radius = AiArrayAllocate((AtInt)abcRadius->size(),1,AI_TYPE_FLOAT);
        for(size_t i=0;i<abcRadius->size();++i)
          AiArraySetFlt(radius,(AtULong)i,abcRadius->get()[i]);
        AiNodeSetArray(shapeNode,"radius",radius);
      }

      // check if we have colors
      Alembic::Abc::IC4fArrayProperty propColor;
      if( getArbGeomParamPropertyAlembic_Permissive( typedObject, "color", propColor ) )
      {
        Alembic::Abc::C4fArraySamplePtr abcColors = propColor.getValue(sampleInfo.floorIndex);
        AtBoolean result = false;
        if(abcColors->size() == 1)
          result = AiNodeDeclare(shapeNode, "Color", "constant RGBA");
        else
          result = AiNodeDeclare(shapeNode, "Color", "uniform RGBA");

        if(result)
        {
          AtArray * colors = AiArrayAllocate((AtInt)abcColors->size(),1,AI_TYPE_RGBA);
          AtRGBA color;
          for(size_t i=0; i<abcColors->size(); ++i)
          {
            color.r = abcColors->get()[i].r;
            color.g = abcColors->get()[i].g;
            color.b = abcColors->get()[i].b;
            color.a = abcColors->get()[i].a;
            AiArraySetRGBA(colors, (AtULong)i, color);
          }
          AiNodeSetArray(shapeNode, "Color", colors);
        }
      }
    }

    // access the positions
    if(pos == NULL)
      pos = AiArrayAllocate((AtInt)(abcPos->size() * 3), (AtInt)minNumSamples, AI_TYPE_FLOAT);

    // if we have to interpolate
    if(sampleInfo.alpha <= sampleTolerance)
    {
      for(size_t i=0; i<abcPos->size(); ++i)
      {
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x);
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y);
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z);
      }
    }
    else
    {
      float alpha = (float)sampleInfo.alpha;
      float timeAlpha = (float)(typedObject.getSchema().getTimeSampling()->getSampleTime(sampleInfo.ceilIndex) - typedObject.getSchema().getTimeSampling()->getSampleTime(sampleInfo.floorIndex)) * alpha;

      Alembic::Abc::V3fArraySamplePtr abcVel = sample.getVelocities();
      for(size_t i=0; i<abcPos->size(); ++i)
      {
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].x + timeAlpha * abcVel->get()[i].x);
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].y + timeAlpha * abcVel->get()[i].y);
        AiArraySetFlt(pos,posOffset++,abcPos->get()[i].z + timeAlpha * abcVel->get()[i].z);
      }
    }
  }

  AiNodeSetArray(shapeNode, "points", pos);
  return shapeNode;
}

