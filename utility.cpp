#include "Alembic.h"
#include "AlembicMax.h"
#include "Utility.h"
#include "SceneEnumProc.h"
#include "AlembicIntermediatePolyMesh3DSMax.h"
#include <boost/algorithm/string.hpp>

SampleInfo getSampleInfo
(
   double iFrame,
   Alembic::AbcCoreAbstract::TimeSamplingPtr iTime,
   size_t numSamps
)
{
   SampleInfo result;
   if (numSamps == 0)
      numSamps = 1;

   std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
   iTime->getFloorIndex(iFrame, numSamps);

   result.floorIndex = floorIndex.first;
   result.ceilIndex = result.floorIndex;

   if (fabs(iFrame - floorIndex.second) < 0.0001) {
      result.alpha = 0.0f;
      return result;
   }

   std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
   iTime->getCeilIndex(iFrame, numSamps);

   if (fabs(iFrame - ceilIndex.second) < 0.0001) {
      result.floorIndex = ceilIndex.first;
      result.ceilIndex = result.floorIndex;
      result.alpha = 0.0f;
      return result;
   }

   if (result.floorIndex == ceilIndex.first) {
      result.alpha = 0.0f;
      return result;
   }

   result.ceilIndex = ceilIndex.first;

   result.alpha = (iFrame - floorIndex.second) / (ceilIndex.second - floorIndex.second);
   return result;
}

std::string buildIdentifierFromRef(const SceneEntry &in_Ref)
{
   std::string result;
   INode *pNode = in_Ref.node;

   // Build the base string
   result = std::string("/")+ std::string(pNode->GetName()) + result;
   result = std::string("/")+ std::string(pNode->GetName()) + std::string("Xfo") + result;

   // Look for any nodes that we might add to the alembic hiearchy
   pNode = pNode->GetParentNode();
   while(pNode && !pNode->IsRootNode())
   {
       if (IsModelTransformNode(pNode))
           result = std::string("/")+ std::string(pNode->GetName()) + std::string("Xfo") + result;
       
       pNode = pNode->GetParentNode();
   }

   return result;
}

std::string buildModelIdFromXFormId(const std::string &xformId)
{
    size_t start = xformId.rfind("/");
    start += 1;
    size_t end = xformId.rfind("Xfo");
    end -=1;
    std::string modelName = xformId.substr(start, end-start+1);
    modelName = xformId + std::string("/") + modelName;
    return modelName;
}

std::string getIdentifierFromRef(const SceneEntry &in_Ref)
{
    return in_Ref.fullname;
}

std::string getModelFullName( const std::string &identifier )
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
}

std::string getModelName( const std::string &identifier )
{
    // Max Scene nodes are also identified by their transform nodes since an INode contains
    // both the transform and the shape.  So if we find an "xfo" at the end of the identifier
    // then we extract the model name from the identifier
    std::string modelName;
    size_t pos = identifier.rfind("Xfo", identifier.length()-3, 3);
    if (pos == identifier.npos)
        modelName = identifier;
    else
        modelName = identifier.substr(0, identifier.length()-3);

    return modelName;
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
}

void AlembicDebug_PrintTransform(Matrix3 &m)
{
    ESS_LOG_INFO("Matrix");
    ESS_LOG_INFO("======");

    for (int i=0; i < 4; i += 1)
    {
        ESS_LOG_INFO("Row " << i << " (" << m.GetRow(i).x << ", " << m.GetRow(i).y << ", " << m.GetRow(i).z << " )" );
    }
}

Imath::M33d extractRotation(Imath::M44d& m)
{
	double values[3][3];

	for(int i=0; i<3; i++){
		for(int j=0; j<3; j++){
			values[i][j] = m[i][j];
		}
	}
	
	return Imath::M33d(values);
}

void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Matrix3 &result)
{
    // Rotate the max matrix into an alembic reference frame, a right handed co-ordinate system
    // We set up an alembic reference frame relative to Max's coordinate system
    Matrix3 AlembicRefFrame(Point3(1,0,0), Point3(0,0,1), Point3(0,-1,0), Point3(0,0,0));
    Matrix3 AlembicRefFrameInverse = AlembicRefFrame;
    AlembicRefFrameInverse.Invert();
    result = AlembicRefFrame * maxMatrix * AlembicRefFrameInverse;

    // Scale the translation
    Point3 meterTrans = result.GetTrans();
    result.SetTrans(meterTrans);
}
void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, Alembic::Abc::M44d& iMatrix )
{
	Matrix3 alembicMatrix;
    ConvertMaxMatrixToAlembicMatrix(maxMatrix, alembicMatrix);
    iMatrix = Alembic::Abc::M44d( alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
                                 alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
                                 alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
                                 alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);
}

void ConvertAlembicMatrixToMaxMatrix( const Matrix3 &alembicMatrix, Matrix3 &result)
{
    // Rotate the max matrix into an alembic reference frame, a right handed co-ordinate system
    // We set up an alembic reference frame relative to Max's coordinate system
    Matrix3 AlembicRefFrame(Point3(1,0,0), Point3(0,0,1), Point3(0,-1,0), Point3(0,0,0));
    Matrix3 AlembicRefFrameInverse = AlembicRefFrame;
    AlembicRefFrameInverse.Invert();
    result = AlembicRefFrameInverse * alembicMatrix * AlembicRefFrame;

    // Scale the translation
    Point3 inchesTrans = result.GetTrans();
    result.SetTrans(inchesTrans);
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

Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::IObject object)
{
   const Alembic::Abc::MetaData &md = object.getMetaData();
   if(Alembic::AbcGeom::IXform::matches(md)) {
      return Alembic::AbcGeom::IXform(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
      return Alembic::AbcGeom::IPolyMesh(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ICurves::matches(md)) {
      return Alembic::AbcGeom::ICurves(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
      return Alembic::AbcGeom::INuPatch(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::IPoints::matches(md)) {
      return Alembic::AbcGeom::IPoints(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ISubD::matches(md)) {
      return Alembic::AbcGeom::ISubD(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   } else if(Alembic::AbcGeom::ICamera::matches(md)) {
      return Alembic::AbcGeom::ICamera(object,Alembic::Abc::kWrapExisting).getSchema().getTimeSampling();
   }
   return Alembic::Abc::TimeSamplingPtr();
}


int GetParamIdByName( Animatable *pBaseObject, int pblockIndex, char const* pParamName ) 
{
	IParamBlock2 * pParamBlock2 = pBaseObject->GetParamBlockByID( pblockIndex );
	for( int iParamID = 0; iParamID < pParamBlock2->NumParams(); iParamID ++ ) 
    {
		ParamDef &paramDef = pParamBlock2->GetParamDef( iParamID );
		if( strcmp( paramDef.int_name, pParamName ) == 0 ) 
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

class HierarchyPathResolver : public ITreeEnumProc
{
public:

	std::vector<std::string> parts;
	INode* pRetNode;

	HierarchyPathResolver(const std::string& path):pRetNode(NULL)
	{
		boost::split(parts, path, boost::is_any_of("/"));
		
		if(parts.size() > 2){
			//delete the parent transform of the leaf node, since these nodes were merged on import
			parts[parts.size()-2] = parts[parts.size()-1];
			parts.pop_back();
		}
	}

	void walkToChild(INode* node, int& childIndex)
	{
		if(childIndex >= parts.size()){
			return;
		}
		std::string childName = parts[childIndex];
		
		for(int i=0; i<node->NumberOfChildren(); i++){

			INode* childNode = node->GetChildNode(i);

			if (strcmp(childNode->GetName(), childName.c_str()) == 0){
				childIndex++;
				pRetNode = childNode;
				walkToChild(childNode, childIndex);		
			}
		}
	}

	int callback( INode* node )
	{
		int enumCode = TREE_CONTINUE;

		if(parts.size() <= 1){
			return TREE_ABORT;
		}

		//skip the first entry because we split based on slash, and the path starts with a a slash,
		//so the first entry is an empty string
		const char* name = node->GetName();
		INode* pParent = node->GetParentNode();
		if (pParent && pParent->IsRootNode() && strcmp(node->GetName(), parts[1].c_str()) == 0)
		{
			int partIndex = 2;
			pRetNode = node;
			walkToChild(node, partIndex);
			return TREE_ABORT;
		}        

		return TREE_CONTINUE;
	}

};

INode* GetNodeFromHierarchyPath(const std::string& path)
{
	HierarchyPathResolver resolver(path);
    IScene *pScene = GET_MAX_INTERFACE()->GetScene();
    pScene->EnumTree(&resolver);
	return resolver.pRetNode;
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
		if (strcmp(node->GetName(), name.c_str()) == 0)
		{
			pRetNode = node;
			return TREE_ABORT;
		}        

		return TREE_CONTINUE;
	}

};

INode* GetNodeFromName(const std::string& name)
{
	NodeFindByName resolver(name);
    IScene *pScene = GET_MAX_INTERFACE()->GetScene();
    pScene->EnumTree(&resolver);
	return resolver.pRetNode;
}

INode* GetChildNodeFromName(const std::string& name, INode* pParent)
{
	if(pParent){
		const char* pname = pParent->GetName();
		for(int i=0; i<pParent->NumberOfChildren(); i++){
			INode* childNode = pParent->GetChildNode(i);
			const char* cname = childNode->GetName();

			if (strcmp(childNode->GetName(), name.c_str()) == 0){
				return childNode;	
			}
		}
		return NULL;
	}
	else{ //in this case, the node will have root node as parent, which is only exposed indirectly in max via EnumTree interface
		HierarchyPathResolver resolver(std::string("/") + name);
		IScene *pScene = GET_MAX_INTERFACE()->GetScene();
		pScene->EnumTree(&resolver);
		return resolver.pRetNode;
	}
}

