#include "instance.h"

AtNode *createInstanceNode(nodeData &nodata, userData * ud, int i)
{
  Alembic::AbcGeom::IPoints typedObject(ud->gIObjects[i].abc, Alembic::Abc::kWrapExisting);

  instanceCloudInfo * info = ud->gIObjects[i].instanceCloud;

  // check that we have the masternode
  size_t id = (size_t)ud->gIObjects[i].ID;
  size_t instanceID = (size_t)ud->gIObjects[i].instanceID;
  if(instanceID >= info->groupInfos.size())
  {
    AiMsgError("[ExocortexAlembicArnold] Instance '%s.%d' has an invalid instanceID  . Aborting.",ud->gIObjects[i].abc.getFullName().c_str(),(int)id);
    return NULL;
  }
  size_t groupID = (size_t)ud->gIObjects[i].instanceGroupID;
  if(groupID >= info->groupInfos[instanceID].identifiers.size())
  {
    AiMsgError("[ExocortexAlembicArnold] Instance '%s.%d' has an invalid instanceGroupID. Aborting.",ud->gIObjects[i].abc.getFullName().c_str(),(int)id);
    return NULL;
  }

  instanceGroupInfo * group = &info->groupInfos[instanceID];

  // get the right centroidTime
  float centroidTime = ud->gCentroidTime;
  if(info->time.size() > 0)
  {
    centroidTime = info->time[0]->get()[id < info->time[0]->size() ? id : info->time[0]->size() - 1];
    if(info->time.size() > 1)
      centroidTime = (1.0f - info->timeAlpha) * centroidTime + info->timeAlpha * info->time[1]->get()[id < info->time[1]->size() ? id : info->time[1]->size() - 1];
    centroidTime = roundCentroid(centroidTime);
  }

  std::map<float,AtNode*>::iterator it = group->nodes[groupID].find(centroidTime);
  if(it == group->nodes[groupID].end())
  {
    AiMsgError("[ExocortexAlembicArnold] Cannot find masterNode '%s' for centroidTime '%f'. Aborting.",group->identifiers[groupID].c_str(),centroidTime);
    return NULL;
  }
  AtNode *usedMasterNode = it->second;

  AtNode *shapeNode = AiNode("ginstance");

  // setup name, id and the master node
  AiNodeSetStr(shapeNode, "name", getNameFromIdentifier(ud->gIObjects[i].abc.getFullName(),ud->gIObjects[i].ID,(long)groupID).c_str());
  AiNodeSetInt(shapeNode, "id", ud->gIObjects[i].instanceID); 
  AiNodeSetPtr(shapeNode, "node", usedMasterNode);

  // declare color on the ginstance
  if(info->color.size() > 0 && AiNodeDeclare(shapeNode, "Color", "constant RGBA"))
  {
    Alembic::Abc::C4f color = info->color[0]->get()[id < info->color[0]->size() ? id : info->color[0]->size() - 1];
    AiNodeSetRGBA(shapeNode, "Color", color.r, color.g, color.b, color.a);
  }

  // now let's take care of the transform
  AtArray * matrices = AiArrayAllocate(1,(AtInt)ud->gMbKeys.size(),AI_TYPE_MATRIX);
  for(size_t j=0;j<ud->gMbKeys.size(); ++j)
  {
    SampleInfo sampleInfo = getSampleInfo(
      ud->gMbKeys[j],
      typedObject.getSchema().getTimeSampling(),
      typedObject.getSchema().getNumSamples()
    );

    Alembic::Abc::M44f matrixAbc;
    matrixAbc.makeIdentity();
    const size_t floorIndex = j << 1;
    const size_t ceilIndex =  floorIndex + 1;

    // apply translation
    if(info->pos[floorIndex]->size() == info->pos[ceilIndex]->size())
    {
      matrixAbc.setTranslation(float(1.0 - sampleInfo.alpha) * info->pos[floorIndex]->get()[id < info->pos[floorIndex]->size() ? id : info->pos[floorIndex]->size() - 1] + 
                               float(sampleInfo.alpha) * info->pos[ceilIndex]->get()[id < info->pos[ceilIndex]->size() ? id : info->pos[ceilIndex]->size() - 1]);
    }
    else
    {
      const float timeAlpha = getTimeOffsetFromObject( typedObject, sampleInfo );

      matrixAbc.setTranslation(info->pos[floorIndex]->get()[id < info->pos[floorIndex]->size() ? id : info->pos[floorIndex]->size() - 1] + 
                               info->vel[floorIndex]->get()[id < info->vel[floorIndex]->size() ? id : info->vel[floorIndex]->size() - 1] * timeAlpha);
    }

    // now take care of rotation
    if(info->rot.size() == ud->gMbKeys.size())
    {
      Alembic::Abc::Quatf rotAbc = info->rot[j]->get()[id < info->rot[j]->size() ? id : info->rot[j]->size() - 1];
      if(info->ang.size() == ud->gMbKeys.size() && sampleInfo.alpha > 0.0)
      {
        Alembic::Abc::Quatf angAbc = info->ang[j]->get()[id < info->ang[j]->size() ? id : info->ang[j]->size() -1] * (float)sampleInfo.alpha;
        if(angAbc.axis().length2() != 0.0f && angAbc.r != 0.0f)
        {
          rotAbc = angAbc * rotAbc;
          rotAbc.normalize();
        }
      }
      Alembic::Abc::M44f matrixAbcRot;
      matrixAbcRot.setAxisAngle(rotAbc.axis(),rotAbc.angle());
      matrixAbc = matrixAbcRot * matrixAbc;
    }

    // and finally scaling
    if(info->scale.size() == ud->gMbKeys.size() * 2)
    {
      const Alembic::Abc::V3f scalingAbc = info->scale[floorIndex]->get()[id < info->scale[floorIndex]->size() ? id : info->scale[floorIndex]->size() - 1] * 
                                           info->width[floorIndex]->get()[id < info->width[floorIndex]->size() ? id : info->width[floorIndex]->size() - 1] * float(1.0 - sampleInfo.alpha) + 
                                           info->scale[ceilIndex]->get()[id < info->scale[ceilIndex]->size() ? id : info->scale[ceilIndex]->size() - 1] * 
                                           info->width[ceilIndex]->get()[id < info->width[ceilIndex]->size() ? id : info->width[ceilIndex]->size() - 1] * float(sampleInfo.alpha);
      matrixAbc.scale(scalingAbc);
    }
    else
    {
      const float width = info->width[floorIndex]->get()[id < info->width[floorIndex]->size() ? id : info->width[floorIndex]->size() - 1] * float(1.0 - sampleInfo.alpha) + 
                          info->width[ceilIndex]->get()[id < info->width[ceilIndex]->size() ? id : info->width[ceilIndex]->size() - 1] * float(sampleInfo.alpha);
      matrixAbc.scale(Alembic::Abc::V3f(width,width,width));
    }

    // if we have offset matrices
    if(group->parents.size() > groupID && group->matrices.size() > groupID)
    {
      if(group->objects[groupID].valid() && group->parents[groupID].valid())
      {
        // we have a matrix map and a parent.
        // now we need to check if we already exported the matrices
        std::map<float,std::vector<Alembic::Abc::M44f> >::iterator it;
        std::vector<Alembic::Abc::M44f> offsets;
        it = group->matrices[groupID].find(centroidTime);
        if(it == group->matrices[groupID].end())
        {
          std::vector<float> samples(ud->gMbKeys.size());
          offsets.resize(ud->gMbKeys.size());
          for(AtInt sampleIndex=0;sampleIndex<(AtInt)ud->gMbKeys.size(); ++sampleIndex)
          {
            offsets[sampleIndex].makeIdentity();
            // centralize the time once more
            samples[sampleIndex] = centroidTime + ud->gMbKeys[sampleIndex] - ud->gCentroidTime;
          }

          // if the transform differs, we need to compute the offset matrices
          // get the parent, which should be a transform
          Alembic::Abc::IObject parent = group->parents[groupID];
          Alembic::Abc::IObject xform = group->objects[groupID].getParent();
          while(Alembic::AbcGeom::IXform::matches(xform.getMetaData()) && xform.getFullName() != parent.getFullName())
          {
            // cast to a xform
            Alembic::AbcGeom::IXform parentXform(xform,Alembic::Abc::kWrapExisting);
            if(parentXform.getSchema().getNumSamples() == 0)
              break;

            // loop over all samples
            for(size_t sampleIndex=0;sampleIndex<ud->gMbKeys.size(); ++sampleIndex)
            {
              SampleInfo sampleInfo = getSampleInfo(
                 samples[sampleIndex],
                 parentXform.getSchema().getTimeSampling(),
                 parentXform.getSchema().getNumSamples()
              );

              // get the data and blend it if necessary
              Alembic::AbcGeom::XformSample sample;
              parentXform.getSchema().get(sample,sampleInfo.floorIndex);
              Alembic::Abc::M44f abcMatrix;
              Alembic::Abc::M44d abcMatrixd = sample.getMatrix();
              for(int x=0;x<4;x++)
                 for(int y=0;y<4;y++)
                    abcMatrix[x][y] = (float)abcMatrixd[x][y];
               
              if(sampleInfo.alpha >= sampleTolerance)
              {
                parentXform.getSchema().get(sample,sampleInfo.ceilIndex);
                Alembic::Abc::M44d ceilAbcMatrixd = sample.getMatrix();
                Alembic::Abc::M44f ceilAbcMatrix;
                for(int x=0;x<4;x++)
                  for(int y=0;y<4;y++)
                     ceilAbcMatrix[x][y] = (float)ceilAbcMatrixd[x][y];
                abcMatrix = float(1.0 - sampleInfo.alpha) * abcMatrix + float(sampleInfo.alpha) * ceilAbcMatrix;
              }

              offsets[sampleIndex] = abcMatrix * offsets[sampleIndex];
            }

            // go upwards
            xform = xform.getParent();
          }
          group->matrices[groupID].insert(std::pair<float,std::vector<Alembic::Abc::M44f> >(centroidTime,offsets));
        }
        else
          offsets = it->second;

        // this means we have the right amount of matrices to blend against
        if(offsets.size() > j)
          matrixAbc = offsets[j] * matrixAbc;
      }
    }

    // store it to the array
    AiArraySetMtx(matrices,(AtULong)j,matrixAbc.x);
  }

  AiNodeSetArray(shapeNode,"matrix",matrices);
  AiNodeSetBool(shapeNode, "inherit_xform", FALSE);

  return shapeNode;
}
