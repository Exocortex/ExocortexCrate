#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include <iparamb2.h>
#include <MeshNormalSpec.h>
#include <Assetmanagement\AssetType.h>
#include "alembic.h"
#include "AlembicXForm.h"
#include "AlembicVisibilityController.h"
#include "AlembicNames.h"
#include "AlembicMeshUtilities.h"
#include <exprlib.h>
#include <ifnpub.h> 
#include <maxscript\maxscript.h>
#include "AlembicMAXScript.h" 

bool isAlembicMeshValid( Alembic::AbcGeom::IObject *pIObj ) {
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
	}

	if(!objMesh.valid() && !objSubD.valid()) {
		return false;
	}
	return true;
}

bool isAlembicMeshNormals( Alembic::AbcGeom::IObject *pIObj, bool& isConstant ) {
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
       if( objMesh.getSchema().getNormalsParam().valid() ) {
			isConstant = objMesh.getSchema().getNormalsParam().isConstant();
			return true;
		}
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
	}

	isConstant = true;
	return false;
}


bool isAlembicMeshPositions( Alembic::AbcGeom::IObject *pIObj, bool& isConstant ) {
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
		isConstant = objMesh.getSchema().getPositionsProperty().isConstant();
		return true;
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
		isConstant = objSubD.getSchema().getPositionsProperty().isConstant();
		return true;
	}
	isConstant = true;
	return false;
}

bool isAlembicMeshUVWs( Alembic::AbcGeom::IObject *pIObj, bool& isConstant ) {
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
		if( objMesh.getSchema().getUVsParam().valid() ) {
			isConstant = objMesh.getSchema().getUVsParam().isConstant();
			return true;
		}
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
		if( objSubD.getSchema().getUVsParam().valid() ) {
			isConstant = objSubD.getSchema().getUVsParam().isConstant();
			return true;
		}
	}
	isConstant = true;

	return false;
}

bool isAlembicMeshTopoDynamic( Alembic::AbcGeom::IObject *pIObj ) {
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
	}

	Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample;
	Alembic::AbcGeom::ISubDSchema::Sample subDSample;

	bool hasDynamicTopo = false;
	if(objMesh.valid())
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objMesh.getSchema(),".faceCounts");
		if(faceCountProp.valid()) {
			hasDynamicTopo = !faceCountProp.isConstant();
		}
	}
	else
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objSubD.getSchema(),".faceCounts");
		if(faceCountProp.valid()) {
			hasDynamicTopo = !faceCountProp.isConstant();
		}
	}  
	return hasDynamicTopo;
}


void AlembicImport_FillInPolyMesh_Internal(alembic_fillmesh_options &options);

void AlembicImport_FillInPolyMesh(alembic_fillmesh_options &options)
{ 
	ESS_STRUCTURED_EXCEPTION_REPORTING_START
		AlembicImport_FillInPolyMesh_Internal( options );
	ESS_STRUCTURED_EXCEPTION_REPORTING_END
}

void validateMeshes( alembic_fillmesh_options &options, char* szName ) {
	if (options.pMNMesh != NULL)
	{
		//ESS_LOG_WARNING( "options.pMNMesh->MNDebugPrint() ------------------------------------------------------------ section: " << szName );
		//options.pMNMesh->MNDebugPrint( FALSE );
		//if( ! options.pMNMesh->CheckAllData() ) {
		//	ESS_LOG_WARNING( "options.pMNMesh->CheckAllData() failed, section: " << szName );
		//	options.pMNMesh->MNDebugPrint();
		//}
	}
}

void AlembicImport_FillInPolyMesh_Internal(alembic_fillmesh_options &options)
{
   float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

   Alembic::AbcGeom::IPolyMesh objMesh;
   Alembic::AbcGeom::ISubD objSubD;

   if(Alembic::AbcGeom::IPolyMesh::matches((*options.pIObj).getMetaData()))
       objMesh = Alembic::AbcGeom::IPolyMesh(*options.pIObj,Alembic::Abc::kWrapExisting);
   else
       objSubD = Alembic::AbcGeom::ISubD(*options.pIObj,Alembic::Abc::kWrapExisting);

   if(!objMesh.valid() && !objSubD.valid())
       return;

  double sampleTime = GetSecondsFromTimeValue(options.dTicks);

  SampleInfo sampleInfo;
   if(objMesh.valid())
      sampleInfo = getSampleInfo(
         sampleTime,
         objMesh.getSchema().getTimeSampling(),
         objMesh.getSchema().getNumSamples()
      );
   else
      sampleInfo = getSampleInfo(
         sampleTime,
         objSubD.getSchema().getTimeSampling(),
         objSubD.getSchema().getNumSamples()
      );

   Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample;
   Alembic::AbcGeom::ISubDSchema::Sample subDSample;

   if(objMesh.valid())
       objMesh.getSchema().get(polyMeshSample,sampleInfo.floorIndex);
   else
       objSubD.getSchema().get(subDSample,sampleInfo.floorIndex);

   int currentNumVerts = (options.pMNMesh != NULL) ? options.pMNMesh->numv :
	   (options.pMesh != NULL) ? options.pMesh->getNumVerts() : 0;

  	   Alembic::Abc::P3fArraySamplePtr meshPos;
       Alembic::Abc::V3fArraySamplePtr meshVel;

	   bool hasDynamicTopo = false;
       if(objMesh.valid())
       {
           meshPos = polyMeshSample.getPositions();
           meshVel = polyMeshSample.getVelocities();

           Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objMesh.getSchema(),".faceCounts");
           if(faceCountProp.valid())
               hasDynamicTopo = !faceCountProp.isConstant();
       }
       else
       {
           meshPos = subDSample.getPositions();
           meshVel = subDSample.getVelocities();

           Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objSubD.getSchema(),".faceCounts");
           if(faceCountProp.valid())
               hasDynamicTopo = !faceCountProp.isConstant();
       }   


   if(  ( options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST ) ||
	   ( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX ) ) {
		   if (currentNumVerts != meshPos->size())
		   {
			   int numVerts = static_cast<int>(meshPos->size());
			   if (options.pMNMesh != NULL)
			   {
				   options.pMNMesh->setNumVerts(numVerts);
					MNVert* pMeshVerties = options.pMNMesh->V(0);
				   for(int i=0;i<numVerts;i++)
				   {
					   pMeshVerties[i].p = Point3(0,0,0);
				   }
			   }
			   if (options.pMesh != NULL)
			   {
				   options.pMesh->setNumVerts(numVerts);
				   Point3 *pVerts = options.pMesh->verts;
				   for(int i=0;i<numVerts;i++)
				   {
					   pVerts[i] = Point3(0,0,0);
				   }
			   }
		   } 

		validateMeshes( options, "ALEMBIC_DATAFILL_FACELIST | ALEMBIC_DATAFILL_VERTEX" );
   }

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX )
   {
	   Imath::V3f const* pPositionArray = ( meshPos.get() != NULL ) ? meshPos->get() : NULL;
	   Imath::V3f const* pVelocityArray = ( meshVel.get() != NULL ) ? meshVel->get() : NULL;

	  assert( pPositionArray != NULL );
	
	   std::vector<Imath::V3f> vArray;
	   vArray.reserve(meshPos->size());
	   //P3fArraySample* pPositionArray = meshPos->get();
	   for(size_t i=0;i<meshPos->size();i++) {
		  vArray.push_back(pPositionArray[i]);
	   }

	   // blend - either between samples or using point velocities
	   if(sampleInfo.alpha != 0.0)
	   {
           bool bSampleInterpolate = false;
           bool bVelInterpolate = false;

		  if(objMesh.valid())
		  {
              Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample2;
              objMesh.getSchema().get(polyMeshSample2,sampleInfo.ceilIndex);
              meshPos = polyMeshSample2.getPositions();
             
              if(meshPos->size() == vArray.size() && !hasDynamicTopo)
                  bSampleInterpolate = true;
              else if(meshVel && meshVel->size() == meshPos->size())
                  bVelInterpolate = true;
          }
		  else
		  {
              Alembic::AbcGeom::ISubDSchema::Sample subDSample2;
              objSubD.getSchema().get(subDSample2,sampleInfo.ceilIndex);
              meshPos = subDSample2.getPositions();
              bSampleInterpolate = true;
		  }

		  float sampleInfoAlpha = (float)sampleInfo.alpha; 
          if (bSampleInterpolate)
          {
			  pPositionArray = meshPos->get();

              for(size_t i=0;i<meshPos->size();i++)
              {	
                  vArray[i] += (pPositionArray[i] - vArray[i]) * sampleInfoAlpha; 
              }
          }
          else if (bVelInterpolate)
          {
			  assert( pVelocityArray != NULL );
			  pVelocityArray = meshVel->get();

              for(size_t i=0;i<meshVel->size();i++)
              {
                  vArray[i] += pVelocityArray[i] * sampleInfoAlpha;                  
              }
          }
	   }

	   if( options.fVertexAlpha != 1.0f ) {
		   for( int i = 0; i < vArray.size(); i ++ ) {
			   vArray[i] *= options.fVertexAlpha;
		   }
	   }
	 
	   if (options.pMNMesh != NULL)
	   {
		   MNVert* pMeshVerties = options.pMNMesh->V(0);
		 
		   for(int i=0;i<vArray.size();i++)
		   {
			   pMeshVerties[i].p += ConvertAlembicPointToMaxPoint(vArray[i], masterScaleUnitMeters );
		   }
	   }
	   else if (options.pMesh != NULL)
	   {
		   Point3 *pVerts = options.pMesh->verts;
		   for(int i=0;i<vArray.size();i++)
		   {
			   pVerts[i] += ConvertAlembicPointToMaxPoint(vArray[i], masterScaleUnitMeters );
		   }
	   }
	   else {
		   ESS_LOG_WARNING("Should not get here." );
	   }
		validateMeshes( options, "ALEMBIC_DATAFILL_VERTEX" );
    }

  

   Alembic::Abc::Int32ArraySamplePtr meshFaceCount;
   Alembic::Abc::Int32ArraySamplePtr meshFaceIndices;

   if (objMesh.valid())
   {
       meshFaceCount = polyMeshSample.getFaceCounts();
       meshFaceIndices = polyMeshSample.getFaceIndices();
   }
   else
   {
       meshFaceCount = subDSample.getFaceCounts();
       meshFaceIndices = subDSample.getFaceIndices();
   }

   boost::int32_t const* pMeshFaceCount = ( meshFaceCount.get() != NULL ) ? meshFaceCount->get() : NULL;
   boost::int32_t const* pMeshFaceIndices = ( meshFaceIndices.get() != NULL ) ? meshFaceIndices->get() : NULL;

   int numFaces = static_cast<int>(meshFaceCount->size());

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST )
   {
        // Set up the index buffer
        int offset = 0;
		if (options.pMNMesh != NULL)
		{
			bool onlyTriangles = true;

		    options.pMNMesh->setNumFaces(numFaces);
			MNFace *pFaces = options.pMNMesh->F(0);
			for (int i = 0; i < numFaces; ++i)
			{
				int degree = pMeshFaceCount[i];

				if( degree > 3 ) {
					onlyTriangles = false;
				}
				MNFace *pFace = &(pFaces[i]);
				pFace->SetDeg(degree);
				pFace->material = 1;
				
				for (int j = degree - 1; j >= 0; --j)
				{
					pFace->vtx[j] = pMeshFaceIndices[offset];
					pFace->edg[j] = -1;
					++offset;
				}
			}
			if( ! onlyTriangles ) {
				options.pMNMesh->SetFlag( MN_MESH_NONTRI, TRUE );
			}
			else {
				options.pMNMesh->SetFlag( MN_MESH_NONTRI, FALSE );
			}
			options.pMNMesh->SetFlag( MN_MESH_FILLED_IN, FALSE );
		    // this can fail if the mesh isn't correctly filled in.
			if( ! options.pMNMesh->GetFlag( MN_MESH_FILLED_IN ) ) {
				options.pMNMesh->FillInMesh();
			}
		}
		else if (options.pMesh != NULL )
		{
		    options.pMesh->setNumFaces(numFaces);
			Face *pFaces = options.pMesh->faces;
			for (int i = 0; i < numFaces; ++i)
			{
				int degree = pMeshFaceCount[i];
				if ( degree == 3)
				{
					// three vertex indices of a triangle
					int v2 = pMeshFaceIndices[offset];
					int v1 = pMeshFaceIndices[offset+1];
					int v0 = pMeshFaceIndices[offset+2];

					offset += 3;

					// vertex positions
					Face *pFace = &(pFaces[i]);
					pFace->setMatID(1);
					pFace->setEdgeVisFlags(1, 1, 1);
					pFace->setVerts(v0, v1, v2);
				}
			}
		}
		else {
			ESS_LOG_WARNING("Should not get here." );
		}
		validateMeshes( options, "ALEMBIC_DATAFILL_FACELIST" );
   }


   if ( objMesh.valid() && ( options.nDataFillFlags & ALEMBIC_DATAFILL_NORMALS ) )
   {
       Alembic::AbcGeom::IN3fGeomParam meshNormalsParam = objMesh.getSchema().getNormalsParam();

       if(meshNormalsParam.valid())
       {
		   Alembic::Abc::N3fArraySamplePtr meshNormalsFloor = meshNormalsParam.getExpandedValue(sampleInfo.floorIndex).getVals();
           std::vector<Point3> normalsToSet;
           normalsToSet.reserve(meshNormalsFloor->size());
           Alembic::Abc::N3fArraySamplePtr meshNormalsCeil = meshNormalsParam.getExpandedValue(sampleInfo.ceilIndex).getVals();

           Imath::V3f const* pMeshNormalsFloor = ( meshNormalsFloor.get() != NULL ) ? meshNormalsFloor->get() : NULL;
		   Imath::V3f const* pMeshNormalsCeil = ( meshNormalsCeil.get() != NULL ) ? meshNormalsCeil->get() : NULL;

		   assert( pMeshNormalsFloor != NULL );
		   assert( pMeshNormalsCeil != NULL );

           // Blend 
           if (sampleInfo.alpha != 0.0f && meshNormalsFloor->size() == meshNormalsCeil->size())
           {
               int offset = 0;
               for (int i = 0; i < numFaces; i += 1)
               {
                   int degree = pMeshFaceCount[i];
                   int first = offset + degree - 1;
                   int last = offset;
                   for (int j = first; j >= last; j -= 1)
                   {
					   Imath::V3f interpolatedNormal = pMeshNormalsFloor[j] + (pMeshNormalsCeil[j] - pMeshNormalsFloor[j]) * float(sampleInfo.alpha);
                       normalsToSet.push_back( ConvertAlembicNormalToMaxNormal_Normalized( interpolatedNormal ) );
                   }

                   offset += degree;
               }
           }
           else
           {
               int offset = 0;
               for (int i = 0; i < numFaces; i += 1)
               {
                   int degree = pMeshFaceCount[i];
                   int first = offset + degree - 1;
                   int last = offset;
                   for (int j = first; j >= last; j -=1)
                   {
                       if (j > meshNormalsFloor->size())
                       {
                           ESS_LOG_ERROR("Normal at Face " << i << ", Vertex " << j << "does not exist at sample time " << sampleTime);
                           normalsToSet.push_back( Point3(0,0,0) );
                           continue;
                       }
                                            
                       normalsToSet.push_back( ConvertAlembicNormalToMaxNormal( pMeshNormalsFloor[j] ) );
                   }

                   offset += degree;
               }
           }

		   if( options.fVertexAlpha != 1.0f ) 
           {
			   for( int i = 0; i < normalsToSet.size(); i ++) 
               {
				normalsToSet[i] *= options.fVertexAlpha;
			   }
		   }

           // Set up the specify normals
           if (options.pMNMesh != NULL)
           {
               options.pMNMesh->SpecifyNormals();
               MNNormalSpec *normalSpec = options.pMNMesh->GetSpecifiedNormals();
               normalSpec->ClearNormals();
               normalSpec->SetNumFaces(numFaces);
               normalSpec->SetFlag(MESH_NORMAL_MODIFIER_SUPPORT, true);
               normalSpec->SetNumNormals((int)normalsToSet.size());

               for (int i = 0; i < normalsToSet.size(); i += 1)
               {
                   normalSpec->Normal(i) = normalsToSet[i];
                   normalSpec->SetNormalExplicit(i, true);
               }

               // Set up the normal faces
               int offset = 0;
               for (int i =0; i < numFaces; i += 1)
               {
                   int degree = pMeshFaceCount[i];

                   MNNormalFace &normalFace = normalSpec->Face(i);
                   normalFace.SetDegree(degree);
                   normalFace.SpecifyAll();

                   for (int j = 0; j < degree; ++j)
                   {
                       normalFace.SetNormalID(j, offset);
                       ++offset;
                   }
               }

               // Fill in any normals we may have not gotten specified.  Also allocates space for the RVert array
               // which we need for doing any normal vector queries
               normalSpec->CheckNormals();
               options.pMNMesh->checkNormals(TRUE);
           }

           if (options.pMesh != NULL)
           {
               options.pMesh->SpecifyNormals();
               MeshNormalSpec *normalSpec = options.pMesh->GetSpecifiedNormals();
               normalSpec->ClearNormals();
               normalSpec->SetNumFaces(numFaces);
               normalSpec->SetFlag(MESH_NORMAL_MODIFIER_SUPPORT, true);
               normalSpec->SetNumNormals((int)normalsToSet.size());

               for (int i = 0; i < normalsToSet.size(); i += 1)
               {
                   normalSpec->Normal(i) = normalsToSet[i];
                   normalSpec->SetNormalExplicit(i, true);
               }

               // Set up the normal faces
               for (int i =0; i < numFaces; i += 1)
               {
                   MeshNormalFace &normalFace = normalSpec->Face(i);
                   normalFace.SpecifyAll();
                   normalFace.SetNormalID(0, i*3);
                   normalFace.SetNormalID(1, i*3+1);
                   normalFace.SetNormalID(2, i*3+2);
               }

               // Fill in any normals we may have not gotten specified.  Also allocates space for the RVert array
               // which we need for doing any normal vector queries
               normalSpec->CheckNormals();
               options.pMesh->checkNormals(TRUE);
           }
       }
    validateMeshes( options, "ALEMBIC_DATAFILL_NORMALS" );
  }



   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_UVS )
   {
       Alembic::AbcGeom::IV2fGeomParam meshUvParam;
       if(objMesh.valid())
           meshUvParam = objMesh.getSchema().getUVsParam();
       else
           meshUvParam = objSubD.getSchema().getUVsParam();

       if(meshUvParam.valid())
       {
           SampleInfo sampleInfo = getSampleInfo(
               sampleTime,
               meshUvParam.getTimeSampling(),
               meshUvParam.getNumSamples()
               );

           Alembic::Abc::V2fArraySamplePtr meshUVsFloor = meshUvParam.getExpandedValue(sampleInfo.floorIndex).getVals();
           Alembic::Abc::V2fArraySamplePtr meshUVsCeil = meshUvParam.getExpandedValue(sampleInfo.ceilIndex).getVals();
           std::vector<Point3> uvsToSet;
           uvsToSet.reserve(meshUVsFloor->size());

           // Blend
           if (sampleInfo.alpha != 0.0f && meshUVsFloor->size() == meshUVsCeil->size())
           {
               int offset = 0;
               for (int i = 0; i < numFaces; i += 1)
               {
                   int degree = meshFaceCount->get()[i];
                   int first = offset + degree - 1;
                   int last = offset;
                   for (int j = first; j >= last; j -= 1)
                   {
                       Point3 floorUV(meshUVsFloor->get()[j].x, meshUVsFloor->get()[j].y, 0.0f);
                       Point3 ceilUV(meshUVsCeil->get()[j].x, meshUVsCeil->get()[j].y, 0.0f);
                       Point3 delta = (ceilUV - floorUV) * float(sampleInfo.alpha);
                       Point3 maxUV = floorUV + delta;
                       uvsToSet.push_back(maxUV);
                   }

                   offset += degree;
               }
           }
           else
           {
               int offset = 0;
               for (int i = 0; i < numFaces; i += 1)
               {
                   int degree = meshFaceCount->get()[i];
                   int first = offset + degree - 1;
                   int last = offset;
                   for (int j = first; j >= last; j -=1)
                   {
                       Point3 maxUV(meshUVsFloor->get()[j].x, meshUVsFloor->get()[j].y, 0.0f);
                       uvsToSet.push_back(maxUV);
                   }

                   offset += degree;
               }
           }


           // Set up the default texture map channel
           if (options.pMNMesh != NULL)
           {
               int numVertices = static_cast<int>(uvsToSet.size());

               options.pMNMesh->SetMapNum(2);
               options.pMNMesh->InitMap(0);
               options.pMNMesh->InitMap(1);
               MNMap *map = options.pMNMesh->M(1);
               map->setNumVerts(numVertices);
               map->setNumFaces(numFaces);

			   Point3* mapV = map->v;
               for (int i = 0; i < numVertices; i += 1)
               {
                   mapV[i] = uvsToSet[i];
               }

               int offset = 0;
               MNMapFace* mapF = map->f;
               for (int i =0; i < numFaces; i += 1)
               {
                   int degree;
                   if (i < options.pMNMesh->numf)
                       degree = options.pMNMesh->F(i)->deg;
                   else
                       degree = 0;

                   mapF[i].SetSize(degree);

                   for (int j = 0; j < degree; j ++)
                   {
                       mapF[i].tv[j] = offset;
                       ++offset;
                   }
               }
           }
           
           if (options.pMesh != NULL)
           {
               options.pMesh->setNumMaps(2);
               options.pMesh->setMapSupport(1, TRUE);
               MeshMap &map = options.pMesh->Map(1);
               map.setNumVerts((int)uvsToSet.size());
               map.setNumFaces(numFaces);

               // Set the map texture vertices
			   for (int i = 0; i < uvsToSet.size(); i += 1) {
                    map.tv[i] = uvsToSet[i];
			   }

               // Set up the map texture faces
			   for (int i =0; i < numFaces; i += 1) {
                   map.tf[i].setTVerts(i*3, i*3+1, i*3+2);					
			   }
           }
       }
	   validateMeshes( options, "ALEMBIC_DATAFILL_UVS" );
   }

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_MATERIALIDS )
   {
       Alembic::Abc::IUInt32ArrayProperty materialIds;
       if(objMesh.valid() && objMesh.getSchema().getPropertyHeader( ".materialids" )) 
           materialIds = Alembic::Abc::IUInt32ArrayProperty(objMesh.getSchema(), ".materialids");
       else if (objSubD.valid() && objSubD.getSchema().getPropertyHeader( ".materialids" ))
           materialIds = Alembic::Abc::IUInt32ArrayProperty(objSubD.getSchema(), ".materialids");

       // If we don't detect a material id property then we try the facesets.  The order in the face set array is assumed
       // to be the material id
       if (materialIds.valid() && materialIds.getNumSamples() > 0)
       {
            Alembic::Abc::UInt32ArraySamplePtr materialIdsPtr = materialIds.getValue(sampleInfo.floorIndex);

            if (materialIdsPtr->size() == numFaces)
            {
                for (int i = 0; i < materialIdsPtr->size(); i += 1)
                {
                    int nMatId = materialIdsPtr->get()[i];
                    if (options.pMNMesh != NULL && i < options.pMNMesh->numf)
                    {
                        options.pMNMesh->f[i].material = nMatId;
                    }
                    else if (options.pMesh != NULL && i < options.pMesh->getNumFaces())
                    {
                        options.pMesh->faces[i].setMatID(nMatId);
                    }
                }
            }
       }
       else
       {
           std::vector<std::string> faceSetNames;
           if(objMesh.valid())
               objMesh.getSchema().getFaceSetNames(faceSetNames);
           else
               objSubD.getSchema().getFaceSetNames(faceSetNames);

           for(size_t j=0;j<faceSetNames.size();j++)
           {
               Alembic::AbcGeom::IFaceSetSchema faceSet;
               if(objMesh.valid())
                   faceSet = objMesh.getSchema().getFaceSet(faceSetNames[j]).getSchema();
               else
                   faceSet = objSubD.getSchema().getFaceSet(faceSetNames[j]).getSchema();

               Alembic::AbcGeom::IFaceSetSchema::Sample faceSetSample = faceSet.getValue();
               Alembic::Abc::Int32ArraySamplePtr faces = faceSetSample.getFaces();

               int nMatId = (int) j;
               for(size_t k=0;k<faces->size();k++)
               {
                   int faceId = faces->get()[k];
                   if (options.pMNMesh != NULL && faceId < options.pMNMesh->numf)
                   {
                       options.pMNMesh->f[faceId].material = nMatId;
                   }
                   else if (options.pMesh != NULL && faceId < options.pMesh->getNumFaces())
                   {
                       options.pMesh->faces[faceId].setMatID(nMatId);
                   }
               }
           }
       }
	   validateMeshes( options, "ALEMBIC_DATAFILL_MATERIALIDS" );
   }
 
  // This isn't required if we notify 3DS Max properly via the channel flags for vertex changes.
  if (options.pMNMesh != NULL)
   {
	   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST ) {
		   options.pMNMesh->InvalidateTopoCache();
	 		options.pMNMesh->InvalidateGeomCache();
		}
	   else {
		  if( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX ) {
			options.pMNMesh->InvalidateGeomCache();
		  }
	   }
   }

   if (options.pMesh != NULL)
   {	
	   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST ) {
		   options.pMesh->InvalidateTopologyCache();
	 	   options.pMesh->InvalidateGeomCache();
	  }
	   else {
			if( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX ) {
			   options.pMesh->InvalidateGeomCache();
		  }
	   }
   }
}

bool AlembicImport_IsPolyObject(Alembic::AbcGeom::IPolyMeshSchema::Sample &polyMeshSample)
{
    Alembic::Abc::Int32ArraySamplePtr meshFaceCount = polyMeshSample.getFaceCounts();

    // Go through each face and check the number of vertices.  If a face does not have 3 vertices,
    // we consider it to be a polymesh, otherwise it is a triangle mesh
    for (size_t i = 0; i < meshFaceCount->size(); ++i)
    {
        int vertexCount = meshFaceCount->get()[i];
        if (vertexCount != 3)
        {
            return true;
        }
    }

    return false;
}

int AlembicImport_PolyMesh(const std::string &path, const std::string &identifier, alembic_importoptions &options)
{
	// Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(path,identifier);
	if(!iObj.valid())
	{
		return alembic_failure;
	}

	// Fill in the mesh
    alembic_fillmesh_options dataFillOptions;
    dataFillOptions.pIObj = &iObj;
    dataFillOptions.pMesh = NULL;
    dataFillOptions.pMNMesh = NULL;
    dataFillOptions.dTicks = GET_MAX_INTERFACE()->GetTime();

    dataFillOptions.nDataFillFlags = ALEMBIC_DATAFILL_VERTEX|ALEMBIC_DATAFILL_FACELIST;
    dataFillOptions.nDataFillFlags |= options.importNormals ? ALEMBIC_DATAFILL_NORMALS : 0;
    dataFillOptions.nDataFillFlags |= options.importUVs ? ALEMBIC_DATAFILL_UVS : 0;
    dataFillOptions.nDataFillFlags |= options.importBboxes ? ALEMBIC_DATAFILL_BOUNDINGBOX : 0;
    dataFillOptions.nDataFillFlags |= options.importMaterialIds ? ALEMBIC_DATAFILL_MATERIALIDS : 0;
    
    // Create the poly or tri object and place it in the scene
    // Need to use the attach to existing import flag here 
    if (!Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()))
    {
        return alembic_failure;
    }

    Alembic::AbcGeom::IPolyMesh objMesh = Alembic::AbcGeom::IPolyMesh(iObj, Alembic::Abc::kWrapExisting);
    if (!objMesh.valid())
    {
        return alembic_failure;
    }

    // TODO: Do we need to check more than the first sample?
    Alembic::AbcGeom::IPolyMeshSchema::Sample polyMeshSample;
    objMesh.getSchema().get(polyMeshSample, 0);

    Object *newObject = NULL;
    if (AlembicImport_IsPolyObject(polyMeshSample))
    {
		
		PolyObject *pPolyObject = (PolyObject *) GetPolyObjDescriptor()->Create();
		dataFillOptions.pMNMesh = &(pPolyObject->GetMesh());
	    newObject = pPolyObject;
    }
    else
    {
        TriObject *pTriObj = (TriObject *) GetTriObjDescriptor()->Create();
	    dataFillOptions.pMesh = &( pTriObj->GetMesh() );
	    newObject = pTriObj;
    }

    if (newObject == NULL)
    {
        return alembic_failure;
    }

	// we will not be filling in the initial polymesh data.
	//AlembicImport_FillInPolyMesh(dataFillOptions);

	// Create the object pNode
	INode *pNode = GET_MAX_INTERFACE()->CreateObjectNode(newObject, iObj.getName().c_str());
	if (pNode == NULL)
    {
		return alembic_failure;
    }

	//TimeValue now =  GET_MAX_INTERFACE()->GetTime();

	TimeValue zero( 0 );

	std::vector<Modifier*> modifiersToEnable;

	bool isDynamicTopo = isAlembicMeshTopoDynamic( &iObj );

	//ESS_LOG_INFO( "Node: " << pNode->GetName() );
	//ESS_LOG_INFO( "isDynamicTopo: " << isDynamicTopo );
	{
		// Create the polymesh modifier
		Modifier *pModifier = static_cast<Modifier*>
			(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_MESH_TOPO_MODIFIER_CLASSID));

		pModifier->DisableMod();

		// Set the alembic id
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, path.c_str());
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, identifier.c_str() );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
		if( isDynamicTopo ) {
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geometry" ), zero, TRUE );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "normals" ), zero, ( options.importNormals ? TRUE : FALSE ) );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "uvs" ), zero, ( options.importMaterialIds ? TRUE : FALSE ) );
		}
		else {
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geometry" ), zero, FALSE );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "normals" ), zero, FALSE );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "uvs" ), zero, FALSE );
		}

		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
	
		// Add the modifier to the pNode
		GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);

		if( isDynamicTopo ) {
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$'%s'.modifiers[#Alembic_Mesh_Topology].time", pNode->GetName() );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}

		modifiersToEnable.push_back( pModifier );
	}
	bool isUVWContant = true;
	if( ( ! isDynamicTopo ) && isAlembicMeshUVWs( &iObj, isUVWContant ) ) {
		//ESS_LOG_INFO( "isUVWContant: " << isUVWContant );
	// Create the polymesh modifier
		Modifier *pModifier = static_cast<Modifier*>
			(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_MESH_UVW_MODIFIER_CLASSID));

		pModifier->DisableMod();

		// Set the alembic id
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, path.c_str());
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, identifier.c_str() );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );

	
		// Add the modifier to the pNode
		GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);
		if( ! isUVWContant ) {
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$'%s'.modifiers[#Alembic_Mesh_UVW].time", pNode->GetName() );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}

		if( options.importUVs ) {
			modifiersToEnable.push_back( pModifier );
		}
	}
	bool isGeomContant = true;
	if( ( ! isDynamicTopo ) && isAlembicMeshPositions( &iObj, isGeomContant ) ) {
		//ESS_LOG_INFO( "isGeomContant: " << isGeomContant );
		// Create the polymesh modifier
		Modifier *pModifier = static_cast<Modifier*>
			(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_MESH_GEOM_MODIFIER_CLASSID));

		pModifier->DisableMod();

		// Set the alembic id
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, path.c_str());
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, identifier.c_str() );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geoAlpha" ), zero, 1.0f );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
	
		// Add the modifier to the pNode
		GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);

		if( ! isGeomContant ) {
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$'%s'.modifiers[#Alembic_Mesh_Geometry].time", pNode->GetName() );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}

		modifiersToEnable.push_back( pModifier );
	}
	bool isNormalsContant = true;
	if( ( ! isDynamicTopo ) && isAlembicMeshNormals( &iObj, isNormalsContant ) ) {
		//ESS_LOG_INFO( "isNormalsContant: " << isNormalsContant );
		// Create the polymesh modifier
		Modifier *pModifier = static_cast<Modifier*>
			(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_MESH_NORMALS_MODIFIER_CLASSID));

		pModifier->DisableMod();

		// Set the alembic id
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, path.c_str());
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, identifier.c_str() );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
	
		// Add the modifier to the pNode
		GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);

		if( ! isNormalsContant ) {
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$'%s'.modifiers[#Alembic_Mesh_Normals].time", pNode->GetName() );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}
	
		if( options.importNormals ) {
			modifiersToEnable.push_back( pModifier );
		}
	}

    // Add the new inode to our current scene list
    SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, newObject, OBTYPE_MESH, &std::string(iObj.getFullName())); 
    options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl( path.c_str(), identifier.c_str(), iObj, pNode, options);

	for( int i = 0; i < modifiersToEnable.size(); i ++ ) {
		modifiersToEnable[i]->EnableMod();
	}

	return 0;
}
