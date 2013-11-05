#include "stdafx.h"
#include "AlembicIntermediatePolymeshXSI.h"

using namespace XSI;
using namespace XSI::MATH;


CValue GetOption(std::map<XSI::CString,XSI::CValue>& options, const CString & in_Name)
{
   std::map<XSI::CString,XSI::CValue>::iterator it = options.find(in_Name);
   if(it != options.end())
      return it->second;
   return CValue(false);
}


void IntermediatePolyMeshXSI::Save(XSI::Primitive prim, double time, std::map<XSI::CString,XSI::CValue>& options, int mNumSampes)
{
   XSI::PolygonMesh mesh = prim.GetGeometry(time);

   CVector3Array pos = mesh.GetVertices().GetPositionArray();
   LONG vertCount = pos.GetCount();

   posVec.resize(vertCount);
   for(LONG i=0;i<vertCount;i++)
   {
      posVec[i].x = (float)pos[i].GetX();
      posVec[i].y = (float)pos[i].GetY();
      posVec[i].z = (float)pos[i].GetZ();
      bbox.extendBy(posVec[i]);
   }


   // get the faces
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


   // we also need to store the face counts as well as face indices
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


   bool exportNormals = GetOption(options, L"exportNormals");
   if(exportNormals)
   {
      std::vector<Abc::N3f> normalVec;
      CVector3Array normals = mesh.GetVertices().GetNormalArray();

      CGeometryAccessor accessor = mesh.GetGeometryAccessor(siConstructionModeSecondaryShape);
      CRefArray userNormalProps = accessor.GetUserNormals();
      CFloatArray shadingNormals;
      accessor.GetNodeNormals(shadingNormals);
      if(userNormalProps.GetCount() > 0)
      {
         ClusterProperty userNormalProp(userNormalProps[0]);
         Cluster cluster(userNormalProp.GetParent());
         CLongArray elements = cluster.GetElements().GetArray();
         CDoubleArray userNormals = userNormalProp.GetElements().GetArray();
         for(LONG i=0;i<elements.GetCount();i++)
         {
            LONG sampleIndex = elements[i] * 3;
            if(sampleIndex >= shadingNormals.GetCount())
               continue;
            shadingNormals[sampleIndex++] = (float)userNormals[i*3+0];
            shadingNormals[sampleIndex++] = (float)userNormals[i*3+1];
            shadingNormals[sampleIndex++] = (float)userNormals[i*3+2];
         }
      }
      normalVec.resize(shadingNormals.GetCount() / 3);

      for(LONG i=0;i<sampleCount;i++)
      {
         LONG lookedup = sampleLookup[i];
         CVector3 normal;
         normal.PutX(shadingNormals[lookedup * 3 + 0]);
         normal.PutY(shadingNormals[lookedup * 3 + 1]);
         normal.PutZ(shadingNormals[lookedup * 3 + 2]);
         normalVec[i].x = (float)normal.GetX();
         normalVec[i].y = (float)normal.GetY();
         normalVec[i].z = (float)normal.GetZ();
      }

      createIndexedArray<Abc::N3f, SortableV3f>(mFaceIndicesVec, normalVec, mIndexedNormals.values, mIndexedNormals.indices);
   }


   ICEAttribute velocitiesAttr = mesh.GetICEAttributeFromName(L"PointVelocity");
   if(velocitiesAttr.IsDefined() && velocitiesAttr.IsValid())
   {
      CICEAttributeDataArrayVector3f velocitiesData;
      velocitiesAttr.GetDataArray(velocitiesData);
      
      bool bAllZero = true;
      {
         ESS_PROFILE_SCOPE("PointVelocity-zero-check");

         bAllZero = true;
         for(ULONG i=0;i<velocitiesData.GetCount(); i++){
            CVector3 vel;
            vel.PutX(velocitiesData[i].GetX());
            vel.PutY(velocitiesData[i].GetY());
            vel.PutZ(velocitiesData[i].GetZ());

            if(vel.GetX() != 0.0 || vel.GetY() != 0.0 || vel.GetZ() != 0.0){
               bAllZero = false;
            }
         }
      }

      if(!bAllZero)
      {
         mVelocitiesVec.resize(vertCount);
         for(LONG i=0;i<vertCount;i++)
         {
            CVector3 vel;
            vel.PutX(velocitiesData[i].GetX());
            vel.PutY(velocitiesData[i].GetY());
            vel.PutZ(velocitiesData[i].GetZ());
            mVelocitiesVec[i].x = (float)vel.GetX();
            mVelocitiesVec[i].y = (float)vel.GetY();
            mVelocitiesVec[i].z = (float)vel.GetZ();
         }
      }

   }



   // also check if we need to store UV
   CRefArray clusters = mesh.GetClusters();
   if((bool)GetOption(options, L"exportUVs"))
   {
      CGeometryAccessor accessor = mesh.GetGeometryAccessor(siConstructionModeSecondaryShape);
      CRefArray uvPropRefs = accessor.GetUVs();

      // if we now finally found a valid uvprop
      if(uvPropRefs.GetCount() > 0)
      {
         mIndexedUVSet.resize(uvPropRefs.GetCount());

         // loop over all uvsets
         for(LONG uvI=0; uvI<uvPropRefs.GetCount(); uvI++)
         {
            std::vector<Abc::V2f> uvVec;
            uvVec.resize(sampleCount);
            
            CDoubleArray uvValues = ClusterProperty(uvPropRefs[uvI]).GetElements().GetArray();

            for(LONG i=0;i<sampleCount;i++)
            {
               uvVec[i].x = (float)uvValues[sampleLookup[i] * 3 + 0];
               uvVec[i].y = (float)uvValues[sampleLookup[i] * 3 + 1];
            }

            mIndexedUVSet[uvI].name = ClusterProperty(uvPropRefs[uvI]).GetName().GetAsciiString();

            createIndexedArray<Abc::V2f, SortableV2f>(mFaceIndicesVec, uvVec, mIndexedUVSet[uvI].values, mIndexedUVSet[uvI].indices);
         }

         // create the uv options
         if(mUvOptionsVec.size() == 0)
         {
            for(LONG uvI=0;uvI<uvPropRefs.GetCount();uvI++)
            {
               ClusterProperty clusterProperty = (ClusterProperty) uvPropRefs[uvI];
               bool subdsmooth = false;
               if( clusterProperty.GetType() == L"uvspace") {
                  subdsmooth = (bool)clusterProperty.GetParameter(L"subdsmooth").GetValue();      
                  //ESS_LOG_ERROR( "subdsmooth: " << subdsmooth );
               }

               CRefArray children = clusterProperty.GetNestedObjects();
               bool uWrap = false;
               bool vWrap = false;
               for(LONG i=0; i<children.GetCount(); i++)
               {
                  ProjectItem child(children.GetItem(i));
                  CString type = child.GetType();
				// ESS_LOG_ERROR( "  Cluster Property child type: " << type.GetAsciiString() );
                  if(type == L"uvprojdef")
                  {
                     uWrap = (bool)child.GetParameter(L"wrap_u").GetValue();
                     vWrap = (bool)child.GetParameter(L"wrap_v").GetValue();
                     break;
                  }
               }

               // uv wrapping
               mUvOptionsVec.push_back(uWrap ? 1.0f : 0.0f);
               mUvOptionsVec.push_back(vWrap ? 1.0f : 0.0f);
			      mUvOptionsVec.push_back(subdsmooth ? 1.0f : 0.0f);
            }
         }
      }
   }
   

   Property geomApproxProp;
   prim.GetParent3DObject().GetPropertyFromName(L"geomapprox",geomApproxProp);
   bGeomApprox = (Abc::int32_t) geomApproxProp.GetParameterValue(L"gapproxmordrsl");


   if((bool)GetOption(options, L"exportFaceSets") && mFaceSets.empty())//facesets are only exported once
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

         mFaceSets.push_back(FaceSetStruct());
         mFaceSets[mFaceSets.size()-1].name = name;
         for(LONG j=0;j<elements.GetCount();j++){
            mFaceSets[mFaceSets.size()-1].faceIds.push_back(elements[j]);
         }
      }
   }

   // check if we need to export the bindpose (also only for first frame)
   if((bool)GetOption(options, L"exportBindPose") && prim.GetParent3DObject().GetEnvelopes().GetCount() > 0 && mBindPoseVec.empty())
   {
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
   }   


}

void IntermediatePolyMeshXSI::clearNonConstProperties()
{
   bbox = Abc::Box3d();

	posVec.clear();
	mFaceCountVec.clear();
	mFaceIndicesVec.clear();  

   mVelocitiesVec.clear();

   mIndexedNormals.name = std::string();
	mIndexedNormals.indices.clear();
   mIndexedNormals.values.clear();
	mIndexedUVSet.clear();
	
   //mMatIdIndexVec.clear();

   bGeomApprox = 0;
}