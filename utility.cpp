#include "stdafx.h"
#include "Alembic.h"
#include "Utility.h"
#include "SceneEnumProc.h"
#include "AlembicIntermediatePolyMesh3DSMax.h"
#include <hash_map>

void logError( const char* msg ) {
	Exocortex::essLogError( msg );
}
void logWarning( const char* msg ) {
	Exocortex::essLogWarning( msg );
}
void logInfo( const char* msg ) {
	Exocortex::essLogInfo( msg );
}


std::string buildIdentifierFromRef(const SceneEntry &in_Ref)
{
   std::string result;
   INode *pNode = in_Ref.node;

   // Build the base string
   result = std::string("/")+ std::string( EC_MCHAR_to_UTF8( pNode->GetName() ) ) + result;
   result = std::string("/")+ std::string( EC_MCHAR_to_UTF8( pNode->GetName() ) ) + std::string("Xfo") + result;

   // Look for any nodes that we might add to the alembic hiearchy
   pNode = pNode->GetParentNode();
   while(pNode && !pNode->IsRootNode())
   {
       if (IsModelTransformNode(pNode))
           result = std::string("/")+ std::string( EC_MCHAR_to_UTF8( pNode->GetName() ) ) + std::string("Xfo") + result;
       
       pNode = pNode->GetParentNode();
   }

   return result;
}

/*std::string buildModelIdFromXFormId(const std::string &xformId)
{
    size_t start = xformId.rfind("/");
    start += 1;
    size_t end = xformId.rfind("Xfo");
    end -=1;
    std::string modelName = xformId.substr(start, end-start+1);
    modelName = xformId + std::string("/") + modelName;
    return modelName;
}*/

std::string getIdentifierFromRef(const SceneEntry &in_Ref)
{
    return in_Ref.fullname;
}

/*std::string getModelFullName( const std::string &identifier )
{
    // Max Scene nodes are also identified by their transform nodes since an INode contains
    // both the transform and the shape.  So if we find an "xfo" at the end of the identifier
    // then we extract the model name from the identifier
    std::string modelName;
    size_t pos = identifier.rfind("Xfo", identifier.length()-3, 3);
    if (pos == identifier.npos)
    {
        modelName = identifier;
    }
    else
    {
        size_t start = identifier.rfind("/");
        start += 1;
        modelName = identifier.substr(start, identifier.length()-4);
        modelName = identifier + std::string("/") + modelName;
    }

    return modelName;
}*/

void RoundTicksToNearestFrame( int& nTicks, float& fTimeAlpha )
{
	int nOrigTicks = nTicks;
	float fTicks = (float)nTicks/GetTicksPerFrame();
	nTicks = (int)floor(fTicks);
	fTimeAlpha = (fTicks - nTicks)/GetFrameRate();
	nTicks *= GetTicksPerFrame();

	//ESS_LOG_WARNING("TicksPerFrame: "<<GetTicksPerFrame()<<" PolyMesh tick: "<<nOrigTicks<<" roundedTick: "<<nTicks<<" roundedTimeAlpha: "<<fTimeAlpha);
}

double GetSecondsFromTimeValue(TimeValue t)
{ 
    return double(t)/double(GetTicksPerFrame())/double(GetFrameRate()); 
}

int GetTimeValueFromSeconds( double seconds )
{
    double ticks = ( seconds * GetFrameRate() * GetTicksPerFrame() );
    return (int)floor(ticks + 0.5);
}

int GetTimeValueFromFrame( double frame )
{
    double ticks = frame * GetTicksPerFrame();
    return (int)floor(ticks + 0.5f);
}

/*
void AlembicDebug_PrintMeshData( Mesh &mesh, std::vector<VNormal> &sgVertexNormals )
{
    for (int i=0; i<mesh.getNumFaces(); i++) 
    {
        Face *f = &mesh.faces[i];
        
		ESS_LOG_INFO("Mesh Face: " << i);
        ESS_LOG_INFO("============");

        for (int j = 0; j < 3; j += 1)
        {
            int vertexId = f->getVert(j);
            Point3 vertexPos = mesh.getVert(vertexId);
			Point3 vertexNormal = IntermediatePolyMesh3DSMax::GetVertexNormal(&mesh, i, vertexId, sgVertexNormals);
            int matId = f->getMatID();
			ESS_LOG_INFO("Vertex " << vertexId <<
				", Position (" << vertexPos.x << ", " << vertexPos.y << ", " << vertexPos.z <<
				"), Normal (" << vertexNormal.x << ", " << vertexNormal.y << ", " << vertexNormal.z << 
                "), MaterialId (" << matId << ")" );
        }
        ESS_LOG_INFO("");
    }
}*/

void AlembicDebug_PrintTransform(Matrix3 &m)
{
    ESS_LOG_INFO("Matrix");
    ESS_LOG_INFO("======");

    for (int i=0; i < 4; i += 1)
    {
        ESS_LOG_INFO("Row " << i << " (" << m.GetRow(i).x << ", " << m.GetRow(i).y << ", " << m.GetRow(i).z << " )" );
    }
}


void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Matrix3 &result)
{
    // Rotate the max matrix into an alembic reference frame, a right handed co-ordinate system
    // We set up an alembic reference frame relative to Max's coordinate system
    static Matrix3 AlembicRefFrame(Point3(1,0,0), Point3(0,0,1), Point3(0,-1,0), Point3(0,0,0));
    static Matrix3 AlembicRefFrameInverse = AlembicRefFrame;
    static bool isMatrixInverted = false;
    if( ! isMatrixInverted ) {
      AlembicRefFrameInverse.Invert();
      isMatrixInverted = true;
    }
    result = AlembicRefFrame * maxMatrix * AlembicRefFrameInverse;

    // Scale the translation
    //Point3 meterTrans = result.GetTrans();
    //result.SetTrans(meterTrans);
}
void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Abc::M44d& iMatrix )
{
	Matrix3 alembicMatrix;
    ConvertMaxMatrixToAlembicMatrix(maxMatrix, alembicMatrix);
    iMatrix = Abc::M44d( alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
                                 alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
                                 alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
                                 alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);
}

void ConvertAlembicMatrixToMaxMatrix( const Matrix3 &alembicMatrix, Matrix3 &result)
{
    // Rotate the max matrix into an alembic reference frame, a right handed co-ordinate system
    // We set up an alembic reference frame relative to Max's coordinate system
    static Matrix3 AlembicRefFrame(Point3(1,0,0), Point3(0,0,1), Point3(0,-1,0), Point3(0,0,0));
    static Matrix3 AlembicRefFrameInverse = AlembicRefFrame;
    static bool isMatrixInverted = false;
    if( ! isMatrixInverted ) {
      AlembicRefFrameInverse.Invert();
      isMatrixInverted = true;
    }
    result = AlembicRefFrameInverse * alembicMatrix * AlembicRefFrame;

    // Scale the translation
    //Point3 inchesTrans = result.GetTrans();
    //result.SetTrans(inchesTrans);
}



bool CheckIfNodeIsAnimated( INode *pNode )
{
    INode *pAnimatedNode = pNode;
    while (pAnimatedNode)
    {
        if (pAnimatedNode->IsRootNode())
            return false;

        if (pAnimatedNode->IsAnimated())
            return true;

        pAnimatedNode = pAnimatedNode->GetParentNode();
    }

    return false;
}

bool CheckIfObjIsValidForever(Object *obj, TimeValue v)
{
	Interval interval = obj->ObjectValidity(v); 
	return interval.Start() == TIME_NegInfinity && interval.End() == TIME_PosInfinity;
} 

bool IsModelTransformNode( INode *pNode )
{
    return true;
}

INode *GetParentModelTransformNode( INode *pNode )
{
    INode *pModelTransformNode = pNode;

    while (pModelTransformNode)
    {
        pModelTransformNode = pModelTransformNode->GetParentNode();

        if (pModelTransformNode->IsRootNode())
            return 0;

        if (IsModelTransformNode(pModelTransformNode))
            return pModelTransformNode;
    }

    return 0;
}

void LockNodeTransform(INode *pNode, bool bLock)
{
    if (pNode)
    {
        BOOL bBigBoolLock = bLock ? TRUE:FALSE;

        // Position
        pNode->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_X, bBigBoolLock);
        pNode->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_Y, bBigBoolLock);
        pNode->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_Z, bBigBoolLock);

         // Rotation
        pNode->SetTransformLock(INODE_LOCKROT, INODE_LOCK_X, bBigBoolLock);
        pNode->SetTransformLock(INODE_LOCKROT, INODE_LOCK_Y, bBigBoolLock);
        pNode->SetTransformLock(INODE_LOCKROT, INODE_LOCK_Z, bBigBoolLock);

         // Scale
        pNode->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_X, bBigBoolLock);
        pNode->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_Y, bBigBoolLock);
        pNode->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_Z, bBigBoolLock);
    }
}

int GetParamIdByName( Animatable *pBaseObject, int pblockIndex, char const* pParamName ) 
{
	IParamBlock2 * pParamBlock2 = pBaseObject->GetParamBlockByID( pblockIndex );
	for( int iParamID = 0; iParamID < pParamBlock2->NumParams(); iParamID ++ ) 
    {
		ParamDef &paramDef = pParamBlock2->GetParamDef( iParamID );
		if( strcmp( EC_MCHAR_to_UTF8( paramDef.int_name ).c_str(), pParamName ) == 0 ) 
        {
			return iParamID;
		}
	}

	return -1;
}

// Return a pointer to a TriObject given an INode or return NULL
// if the node cannot be converted to a TriObject
TriObject* GetTriObjectFromNode(INode *iNode, const TimeValue t, bool &deleteIt)
{
	deleteIt = false;
	if (iNode == NULL) return NULL;
	Object *obj = iNode->EvalWorldState(t).obj;
	obj = GetPFObject(obj);
	if (obj == NULL) return NULL;
	if (obj->IsParticleSystem()) return NULL;
    if (obj->SuperClassID()==GEOMOBJECT_CLASS_ID) {
		if (obj->IsSubClassOf(triObjectClassID)) {
		  return (TriObject*)obj;
		} else if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) { 
			TriObject *tri = (TriObject *) obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			// Note that the TriObject should only be deleted
			// if the pointer to it is not equal to the object
			// pointer that called ConvertToType()
			if (obj != tri) deleteIt = true;
			return tri;
		} else {
			return NULL;
		}
	} else {
		return NULL;
	}
}

std::string alembicPathToMaxPath(const std::string& path)
{
   std::vector<std::string> parts;

   boost::split(parts, path, boost::is_any_of("/"));

   if(parts.size() > 2){
	   parts.pop_back();
   }

   std::stringstream result;

   for(int i=0; i<parts.size(); i++){
	   parts[i] = removeXfoSuffix(parts[i]);
       result << parts[i];
       if(i < parts.size()-1) result << "/";
   }

   return result.str();
}

INode* GetNodeFromHierarchyPath(const std::string& path)
{
	std::vector<std::string> parts;
	
	boost::split(parts, path, boost::is_any_of("/"));
	
	if(parts.size() > 2){
		//delete the parent transform of the leaf node, since these nodes were merged on import
		parts[parts.size()-2] = parts[parts.size()-1];
		parts.pop_back();
	}

	if(parts.size() >= 2){
		for(int i=0; i<parts.size()-1; i++){
			parts[i] = removeXfoSuffix(parts[i]);
		}
	}

    INode* pNode = GET_MAX_INTERFACE()->GetRootNode();

    if(!pNode){
      return NULL;
    }

	for(int p=parts.size()-1; p>=1; p--){

	  std::string childName = parts[p];

      bool bFound = false;
	  for(int i=0; i<pNode->NumberOfChildren(); i++){

	     INode* childNode = pNode->GetChildNode(i);

         //const char* cName = childNode->GetName();

	     if (strcmp( EC_MCHAR_to_UTF8( childNode->GetName() ).c_str(), childName.c_str()) == 0){
		     bFound = true;
		     pNode = childNode;
	     }
	  } 
      if(!bFound){
         return NULL;
      }
	}
      
   return pNode;
}




typedef std::map<std::string, INode*> INodeMap;

void buildINodeMapForChild(INodeMap& nodeMap, INode* node, std::string path)
{
   nodeMap[path] = node;

   //ESS_LOG_WARNING("caching INode "<<path);

   path += "/";
   for(int i=0; i<node->NumberOfChildren(); i++){
      INode* childNode = node->GetChildNode(i);
      buildINodeMapForChild(nodeMap, childNode, path + EC_MCHAR_to_UTF8(childNode->GetName()));
   }
}

void buildINodeMap(INodeMap& nodeMap)
{
   INode* node = GET_MAX_INTERFACE()->GetRootNode();
   std::string path("/");
   for(int i=0; i<node->NumberOfChildren(); i++){
      INode* childNode = node->GetChildNode(i);
      buildINodeMapForChild(nodeMap, childNode, path + EC_MCHAR_to_UTF8(childNode->GetName()));
   }
}


class NodeFindByName : public ITreeEnumProc
{
public:

	std::string name;
	INode* pRetNode;

	NodeFindByName(const std::string& name):name(name), pRetNode(NULL)
	{}

	int callback( INode* node )
	{

		//skip the first entry because we split based on slash, and the path starts with a a slash,
		//so the first entry is an empty string
		if (strcmp( EC_MCHAR_to_UTF8( node->GetName() ).c_str(), name.c_str()) == 0)
		{
			pRetNode = node;
			return TREE_ABORT;
		}        

		return TREE_CONTINUE;
	}

};

//INode* GetNodeFromName(const std::string& name)
//{
//	NodeFindByName resolver(name);
//    IScene *pScene = GET_MAX_INTERFACE()->GetScene();
//    pScene->EnumTree(&resolver);
//	return resolver.pRetNode;
//}
//
INode* GetChildNodeFromName(const std::string& name, INode* pParent)
{
   if(!pParent){
	  pParent = GET_MAX_INTERFACE()->GetRootNode();
   }

	std::string pname = EC_MCHAR_to_UTF8( pParent->GetName() );
	for(int i=0; i<pParent->NumberOfChildren(); i++){
		INode* childNode = pParent->GetChildNode(i);
		std::string cname = EC_MCHAR_to_UTF8( childNode->GetName() );

		if (strcmp(cname.c_str(), name.c_str()) == 0){
			return childNode;	
		}
	}
	return NULL;
}



class HierarchyPathConstructor : public ITreeEnumProc
{
public:

	std::string nodeName;
	std::string path;
	std::vector<std::string> pathNodeNames;
	bool bFlatten;

	HierarchyPathConstructor(const std::string& n, bool bFlatten):nodeName(n), bFlatten(bFlatten)
	{

	}

	void walkToParent(INode* node)
	{
		INode* pParent = node->GetParentNode();
		if(pParent){
			std::string name = EC_MCHAR_to_UTF8( node->GetName() );
			pathNodeNames.push_back(name);
			walkToParent(pParent);
		}
		else{
			//Note: the hidden 3DS max root node should not be included in the path
		
			if(pathNodeNames.size() > 0){
				int nStart = (int)pathNodeNames.size()-1;
				
				for(int i=nStart; i>=0; i--){
					path+="/";
					path+=pathNodeNames[i];
					path+="Xfo";
				}

				path+="/";
				path+=pathNodeNames[0];
			}
			
		}
	}

	int callback( INode* node )
	{
		int enumCode = TREE_CONTINUE;

		std::string name = EC_MCHAR_to_UTF8( node->GetName() );
		if (strcmp(name.c_str(), nodeName.c_str()) == 0)
		{
			walkToParent(node);
			return TREE_ABORT;
		}        

		return TREE_CONTINUE;
	}

};

std::string getNodeAlembicPath(const std::string& name, bool bFlatten){
	if(bFlatten){
		 std::stringstream nameStream;
		 nameStream<<"/"<<name<<"Xfo/"<<name;
		 return nameStream.str();
	}

	HierarchyPathConstructor pathConstructor(name, bFlatten);
    IScene *pScene = GET_MAX_INTERFACE()->GetScene();
    pScene->EnumTree(&pathConstructor);
	return pathConstructor.path;
}

Modifier* FindModifier(INode* node, char* name)
{
	int i = 0;
	int idx = 0;
	Modifier* pRetMod = NULL;
	while(true){
		Modifier* pMod;
		IDerivedObject* pObj = GET_MAX_INTERFACE()->FindModifier(*node, i, idx, pMod);
		if(!pObj){
			break;
		}

		if(strstr( EC_MSTR_to_UTF8( pMod->GetName() ).c_str(), name) != NULL){
			pRetMod = pMod;
			break;
		}

		//const char* cname = pObj->GetClassName();
		//const char* oname = EC_MCHAR_to_UTF8( pMod->GetObjectName() );
		//const char* name = EC_MSTR_to_UTF8( pMod->GetName() );
		i++;
	}

	return pRetMod;
}

void printControllers(Animatable* anim, int level)
{
	for(int i=0; i<anim->NumSubs(); i++){
		Animatable* an = anim->SubAnim(i);

		if(!an){
			continue;
		}

		MSTR name;
		an->GetClassName(name);
		ESS_LOG_WARNING("("<<level<<", "<<i<<"):"<<name);

		printControllers(an, level+1);
	}

}

void printControllers(Animatable* anim)
{
	if(!anim){
		return;
	}

	MSTR name;
	anim->GetClassName(name);
	ESS_LOG_WARNING("Printing controllers of "<<name);
	printControllers(anim, 0);
	ESS_LOG_WARNING("Done printing");
}

Modifier* FindModifier(INode* node, Class_ID obtype, const char* identifier)
{
    TimeValue zero(0);
    int i = 0;
    int idx = 0;
    Modifier* pRetMod = NULL;
    while(true){
        Modifier* pMod;
        IDerivedObject* pObj = GET_MAX_INTERFACE()->FindModifier(*node, i, idx, pMod);
        if(!pObj){
            break;
        }

        //const char* cname = pObj->GetClassName();
        std::string oname = EC_MCHAR_to_UTF8( pMod->GetObjectName() );
        std::string  name = EC_MSTR_to_UTF8( pMod->GetName() );

		if(pMod->ClassID() == obtype){
            
            IParamBlock2 *pBlock = pMod->GetParamBlockByID(0);
            if(!pBlock){
                continue;
            }

            //const char* modPath = pBlock->GetStr(GetParamIdByName(pMod, 0, "path"), zero);
            std::string modIdentifier = EC_MCHAR_to_UTF8( pBlock->GetStr(GetParamIdByName(pMod, 0, "identifier"), zero) );

            if(/*strcmp(modPath, path) == 0 && */strcmp(modIdentifier.c_str(), identifier) == 0){
                pRetMod = pMod;
                break;
            }

            
        }

        i++;
    }

    return pRetMod;
}

Modifier* FindModifier(INode* node, Class_ID obtype)
{
    TimeValue zero(0);
    int i = 0;
    int idx = 0;
    Modifier* pRetMod = NULL;
    while(true){
        Modifier* pMod;
        IDerivedObject* pObj = GET_MAX_INTERFACE()->FindModifier(*node, i, idx, pMod);
        if(!pObj){
            break;
        }

        //const char* cname = pObj->GetClassName();
        //std::string oname = EC_MCHAR_to_UTF8( pMod->GetObjectName() );
        //std::string name = EC_MSTR_to_UTF8( pMod->GetName() );

		if(pMod->ClassID() == obtype){
            pRetMod = pMod;
            break;  
        }

        i++;
    }

    return pRetMod;
}

void printAnimatables(Animatable* pObj){

	ESS_LOG_INFO("Printing Animatables: ");
	int num0 = pObj->NumSubs();
	for(int i=0; i<num0; i++){
		Animatable* an0 = pObj->SubAnim(i);
		MSTR n0 = pObj->SubAnimName(i);
		ESS_LOG_INFO( "("<<i<<"): "<<n0 );

		int num1 = an0->NumSubs();
		for(int j=0; j<num1; j++){
			Animatable* an1 = an0->SubAnim(j);
			MSTR n1 = an0->SubAnimName(j);
			ESS_LOG_INFO( "("<<i<<", "<<j<<"): "<<n1 );

			int num2 = an1->NumSubs();
			for(int k=0; k<num2; k++){
				Animatable* an2 = an1->SubAnim(k);
				MSTR n2 = an1->SubAnimName(k);
				ESS_LOG_INFO( "("<<i<<", "<<j<<", "<<k<<"): "<<n2 );
			}
		}
	}
	ESS_LOG_INFO("DONE.");
}


Matrix3 TransposeRot(const Matrix3& mat){
	//std::swap(mat.m[1][0], mat.m[0][1]);
	//std::swap(mat.m[2][0], mat.m[0][2]);
	//std::swap(mat.m[2][1], mat.m[1][2]);

	Point3 row1 = mat.GetRow(0);
	Point3 row2 = mat.GetRow(1);
	Point3 row3 = mat.GetRow(2);
	Point3 row4 = mat.GetRow(3);

	Point3 nrow1;
	Point3 nrow2;
	Point3 nrow3;

	nrow1.x = row1.x;
	nrow1.y = row2.x;
	nrow1.z = row3.x;

	nrow2.x = row1.y;
	nrow2.y = row2.y;
	nrow2.z = row3.y;

	nrow3.x = row1.z;
	nrow3.y = row2.z;
	nrow3.z = row3.z;

	return Matrix3(nrow1, nrow2, nrow3, row4);
}


Point3 SmoothGroupNormals::GetVertexNormal(Mesh *mesh, int faceNo, int faceVertNo)
{
	// If we do not a smoothing group, we can't base ourselves on anything else,
	// so we can just return the face normal.
	Face *face = &mesh->faces[faceNo];
	if (face == NULL || face->smGroup == 0)
	{
		return mesh->getFaceNormal(faceNo);
	}

	// Check to see if there is a smoothing group normal
	int vertIndex = face->v[faceVertNo];
	Point3 normal = m_MeshSmoothGroupNormals[vertIndex].GetNormal(face->smGroup);

	if (normal.LengthSquared() > 0.0f)
	{
		return normal.Normalize();
	}

	// If we did not find any normals or the normals offset each other for some
	// reason, let's just let max tell us what it thinks the normal should be.
	return mesh->getNormal(vertIndex);
}

Point3 SmoothGroupNormals::GetVertexNormal(MNMesh *mesh, int faceNo, int faceVertNo)
{
	// If we do not a smoothing group, we can't base ourselves on anything else,
	// so we can just return the face normal.
	MNFace *face = mesh->F(faceNo);
	if (face == NULL || face->smGroup == 0)
	{
		return mesh->GetFaceNormal(faceNo);
	}

	// Check to see if there is a smoothing group normal
	int vertIndex = face->vtx[faceVertNo];
	Point3 normal = m_MeshSmoothGroupNormals[vertIndex].GetNormal(face->smGroup);

	if (normal.LengthSquared() > 0.0f)
	{
		return normal.Normalize();
	}

	// If we did not find any normals or the normals offset each other for some
	// reason, let's just let max tell us what it thinks the normal should be.
	return mesh->GetVertexNormal(vertIndex);
}

void SmoothGroupNormals::BuildMeshSmoothingGroupNormals(Mesh &mesh)
{
	m_MeshSmoothGroupNormals.resize(mesh.numVerts);
    
	for (int i = 0; i < mesh.numFaces; i++) 
	{     
		Face *face = &mesh.faces[i];
		Point3 faceNormal = mesh.getFaceNormal(i);
		for (int j=0; j<3; j++) 
		{       
			m_MeshSmoothGroupNormals[face->v[j]].AddNormal(faceNormal, face->smGroup);     
		}     
	}   
    
	for (int i=0; i < mesh.numVerts; i++) 
	{     
		m_MeshSmoothGroupNormals[i].Normalize(); 
	}
}

void SmoothGroupNormals::BuildMeshSmoothingGroupNormals(MNMesh &mesh)
{
	m_MeshSmoothGroupNormals.resize(mesh.numv);
    
	for (int i = 0; i < mesh.numf; i++) 
	{     
		MNFace *face = &mesh.f[i];
		Point3 faceNormal = mesh.GetFaceNormal(i);
		for (int j=0; j<face->deg; j++) 
		{       
			m_MeshSmoothGroupNormals[face->vtx[j]].AddNormal(faceNormal, face->smGroup);     
		}     
	}   
    
	for (int i=0; i < mesh.numv; i++) 
	{     
		m_MeshSmoothGroupNormals[i].Normalize();   
	}
}

void SmoothGroupNormals::ClearMeshSmoothingGroupNormals()
{
	for (int i=0; i < m_MeshSmoothGroupNormals.size(); i++) 
	{   
		VNormal *ptr = m_MeshSmoothGroupNormals[i].next;
		while (ptr)
		{
			VNormal *tmp = ptr;
			ptr = ptr->next;
			delete tmp;
		}
	}

	m_MeshSmoothGroupNormals.clear();  
}

void printChannelIntervals(TimeValue t, Object* obj)
{
   Interval topoInterval = obj->ChannelValidity(t, TOPO_CHAN_NUM);
   ESS_LOG_WARNING( "TopoInterval Start: " << topoInterval.Start() << " End: " << topoInterval.End() );

   Interval geoInterval = obj->ChannelValidity(t, GEOM_CHAN_NUM);
   ESS_LOG_WARNING( "GeoInterval Start: " << geoInterval.Start() << " End: " << geoInterval.End() );

   Interval texInterval = obj->ChannelValidity(t, TEXMAP_CHAN_NUM);
   ESS_LOG_WARNING( "TexInterval Start: " << texInterval.Start() << " End: " << texInterval.End() );
}


int createNode(AbcG::IObject& iObj, SClass_ID superID, Class_ID classID, INode** pMaxNode, bool& bReplaceExisting)
{
   INode *pNode = *pMaxNode;

   if(!pNode){
      Object* newObject = reinterpret_cast<Object*>( GET_MAX_INTERFACE()->CreateInstance(superID, classID) );
      if (newObject == NULL){
         return alembic_failure;
      }
      std::string name = removeXfoSuffix(iObj.getParent().getName().c_str());
      pNode = GET_MAX_INTERFACE()->CreateObjectNode(newObject, EC_UTF8_to_TCHAR(name.c_str()));
      if (pNode == NULL){
         return alembic_failure;
      }
      *pMaxNode = pNode;
   }
   else{
      bReplaceExisting = true;
   }

   return alembic_success;
}
