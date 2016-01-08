#include "curves.h"
#include "stdafx.h"

AtNode *createCurvesNode(nodeData &nodata, userData *ud,
                         std::vector<float> &samples, int i)
{
  Alembic::AbcGeom::ICurves typedObject(nodata.object,
                                        Alembic::Abc::kWrapExisting);
  size_t minNumSamples = typedObject.getSchema().getNumSamples() == 1
                             ? typedObject.getSchema().getNumSamples()
                             : samples.size();

  shiftedProcessing(nodata, ud);

  AtNode *shapeNode = AiNode("curves");
  nodata.createdShifted = false;

  // create arrays to hold the data
  AtArray *pos = NULL;

  // loop over all samples
  AtULong posOffset = 0;
  size_t totalNumPoints = 0;
  size_t totalNumPositions = 0;
  for (size_t sampleIndex = 0; sampleIndex < minNumSamples; ++sampleIndex) {
    SampleInfo sampleInfo = getSampleInfo(
        samples[sampleIndex], typedObject.getSchema().getTimeSampling(),
        typedObject.getSchema().getNumSamples());

    // get the floor sample
    Alembic::AbcGeom::ICurvesSchema::Sample sample;
    typedObject.getSchema().get(sample, sampleInfo.floorIndex);

    // access the num points
    Alembic::Abc::Int32ArraySamplePtr abcNumPoints =
        sample.getCurvesNumVertices();

    // take care of the topology
    if (sampleIndex == 0) {
      // hard coded pixel width, basis and mode
      AiNodeSetFlt(shapeNode, "min_pixel_width", 0.25f);
      AiNodeSetStr(shapeNode, "basis", "catmull-rom");
      AiNodeSetStr(shapeNode, "mode", ud->gCurvesMode.c_str());

      // setup the num_points
      AtArray *numPoints =
          AiArrayAllocate((AtInt)abcNumPoints->size(), 1, AI_TYPE_UINT);
      for (size_t i = 0; i < abcNumPoints->size(); i++) {
        totalNumPoints += abcNumPoints->get()[i];
        totalNumPositions += abcNumPoints->get()[i] + 2;
        AiArraySetUInt(numPoints, (AtULong)i,
                       (AtUInt)(abcNumPoints->get()[i] + 2));
      }
      AiNodeSetArray(shapeNode, "num_points", numPoints);

      // check if we have a radius
      Alembic::Abc::IFloatArrayProperty propRadius;
      if (getArbGeomParamPropertyAlembic(typedObject, "radius", propRadius)) {
        Alembic::Abc::FloatArraySamplePtr abcRadius =
            propRadius.getValue(sampleInfo.floorIndex);

        AtArray *radius =
            AiArrayAllocate((AtInt)abcRadius->size(), 1, AI_TYPE_FLOAT);
        for (size_t i = 0; i < abcRadius->size(); ++i) {
          AiArraySetFlt(radius, (AtULong)i, abcRadius->get()[i]);
        }
        AiNodeSetArray(shapeNode, "radius", radius);
      }

      // check if we have uvs
      Alembic::AbcGeom::IV2fGeomParam uvsParam =
          typedObject.getSchema().getUVsParam();
      if (uvsParam.valid()) {
        Alembic::Abc::V2fArraySamplePtr abcUvs =
            uvsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
        if (AiNodeDeclare(shapeNode, "Texture_Projection", "uniform POINT2")) {
          AtArray *uvs =
              AiArrayAllocate((AtInt)abcUvs->size(), 1, AI_TYPE_POINT2);
          AtPoint2 uv;
          for (size_t i = 0; i < abcUvs->size(); i++) {
            uv.x = abcUvs->get()[i].x;
            uv.y = abcUvs->get()[i].y;
            AiArraySetPnt2(uvs, (AtULong)i, uv);
          }
          AiNodeSetArray(shapeNode, "Texture_Projection", uvs);
        }
      }

      // check if we have colors
      Alembic::Abc::IC4fArrayProperty propColor;
      if (getArbGeomParamPropertyAlembic(typedObject, "color", propColor)) {
        Alembic::Abc::C4fArraySamplePtr abcColors =
            propColor.getValue(sampleInfo.floorIndex);
        AtBoolean result = false;
        if (abcColors->size() == 1) {
          result = AiNodeDeclare(shapeNode, "Color", "constant RGBA");
        }
        else if (abcColors->size() == abcNumPoints->size()) {
          result = AiNodeDeclare(shapeNode, "Color", "uniform RGBA");
        }
        else {
          result = AiNodeDeclare(shapeNode, "Color", "varying RGBA");
        }

        if (result) {
          AtArray *colors =
              AiArrayAllocate((AtInt)abcColors->size(), 1, AI_TYPE_RGBA);
          AtRGBA color;
          for (size_t i = 0; i < abcColors->size(); ++i) {
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
    Alembic::Abc::P3fArraySamplePtr abcPos = sample.getPositions();
    if (pos == NULL)
      pos = AiArrayAllocate((AtInt)(totalNumPositions * 3),
                            (AtInt)minNumSamples, AI_TYPE_FLOAT);

    // if we have to interpolate
    bool done = false;
    if (sampleInfo.alpha > sampleTolerance) {
      Alembic::AbcGeom::ICurvesSchema::Sample sample2;
      typedObject.getSchema().get(sample2, sampleInfo.ceilIndex);
      Alembic::Abc::P3fArraySamplePtr abcPos2 = sample2.getPositions();
      float alpha = (float)sampleInfo.alpha;
      float ialpha = 1.0f - alpha;
      size_t offset = 0;
      if (abcPos2->size() == abcPos->size()) {
        for (size_t i = 0; i < abcNumPoints->size(); ++i) {
          // add the first and last point manually (catmull clark)
          for (size_t j = 0; j < abcNumPoints->get()[i]; ++j) {
            AiArraySetFlt(pos, posOffset++,
                          abcPos->get()[offset].x * ialpha +
                              abcPos2->get()[offset].x * alpha);
            AiArraySetFlt(pos, posOffset++,
                          abcPos->get()[offset].y * ialpha +
                              abcPos2->get()[offset].y * alpha);
            AiArraySetFlt(pos, posOffset++,
                          abcPos->get()[offset].z * ialpha +
                              abcPos2->get()[offset].z * alpha);
            if (j == 0 || j == abcNumPoints->get()[i] - 1) {
              AiArraySetFlt(pos, posOffset++,
                            abcPos->get()[offset].x * ialpha +
                                abcPos2->get()[offset].x * alpha);
              AiArraySetFlt(pos, posOffset++,
                            abcPos->get()[offset].y * ialpha +
                                abcPos2->get()[offset].y * alpha);
              AiArraySetFlt(pos, posOffset++,
                            abcPos->get()[offset].z * ialpha +
                                abcPos2->get()[offset].z * alpha);
            }
            ++offset;
          }
        }
        done = true;
      }
      else {
        Alembic::Abc::P3fArraySamplePtr abcVel = sample.getPositions();
        if (abcVel) {
          if (abcVel->size() == abcPos->size()) {
            for (size_t i = 0; i < abcNumPoints->size(); ++i) {
              // add the first and last point manually (catmull clark)
              for (size_t j = 0; j < abcNumPoints->get()[i]; ++j) {
                AiArraySetFlt(
                    pos, posOffset++,
                    abcPos->get()[offset].x + abcVel->get()[offset].x * alpha);
                AiArraySetFlt(
                    pos, posOffset++,
                    abcPos->get()[offset].y + abcVel->get()[offset].y * alpha);
                AiArraySetFlt(
                    pos, posOffset++,
                    abcPos->get()[offset].z + abcVel->get()[offset].z * alpha);
                if (j == 0 || j == abcNumPoints->get()[i] - 1) {
                  AiArraySetFlt(pos, posOffset++,
                                abcPos->get()[offset].x +
                                    abcVel->get()[offset].x * alpha);
                  AiArraySetFlt(pos, posOffset++,
                                abcPos->get()[offset].y +
                                    abcVel->get()[offset].y * alpha);
                  AiArraySetFlt(pos, posOffset++,
                                abcPos->get()[offset].z +
                                    abcVel->get()[offset].z * alpha);
                }
                ++offset;
              }
            }
            done = true;
          }
        }
      }
    }

    if (!done) {
      size_t offset = 0;
      for (size_t i = 0; i < abcNumPoints->size(); ++i) {
        // add the first and last point manually (catmull clark)
        for (size_t j = 0; j < abcNumPoints->get()[i]; ++j) {
          AiArraySetFlt(pos, posOffset++, abcPos->get()[offset].x);
          AiArraySetFlt(pos, posOffset++, abcPos->get()[offset].y);
          AiArraySetFlt(pos, posOffset++, abcPos->get()[offset].z);
          if (j == 0 || j == abcNumPoints->get()[i] - 1) {
            AiArraySetFlt(pos, posOffset++, abcPos->get()[offset].x);
            AiArraySetFlt(pos, posOffset++, abcPos->get()[offset].y);
            AiArraySetFlt(pos, posOffset++, abcPos->get()[offset].z);
          }
          ++offset;
        }
      }
    }
  }

  AiNodeSetArray(shapeNode, "points", pos);
  return shapeNode;
}
