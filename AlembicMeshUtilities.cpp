#include "stdafx.h"
#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicArchiveStorage.h"
#include "utility.h"
#include "AlembicXForm.h"
#include "AlembicVisibilityController.h"
#include "AlembicNames.h"
#include "AlembicMeshUtilities.h"
#include "AlembicMAXScript.h" 
#include "AlembicMetadataUtils.h"

#include "CommonProfiler.h"
#include "CommonMeshUtilities.h"

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
   ESS_PROFILE_FUNC();
   AbcG::IPolyMesh objMesh;
   AbcG::ISubD objSubD;

   if(AbcG::IPolyMesh::matches((*options.pIObj).getMetaData()))
       objMesh = AbcG::IPolyMesh(*options.pIObj,Abc::kWrapExisting);
   else
       objSubD = AbcG::ISubD(*options.pIObj,Abc::kWrapExisting);

   if(!objMesh.valid() && !objSubD.valid())
       return;


   int nTicks = options.dTicks;
   float fRoundedTimeAlpha = 0.0f;
   if(options.nDataFillFlags & ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES){
      RoundTicksToNearestFrame(nTicks, fRoundedTimeAlpha);
   }
  double sampleTime = GetSecondsFromTimeValue(nTicks);

  SampleInfo sampleInfo;
  if(objMesh.valid()) {
		ESS_PROFILE_SCOPE("getSampleInfo");
      sampleInfo = getSampleInfo(
         sampleTime,
         objMesh.getSchema().getTimeSampling(),
         objMesh.getSchema().getNumSamples()
      );
  }
   else
      sampleInfo = getSampleInfo(
         sampleTime,
         objSubD.getSchema().getTimeSampling(),
         objSubD.getSchema().getNumSamples()
      );

   AbcG::IPolyMeshSchema::Sample polyMeshSample;
   AbcG::ISubDSchema::Sample subDSample;

   if(objMesh.valid())
       objMesh.getSchema().get(polyMeshSample,sampleInfo.floorIndex);
   else
       objSubD.getSchema().get(subDSample,sampleInfo.floorIndex);

   int currentNumVerts = options.pMNMesh->numv;

  	   Abc::P3fArraySamplePtr meshPos;
       Abc::V3fArraySamplePtr meshVel;

       bool hasDynamicTopo = options.pObjectCache->isMeshTopoDynamic;//isAlembicMeshTopoDynamic( options.pIObj );
       if(objMesh.valid())
       {
     		ESS_PROFILE_SCOPE("Mesh getPositions/getVelocities/faceCountProp");
          meshPos = polyMeshSample.getPositions();
           meshVel = polyMeshSample.getVelocities();
       }
       else
       {
     		ESS_PROFILE_SCOPE("SubD getPositions/getVelocities/faceCountProp");
           meshPos = subDSample.getPositions();
           meshVel = subDSample.getVelocities();
       }

	//MH: What is this code for? //related to vertex blending
	//note that the fillInMesh call will crash if the points are not initilaized (tested max 2013)
   if(  ( options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST ) ||
	   ( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX ) ) {
		   if (currentNumVerts != meshPos->size() && ! options.pMNMesh->GetFlag( MN_MESH_RATSNEST ) )
 		   {
       		ESS_PROFILE_SCOPE("resize and clear vertices");
			   int numVerts = static_cast<int>(meshPos->size());
			   
			   options.pMNMesh->setNumVerts(numVerts);
				MNVert* pMeshVerties = options.pMNMesh->V(0);
			   for(int i=0;i<numVerts;i++)
			   {
				   pMeshVerties[i].p = Point3(0,0,0);
			   }
		   }

		validateMeshes( options, "ALEMBIC_DATAFILL_FACELIST | ALEMBIC_DATAFILL_VERTEX" );
   }

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX )
   {
       		ESS_PROFILE_SCOPE("ALEMBIC_DATAFILL_VERTEX");
	   Abc::V3f const* pPositionArray = ( meshPos.get() != NULL ) ? meshPos->get() : NULL;
	   Abc::V3f const* pVelocityArray = ( meshVel.get() != NULL ) ? meshVel->get() : NULL;

	   if( pPositionArray ){
	
		   std::vector<Abc::V3f> vArray;
		   vArray.reserve(meshPos->size());
		   //P3fArraySample* pPositionArray = meshPos->get();
		   for(size_t i=0;i<meshPos->size();i++) {
			  vArray.push_back(pPositionArray[i]);
		   }

		   // blend - either between samples or using point velocities
		   if( ((options.nDataFillFlags & ~ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES) && sampleInfo.alpha != 0.0f ) || 
			   ((options.nDataFillFlags & ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES) && fRoundedTimeAlpha != 0.0f ) )
		   {
			   bool bSampleInterpolate = false;
			   bool bVelInterpolate = false;

			  if(objMesh.valid())
			  {
				  AbcG::IPolyMeshSchema::Sample polyMeshSample2;
				  objMesh.getSchema().get(polyMeshSample2,sampleInfo.ceilIndex);
				  meshPos = polyMeshSample2.getPositions();

				  const int posSize = meshPos ? (const int)meshPos->size() : 0;
				  const int velSize = meshVel ? (const int)meshVel->size() : 0;
	             
				  if(meshPos->size() == vArray.size() && !hasDynamicTopo)
					  bSampleInterpolate = true;
				  else if(meshVel && meshVel->size() == vArray.size())
					  bVelInterpolate = true;
			  }
			  else
			  {
				  AbcG::ISubDSchema::Sample subDSample2;
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

				  float timeAlpha;
				  if(options.nDataFillFlags & ALEMBIC_DATAFILL_IGNORE_SUBFRAME_SAMPLES){
				      timeAlpha = fRoundedTimeAlpha;	
				  }
				  else {
					  timeAlpha = getTimeOffsetFromObject( *options.pIObj, sampleInfo );
				  }
				  for(size_t i=0;i<meshVel->size();i++)
				  {
					  vArray[i] += pVelocityArray[i] * timeAlpha;                  
				  }
			  }
		   }

		   if( options.fVertexAlpha != 1.0f ) {
			   for( int i = 0; i < vArray.size(); i ++ ) {
				   vArray[i] *= options.fVertexAlpha;
			   }
		   }
		 
		
		  // MNVert* pMeshVerties = options.pMNMesh->V(0);
		   ;
		   for(int i=0;i<vArray.size();i++)
		   {
			   if( options.bAdditive ) {
				   //pMeshVerties[i].p += 
				   options.pObject->SetPoint( i, options.pObject->GetPoint( i ) + ConvertAlembicPointToMaxPoint(vArray[i]) ); 
			   }
			   else {
				   //pMeshVerties[i].p = ConvertAlembicPointToMaxPoint(vArray[i]);
				   options.pObject->SetPoint( i, ConvertAlembicPointToMaxPoint(vArray[i]) ); 
			   }
		   }
		   validateMeshes( options, "ALEMBIC_DATAFILL_VERTEX" );
	   }
    }

  

   Abc::Int32ArraySamplePtr meshFaceCount;
   Abc::Int32ArraySamplePtr meshFaceIndices;

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

   Abc::int32_t const* pMeshFaceCount = ( meshFaceCount.get() != NULL ) ? meshFaceCount->get() : NULL;
   Abc::int32_t const* pMeshFaceIndices = ( meshFaceIndices.get() != NULL ) ? meshFaceIndices->get() : NULL;

   int numFaces = static_cast<int>(meshFaceCount->size());
   int numIndices = static_cast<int>(meshFaceIndices->size());

   int sampleCount = 0;
   for(int i=0; i<numFaces; i++){
		int degree = pMeshFaceCount[i];
		sampleCount+=degree;
   }

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST )
   {
       		ESS_PROFILE_SCOPE("ALEMBIC_DATAFILL_FACELIST");
	   if(sampleCount == numIndices)
	   {
	   
        // Set up the index buffer
        int offset = 0;
		
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
				//pFace->edg[j] = -1;
				++offset;
			}
		}
		if( ! onlyTriangles ) {
			options.pMNMesh->SetFlag( MN_MESH_NONTRI, TRUE );
		}
		else {
			options.pMNMesh->SetFlag( MN_MESH_NONTRI, FALSE );
		}

		//the FillInMesh call breaks the topology of some meshes in 3DS Max 2012
		//(my test case in referenced here: https://github.com/Exocortex/ExocortexAlembic3DSMax/issues/191)
		//the FillInMesh call is necessary to prevent all meshes from crashing 3DS Max 2010 and 2011
		//Tested in 2013, some simples meshes seem to crash, so I'm putting it back in

#if 1//MAX_PRODUCT_YEAR_NUMBER < 2012
  	  if( ! options.pMNMesh->GetFlag( MN_MESH_FILLED_IN ) ) {
			  //HighResolutionTimer tFillInMesh;
			  {      	
				  ESS_PROFILE_SCOPE("FillInMesh");
 
				  options.pMNMesh->FillInMesh();
			  }
			  //ESS_LOG_WARNING("FillInMesh time: "<<tFillInMesh.elapsed());
			  if( options.pMNMesh->GetFlag(MN_MESH_RATSNEST) ) {
				  ESS_LOG_ERROR( "Mesh is a 'Rat's Nest' (more than 2 faces per edge) and not fully supported, fileName: " << options.fileName << " identifier: " << options.identifier );
			  }
		  }

#endif
	
		validateMeshes( options, "ALEMBIC_DATAFILL_FACELIST" );

	   }//if(sampleCount != numIndices)
	   else{
			ESS_LOG_WARNING("faceCount, index array mismatch. Not filling in indices (did you check 'dynamic topology' when exporting?).");
	   }
   }

	if( ( options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST ) &&
	   ( ! ( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX ) ) ) {
		ESS_PROFILE_SCOPE("Reset mesh vertices to (0,0,0)");
		 
		   MNVert* pMeshVerties = options.pMNMesh->V(0);
		   for(int i=0;i<meshPos->size();i++)
		   {
			   pMeshVerties[i].p = Point3(0,0,0);
		   }
		
	}

   if ( objMesh.valid() && ( options.nDataFillFlags & ALEMBIC_DATAFILL_NORMALS ) )
   {
		ESS_PROFILE_SCOPE("ALEMBIC_DATAFILL_NORMALS");
       AbcG::IN3fGeomParam meshNormalsParam = objMesh.getSchema().getNormalsParam();
       if(meshNormalsParam.valid())
       {
		   std::vector<Abc::V3f> normalValuesFloor, normalValuesCeil;
		   std::vector<AbcA::uint32_t> normalIndicesFloor, normalIndicesCeil;

			bool normalsFloor = getIndexAndValues( meshFaceIndices, meshNormalsParam, sampleInfo.floorIndex,
					   normalValuesFloor, normalIndicesFloor );
			bool normalsCeil = getIndexAndValues( meshFaceIndices, meshNormalsParam, sampleInfo.ceilIndex,
					   normalValuesCeil, normalIndicesCeil );

			if( ! normalsFloor ) {
			   ESS_LOG_ERROR( "Mesh normals are not set because they are not valid." );
			}
			else if( normalIndicesFloor.size() != sampleCount) {
			   //The last check ensures that we will not exceed the array bounds of PMeshNormalsFloor
			   ESS_LOG_ERROR( "Mesh normals are not set because their index count (" << normalIndicesFloor.size() << ") doesn't match the face index count (" << sampleCount << ")" );
		   }
		   else {

			   // Set up the specify normals
			   if( options.pMNMesh->GetSpecifiedNormals() == NULL ) {
					options.pMNMesh->SpecifyNormals();
			   }

			   //NOTE: options.pMNMesh->ClearSpecifiedNormals() will make getSpecifiedNormals return NULL again
  			   MNNormalSpec *normalSpec = options.pMNMesh->GetSpecifiedNormals();
			   normalSpec->SetParent( options.pMNMesh );
			   if( normalSpec->GetNumFaces() != numFaces || normalSpec->GetNumNormals() != (int)normalValuesFloor.size()) {
					//normalSpec->ClearAndFree();
				    normalSpec->SetNumFaces(numFaces);
					normalSpec->SetNumNormals((int)normalValuesFloor.size());
			   }
			   normalSpec->SetFlag(MESH_NORMAL_MODIFIER_SUPPORT, true);
			   normalSpec->SetAllExplicit(true); //this call is probably more efficient than the per vertex one since 3DS Max uses bit flags

			   // set normal values
			    if (sampleInfo.alpha != 0.0f && normalsCeil && normalValuesFloor.size() == normalValuesCeil.size() && !hasDynamicTopo )
			   {
				   for (int i = 0; i < normalValuesFloor.size(); i ++ ) {
					   Abc::V3f interpolatedNormal = normalValuesFloor[i] + (normalValuesCeil[i] - normalValuesFloor[i]) * float(sampleInfo.alpha);
			           interpolatedNormal *= options.fVertexAlpha;
					   normalSpec->Normal(i) = ConvertAlembicNormalToMaxNormal_Normalized( interpolatedNormal );
				   }
			   }
			   else
			   {
				   for (int i = 0; i < normalValuesFloor.size(); i ++ ) {
					   Abc::V3f interpolatedNormal = normalValuesFloor[i];
			           interpolatedNormal *= options.fVertexAlpha;
					   normalSpec->Normal(i) = ConvertAlembicNormalToMaxNormal_Normalized( interpolatedNormal );
				   }
			   }

			   // set normal indices
				{
				   int offset = 0;
				   for (int i = 0; i < numFaces; i++){
					   int degree = pMeshFaceCount[i];					   
					   MNNormalFace &normalFace = normalSpec->Face(i);
					   normalFace.SetDegree(degree);
					   for (int j = 0; j < degree; j ++ ) {
							normalFace.SetNormalID( degree - j - 1, (int) normalIndicesFloor[offset] );
						   offset ++;
					   }
				   }
			   }


			   //3DS Max documentation on MNMesh::checkNormals() - checks our flags and calls BuildNormals, ComputeNormals as needed. 
			   //MHahn: Probably not necessary since we explicility setting every normals
			   //normalSpec->CheckNormals();


			   // since we commented out check normals above, we need to set these explicitly.
			   normalSpec->SetFlag(MESH_NORMAL_NORMALS_BUILT, TRUE);
			   normalSpec->SetFlag(MESH_NORMAL_NORMALS_COMPUTED, TRUE);

			   //Also allocates space for the RVert array which we need for doing any normal vector queries
			   //options.pMNMesh->checkNormals(TRUE);
			   //MHahn: switched to build normals call, since all we need to is build the RVert array. Note: documentation says we only need
			   //to this if we query the MNMesh to ask about vertices. Do we actually need this capability?
			   options.pMNMesh->buildNormals();

		   }
       }
        validateMeshes( options, "ALEMBIC_DATAFILL_NORMALS" );
   }

   if( options.nDataFillFlags & ALEMBIC_DATAFILL_ALLOCATE_UV_STORAGE )
   {
      ESS_PROFILE_SCOPE("ALEMBIC_DATAFILL_ALLOCATE_UV_STORAGE");
	  //we can probably set this to the actual number of channels required if necessary
       options.pMNMesh->SetMapNum(100);
       options.pMNMesh->InitMap(0);
   }

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_UVS )
   {
      ESS_PROFILE_SCOPE("ALEMBIC_DATAFILL_UVS");
		std::string strObjectIdentifier = options.identifier;
		size_t found = strObjectIdentifier.find_last_of(":");
		strObjectIdentifier = strObjectIdentifier.substr(found+1);
		std::istringstream is(strObjectIdentifier);
		int uvI = 0;
		is >> uvI;

		uvI--;

		AbcG::IV2fGeomParam meshUvParam;
		if(objMesh.valid()){
			if(uvI == 0){
				meshUvParam = objMesh.getSchema().getUVsParam();
			}
			else{
				std::stringstream storedUVNameStream;
				storedUVNameStream<<"uv"<<uvI;
				if(objMesh.getSchema().getPropertyHeader( storedUVNameStream.str() ) != NULL){
					meshUvParam = AbcG::IV2fGeomParam( objMesh.getSchema(), storedUVNameStream.str());
				}
			}
		}
		else{
			if(uvI == 0){
				meshUvParam = objSubD.getSchema().getUVsParam();
			}
			else{
				std::stringstream storedUVNameStream;
				storedUVNameStream<<"uv"<<uvI;
				if(objSubD.getSchema().getPropertyHeader( storedUVNameStream.str() ) != NULL){
					meshUvParam = AbcG::IV2fGeomParam( objSubD.getSchema(), storedUVNameStream.str());
				}
			}
		}

		//add 1 since channel 0 is reserved for colors
		uvI++;

       if(meshUvParam.valid())
       {
           SampleInfo sampleInfo = getSampleInfo(
               sampleTime,
               meshUvParam.getTimeSampling(),
               meshUvParam.getNumSamples()
               );

		    std::vector<Abc::V2f> uvValuesFloor, uvValuesCeil;
		    std::vector<AbcA::uint32_t> uvIndicesFloor, uvIndicesCeil;

			bool uvFloor = getIndexAndValues( meshFaceIndices, meshUvParam, sampleInfo.floorIndex,
					   uvValuesFloor, uvIndicesFloor );
			bool uvCeil = getIndexAndValues( meshFaceIndices, meshUvParam, sampleInfo.ceilIndex,
					   uvValuesCeil, uvIndicesCeil );

		   if( !uvFloor || !uvCeil || uvValuesFloor.size() == 0) {
			   ESS_LOG_WARNING( "Mesh UVs are in an invalid state in Alembic file, ignoring." );
		   }
		   else {
			   // Set up the default texture map channel
				if(options.pMNMesh->MNum() > uvI)
				{
				   options.pMNMesh->InitMap(uvI);
				   MNMap *map = options.pMNMesh->M(uvI);
				   map->setNumVerts((int)uvValuesFloor.size());
				   map->setNumFaces(numFaces);

				   // set values
				   Point3* mapV = map->v;
				   if (sampleInfo.alpha != 0.0f && uvValuesFloor.size() == uvValuesCeil.size())
				   {
						for (int i = 0; i < uvValuesFloor.size(); i ++ )
					   {
						   Abc::V2f uv = uvValuesFloor[i] + ( uvValuesCeil[i] - uvValuesFloor[i] ) * float(sampleInfo.alpha);
						   mapV[i] = Point3( uv.x, uv.y, 0.0f );
					   }
				   }
				   else {
					   for (int i = 0; i < uvValuesFloor.size(); i ++ )
					   {
						   Abc::V2f uv =  uvValuesFloor[i];
						   mapV[i] = Point3( uv.x, uv.y, 0.0f );
					   }
				   }

				   // set indices
				   int offset = 0;
				   MNMapFace* mapF = map->f;
				   for (int i = 0; i < numFaces; i += 1)
				   {
					   int degree = 0; 
					   if (i < options.pMNMesh->numf) {
						   degree = options.pMNMesh->F(i)->deg;
					   }
					   mapF[i].SetSize(degree);
					   for (int j = 0; j < degree; j ++)
					   {
						   mapF[i].tv[degree - j - 1] = (int) uvIndicesFloor[ offset ];
						   ++offset;
					   }
				   }
			   }
			   else{
					ESS_LOG_WARNING( "UV channels have not been allocated. Cannot load uv channel "<<uvI );
			   }

		   }
       }
	   validateMeshes( options, "ALEMBIC_DATAFILL_UVS" );
   }

   if ( options.nDataFillFlags & ALEMBIC_DATAFILL_MATERIALIDS )
   {
 ESS_PROFILE_SCOPE("ALEMBIC_DATAFILL_MATERIALIDS");
	      Abc::IUInt32ArrayProperty materialIds;
       if(objMesh.valid() && objMesh.getSchema().getPropertyHeader( ".materialids" )) 
           materialIds = Abc::IUInt32ArrayProperty(objMesh.getSchema(), ".materialids");
       else if (objSubD.valid() && objSubD.getSchema().getPropertyHeader( ".materialids" ))
           materialIds = Abc::IUInt32ArrayProperty(objSubD.getSchema(), ".materialids");

       // If we don't detect a material id property then we try the facesets.  The order in the face set array is assumed
       // to be the material id
       if (materialIds.valid() && materialIds.getNumSamples() > 0)
       {
            Abc::UInt32ArraySamplePtr materialIdsPtr = materialIds.getValue(sampleInfo.floorIndex);

            if (materialIdsPtr->size() == numFaces)
            {
                for (int i = 0; i < materialIdsPtr->size(); i += 1)
                {
                    int nMatId = materialIdsPtr->get()[i];
                    if ( i < options.pMNMesh->numf)
                    {
                        options.pMNMesh->f[i].material = nMatId;
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
               AbcG::IFaceSetSchema faceSet;
               if(objMesh.valid())
                   faceSet = objMesh.getSchema().getFaceSet(faceSetNames[j]).getSchema();
               else
                   faceSet = objSubD.getSchema().getFaceSet(faceSetNames[j]).getSchema();
 
               AbcG::IFaceSetSchema::Sample faceSetSample = faceSet.getValue();
               Abc::Int32ArraySamplePtr faces = faceSetSample.getFaces();

               int nMatId = (int)j;
               for(size_t k=0;k<faces->size();k++)
               {
                   int faceId = faces->get()[k];
                   if ( faceId < options.pMNMesh->numf)
                   {
                       options.pMNMesh->f[faceId].material = nMatId;
                   }
               }
           }
       }
	   validateMeshes( options, "ALEMBIC_DATAFILL_MATERIALIDS" );
   }
 
 /*   if( options.pMNMesh->GetSpecifiedNormals() == NULL ) {
		 {
		 ESS_PROFILE_SCOPE("SpecifyNormals");

		options.pMNMesh->SpecifyNormals();
		 }
		 {
		 ESS_PROFILE_SCOPE("CheckNormals");
		options.pMNMesh->GetSpecifiedNormals()->CheckNormals();
		 }
		 {
		 ESS_PROFILE_SCOPE("CheckNormals");

	    options.pMNMesh->checkNormals(TRUE);
			  }
	}*/

  // This isn't required if we notify 3DS Max properly via the channel flags for vertex changes.
   //options.pMNMesh->MNDebugPrint();
   if ( (options.nDataFillFlags & ALEMBIC_DATAFILL_FACELIST) || (options.nDataFillFlags & ALEMBIC_DATAFILL_NORMALS) ) {
		 ESS_PROFILE_SCOPE("InvalidateTopoCache/InvalidateGeomCache");
	     options.pMNMesh->InvalidateTopoCache();
		 options.pMNMesh->InvalidateGeomCache();
	}
   else {
	  if( options.nDataFillFlags & ALEMBIC_DATAFILL_VERTEX ) {
		 ESS_PROFILE_SCOPE("InvalidateGeomCache");
		 // options.pMNMesh->InvalidateGeomCache();
		 options.pObject->PointsWereChanged();
	  }
   }
}

bool AlembicImport_IsPolyObject(AbcG::IPolyMeshSchema::Sample &polyMeshSample)
{
    Abc::Int32ArraySamplePtr meshFaceCount = polyMeshSample.getFaceCounts();

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

void addAlembicMaterialsModifier(INode *pNode, AbcG::IObject& iObj)
{

	AbcG::IPolyMesh objMesh;
	AbcG::ISubD objSubD;
	int nLastSample = 0;

	if(AbcG::IPolyMesh::matches(iObj.getMetaData())){
	   objMesh = AbcG::IPolyMesh(iObj,Abc::kWrapExisting);
	   nLastSample = (int)objMesh.getSchema().getNumSamples()-1;
	}
	else{
	   objSubD = AbcG::ISubD(iObj,Abc::kWrapExisting);
	   nLastSample = (int)objSubD.getSchema().getNumSamples()-1;
	}

	if(!objMesh.valid() && !objSubD.valid()){
	   return;
	}

	SampleInfo sampleInfo;
	if(objMesh.valid()){
		sampleInfo = getSampleInfo(nLastSample, objMesh.getSchema().getTimeSampling(), objMesh.getSchema().getNumSamples());
	}
	else{
		sampleInfo = getSampleInfo(nLastSample, objSubD.getSchema().getTimeSampling(), objSubD.getSchema().getNumSamples());
	}

	Abc::IStringArrayProperty matNamesProperty;
	if(objMesh.valid() && objMesh.getSchema().getPropertyHeader(".materialnames")){
		matNamesProperty = Abc::IStringArrayProperty(objMesh.getSchema(), ".materialnames");
	}
	else if(objSubD.valid() && objSubD.getSchema().getPropertyHeader(".materialnames")){
		matNamesProperty = Abc::IStringArrayProperty(objSubD.getSchema(), ".materialnames");
	}

	std::vector<std::string> faceSetNames;

	if(!matNamesProperty.valid() || matNamesProperty.getNumSamples() == 0){//if we couldn't read the .materialnames property, look for the faceset names

		if(objMesh.valid()){
			sampleInfo = getSampleInfo(0, objMesh.getSchema().getTimeSampling(), objMesh.getSchema().getNumSamples());
		}
		else{
			sampleInfo = getSampleInfo(0, objSubD.getSchema().getTimeSampling(), objSubD.getSchema().getNumSamples());
		}

		if(objMesh.valid()){
			objMesh.getSchema().getFaceSetNames(faceSetNames);
		}
		else{
			objSubD.getSchema().getFaceSetNames(faceSetNames);
		}
	}
	else{

		Abc::StringArraySamplePtr matNamesSamplePtr = matNamesProperty.getValue(sampleInfo.floorIndex);
		size_t len = matNamesSamplePtr->size();

		for(size_t i=0; i<len; i++){

			faceSetNames.push_back(matNamesSamplePtr->get()[i]);
		}
	}

	if(faceSetNames.size() <= 0){
		return;
	}

	std::string names("");

	for(size_t j=0;j<faceSetNames.size();j++)
	{
		const char* name = faceSetNames[j].c_str();
		names+="\"";
		names+=name;
		names+="\"";
		if(j != faceSetNames.size()-1){
			names+=", ";
		}
	}

	GET_MAX_INTERFACE()->SelectNode( pNode );

	const size_t bufSize = names.size() + 500;

	char* szBuffer = new char[bufSize];
	sprintf_s(szBuffer, bufSize,
			"AlembicMaterialModifier = EmptyModifier()\n"
			"AlembicMaterialModifier.name = \"Alembic Materials\"\n"
			"addmodifier $ AlembicMaterialModifier\n"

			"AlembicMaterialCA = attributes AlembicMaterialModifier\n"
			"(\n"	
				"rollout AlembicMaterialModifierRLT \"Alembic Materials\"\n"
				"(\n"
					"listbox eTestList \"\" items:#(%s)\n"
				")\n"
			")\n"

			"custattributes.add $.modifiers[\"Alembic Materials\"] AlembicMaterialCA baseobject:false\n"
			"$.modifiers[\"Alembic Materials\"].enabled = false"
			//"$.modifiers[\"Alembic Materials\"].enabled = false\n"
			//"if $.modifiers[\"Alembic Mesh Normals\"] != undefined then (\n"
			//"$.modifiers[\"Alembic Mesh Normals\"].enabled = true\n"
			//")\n"
			//"if $.modifiers[\"Alembic Mesh Topology\"] != undefined then (\n"
			//"$.modifiers[\"Alembic Mesh Topology\"].enabled = true\n",
			//")\n"
			,
			names.c_str()
	);

	ExecuteMAXScriptScript( EC_UTF8_to_TCHAR( szBuffer ) );

	delete[] szBuffer;
}

int AlembicImport_PolyMesh(const std::string &path, AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode)
{
  ESS_PROFILE_FUNC();
	const std::string& identifier = iObj.getFullName();

	// Fill in the mesh
    alembic_fillmesh_options dataFillOptions;
    dataFillOptions.pIObj = &iObj;
    dataFillOptions.pMNMesh = NULL;
    dataFillOptions.dTicks = GET_MAX_INTERFACE()->GetTime();

    dataFillOptions.nDataFillFlags = ALEMBIC_DATAFILL_VERTEX|ALEMBIC_DATAFILL_FACELIST;
    dataFillOptions.nDataFillFlags |= options.importNormals ? ALEMBIC_DATAFILL_NORMALS : 0;
    dataFillOptions.nDataFillFlags |= options.importUVs ? ALEMBIC_DATAFILL_UVS : 0;
    dataFillOptions.nDataFillFlags |= options.importBboxes ? ALEMBIC_DATAFILL_BOUNDINGBOX : 0;
    dataFillOptions.nDataFillFlags |= options.importMaterialIds ? ALEMBIC_DATAFILL_MATERIALIDS : 0;

    // Create the poly or tri object and place it in the scene
    // Need to use the attach to existing import flag here 
	Object *newObject = NULL;
	AbcG::IPolyMesh objMesh;
	AbcG::ISubD objSubD;

	if( AbcG::IPolyMesh::matches(iObj.getMetaData()) ) {

		objMesh = AbcG::IPolyMesh(iObj, Abc::kWrapExisting);
		if (!objMesh.valid())
		{
			return alembic_failure;
		}

		if( objMesh.getSchema().getNumSamples() == 0 ) {
			ESS_LOG_WARNING( "Alembic Mesh set has 0 samples, ignoring." );
			return alembic_failure;
		}

		//AbcG::IPolyMeshSchema::Sample polyMeshSample;
		//objMesh.getSchema().get(polyMeshSample, 0);

		//if (AlembicImport_IsPolyObject(polyMeshSample))
		//{
		//	
		//	PolyObject *pPolyObject = (PolyObject *) GetPolyObjDescriptor()->Create();
		//	dataFillOptions.pMNMesh = &(pPolyObject->GetMesh());
		//	newObject = pPolyObject;
		//}
		/*else
		{
			TriObject *pTriObj = (TriObject *) GetTriObjDescriptor()->Create();
			dataFillOptions.pMesh = &( pTriObj->GetMesh() );
			newObject = pTriObj;
		}*/
	}
	else if( AbcG::ISubD::matches(iObj.getMetaData()) )
	{
		objSubD = AbcG::ISubD(iObj, Abc::kWrapExisting);
		if (!objSubD.valid())
		{
			return alembic_failure;
		}

		if( objSubD.getSchema().getNumSamples() == 0 ) {
			ESS_LOG_WARNING( "Alembic SubD set has 0 samples, ignoring." );
			return alembic_failure;
		}

		//AbcG::ISubDSchema::Sample subDSample;
		//objSubD.getSchema().get(subDSample, 0);

		//PolyObject *pPolyObject = (PolyObject *) GetPolyObjDescriptor()->Create();
		//dataFillOptions.pMNMesh = &(pPolyObject->GetMesh());
		//newObject = pPolyObject;
	}
	else {
		return alembic_failure;
	}

   // Create the object pNode
	INode *pNode = *pMaxNode;
	bool bReplaceExistingModifiers = false;
	if(!pNode){
		Object* newObject = (PolyObject*)GetPolyObjDescriptor()->Create();
		if (newObject == NULL)
		{
			return alembic_failure;
		}
      Abc::IObject parent = iObj.getParent();
      std::string name = removeXfoSuffix(iObj.getName().c_str());
      pNode = GET_MAX_INTERFACE()->CreateObjectNode(newObject, EC_UTF8_to_TCHAR(name.c_str()));
		if (pNode == NULL){
			return alembic_failure;
		}
		*pMaxNode = pNode;
	}
	else{
		bReplaceExistingModifiers = true;
	}

	TimeValue zero( 0 );

	std::vector<Modifier*> modifiersToEnable;

	bool isDynamicTopo = isAlembicMeshTopoDynamic( &iObj );
	//isDynamicTopo = true;


	GET_MAX_INTERFACE()->SelectNode( pNode );


	if( !FindModifier(pNode, "Alembic Metadata") ){
		importMetadata(iObj);
	}

	if( !FindModifier(pNode, "Alembic Materials") ){
		addAlembicMaterialsModifier(pNode, iObj);
	}

	//alembicMeshInfo meshInfo;
	//meshInfo.open(path, identifier);
	//meshInfo.setSample(0);

	//ESS_LOG_INFO( "Node: " << pNode->GetName() );
	//ESS_LOG_INFO( "isDynamicTopo: " << isDynamicTopo );
	if( isAlembicMeshTopology( &iObj ) )
	{
		// Create the polymesh modifier
		Modifier *pModifier = NULL;
		bool bCreatedModifier = false;
		if(bReplaceExistingModifiers){
			pModifier = FindModifier(pNode, ALEMBIC_MESH_TOPO_MODIFIER_CLASSID, identifier.c_str());
		}
		if(!pModifier){
			pModifier = static_cast<Modifier*>
				(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_MESH_TOPO_MODIFIER_CLASSID));
			bCreatedModifier = true;
		}
		pModifier->DisableMod();

		// Set the alembic id
		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );
		
		if(bCreatedModifier){

			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifier.c_str() ) );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
			if( isDynamicTopo ) {
				pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geometry" ), zero, TRUE );
				pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "normals" ), zero, ( options.importNormals ? TRUE : FALSE ) );
				pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "uvs" ), zero, FALSE );
			}
			else {
				pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geometry" ), zero, FALSE );
				pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "normals" ), zero, FALSE );
				pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "uvs" ), zero, FALSE );
			}

			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
		
			// Add the modifier to the pNode
			GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);
		}

		if( isDynamicTopo ) {
			GET_MAX_INTERFACE()->SelectNode( pNode );
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$.modifiers[#Alembic_Mesh_Topology].time" );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}

		modifiersToEnable.push_back( pModifier );
	}
	bool isUVWContant = true;
	if( /*!isDynamicTopo &&*/ options.importUVs && isAlembicMeshUVWs( &iObj, isUVWContant ) ) {
		//ESS_LOG_INFO( "isUVWContant: " << isUVWContant );

		AbcG::IV2fGeomParam meshUVsParam;
		if(objMesh.valid()){
			meshUVsParam = objMesh.getSchema().getUVsParam();
		}
		else{ 
			meshUVsParam = objSubD.getSchema().getUVsParam();
		}

		if(meshUVsParam.valid())
		{
			size_t numUVSamples = meshUVsParam.getNumSamples();
			Abc::V2fArraySamplePtr meshUVs = meshUVsParam.getExpandedValue(0).getVals();
			if(meshUVs->size() > 0)
			{
				// check if we have a uv set names prop
				std::vector<std::string> uvSetNames;
				if(objMesh.valid() && objMesh.getSchema().getPropertyHeader( ".uvSetNames" ) != NULL ){
					Abc::IStringArrayProperty uvSetNamesProp = Abc::IStringArrayProperty( objMesh.getSchema(), ".uvSetNames" );
					Abc::StringArraySamplePtr ptr = uvSetNamesProp.getValue(0);
					for(size_t i=0;i<ptr->size();i++){
						uvSetNames.push_back(ptr->get()[i].c_str());
					}
				}
				else if ( objSubD.valid() && objSubD.getSchema().getPropertyHeader( ".uvSetNames" ) != NULL ){
					Abc::IStringArrayProperty uvSetNamesProp = Abc::IStringArrayProperty( objSubD.getSchema(), ".uvSetNames" );
					Abc::StringArraySamplePtr ptr = uvSetNamesProp.getValue(0);
					for(size_t i=0;i<ptr->size();i++){
						uvSetNames.push_back(ptr->get()[i].c_str());
					}
				}

				if(uvSetNames.size() == 0){
					uvSetNames.push_back("Default");
				}

				for(int i=0; i<uvSetNames.size(); i++){

					int channelNumber = 0;
					std::string uvName = uvSetNames[i];
					if( ! parseTrailingNumber( uvName, "map", channelNumber ) ) {
						if( ! parseTrailingNumber( uvName, "channel_", channelNumber ) ) {
							if( ! parseTrailingNumber( uvName, "Channel_", channelNumber ) ) {
								channelNumber = (i+1);
							}
						}
					}
					std::stringstream identifierStream;					
					identifierStream<<identifier<<":"<<channelNumber;

					// Create the polymesh modifier
					Modifier *pModifier = NULL;
					bool bCreatedModifier = false;
					if(bReplaceExistingModifiers){
						pModifier = FindModifier(pNode, ALEMBIC_MESH_UVW_MODIFIER_CLASSID, identifierStream.str().c_str());
					}
					if(!pModifier){
						pModifier = static_cast<Modifier*>
							(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_MESH_UVW_MODIFIER_CLASSID));
						bCreatedModifier = true;
					}
					pModifier->DisableMod();
					
					pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );
					
					if(bCreatedModifier){

						pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifierStream.str().c_str() ) );
						pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
						pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );

						// Add the modifier to the pNode
						GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);
					}

					if( ! isUVWContant ) {
						GET_MAX_INTERFACE()->SelectNode( pNode );
						char szControllerName[10000];
						sprintf_s( szControllerName, 10000, "$.modifiers[#Alembic_Mesh_UVW].time" );
						AlembicImport_ConnectTimeControl( szControllerName, options );
					}

					modifiersToEnable.push_back( pModifier );
				}
			}
		}


	}
	bool isGeomContant = true;
	if( ( ! isDynamicTopo ) && isAlembicMeshPositions( &iObj, isGeomContant ) ) {
		//ESS_LOG_INFO( "isGeomContant: " << isGeomContant );
		
		Modifier *pModifier = NULL;
		bool bCreatedModifier = false;
		if(bReplaceExistingModifiers){
			pModifier = FindModifier(pNode, ALEMBIC_MESH_GEOM_MODIFIER_CLASSID, identifier.c_str());
		}
		if(!pModifier){
			pModifier = static_cast<Modifier*>
				(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_MESH_GEOM_MODIFIER_CLASSID));
			bCreatedModifier = true;
		}

		pModifier->DisableMod();

		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );

		if(bCreatedModifier){

			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifier.c_str() ) );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "geoAlpha" ), zero, 1.0f );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
		
			// Add the modifier to the pNode
			GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);
		}

		if( ! isGeomContant ) {
			GET_MAX_INTERFACE()->SelectNode( pNode );
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$.modifiers[#Alembic_Mesh_Geometry].time" );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}

		modifiersToEnable.push_back( pModifier );
	}
	bool isNormalsContant = true;
	if( ( ! isDynamicTopo ) && isAlembicMeshNormals( &iObj, isNormalsContant ) ) {
		//ESS_LOG_INFO( "isNormalsContant: " << isNormalsContant );
		
		Modifier *pModifier = NULL;
		bool bCreatedModifier = false;
		if(bReplaceExistingModifiers){
			pModifier = FindModifier(pNode, ALEMBIC_MESH_NORMALS_MODIFIER_CLASSID, identifier.c_str());
		}
		if(!pModifier){
			pModifier = static_cast<Modifier*>
				(GET_MAX_INTERFACE()->CreateInstance(OSM_CLASS_ID, ALEMBIC_MESH_NORMALS_MODIFIER_CLASSID));
			bCreatedModifier = true;
		}

		pModifier->DisableMod();

		pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "path" ), zero, EC_UTF8_to_TCHAR( path.c_str() ) );

		if(bCreatedModifier){

			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "identifier" ), zero, EC_UTF8_to_TCHAR( identifier.c_str() ) );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "time" ), zero, 0.0f );
			pModifier->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pModifier, 0, "muted" ), zero, FALSE );
		
			// Add the modifier to the pNode
			GET_MAX_INTERFACE()->AddModifier(*pNode, *pModifier);
		}

		if( ! isNormalsContant ) {
			GET_MAX_INTERFACE()->SelectNode( pNode );
			char szControllerName[10000];
			sprintf_s( szControllerName, 10000, "$.modifiers[#Alembic_Mesh_Normals].time" );
			AlembicImport_ConnectTimeControl( szControllerName, options );
		}

		if( options.importNormals ) {
			modifiersToEnable.push_back( pModifier );
		}
	}

	if( AbcG::ISubD::matches(iObj.getMetaData()) )
	{
		GET_MAX_INTERFACE()->SelectNode( pNode );

		char* szBuffer = "addmodifier $ (meshsmooth())\n"
						 "$.modifiers[#MeshSmooth].iterations = 1\n";

		ExecuteMAXScriptScript( EC_UTF8_to_TCHAR( szBuffer ) );
	}

    // Add the new inode to our current scene list
   // SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, newObject, OBTYPE_MESH, &std::string(iObj.getFullName())); 
    //options.currentSceneList.Append(pEntry);

    // Set the visibility controller
    AlembicImport_SetupVisControl( path.c_str(), identifier.c_str(), iObj, pNode, options);

	for( int i = 0; i < modifiersToEnable.size(); i ++ ) {
		modifiersToEnable[i]->EnableMod();
	}


	return 0;
}
