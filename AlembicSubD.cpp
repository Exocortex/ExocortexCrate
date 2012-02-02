#include "AlembicSubD.h"
#include "AlembicPolyMsh.h"
#include "AlembicXform.h"

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_polygonmesh.h>
#include <xsi_vertex.h>
#include <xsi_polygonface.h>
#include <xsi_sample.h>
#include <xsi_math.h>
#include <xsi_context.h>
#include <xsi_operatorcontext.h>
#include <xsi_customoperator.h>
#include <xsi_factory.h>
#include <xsi_parameter.h>
#include <xsi_ppglayout.h>
#include <xsi_ppgitem.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_clusterproperty.h>
#include <xsi_cluster.h>
#include <xsi_geometryaccessor.h>
#include <xsi_material.h>
#include <xsi_materiallibrary.h>
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>

using namespace XSI;
using namespace MATH;

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

AlembicSubD::AlembicSubD(const XSI::CRef & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   Primitive prim(GetRef());
   CString subDName(prim.GetParent3DObject().GetName());
   CString xformName(subDName+L"Xfo");
   Alembic::AbcGeom::OXform xform(GetOParent(),xformName.GetAsciiString(),GetJob()->GetAnimatedTs());
   Alembic::AbcGeom::OSubD subD(xform,subDName.GetAsciiString(),GetJob()->GetAnimatedTs());
   AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(subD,GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
   mSubDSchema = subD.getSchema();
}

AlembicSubD::~AlembicSubD()
{
   // we have to clear this prior to destruction
   // this is a workaround for issue-171
   mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicSubD::GetCompound()
{
   return mSubDSchema;
}

XSI::CStatus AlembicSubD::Save(double time)
{
   // store the transform
   Primitive prim(GetRef());
   SaveXformSample(GetRef(1),mXformSchema,mXformSample,time);

   // store the metadata
   SaveMetaData(prim.GetParent3DObject().GetRef(),this);

   // set the visibility
   Property visProp;
   prim.GetParent3DObject().GetPropertyFromName(L"Visibility",visProp);
   if(isRefAnimated(visProp.GetRef()) || mNumSamples == 0)
   {
      bool visibility = visProp.GetParameterValue(L"rendvis",time);
      mOVisibility.set(visibility ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
   }

   // check if the mesh is animated
   if(mNumSamples > 0) {
      if(!isRefAnimated(GetRef()))
         return CStatus::OK;
   }

   // determine if we are a pure point cache
   bool purePointCache = (bool)GetJob()->GetOption(L"exportPurePointCache");

   // access the mesh
   PolygonMesh mesh = prim.GetGeometry(time);
   CVector3Array pos = mesh.GetVertices().GetPositionArray();
   LONG vertCount = pos.GetCount();

   // prepare the bounding box
   Alembic::Abc::Box3d bbox;

   // allocate the points and normals
   std::vector<Alembic::Abc::V3f> posVec(vertCount);
   for(LONG i=0;i<vertCount;i++)
   {
      posVec[i].x = (float)pos[i].GetX();
      posVec[i].y = (float)pos[i].GetY();
      posVec[i].z = (float)pos[i].GetZ();
      bbox.extendBy(posVec[i]);
   }

   // allocate the sample for the points
   if(posVec.size() == 0)
   {
      bbox.extendBy(Alembic::Abc::V3f(0,0,0));
      posVec.push_back(Alembic::Abc::V3f(FLT_MAX,FLT_MAX,FLT_MAX));
   }
   Alembic::Abc::P3fArraySample posSample(&posVec.front(),posVec.size());

   // store the positions && bbox
   mSubDSample.setPositions(posSample);
   mSubDSample.setSelfBounds(bbox);

   // abort here if we are just storing points
   if(purePointCache)
   {
      if(mNumSamples == 0)
      {
         // store a dummy empty topology
         mFaceCountVec.push_back(0);
         mFaceIndicesVec.push_back(0);
         Alembic::Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),mFaceCountVec.size());
         Alembic::Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),mFaceIndicesVec.size());
         mSubDSample.setFaceCounts(faceCountSample);
         mSubDSample.setFaceIndices(faceIndicesSample);
      }

      mSubDSchema.set(mSubDSample);
      mNumSamples++;
      return CStatus::OK;
   }

   // check if we support changing topology
   bool dynamicTopology = (bool)GetJob()->GetOption(L"exportDynamicTopology");

   CPolygonFaceRefArray faces = mesh.GetPolygons();
   LONG faceCount = faces.GetCount();
   LONG sampleCount = mesh.GetSamples().GetCount();

   // create a sample look table
   LONG offset = 0;
   CLongArray sampleLookup(sampleCount);
   for(LONG i=0;i<faces.GetCount();i++)
   {
      PolygonFace face(faces[i]);
      CLongArray samples = face.GetSamples().GetIndexArray();
      for(LONG j=samples.GetCount()-1;j>=0;j--)
         sampleLookup[offset++] = samples[j];
   }

   // check if we should export the velocities
   if(dynamicTopology)
   {
      ICEAttribute velocitiesAttr = mesh.GetICEAttributeFromName(L"PointVelocity");
      if(velocitiesAttr.IsDefined() && velocitiesAttr.IsValid())
      {
         CICEAttributeDataArrayVector3f velocitiesData;
         velocitiesAttr.GetDataArray(velocitiesData);

         mVelocitiesVec.resize(vertCount);
         for(LONG i=0;i<vertCount;i++)
         {
            mVelocitiesVec[i].x = velocitiesData[i].GetX();
            mVelocitiesVec[i].y = velocitiesData[i].GetY();
            mVelocitiesVec[i].z = velocitiesData[i].GetZ();
         }

         if(mVelocitiesVec.size() == 0)
            mVelocitiesVec.push_back(Alembic::Abc::V3f(0,0,0));
         Alembic::Abc::V3fArraySample sample = Alembic::Abc::V3fArraySample(&mVelocitiesVec.front(),mVelocitiesVec.size());
         mSubDSample.setVelocities(sample);
      }
   }

   // if we are the first frame!
   if(mNumSamples == 0 || dynamicTopology)
   {
      // we also need to store the face counts as well as face indices
      if(mFaceIndicesVec.size() != sampleCount || sampleCount == 0)
      {
         mFaceCountVec.resize(faceCount);
         mFaceIndicesVec.resize(sampleCount);

         offset = 0;
         for(LONG i=0;i<faceCount;i++)
         {
            PolygonFace face(faces[i]);
            CLongArray indices = face.GetVertices().GetIndexArray();
            mFaceCountVec[i] = indices.GetCount();
            for(LONG j=indices.GetCount()-1;j>=0;j--)
               mFaceIndicesVec[offset++] = indices[j];
         }

         if(mFaceIndicesVec.size() == 0)
         {
            mFaceCountVec.push_back(0);
            mFaceIndicesVec.push_back(0);
         }
         Alembic::Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),mFaceCountVec.size());
         Alembic::Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),mFaceIndicesVec.size());

         mSubDSample.setFaceCounts(faceCountSample);
         mSubDSample.setFaceIndices(faceIndicesSample);
      }

      // set the subd level
      Property geomApproxProp;
      prim.GetParent3DObject().GetPropertyFromName(L"geomapprox",geomApproxProp);
      mSubDSample.setFaceVaryingInterpolateBoundary(geomApproxProp.GetParameterValue(L"gapproxmordrsl"));

      // also check if we need to store UV
      CRefArray clusters = mesh.GetClusters();
      if((bool)GetJob()->GetOption(L"exportUVs"))
      {
         CRef uvPropRef;
         for(LONG i=0;i<clusters.GetCount();i++)
         {
            Cluster cluster(clusters[i]);
            if(!cluster.GetType().IsEqualNoCase(L"sample"))
               continue;
            CRefArray props(cluster.GetLocalProperties());
            for(LONG k=0;k<props.GetCount();k++)
            {
               ClusterProperty prop(props[k]);
               if(prop.GetType().IsEqualNoCase(L"uvspace"))
               {
                  uvPropRef = props[k];
                  break;
               }
            }
            if(uvPropRef.IsValid())
               break;
         }
         if(uvPropRef.IsValid())
         {
            // ok, great, we found UVs, let's set them up
            mUvVec.resize(sampleCount);
            CDoubleArray uvValues = ClusterProperty(uvPropRef).GetElements().GetArray();

            for(LONG i=0;i<sampleCount;i++)
            {
               mUvVec[i].x = (float)uvValues[sampleLookup[i] * 3 + 0];
               mUvVec[i].y = (float)uvValues[sampleLookup[i] * 3 + 1];
            }

            // now let's sort the normals 
            size_t uvCount = mUvVec.size();
            size_t uvIndexCount = 0;
            if((bool)GetJob()->GetOption(L"indexedUVs")) {
               std::map<SortableV2f,size_t> uvMap;
               std::map<SortableV2f,size_t>::const_iterator it;
               size_t sortedUVCount = 0;
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
            mSubDSample.setUVs(uvSample);
         }
      }

      // sweet, now let's have a look at face sets
      if(GetJob()->GetOption(L"exportFaceSets") && mNumSamples == 0)
      {
         for(LONG i=0;i<clusters.GetCount();i++)
         {
            Cluster cluster(clusters[i]);
            if(!cluster.GetType().IsEqualNoCase(L"poly"))
               continue;

            CLongArray elements = cluster.GetElements().GetArray();
            if(elements.GetCount() == 0)
               continue;

            std::string name(cluster.GetName().GetAsciiString());

            mFaceSetsVec.push_back(std::vector<int32_t>());
            std::vector<int32_t> & faceSetVec = mFaceSetsVec.back();
            for(LONG j=0;j<elements.GetCount();j++)
               faceSetVec.push_back(elements[j]);

            if(faceSetVec.size() > 0)
            {
               Alembic::AbcGeom::OFaceSet faceSet = mSubDSchema.createFaceSet(name);
               Alembic::AbcGeom::OFaceSetSchema::Sample faceSetSample(Alembic::Abc::Int32ArraySample(&faceSetVec.front(),faceSetVec.size()));
               faceSet.getSchema().set(faceSetSample);
            }
         }
      }

      // save the sample
      mSubDSchema.set(mSubDSample);

      // check if we need to export the bindpose
      if(GetJob()->GetOption(L"exportBindPose") && prim.GetParent3DObject().GetEnvelopes().GetCount() > 0 && mNumSamples == 0)
      {
         mBindPoseProperty = OV3fArrayProperty(mSubDSchema, ".bindpose", mSubDSchema.getMetaData(), GetJob()->GetAnimatedTs());

         // store the positions of the modeling stack into here
         PolygonMesh bindPoseGeo = prim.GetGeometry(time, siConstructionModeModeling);
         CVector3Array bindPosePos = bindPoseGeo.GetPoints().GetPositionArray();
         mBindPoseVec.resize((size_t)bindPosePos.GetCount());
         for(LONG i=0;i<bindPosePos.GetCount();i++)
         {
            mBindPoseVec[i].x = (float)bindPosePos[i].GetX();
            mBindPoseVec[i].y = (float)bindPosePos[i].GetY();
            mBindPoseVec[i].z = (float)bindPosePos[i].GetZ();
         }

         Alembic::Abc::V3fArraySample sample;
         if(mBindPoseVec.size() > 0)
            sample = Alembic::Abc::V3fArraySample(&mBindPoseVec.front(),mBindPoseVec.size());
         mBindPoseProperty.set(sample);
      }   
   }
   else
   {
      mSubDSchema.set(mSubDSample);
   }

   mNumSamples++;

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_geomapprox_Define( CRef& in_ctxt )
{
   return alembicOp_Define(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_geomapprox_DefineLayout( CRef& in_ctxt )
{
   return alembicOp_DefineLayout(in_ctxt);
}

XSIPLUGINCALLBACK CStatus alembic_geomapprox_Update( CRef& in_ctxt )
{
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

   Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;

   Alembic::AbcGeom::ISubD obj(iObj,Alembic::Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   Alembic::AbcGeom::ISubDSchema::Sample sample;
   obj.getSchema().get(sample,0);

   Property prop(ctxt.GetOutputTarget());
   LONG subDLevel = sample.getFaceVaryingInterpolateBoundary();
   prop.PutParameterValue(L"gapproxmosl",subDLevel);
   prop.PutParameterValue(L"gapproxmordrsl",subDLevel);

   return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_geomapprox_Term(CRef & in_ctxt)
{
   return alembicOp_Term(in_ctxt);
}
