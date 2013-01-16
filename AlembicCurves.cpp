#include "stdafx.h"
#include "AlembicCurves.h"
#include "AlembicXform.h"
 
using namespace XSI;
using namespace MATH;


AlembicCurves::AlembicCurves(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
: AlembicObject(eNode, in_Job, oParent)
{
	AbcG::OCurves curves(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());

   // create the generic properties
   mOVisibility = CreateVisibilityProperty(curves,GetJob()->GetAnimatedTs());

   mCurvesSchema = curves.getSchema();

   // create all properties
   mRadiusProperty = Abc::OFloatArrayProperty(mCurvesSchema.getArbGeomParams(), ".radius", mCurvesSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mColorProperty = Abc::OC4fArrayProperty(mCurvesSchema.getArbGeomParams(), ".color", mCurvesSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mFaceIndexProperty = Abc::OInt32ArrayProperty(mCurvesSchema.getArbGeomParams(), ".face_index", mCurvesSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mVertexIndexProperty = Abc::OInt32ArrayProperty(mCurvesSchema.getArbGeomParams(), ".vertex_index", mCurvesSchema.getMetaData(), GetJob()->GetAnimatedTs() );
}

AlembicCurves::~AlembicCurves()
{
   // we have to clear this prior to destruction
   // this is a workaround for issue-171
   mOVisibility.reset();
}

Abc::OCompoundProperty AlembicCurves::GetCompound()
{
   return mCurvesSchema;
}

XSI::CStatus AlembicCurves::Save(double time)
{
   // store the transform
   Primitive prim(GetRef(REF_PRIMITIVE));
   const bool globalSpace = GetJob()->GetOption(L"globalSpace");

   // query the global space
   CTransformation globalXfo;
   if(globalSpace)
      globalXfo = KinematicState(GetRef(REF_GLOBAL_TRANS)).GetTransform(time);
   CTransformation globalRotation;
   globalRotation.SetRotation(globalXfo.GetRotation());

   // set the visibility
   Property visProp;
   prim.GetParent3DObject().GetPropertyFromName(L"Visibility",visProp);
   if(isRefAnimated(visProp.GetRef()) || mNumSamples == 0)
   {
      const bool visibility = visProp.GetParameterValue(L"rendvis",time);
      mOVisibility.set(visibility ?AbcG::kVisibilityVisible :AbcG::kVisibilityHidden);
   }

   // store the metadata
   SaveMetaData(GetRef(REF_NODE),this);

   // check if the crvlist is animated
   if(mNumSamples > 0) {
      if(!isRefAnimated(GetRef(REF_PRIMITIVE),false,globalSpace))
         return CStatus::OK;
   }

   const bool guideCurves = GetJob()->GetOption(L"guideCurves");

   // access the crvlist
   if(prim.GetType().IsEqualNoCase(L"crvlist"))
   {
      NurbsCurveList crvlist = prim.GetGeometry(time);
      CVector3Array pos = crvlist.GetPoints().GetPositionArray();
      ULONG vertCount = pos.GetCount();

      // prepare the bounding box
      Abc::Box3d bbox;

      // allocate the points and normals
      std::vector<Abc::V3f> posVec(vertCount);
      for(ULONG i=0;i<vertCount;i++)
      {
         if(globalSpace)
            pos[i] = MapObjectPositionToWorldSpace(globalXfo,pos[i]);
         posVec[i].x = (float)pos[i].GetX();
         posVec[i].y = (float)pos[i].GetY();
         posVec[i].z = (float)pos[i].GetZ();
         bbox.extendBy(posVec[i]);
      }

      // store the bbox
      mCurvesSample.setSelfBounds(bbox);

      // allocate for the points and normals
      Abc::P3fArraySample posSample(&posVec.front(),posVec.size());

      // if we are the first frame!
      if(mNumSamples == 0)
      {
         // we also need to store the face counts as well as face indices
         CNurbsCurveRefArray curves = crvlist.GetCurves();
         mNbVertices.resize((size_t)curves.GetCount());
         for(LONG i=0;i<curves.GetCount();i++)
			 mNbVertices[i] = (Abc::int32_t)NurbsCurve(curves[i]).GetControlPoints().GetCount();

         Abc::Int32ArraySample nbVerticesSample(&mNbVertices.front(),mNbVertices.size());

         mCurvesSample.setPositions(posSample);
         mCurvesSample.setCurvesNumVertices(nbVerticesSample);

         // set the type + wrapping
         CNurbsCurveData curveData;
         NurbsCurve(curves[0]).Get(siSINurbs,curveData);
         mCurvesSample.setType(curveData.m_lDegree == 3 ?AbcG::kCubic :AbcG::kLinear);
         mCurvesSample.setWrap(curveData.m_bClosed ? AbcG::kPeriodic : AbcG::kNonPeriodic);
         mCurvesSample.setBasis(AbcG::kNoBasis);

         // save the sample
         mCurvesSchema.set(mCurvesSample);
      }
      else
      {
         mCurvesSample.setPositions(posSample);
         mCurvesSchema.set(mCurvesSample);
      }
   }
   else if(prim.GetType().IsEqualNoCase(L"hair") && !guideCurves)
   {
		#define CHUNK_SIZE 200000		// same chunck size used in Arnold
		HairPrimitive hairPrim(GetRef(REF_PRIMITIVE));
		LONG totalHairs = prim.GetParameterValue(L"TotalHairs");
		{
			const LONG strandMult = (LONG)prim.GetParameterValue(L"StrandMult");
			if (strandMult > 1)
				totalHairs *= strandMult;
		}
		CRenderHairAccessor accessor = hairPrim.GetRenderHairAccessor(totalHairs, CHUNK_SIZE, time);

		// prepare the bounding box
		Abc::Box3d bbox;
		std::vector<Abc::V3f> posVec;

		while(accessor.Next())
		{
			const int chunckSize = accessor.GetChunkHairCount();
			if (chunckSize == 0)
				break;
			CFloatArray hairPos;
			CStatus result = accessor.GetVertexPositions(hairPos);
			const ULONG vertCount = hairPos.GetCount() / 3;
			size_t posVecOffset = posVec.size();
			posVec.resize(posVec.size() + size_t(vertCount));
			ULONG offset = 0;
			for(ULONG i=0;i<vertCount;i++)
			{
				CVector3 pos;
				pos.PutX(hairPos[offset++]);
				pos.PutY(hairPos[offset++]);
				pos.PutZ(hairPos[offset++]);
				if(globalSpace)
				   pos = MapObjectPositionToWorldSpace(globalXfo,pos);

				Abc::V3f &posV = posVec[posVecOffset+i];
				posV.x = (float)pos.GetX();
				posV.y = (float)pos.GetY();
				posV.z = (float)pos.GetZ();
				bbox.extendBy(posV);
			}

			// if we are the first frame!
			if(mNumSamples == 0)
			{
				{
					CLongArray hairCount;
					accessor.GetVerticesCount(hairCount);
					const int offset = mNbVertices.size();
					mNbVertices.resize(offset + (size_t)hairCount.GetCount());
					for(LONG i=0;i<hairCount.GetCount();i++)
						mNbVertices[offset + i] = (Abc::int32_t)hairCount[i];
					hairCount.Clear();
				}

				 // store the hair radius
				{
					CFloatArray hairRadius;
					accessor.GetVertexRadiusValues(hairRadius);
					const int offset = mRadiusVec.size();
					mRadiusVec.resize(offset + (size_t)hairRadius.GetCount());
					for(LONG i=0;i<hairRadius.GetCount();i++)
						mRadiusVec[offset+i] = hairRadius[i];
					hairRadius.Clear();
				}

				 // store the hair color (if any)
				 if(accessor.GetVertexColorCount() > 0)
				 {
					CFloatArray hairColor;
					accessor.GetVertexColorValues(0,hairColor);
					const int c_offset = mColorVec.size();
					mColorVec.resize(c_offset + (size_t)hairColor.GetCount()/4);
					ULONG offset = 0;
					for(size_t i=0;i<mColorVec.size();i++)
					{
						Imath::C4f &col = mColorVec[c_offset+i];
						col.r = hairColor[offset++];
						col.g = hairColor[offset++];
						col.b = hairColor[offset++];
						col.a = hairColor[offset++];
					}
					hairColor.Clear();
				 }

				 // store the hair color (if any)
				 if(accessor.GetUVCount() > 0)
				 {
					CFloatArray hairUV;
					accessor.GetUVValues(0,hairUV);
					const int u_offset = mUvVec.size();
					mUvVec.resize(u_offset+(size_t)hairUV.GetCount()/3);
					ULONG offset = 0;
					for(size_t i=0;i<mUvVec.size();i++)
					{
						Imath::V2f &uv = mUvVec[u_offset+i];
						uv.x = hairUV[offset++];
						uv.y = 1.0f - hairUV[offset++];
						offset++;
					}
					hairUV.Clear();
				 }
			}
		}

		if(mNumSamples == 0)
		{
			// set the type + wrapping
			mCurvesSample.setType(AbcG::kLinear);
			mCurvesSample.setWrap(AbcG::kNonPeriodic);
			mCurvesSample.setBasis(AbcG::kNoBasis);

			mCurvesSample.setCurvesNumVertices(Abc::Int32ArraySample(mNbVertices));
			mRadiusProperty.set(Abc::FloatArraySample(mRadiusVec));
			if (!mColorVec.empty())
				mColorProperty.set(Abc::C4fArraySample(mColorVec));
			if (!mUvVec.empty())
				mCurvesSample.setUVs(AbcG::OV2fGeomParam::Sample(mUvVec ,AbcG::kVertexScope));
		}
		mCurvesSample.setPositions(Abc::P3fArraySample(posVec));

		// store the bbox
		mCurvesSample.setSelfBounds(bbox);
		mCurvesSchema.set(mCurvesSample);
   }
   else if(prim.GetType().IsEqualNoCase(L"hair") && guideCurves)
   {
      // access the guide hairs
      Geometry geom = prim.GetGeometry(time);
      CPointRefArray pointRefArray(geom.GetPoints());
      CVector3Array pos( pointRefArray.GetPositionArray());	  
      LONG vertCount = pos.GetCount();

      // prepare the bounding box
      Abc::Box3d bbox;
      std::vector<Abc::V3f> posVec;

      LONG numCurves = vertCount/14;
      posVec.resize(numCurves*15); // include base points

      // add curve points
      LONG posIndex=0, posVecIndex=0;
      for(LONG j=0;j<numCurves;j++)
      {
         posVecIndex++;

         // add guide hair points
         for(LONG i=0;i<14;i++,posIndex++,posVecIndex++)
         {
            if(globalSpace)
               pos[posIndex] = MapObjectPositionToWorldSpace(globalXfo,pos[posIndex]);
            posVec[posVecIndex].x = (float)pos[posIndex].GetX();
            posVec[posVecIndex].y = (float)pos[posIndex].GetY();
            posVec[posVecIndex].z = (float)pos[posIndex].GetZ();
         }		  
      }

      // Collect all base points for the roots of the hairs:
      // note that the root point of the guide curves is not part of the hair geometry so
      // you have to go through the hairGen operators input ports, find the connected cluster
      // and iterate the faces in the cluster. the guide hairs are order by cloickwise first hit face vertex
      // Also note that the base point is not on the subdiv surface currently.
      CRef hairGenOpRef;
      hairGenOpRef.Set(GetRef(REF_PRIMITIVE).GetAsText()+L".hairGenOp");
      if(hairGenOpRef.GetAsText().IsEmpty())
      {
         Application().LogMessage(L"Fatal error: The hair doesn't have a hairGenOperator!",siWarningMsg);
         return CStatus::Fail;
      }

      Operator hairGenOp(hairGenOpRef);
      CRef emitterPrimRef,emitterKineRef,emitterClusterRef;
      emitterPrimRef = Port(hairGenOp.GetInputPorts().GetItem(0)).GetTarget();
      emitterKineRef = Port(hairGenOp.GetInputPorts().GetItem(1)).GetTarget();
      emitterClusterRef = Port(hairGenOp.GetInputPorts().GetItem(3)).GetTarget();

      Primitive emitterPrim(emitterPrimRef);
      PolygonMesh emitterGeo = emitterPrim.GetGeometry(time);
      CPointRefArray emitterPointRefArray(emitterGeo.GetPoints());
      CLongArray emitterPntIndex;

	  std::vector<Abc::int32_t> faceIndices;

      if( !SIObject(emitterPrimRef).GetType().IsEqualNoCase(L"polymsh"))
      {
         Application().LogMessage(L"Error: The hair needs to be emitted from a polygon",siWarningMsg);
         return CStatus::Fail;
      }
      else if ( !SIObject(emitterClusterRef).GetType().IsEqualNoCase(L"poly"))
      {
         // assume that there is a guide curve per vertex in the emitter geometry
         // hence the base points should correspond to point ids
         assert( numCurves==emitterPointRefArray.GetCount());
         for(long i=0;i<numCurves;i++)
            emitterPntIndex.Add(i);

		 // add all the faces to the face indices
		 int numPolys = emitterGeo.GetPolygons().GetCount();
		 faceIndices.resize( numPolys);
		 for ( int i=0; i<numPolys; i++)
			faceIndices[i] = i;
      }
      else
      {
         // otherwise use the cluster
         Cluster emitterCluster(emitterClusterRef);
         CLongArray clusterElements = emitterCluster.GetElements().GetArray();
         CLongArray emitterPntUsed(emitterPointRefArray.GetCount());

		 faceIndices.resize( clusterElements.GetCount());
         CLongArray facetIndex;
         for(long i=0;i<clusterElements.GetCount();i++)
         {
			faceIndices[i] = clusterElements[i];
            facetIndex = Facet(emitterGeo.GetFacets().GetItem(clusterElements[i])).GetPoints().GetIndexArray();
            for(long j=0;j<facetIndex.GetCount();j++)
            {
               if(emitterPntUsed[facetIndex[j]]==0)
               {
                  emitterPntUsed[facetIndex[j]] = 1;
                  emitterPntIndex.Add(facetIndex[j]);
               }
            }
         }
      }

      long curveCount = emitterPntIndex.GetCount();
      assert( curveCount == numCurves);

      // add base points
      posVecIndex=0;
      for(LONG j=0;j<numCurves;j++)
      {
         // add base point
         CVector3 base = Point(emitterPointRefArray.GetItem(emitterPntIndex[j])).GetPosition();
         if(globalSpace)
            base = MapObjectPositionToWorldSpace(globalXfo,base);

         posVec[posVecIndex].x = (float)base.GetX();
         posVec[posVecIndex].y = (float)base.GetY();
         posVec[posVecIndex].z = (float)base.GetZ();

         posVecIndex += 15;
      }

      mCurvesSample.setPositions(Abc::P3fArraySample(posVec));

      // caclulate bbox
      for ( LONG i=0; i<curveCount*15; i++)
         bbox.extendBy(posVec[i]);

      // store the bbox
      mCurvesSample.setSelfBounds(bbox);

      // if we are the first frame then define the curves and store the indices
      if(mNumSamples == 0)
      {
         const int hairVertCount = 15;

         mNbVertices.clear();
         mNbVertices.resize(numCurves, hairVertCount);
         mCurvesSample.setCurvesNumVertices(Abc::Int32ArraySample(mNbVertices));

         // set the type + wrapping
         mCurvesSample.setType(AbcG::kLinear);
         mCurvesSample.setWrap(AbcG::kNonPeriodic);
         mCurvesSample.setBasis(AbcG::kNoBasis);

		// store the vertex indices
		 std::vector<Abc::int32_t> vertexIndices( emitterPntIndex.GetCount());
		 for(LONG i=0;i<emitterPntIndex.GetCount();i++)
			vertexIndices[i] = emitterPntIndex[i];
		 mVertexIndexProperty.set(Abc::Int32ArraySample(vertexIndices));

		 // store the face indices
		 mFaceIndexProperty.set(Abc::Int32ArraySample(faceIndices));
      }
      mCurvesSchema.set(mCurvesSample);
   }
   else if(prim.GetType().IsEqualNoCase(L"pointcloud"))
   {
      // prepare the bounding box
      Abc::Box3d bbox;

      Geometry geo = prim.GetGeometry(time);
      ICEAttribute attr = geo.GetICEAttributeFromName(L"StrandPosition");
      size_t vertexCount = 0;
      std::vector<Abc::V3f> posVec;
      if(attr.IsDefined() && attr.IsValid())
      {
         CICEAttributeDataArray2DVector3f data;
         attr.GetDataArray2D(data);

         CICEAttributeDataArrayVector3f sub;
         mNbVertices.resize(data.GetCount());
         for(ULONG i=0;i<data.GetCount();i++)
         {
            data.GetSubArray(i,sub);
            vertexCount += sub.GetCount();
            mNbVertices[i] = (Abc::int32_t)sub.GetCount();
         }
         mCurvesSample.setCurvesNumVertices(Abc::Int32ArraySample(&mNbVertices.front(),mNbVertices.size()));

         // set wrap parameters
         mCurvesSample.setType(AbcG::kLinear);
         mCurvesSample.setWrap(AbcG::kNonPeriodic);
		 mCurvesSample.setBasis(AbcG::kNoBasis);

         posVec.resize(vertexCount);
         size_t offset = 0;
         for(ULONG i=0;i<data.GetCount();i++)
         {
            data.GetSubArray(i,sub);
            for(ULONG j=0;j<sub.GetCount();j++)
            {
               CVector3 pos;
               pos.PutX(sub[j].GetX());
               pos.PutY(sub[j].GetY());
               pos.PutZ(sub[j].GetZ());
               if(globalSpace)
                  pos = MapObjectPositionToWorldSpace(globalXfo,pos);
               posVec[offset].x = (float)pos.GetX();
               posVec[offset].y = (float)pos.GetY();
               posVec[offset].z = (float)pos.GetZ();
               bbox.extendBy(posVec[offset]);
               offset++;
            }
         }

         if(vertexCount > 0)
            mCurvesSample.setPositions(Abc::P3fArraySample(&posVec.front(),posVec.size()));
         else
            mCurvesSample.setPositions(Abc::P3fArraySample());
      }

      // store the bbox
      mCurvesSample.setSelfBounds(bbox);

      if(vertexCount > 0)
      {
         ICEAttribute attr = geo.GetICEAttributeFromName(L"StrandSize");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArray2DFloat data;
            attr.GetDataArray2D(data);

            CICEAttributeDataArrayFloat sub;

            mRadiusVec.resize(vertexCount);
            size_t offset = 0;
            for(ULONG i=0;i<data.GetCount();i++)
            {
               data.GetSubArray(data.IsConstant() ? 0 : i,sub);
               for(ULONG j=0;j<sub.GetCount();j++)
                  mRadiusVec[offset++] = (float)sub[j];
            }
            mRadiusProperty.set(Abc::FloatArraySample(&mRadiusVec.front(),mRadiusVec.size()));
         }
         else
         {
            attr = geo.GetICEAttributeFromName(L"Size");
            if(attr.IsDefined() && attr.IsValid())
            {
               CICEAttributeDataArrayFloat data;
               attr.GetDataArray(data);

               mRadiusVec.resize(data.GetCount());
               for(ULONG i=0;i<data.GetCount();i++)
                  mRadiusVec[i] = (float)data[data.IsConstant() ? 0 : i];
               mRadiusProperty.set(Abc::FloatArraySample(&mRadiusVec.front(),mRadiusVec.size()));
            }
         }

         attr = geo.GetICEAttributeFromName(L"StrandColor");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArray2DColor4f data;
            attr.GetDataArray2D(data);

            CICEAttributeDataArrayColor4f sub;

            mColorVec.resize(vertexCount);
            size_t offset = 0;
            for(ULONG i=0;i<data.GetCount();i++)
            {
               data.GetSubArray(data.IsConstant() ? 0 : i,sub);
               for(ULONG j=0;j<sub.GetCount();j++)
               {
                  mColorVec[offset].r = (float)sub[j].GetR();
                  mColorVec[offset].g = (float)sub[j].GetG();
                  mColorVec[offset].b = (float)sub[j].GetB();
                  mColorVec[offset].a = (float)sub[j].GetA();
                  offset++;
               }
            }
            mColorProperty.set(Abc::C4fArraySample(&mColorVec.front(),mColorVec.size()));
         }
         else
         {
            attr = geo.GetICEAttributeFromName(L"Color");
            if(attr.IsDefined() && attr.IsValid())
            {
               CICEAttributeDataArrayColor4f data;
               attr.GetDataArray(data);

               mColorVec.resize(data.GetCount());
               for(ULONG i=0;i<data.GetCount();i++)
               {
                  mColorVec[i].r = (float)data[i].GetR();
                  mColorVec[i].g = (float)data[i].GetG();
                  mColorVec[i].b = (float)data[i].GetB();
                  mColorVec[i].a = (float)data[i].GetA();
               }
               mColorProperty.set(Abc::C4fArraySample(&mColorVec.front(),mColorVec.size()));
            }
         }

         attr = geo.GetICEAttributeFromName(L"StrandUVs");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArray2DVector2f data;
            attr.GetDataArray2D(data);

            CICEAttributeDataArrayVector2f sub;

            mUvVec.resize(vertexCount);
            size_t offset = 0;
            for(ULONG i=0;i<data.GetCount();i++)
            {
               data.GetSubArray(data.IsConstant() ? 0 : i,sub);
               for(ULONG j=0;j<sub.GetCount();j++)
               {
                  mUvVec[offset].x = (float)sub[j].GetX();
                  mUvVec[offset].y = (float)sub[j].GetY();
                  offset++;
               }
            }
            mCurvesSample.setUVs(AbcG::OV2fGeomParam::Sample(Abc::V2fArraySample(&mUvVec.front(),mUvVec.size()),AbcG::kVertexScope));
         }
         else
         {
            attr = geo.GetICEAttributeFromName(L"UVs");
            if(attr.IsDefined() && attr.IsValid())
            {
               CICEAttributeDataArrayVector2f data;
               attr.GetDataArray(data);

               mUvVec.resize(data.GetCount());
               for(ULONG i=0;i<data.GetCount();i++)
               {
                  mUvVec[i].x = (float)data[i].GetX();
                  mUvVec[i].y = (float)data[i].GetY();
               }
               mCurvesSample.setUVs(AbcG::OV2fGeomParam::Sample(Abc::V2fArraySample(&mUvVec.front(),mUvVec.size()),AbcG::kVertexScope));
            }
         }

         attr = geo.GetICEAttributeFromName(L"StrandVelocity");
         if(attr.IsDefined() && attr.IsValid())
         {
            CICEAttributeDataArray2DVector3f data;
            attr.GetDataArray2D(data);

            CICEAttributeDataArrayVector3f sub;

            mVelVec.resize(vertexCount);
            size_t offset = 0;
            for(ULONG i=0;i<data.GetCount();i++)
            {
               data.GetSubArray(data.IsConstant() ? 0 : i,sub);
               for(ULONG j=0;j<sub.GetCount();j++)
               {
                  CVector3 vel;
                  vel.PutX(sub[j].GetX());
                  vel.PutY(sub[j].GetY());
                  vel.PutZ(sub[j].GetZ());
                  if(globalSpace)
                     vel = MapObjectPositionToWorldSpace(globalRotation,vel);
                  mVelVec[offset].x = (float)vel.GetX();
                  mVelVec[offset].y = (float)vel.GetY();
                  mVelVec[offset].z = (float)vel.GetZ();
                  offset++;
               }
            }
            mCurvesSample.setVelocities(Abc::V3fArraySample(&mVelVec.front(),mVelVec.size()));
         }
         else
         {
            attr = geo.GetICEAttributeFromName(L"PointVelocity");
            if(attr.IsDefined() && attr.IsValid())
            {
               CICEAttributeDataArrayVector3f data;
               attr.GetDataArray(data);

               mVelVec.resize(data.GetCount());
               for(ULONG i=0;i<data.GetCount();i++)
               {
                  CVector3 vel;
                  vel.PutX(data[i].GetX());
                  vel.PutY(data[i].GetY());
                  vel.PutZ(data[i].GetZ());
                  if(globalSpace)
                     vel = MapObjectPositionToWorldSpace(globalRotation,vel);
                  mVelVec[i].x = (float)vel.GetX();
                  mVelVec[i].y = (float)vel.GetY();
                  mVelVec[i].z = (float)vel.GetZ();
               }
               mCurvesSample.setVelocities(Abc::V3fArraySample(&mVelVec.front(),mVelVec.size()));
            }
         }
      }
      if(vertexCount > 0)
      {
         mCurvesSchema.set(mCurvesSample);
      }
   }
   mNumSamples++;

   return CStatus::OK;
}

ESS_CALLBACK_START( alembic_crvlist_Define, CRef& )
   return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_crvlist_DefineLayout, CRef& )
   return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_crvlist_Update, CRef& )
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
  AbcG::ICurves obj(iObj,Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );


  AbcG::ICurvesSchema::Sample sample;
   obj.getSchema().get(sample,sampleInfo.floorIndex);

   CVector3Array pos;
   Primitive prim( (CRef)ctxt.GetInputValue(0));
   bool isHair = false;
   if (prim.GetType().IsEqualNoCase(L"hair"))
   {
      Geometry geom = prim.GetGeometry();
      pos = geom.GetPoints().GetPositionArray();
      isHair = true;
   }
   else
   {
      NurbsCurveList curves = prim.GetGeometry();
      pos = curves.GetPoints().GetPositionArray();
   }

   Abc::P3fArraySamplePtr curvePos = sample.getPositions();

   Operator op(ctxt.GetSource());
   updateOperatorInfo( op, sampleInfo, obj.getSchema().getTimeSampling(),
					   pos.GetCount(), (int) curvePos->size());

   if (!isHair)
   {
      if(pos.GetCount() != curvePos->size())
		 return CStatus::OK;
   }
   else
   {
      size_t cacheSize = 14*(curvePos->size()/15);
      if(pos.GetCount() != cacheSize)
         return CStatus::OK;
   }


   size_t alembicSize= curvePos->size();
   size_t xsiSize = pos.GetCount();

   for(size_t i=0, index=0;i<pos.GetCount();i++,index++)
   {
      // guide hairs don't store base point but they are present in the alembic file
      if (isHair && ( i%14==0))
         index++;

      pos[(LONG)i].Set(curvePos->get()[index].x,curvePos->get()[index].y,curvePos->get()[index].z);
   }

   // blend
   if(sampleInfo.alpha != 0.0)
   {
      obj.getSchema().get(sample,sampleInfo.ceilIndex);
      curvePos = sample.getPositions();
      for(size_t i=0, index=0;i<pos.GetCount();i++,index++)
      {
         // guide hairs don't store base point but they are present in the alembic file
         if (isHair && ( i%14==0))
            index++;

         pos[(LONG)i].LinearlyInterpolate(pos[(LONG)i],CVector3(curvePos->get()[index].x,curvePos->get()[index].y,curvePos->get()[index].z),sampleInfo.alpha);
      }
   }

   Primitive(ctxt.GetOutputTarget()).GetGeometry().GetPoints().PutPositionArray(pos);

   return CStatus::OK;
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_crvlist_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END

enum IDs
{
	ID_IN_path = 0,
	ID_IN_identifier = 1,
	ID_IN_renderpath = 2,
	ID_IN_renderidentifier = 3,
	ID_IN_time = 4,
	ID_G_100 = 100,
	ID_OUT_count = 201,
	ID_OUT_vertices = 202,
	ID_OUT_offset = 203,
	ID_OUT_position = 204,
	ID_OUT_velocity = 205,
	ID_OUT_radius = 206,
	ID_OUT_color = 207,
	ID_OUT_uv = 208,
	ID_TYPE_CNS = 400,
	ID_STRUCT_CNS,
	ID_CTXT_CNS,
};

#define ID_UNDEF ((ULONG)-1)

using namespace XSI;
using namespace MATH;

CStatus Register_alembic_curves( PluginRegistrar& in_reg )
{
	ICENodeDef nodeDef;
	nodeDef = Application().GetFactory().CreateICENodeDef(L"alembic_curves",L"alembic_curves");

	CStatus st;
	st = nodeDef.PutColor(255,188,102);
	st.AssertSucceeded( ) ;

   st = nodeDef.PutThreadingModel(XSI::siICENodeSingleThreading);
	st.AssertSucceeded( ) ;

	// Add input ports and groups.
	st = nodeDef.AddPortGroup(ID_G_100);
	st.AssertSucceeded( ) ;

   st = nodeDef.AddInputPort(ID_IN_path,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"path",L"path",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddInputPort(ID_IN_identifier,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"identifier",L"identifier",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddInputPort(ID_IN_renderpath,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"renderpath",L"renderpath",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddInputPort(ID_IN_renderidentifier,ID_G_100,siICENodeDataString,siICENodeStructureSingle,siICENodeContextSingleton,L"renderidentifier",L"renderidentifier",L"",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddInputPort(ID_IN_time,ID_G_100,siICENodeDataFloat,siICENodeStructureSingle,siICENodeContextSingleton,L"time",L"time",0.0f,ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
	
   // Add output ports.
   st = nodeDef.AddOutputPort(ID_OUT_count,siICENodeDataLong,siICENodeStructureSingle,siICENodeContextSingleton,L"count",L"count",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_vertices,siICENodeDataLong,siICENodeStructureArray,siICENodeContextSingleton,L"vertices",L"vertices",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_offset,siICENodeDataLong,siICENodeStructureArray,siICENodeContextSingleton,L"offset",L"offset",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_position,siICENodeDataVector3,siICENodeStructureArray,siICENodeContextSingleton,L"position",L"position",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_velocity,siICENodeDataVector3,siICENodeStructureArray,siICENodeContextSingleton,L"velocity",L"velocity",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_radius,siICENodeDataFloat,siICENodeStructureArray,siICENodeContextSingleton,L"radius",L"radius",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_color,siICENodeDataColor4,siICENodeStructureArray,siICENodeContextSingleton,L"color",L"color",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;
   st = nodeDef.AddOutputPort(ID_OUT_uv,siICENodeDataVector2,siICENodeStructureArray,siICENodeContextSingleton,L"uv",L"uv",ID_UNDEF,ID_UNDEF,ID_UNDEF);
	st.AssertSucceeded( ) ;

	PluginItem nodeItem = in_reg.RegisterICENode(nodeDef);
	nodeItem.PutCategories(L"Alembic");

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_curves_Evaluate(ICENodeContext& in_ctxt)
{
	// The current output port being evaluated...
	ULONG out_portID = in_ctxt.GetEvaluatedOutputPortID( );

	CDataArrayString pathData( in_ctxt, ID_IN_path );
   CString path = pathData[0];
	CDataArrayString identifierData( in_ctxt, ID_IN_identifier );
   CString identifier = identifierData[0];

   // check if we need t addref the archive
   CValue udVal = in_ctxt.GetUserData();
   ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
   if(p == NULL)
   {
      p = new ArchiveInfo;
      p->path = path.GetAsciiString();
      addRefArchive(path);
      CValue val = (CValue::siPtrType) p;
      in_ctxt.PutUserData( val ) ;
   }

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
  AbcG::ICurves obj(iObj,Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

	CDataArrayFloat timeData( in_ctxt, ID_IN_time);
   double time = timeData[0];

   SampleInfo sampleInfo = getSampleInfo(
      time,
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );
  AbcG::ICurvesSchema::Sample sample;
   obj.getSchema().get(sample,sampleInfo.floorIndex);

	switch( out_portID )
	{
      case ID_OUT_count:
		{
			// Get the output port array ...
         CDataArrayLong outData( in_ctxt );
         outData[0] = (LONG)sample.getNumCurves();
   		break;
		}
      case ID_OUT_vertices:
		{
			// Get the output port array ...
         CDataArray2DLong outData( in_ctxt );
         CDataArray2DLong::Accessor acc;

         Abc::Int32ArraySamplePtr ptr = sample.getCurvesNumVertices();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i] = (LONG)ptr->get()[i];
            }
         }
   		break;
		}
      case ID_OUT_offset:
		{
			// Get the output port array ...
         CDataArray2DLong outData( in_ctxt );
         CDataArray2DLong::Accessor acc;

         Abc::Int32ArraySamplePtr ptr = sample.getCurvesNumVertices();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            LONG offset = 0;
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i] = offset;
               offset += (LONG)ptr->get()[i];
            }
         }
   		break;
		}
      case ID_OUT_position:
		{
			// Get the output port array ...
         CDataArray2DVector3f outData( in_ctxt );
         CDataArray2DVector3f::Accessor acc;

         Abc::P3fArraySamplePtr ptr = sample.getPositions();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 1 && ptr->get()[0].x == FLT_MAX)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            bool done = false;

            if(sampleInfo.alpha != 0.0)
            {
               Abc::P3fArraySamplePtr ptr2 = obj.getSchema().getValue(sampleInfo.ceilIndex).getPositions();
               float alpha = (float)sampleInfo.alpha;
               float ialpha = 1.0f - alpha;
               if(ptr2->size() == ptr->size())
               {
                  for(ULONG i=0;i<acc.GetCount();i++)
                  {
                     acc[i].Set(
                        ialpha * ptr->get()[i].x + alpha * ptr2->get()[i].x,
                        ialpha * ptr->get()[i].y + alpha * ptr2->get()[i].y,
                        ialpha * ptr->get()[i].z + alpha * ptr2->get()[i].z);
                  }
                  done = true;
               }
               else
               {
                  Abc::V3fArraySamplePtr vel = sample.getVelocities();
                  if(vel->size() == ptr->size())
                  {
                     float timeAlpha = getTimeOffsetFromSchema( obj.getSchema(), sampleInfo );
                     for(ULONG i=0;i<acc.GetCount();i++)
                     {
                        acc[i].Set(
                           ptr->get()[i].x + timeAlpha * vel->get()[i].x,
                           ptr->get()[i].y + timeAlpha * vel->get()[i].y,
                           ptr->get()[i].z + timeAlpha * vel->get()[i].z);
                     }
                     done = true;
                  }
               }
            }

            if(!done)
            {
               for(ULONG i=0;i<acc.GetCount();i++)
                  acc[i].Set(ptr->get()[i].x,ptr->get()[i].y,ptr->get()[i].z);
            }
         }
   		break;
		}
      case ID_OUT_velocity:
		{
			// Get the output port array ...
         CDataArray2DVector3f outData( in_ctxt );
         CDataArray2DVector3f::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".velocity" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Abc::IV3fArrayProperty prop = Abc::IV3fArrayProperty( obj.getSchema(), ".velocity" );
         if(!prop.valid())
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(prop.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Abc::V3fArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            if(sampleInfo.alpha != 0.0)
            {
               Abc::V3fArraySamplePtr ptr2 = prop.getValue(sampleInfo.ceilIndex);
               float alpha = (float)sampleInfo.alpha;
               float ialpha = 1.0f - alpha;
               for(ULONG i=0;i<acc.GetCount();i++)
               {
                  acc[i].Set(
                     ialpha * ptr->get()[i].x + alpha * ptr2->get()[i].x,
                     ialpha * ptr->get()[i].y + alpha * ptr2->get()[i].y,
                     ialpha * ptr->get()[i].z + alpha * ptr2->get()[i].z);
               }
            }
            else
            {
               for(ULONG i=0;i<acc.GetCount();i++)
                  acc[i].Set(ptr->get()[i].x,ptr->get()[i].y,ptr->get()[i].z);
            }
         }
   		break;
		}
      case ID_OUT_radius:
		{
			// Get the output port array ...
         CDataArray2DFloat outData( in_ctxt );
         CDataArray2DFloat ::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".radius" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Abc::IFloatArrayProperty prop = Abc::IFloatArrayProperty( obj.getSchema(), ".radius" );
         if(!prop.valid())
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(prop.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Abc::FloatArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i] = ptr->get()[i];
            }
         }
   		break;
		}
      case ID_OUT_color:
		{
			// Get the output port array ...
         CDataArray2DColor4f outData( in_ctxt );
         CDataArray2DColor4f::Accessor acc;

         if ( obj.getSchema().getPropertyHeader( ".color" ) == NULL )
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Abc::IC4fArrayProperty prop = Abc::IC4fArrayProperty( obj.getSchema(), ".color" );
         if(!prop.valid())
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(prop.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Abc::C4fArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i].Set(ptr->get()[i].r,ptr->get()[i].g,ptr->get()[i].b,ptr->get()[i].a);
            }
         }
   		break;
		}
      case ID_OUT_uv:
		{
			// Get the output port array ...
         CDataArray2DVector2f outData( in_ctxt );
         CDataArray2DVector2f::Accessor acc;

        AbcG::IV2fGeomParam uvsParam = obj.getSchema().getUVsParam();
         if(!uvsParam)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         if(uvsParam.getNumSamples() == 0)
         {
            acc = outData.Resize(0,0);
            return CStatus::OK;
         }
         Abc::V2fArraySamplePtr ptr = uvsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
         if(ptr == NULL)
            acc = outData.Resize(0,0);
         else if(ptr->size() == 0)
            acc = outData.Resize(0,0);
         else
         {
            acc = outData.Resize(0,(ULONG)ptr->size());
            for(ULONG i=0;i<acc.GetCount();i++)
            {
               acc[i].PutX(ptr->get()[i].x);
               acc[i].PutY(1.0f - ptr->get()[i].y);
            }
         }
   		break;
		}
	};

	return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus alembic_curves_Term(CRef& in_ctxt)
{
   return alembicOp_Term(in_ctxt);
	Context ctxt( in_ctxt );
   CValue udVal = ctxt.GetUserData();
   ArchiveInfo * p = (ArchiveInfo*)(CValue::siPtrType)udVal;
   if(p != NULL)
   {
      delRefArchive(p->path);
      delete(p);
   }
   return CStatus::OK;
}

ESS_CALLBACK_START( alembic_crvlist_topo_Define, CRef& )
   return alembicOp_Define(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_crvlist_topo_DefineLayout, CRef& )
   return alembicOp_DefineLayout(in_ctxt);
ESS_CALLBACK_END

ESS_CALLBACK_START( alembic_crvlist_topo_Update, CRef& )
   OperatorContext ctxt( in_ctxt );

   if((bool)ctxt.GetParameterValue(L"muted"))
      return CStatus::OK;

   CString path = ctxt.GetParameterValue(L"path");
   CString identifier = ctxt.GetParameterValue(L"identifier");

  AbcG::IObject iObj = getObjectFromArchive(path,identifier);
   if(!iObj.valid())
      return CStatus::OK;
  AbcG::ICurves obj(iObj,Abc::kWrapExisting);
   if(!obj.valid())
      return CStatus::OK;

   SampleInfo sampleInfo = getSampleInfo(
      ctxt.GetParameterValue(L"time"),
      obj.getSchema().getTimeSampling(),
      obj.getSchema().getNumSamples()
   );

  AbcG::ICurvesSchema::Sample curveSample;
   obj.getSchema().get(curveSample,sampleInfo.floorIndex);

   // check for valid curve types...!
   if(curveSample.getType() !=AbcG::kLinear &&
      curveSample.getType() !=AbcG::kCubic)
   {
      Application().LogMessage(L"[ExocortexAlembic] Skipping curve '"+identifier+L"', invalid curve type.",siWarningMsg);
      return CStatus::Fail;
   } 

   // prepare the curves 
   Abc::P3fArraySamplePtr curvePos = curveSample.getPositions();
   Abc::Int32ArraySamplePtr curveNbVertices = curveSample.getCurvesNumVertices();

   CVector3Array pos((LONG)curvePos->size());

   CNurbsCurveDataArray curveDatas;
   size_t offset = 0;
  
   for(size_t j=0;j<curveNbVertices->size();j++)
   {
      CNurbsCurveData curveData;
      LONG nbVertices = (LONG)curveNbVertices->get()[j];
	  if( nbVertices == 0 ) {
		  Application().LogMessage(L"[ExocortexAlembic] Softimage does not support 0 size curves in a curve set, ignoring but this may introduce errors on animation." );
		  continue;
	  }

      curveData.m_aControlPoints.Resize(nbVertices);
      curveData.m_aKnots.Resize(nbVertices);
      curveData.m_siParameterization = siUniformParameterization;
      curveData.m_bClosed = curveSample.getWrap() ==AbcG::kPeriodic;
      curveData.m_lDegree = curveSample.getType() ==AbcG::kLinear ? 1 : 3;

      for(LONG k=0;k<nbVertices;k++)
      {
         curveData.m_aControlPoints[k].Set(
            curvePos->get()[offset].x,
            curvePos->get()[offset].y,
            curvePos->get()[offset].z,
            1.0);
         offset++;
      }

      // based on curve type, we do this for linear or closed cubic curves
      if(curveSample.getType() ==AbcG::kLinear || curveData.m_bClosed )
      { 
         curveData.m_aKnots.Resize(nbVertices);
		 for(LONG k=0;k<nbVertices;k++) {
            curveData.m_aKnots[k] = k;
		  }
         if(curveData.m_bClosed)
            curveData.m_aKnots.Add(nbVertices);
      }
      else // cubic open
      {
		 curveData.m_aKnots.Resize(nbVertices+2);
		 curveData.m_aKnots[0] = 0;
		 curveData.m_aKnots[1] = 0;
		 for(LONG k=0;k<nbVertices;k++)
			curveData.m_aKnots[2+k] = k;
		 curveData.m_aKnots[nbVertices+2] = nbVertices-1;
		 curveData.m_aKnots[nbVertices+1] = nbVertices-1;
      }

      curveDatas.Add(curveData);
   }
   
   Operator op(ctxt.GetSource());
   updateOperatorInfo( op, sampleInfo, obj.getSchema().getTimeSampling(),
	                  (int)  offset, (int) offset);

   
   NurbsCurveList curves = Primitive(ctxt.GetOutputTarget()).GetGeometry();
   curves.Set(curveDatas);

   return CStatus::OK;
ESS_CALLBACK_END


ESS_CALLBACK_START( alembic_crvlist_topo_Term, CRef& )
   return alembicOp_Term(in_ctxt);
ESS_CALLBACK_END
