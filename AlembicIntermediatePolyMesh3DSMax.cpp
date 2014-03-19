#include "stdafx.h"
#include "AlembicIntermediatePolyMesh3DSMax.h"
#include "Utility.h"
#include "AlembicWriteJob.h"
#include "CommonLog.h"

// From the SDK
// How to calculate UV's for face mapped materials.
static Point3 basic_tva[3] = { 
	Point3(0.0,0.0,0.0),Point3(1.0,0.0,0.0),Point3(1.0,1.0,0.0)
};
static Point3 basic_tvb[3] = { 
	Point3(1.0,1.0,0.0),Point3(0.0,1.0,0.0),Point3(0.0,0.0,0.0)
};
static int nextpt[3] = {1,2,0};
static int prevpt[3] = {2,0,1};


// Add a normal to the list if the smoothing group bits overlap,
// otherwise create a new vertex normal in the list
void VNormal::AddNormal(Point3 &n,DWORD s) 
{   
    if (!(s&smooth) && init) 
    {     
        if (next)
        {
            next->AddNormal(n,s);     
        }
        else 
        {      
            next = new VNormal(n,s);     
        }   
    }   
    else 
    {     
        norm += n;     
        smooth |= s;     
        init = TRUE;   
    }
} 

// Retrieves a normal if the smoothing groups overlap or there is// only one in the list
Point3 &VNormal::GetNormal( DWORD s )
{   
    if (smooth&s || !next) 
    {
        return norm;   
    }
    else
    {
        return next->GetNormal(s); 
    }
} 

// Normalize each normal in the list
void VNormal::Normalize() 
{   
    VNormal *ptr = next, *prev = this;   
    while (ptr)   
    {     
        if (ptr->smooth&smooth) 
        {      
            norm += ptr->norm;      
            prev->next = ptr->next;       
            delete ptr;      
            ptr = prev->next;     
        }     
        else 
        {      
            prev = ptr;      
            ptr = ptr->next;     
        }   
    }   
    norm = ::Normalize(norm);   
    if (next) 
    {
        next->Normalize();
    }
}

int materialsMergeStr::getUniqueMatId(int matId){	
	
	return getMatEntry(currUniqueHandle, matId).matId;
}

materialStr& materialsMergeStr::getMatEntry(AnimHandle uniqueHandle, int matId)
{

	
	mergedMeshMaterialsMap_it groupIt = groupMatMap.find(uniqueHandle);
	if(groupIt == groupMatMap.end()){
		meshMaterialsMap matMap;
		groupMatMap[uniqueHandle] = matMap;
	}
	meshMaterialsMap* pMatMap = &groupMatMap[uniqueHandle];
	
	meshMaterialsMap_it matIt = pMatMap->find(matId);
	if(matIt == pMatMap->end()){ 
		materialStr& matStr = (*pMatMap)[matId];
		if(!bPreserveIds){
			matStr.matId = nNextMatId;	
			nNextMatId++;
		}
		else{
			matStr.matId = matId;
		}

		std::stringstream nameStream;

		nameStream<<"("<<uniqueHandle<<", "<<matId<<")";

		matStr.name = nameStream.str();
		return matStr; 
	}
	else{
		return (*pMatMap)[matId];
	}
}

void materialsMergeStr::setMatName(int matId, const std::string& name)
{
	getMatEntry(currUniqueHandle, matId).name = name;
}


bool GetOption(std::map<std::string, bool>& mOptions, const std::string& in_Name)
{
    std::map<std::string,bool>::iterator it = mOptions.find(in_Name);
    if(it != mOptions.end())
    {
        return it->second;
    }
    return false;
}

void IntermediatePolyMesh3DSMax::GetIndexedNormalsFromSpecifiedNormals( MNMesh* polyMesh, Imath::M44f& transform44f_I_T, IndexedNormals &indexedNormals ) {
   ESS_LOG_WARNING("GetIndexedNormalsFromSpecifiedNormals::MNMesh");

	EC_ASSERT( polyMesh != NULL );
	MNNormalSpec *normalSpec = polyMesh->GetSpecifiedNormals();
	EC_ASSERT( normalSpec && normalSpec->GetNumNormals() > 0 && normalSpec->GetNumFaces() > 0 );

	indexedNormals.name = "normals";
	indexedNormals.indices.clear();
	indexedNormals.values.clear();
	for( int f = 0; f < normalSpec->GetNumFaces(); f ++ ) {
		MNNormalFace &normalFace = normalSpec->GetFaceArray()[f];
        for (int v = normalFace.GetDegree()-1; v >= 0; v--)
        {
			indexedNormals.indices.push_back( normalFace.GetNormalID( v ) );
		}		
	}

	Point3 *pNormalArray = normalSpec->GetNormalArray();
	for( int v = 0; v < normalSpec->GetNumNormals(); v ++ ) {
		Point3 normal = pNormalArray[v];
      Abc::V3f newNormal = Abc::V3f(normal.x, normal.y, normal.z) * transform44f_I_T;
		indexedNormals.values.push_back( ConvertMaxNormalToAlembicNormal( newNormal ) );
	}
}

void IntermediatePolyMesh3DSMax::GetIndexedNormalsFromSpecifiedNormals( Mesh *triMesh, Imath::M44f& transform44f_I_T, IndexedNormals &indexedNormals ) {
	ESS_LOG_WARNING("GetIndexedNormalsFromSpecifiedNormals::Mesh");
   
   EC_ASSERT( triMesh != NULL );
	MeshNormalSpec *normalSpec = triMesh->GetSpecifiedNormals();
	EC_ASSERT( normalSpec && normalSpec->GetNumNormals() > 0 && normalSpec->GetNumFaces() > 0 );

	indexedNormals.name = "normals";
	indexedNormals.indices.clear();
	indexedNormals.values.clear();
	for( int f = 0; f < normalSpec->GetNumFaces(); f ++ ) {
		MeshNormalFace &normalFace = normalSpec->GetFaceArray()[f];
        for (int v = 3-1; v >= 0; v--)
        {
			indexedNormals.indices.push_back( normalFace.GetNormalID( v ) );
		}		
	}

	Point3 *pNormalArray = normalSpec->GetNormalArray();
	for( int v = 0; v < normalSpec->GetNumNormals(); v ++ ) {
		Point3 normal = pNormalArray[v];
      Abc::V3f newNormal = Abc::V3f(normal.x, normal.y, normal.z) * transform44f_I_T;
		indexedNormals.values.push_back( ConvertMaxNormalToAlembicNormal( newNormal ) );
	}
}

void IntermediatePolyMesh3DSMax::GetIndexedNormalsFromSmoothingGroups( MNMesh* polyMesh, Imath::M44f& transform44f_I_T, std::vector<Abc::int32_t> &faceIndices, IndexedNormals &indexedNormals ) {
	ESS_LOG_WARNING("GetIndexedNormalsFromSmoothingGroups::MNMesh");
   
   EC_ASSERT( polyMesh != NULL );
	MNNormalSpec *normalSpec = polyMesh->GetSpecifiedNormals();
	EC_ASSERT( normalSpec == NULL );

	indexedNormals.name = "normals";
	indexedNormals.indices.clear();
	indexedNormals.values.clear();

	MeshSmoothingGroupNormals smoothingGroupNormals( polyMesh );

	std::vector<Abc::N3f> expandedNormals;

	for (int i = 0; i < polyMesh->FNum(); i++) 
    {
        int degree = polyMesh->F(i)->deg;
        for (int j = degree-1; j >= 0; j--)
        {
            Point3 normal = smoothingGroupNormals.GetVNormal( i, j );
         Abc::V3f newNormal = Abc::V3f(normal.x, normal.y, normal.z) * transform44f_I_T;
			expandedNormals.push_back( ConvertMaxNormalToAlembicNormal( newNormal ) );
		}
	}		

	createIndexedArray<Abc::N3f, SortableV3f>(faceIndices, expandedNormals, indexedNormals.values, indexedNormals.indices);
}

void IntermediatePolyMesh3DSMax::GetIndexedNormalsFromSmoothingGroups( Mesh *triMesh, Imath::M44f& transform44f_I_T, std::vector<Abc::int32_t> &faceIndices, IndexedNormals &indexedNormals ) {
   ESS_LOG_WARNING("GetIndexedNormalsFromSmoothingGroups::Mesh");
   
   EC_ASSERT( triMesh != NULL );
	MeshNormalSpec *normalSpec = triMesh->GetSpecifiedNormals();
	EC_ASSERT( normalSpec == NULL );

	indexedNormals.name = "normals";
	indexedNormals.indices.clear();
	indexedNormals.values.clear();

	MeshSmoothingGroupNormals smoothingGroupNormals( triMesh );

	std::vector<Abc::N3f> expandedNormals;

	for (int i = 0; i < triMesh->getNumFaces(); i++) 
    {
        for (int j = 3-1; j >= 0; j--)
        {
            Point3 normal = smoothingGroupNormals.GetVNormal( i, j );
         Abc::V3f newNormal = Abc::V3f(normal.x, normal.y, normal.z) * transform44f_I_T;
			expandedNormals.push_back( ConvertMaxNormalToAlembicNormal( newNormal ) );
		}
	}

	createIndexedArray<Abc::N3f, SortableV3f>(faceIndices, expandedNormals, indexedNormals.values, indexedNormals.indices);
}

void IntermediatePolyMesh3DSMax::GetIndexedUVsFromChannel( MNMesh *polyMesh, int chanNum, IndexedUVs &indexedUVs ) {
	EC_ASSERT( polyMesh != NULL );

	MNMap *map = polyMesh->M(chanNum);

	if( map->FNum() != polyMesh->FNum() ) {
		ESS_LOG_INFO("Warning: Can't export PolyMesh UV Channel #" << chanNum << " as its map face count (" << map->FNum() << ") doesn't match face count of mesh (" << polyMesh->FNum() << ")" );							
		return;
	}

	std::stringstream nameStream;
	nameStream<<"Channel_"<<chanNum;
	indexedUVs.name = nameStream.str();

	for( int v = 0; v < map->VNum(); v ++ ) {
		UVVert &texCoord = map->V( v );
		indexedUVs.values.push_back( Abc::V2f(texCoord.x, texCoord.y) );
	}

	for (int f=0; f<polyMesh->FNum(); f++) 
	{
		int degree = polyMesh->F(f)->deg;
		for (int j = degree-1; j >= 0; j -= 1)
		{
			if ( j < map->F(f)->deg)
			{
				indexedUVs.indices.push_back( map->F(f)->tv[j] );
			}
			else
			{
				ESS_LOG_INFO("Warning: vertex is missing uv coordinate.");
				indexedUVs.indices.push_back( 0 );
			}
		}
	}
}

void IntermediatePolyMesh3DSMax::GetIndexedUVsFromChannel( Mesh *triMesh, int chanNum, IndexedUVs &indexedUVs ) {
	EC_ASSERT( triMesh != NULL );

	MeshMap& map = triMesh->Map(chanNum);

	if( map.getNumFaces() != triMesh->getNumFaces() ) {
		ESS_LOG_INFO("Warning: Can't export TriMesh UV Channel #" << chanNum << " as its map face count (" << map.getNumFaces() << ") doesn't match face count of mesh (" << triMesh->getNumFaces() << ")" );							
		return;
	}

	std::stringstream nameStream;
	nameStream<<"Channel_"<<chanNum;
	indexedUVs.name = nameStream.str();

	for (int f=0; f< triMesh->getNumFaces(); f++) 
	{
		for (int j = 2; j >= 0; j -= 1)
		{
			indexedUVs.indices.push_back( map.tf[f].t[j] );					
		}
	}
	for (int v=0; v< map.getNumVerts(); v++ ){
		UVVert& texCoord = map.tv[v];
		indexedUVs.values.push_back( Abc::V2f( texCoord.x, texCoord.y ) );					
	}
}

void IntermediatePolyMesh3DSMax::Save(SceneNodePtr eNode, const Imath::M44f& transform44f, const CommonOptions& options, double time)
{
   ESS_PROFILE_FUNC();

   materialsMergeStr* pMatMerge = NULL;
   bool bFirstFrame = time == 0.0;

   SceneNodeMaxPtr maxNode = reinterpret<SceneNode, SceneNodeMax>(eNode);
   Mtl* pMtl = maxNode->getMtl();

   MeshData meshData = maxNode->getMeshData(time);
   Mesh* triMesh = meshData.triMesh;
   MNMesh* polyMesh = meshData.polyMesh;

   int nMatId = -1;//maxNode->getMtlID();


 ////
	//////for transforming the normals
	////Matrix3 meshTM_I_T = meshTM;
	////meshTM_I_T.SetTrans(Point3(0.0, 0.0, 0.0));
	//////the following two steps are necessary because meshTM can contain a scale factor
	////meshTM_I_T = Inverse(meshTM_I_T);
	////meshTM_I_T = TransposeRot(meshTM_I_T);
	
   //for transforming the normals
   Imath::M44f transform44f_I_T = transform44f;
   //only the roatation component should be applied
   transform44f_I_T = transform44f_I_T.setTranslation(Imath::V3f(0.0f, 0.0f, 0.0f));
   //dealing with scaling
   transform44f_I_T = transform44f_I_T.inverse();
   transform44f_I_T = transform44f_I_T.transpose();


    LONG vertCount = (polyMesh != NULL) ? polyMesh->VNum()
                                       : triMesh->getNumVerts();



    // allocate the points and normals
    posVec.resize(vertCount);
    //Matrix3 wm = GetRef().node->GetObjTMAfterWSM(ticks);
    for(LONG i=0;i<vertCount;i++)
    {
        Point3 &maxPoint = (polyMesh != NULL) ? polyMesh->V(i)->p
                                             : triMesh->getVert(i);
		
        posVec[i] = ConvertMaxPointToAlembicPoint( maxPoint );
        posVec[i] *= transform44f;
        bbox.extendBy(posVec[i]);
		//Abc::Box3d& box = bbox;
		//ESS_LOG_INFO( "Archive bbox: min("<<box.min.x<<", "<<box.min.y<<", "<<box.min.z<<") max("<<box.max.x<<", "<<box.max.y<<", "<<box.max.z<<")" );
    }

    // abort here if we are just storing points
	bool purePointCache = options.GetBoolOption("exportPurePointCache");
    if(purePointCache)
    {
        meshData.free();
        return;
    }


	// Get the entire face count and index Count for the mesh
    LONG faceCount = (polyMesh != NULL) ? polyMesh->FNum()
                                       : triMesh->getNumFaces();

	// create an index lookup table
    int sampleCount = 0;
    for(int f = 0; f < faceCount; f += 1)
    {
        int degree = (polyMesh != NULL) ? polyMesh->F(f)->deg : 3;
        sampleCount += degree;
	}

	// we also need to store the face counts as well as face indices

     mFaceCountVec.resize(faceCount);
     mFaceIndicesVec.resize(sampleCount);

     int offset = 0;
     for(LONG f=0;f<faceCount;f++)
     {
        int degree = (polyMesh != NULL) ? polyMesh->F(f)->deg : 3;
        mFaceCountVec[f] = degree;
		for (int i = degree-1; i >= 0; i -= 1)
        {
            int vertIndex = (polyMesh != NULL) ? polyMesh->F(f)->vtx[i]
                                              : triMesh->faces[f].v[i];
			mFaceIndicesVec[offset++] = vertIndex;
        }
     }
 


    // let's check if we have user normals
    if(options.GetBoolOption("exportNormals"))
    {
		if( polyMesh != NULL ) {
			MNNormalSpec *normalSpec = polyMesh->GetSpecifiedNormals();
            if (normalSpec && normalSpec->GetNumNormals() > 0 && normalSpec->GetNumFaces() > 0)
            {
				GetIndexedNormalsFromSpecifiedNormals( polyMesh, transform44f_I_T, mIndexedNormals );
			}
			else {
				polyMesh->buildNormals();
				GetIndexedNormalsFromSmoothingGroups( polyMesh, transform44f_I_T, mFaceIndicesVec, mIndexedNormals );
			}
		}
		if( triMesh != NULL ) {
			MeshNormalSpec *normalSpec = triMesh->GetSpecifiedNormals();
			if (normalSpec && normalSpec->GetNumNormals() > 0 && normalSpec->GetNumFaces() > 0)
            {
				GetIndexedNormalsFromSpecifiedNormals( triMesh, transform44f_I_T, mIndexedNormals );
			}
			else {
				triMesh->buildNormals();
				GetIndexedNormalsFromSmoothingGroups( triMesh, transform44f_I_T, mFaceIndicesVec, mIndexedNormals );
			}
		}
    }

    //for(int i=0; i<mIndexedNormals.indices.size(); i++){
    //   if( mIndexedNormals.indices[i] > mIndexedNormals.values.size()){
    //       ESS_LOG_WARNING("out of bounds index");
    //   }
    //}
  
   //write out the UVs
   if(options.GetBoolOption("exportUVs"))
   {
	  if (polyMesh != NULL)
	  {
			std::vector<int> usedChannels;
			int numMaps = polyMesh->MNum();
			//start at 1 because channel 0 is reserve for vertex colors
			for(int mp=1; mp<numMaps; mp++){
				MNMap* map = polyMesh->M(mp);
				if(!map){
					continue;
				}
				if(map->numv <= 0 || map->numf <= 0){
					continue;
				}
				usedChannels.push_back(mp);
			}

			mIndexedUVSet.resize(usedChannels.size());
			
			for(int i=0; i<usedChannels.size(); i++){
				IndexedUVs &indexedUVs = mIndexedUVSet[i];
				GetIndexedUVsFromChannel( polyMesh, usedChannels[i], indexedUVs );
			}
      }
      else if (triMesh != NULL)
      {
			std::vector<int> usedChannels;
			int numMaps = triMesh->getNumMaps();
			//start at 1 because channel 0 is reserve for vertex colors
			for(int mp=1; mp<numMaps; mp++){
				MeshMap& map = triMesh->Map( mp);
				if(map.vnum <= 0 || map.fnum <= 0){
					continue;
				}
				usedChannels.push_back(mp);
			}

			mIndexedUVSet.resize(usedChannels.size());
			
			for(int i=0; i<usedChannels.size(); i++){
				IndexedUVs &indexedUVs = mIndexedUVSet[i];
				GetIndexedUVsFromChannel( triMesh, usedChannels[i], indexedUVs );
			}
		}		
	}

	// sweet, now let's have a look at face sets (really only for first sample)
	// for 3DS Max, we are mapping this to the material ids
	//std::vector<boost::int32_t> zeroFaceVector;
	if(options.GetBoolOption("exportMaterialIds"))
	{
		int numMatId = 0;
		int numFaces = polyMesh ? polyMesh->numf : triMesh->getNumFaces();
		mMatIdIndexVec.resize(numFaces);
		for (int i = 0; i < numFaces; i += 1)
		{
		  int matId;
		  if(nMatId >= 0){
		    matId = nMatId;
		  }
		  else if(polyMesh){
            matId = polyMesh->f[i].material;
		  }
		  else{
            matId = triMesh->faces[i].getMatID();
		  }
		  
		  int originalMatId = matId;
		  if(pMatMerge){
		      matId = pMatMerge->getUniqueMatId(matId);
		  }
		  mMatIdIndexVec[i] = matId;

		  // Record the face set map if sample zero
		  if (bFirstFrame || pMatMerge)
		  {
			  facesetmap_it it;
			  it = mFaceSets.find(matId);

			  if (it == mFaceSets.end())
			  {
				  FaceSetStruct faceSet;
				  faceSet.faceIds = std::vector<Abc::int32_t>();
				  faceSet.originalMatId = originalMatId;
				  facesetmap_ret_pair ret = mFaceSets.insert( facesetmap_insert_pair(matId, faceSet) );
				  it = ret.first;
				  numMatId++;
			  }

			  it->second.faceIds.push_back(i);
		  }
		}

		// For sample zero, export the material ids as face sets
		if (bFirstFrame || pMatMerge)
		{

		  for ( facesetmap_it it=mFaceSets.begin(); it != mFaceSets.end(); it++)
		  {
			  int i = it->first; 

			  Mtl* pSubMat = NULL;
			  const int numMat = pMtl ? pMtl->NumSubMtls() : 0;
			  if(pMtl && i < numMat){
				  pSubMat = pMtl->GetSubMtl(i);
			  }
			  std::stringstream nameStream;
			  int nMaterialId = it->first+1;
			  if(pSubMat){
				  nameStream<<pSubMat->GetName();
			  }
			  else if(pMtl){
				  nameStream<<pMtl->GetName();
			  }
			  else{
				  nameStream<<"Unassigned";
			  }

			  it->second.name = nameStream.str();

			  if(pMatMerge){
			      pMatMerge->setMatName(it->second.originalMatId, nameStream.str());
			  }
		  }
		}
		
	}

   meshData.free();
      // check if we need to export the bindpose (also only for first frame)
	  /*
	if (bFirstFrame || dynamicTopology)
      if(GetJob()->GetOption(L"exportBindPose") && prim.GetParent3DObject().GetEnvelopes().GetCount() > 0 && mNumSamples == 0)
      {
         mBindPoseProperty = OV3fArrayProperty(mMeshSchema, ".bindpose", mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs());

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

         Abc::V3fArraySample sample;
         if(mBindPoseVec.size() > 0)
            sample = Abc::V3fArraySample(&mBindPoseVec.front(),mBindPoseVec.size());
         mBindPoseProperty.set(sample);
      }
   }
      */

   // check if we should export the velocities
   /*if(dynamicTopology)
   {
      ICEAttribute velocitiesAttr = mesh.GetICEAttributeFromName(L"PointVelocity");
      if(velocitiesAttr.IsDefined() && velocitiesAttr.IsValid())
      {
         CICEAttributeDataArrayVector3f velocitiesData;
         velocitiesAttr.GetDataArray(velocitiesData);

         if(!mVelocityProperty.valid())
            mVelocityProperty = OV3fArrayProperty(mMeshSchema, ".velocities", mMeshSchema.getMetaData(), GetJob()->GetAnimatedTs());

         mVelocitiesVec.resize(vertCount);
         for(LONG i=0;i<vertCount;i++)
         {
            mVelocitiesVec[i].x = velocitiesData[i].GetX();
            mVelocitiesVec[i].y = velocitiesData[i].GetY();
            mVelocitiesVec[i].z = velocitiesData[i].GetZ();
         }

         if(mVelocitiesVec.size() == 0)
            mVelocitiesVec.push_back(Abc::V3f(0,0,0));
         Abc::V3fArraySample sample = Abc::V3fArraySample(&mVelocitiesVec.front(),mVelocitiesVec.size());
         mVelocityProperty.set(sample);
      }
   }
   */

}




void IntermediatePolyMesh3DSMax::make_face_uv(Face *f, Point3 *tv)
{
	int na,nhid,i;
	Point3 *basetv;
	/* make the invisible edge be 2->0 */
	nhid = 2;
	if (!(f->flags&EDGE_A))  nhid=0;
	else if (!(f->flags&EDGE_B)) nhid = 1;
	else if (!(f->flags&EDGE_C)) nhid = 2;
	na = 2-nhid;
	basetv = (f->v[prevpt[nhid]]<f->v[nhid]) ? basic_tva : basic_tvb; 
	for (i=0; i<3; i++) {  
		tv[i] = basetv[na];
		na = nextpt[na];
	}
}

BOOL IntermediatePolyMesh3DSMax::CheckForFaceMap(Mtl* mtl, Mesh* mesh)
{
    if (!mtl || !mesh) {
        return FALSE;
    }

    ULONG matreq = mtl->Requirements(-1);

    // Are we using face mapping?
    if (!(matreq & MTLREQ_FACEMAP)) {
        return FALSE;
    }

    return TRUE;
}


bool IntermediatePolyMesh3DSMax::mergeWith(const CommonIntermediatePolyMesh& srcMesh)
{
    ESS_PROFILE_FUNC();

   IntermediatePolyMesh3DSMax& destMesh = *this;
   const IntermediatePolyMesh3DSMax& srcMeshMax = (IntermediatePolyMesh3DSMax&)srcMesh;

   Abc::uint32_t amountToOffsetFaceIdBy = (Abc::uint32_t)destMesh.mFaceCountVec.size();

   bool bRes = CommonIntermediatePolyMesh::mergeWith(srcMesh);
   if(!bRes) return false;

	for(FaceSetMap::const_iterator it=srcMeshMax.mFaceSets.begin(); it != srcMeshMax.mFaceSets.end(); it++)
	{
		if( destMesh.mFaceSets.find(it->first) == destMesh.mFaceSets.end() ){ // a new key
			destMesh.mFaceSets[it->first] = it->second;
		}
		else{// the key is common

			const facesetmap_vec& srcFaceSetVec = it->second.faceIds;
			facesetmap_vec& destFaceSetVec = destMesh.mFaceSets[it->first].faceIds;

			for(int i=0; i<srcFaceSetVec.size(); i++){
				destFaceSetVec.push_back(amountToOffsetFaceIdBy + srcFaceSetVec[i]);
			}
		}
	}


	for(int i=0; i<srcMeshMax.mMatIdIndexVec.size(); i++)
   {
		destMesh.mMatIdIndexVec.push_back(srcMeshMax.mMatIdIndexVec[i]);
	}



	return true;
}

void IntermediatePolyMesh3DSMax::clear()
{
   //*this = IntermediatePolyMesh3DSMax();


}