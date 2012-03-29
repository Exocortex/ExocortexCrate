#include "Alembic.h"
#include "AlembicMax.h"
#include "Utility.h"
#include "SceneEnumProc.h"
#include "AlembicPolyMsh.h"

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
            Point3 vertexNormal = AlembicPolyMesh::GetVertexNormal(&mesh, i, vertexId, sgVertexNormals);
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


void ConvertMaxMatrixToAlembicMatrix( const Matrix3 &maxMatrix, const float& masterScaleUnitMeters, Matrix3 &result)
{
    // Rotate the max matrix into an alembic reference frame, a right handed co-ordinate system
    // We set up an alembic reference frame relative to Max's coordinate system
    Matrix3 AlembicRefFrame(Point3(1,0,0), Point3(0,0,1), Point3(0,-1,0), Point3(0,0,0));
    Matrix3 AlembicRefFrameInverse = AlembicRefFrame;
    AlembicRefFrameInverse.Invert();
    result = AlembicRefFrame * maxMatrix * AlembicRefFrameInverse;

    // Scale the translation
    Point3 meterTrans = result.GetTrans() * GetInchesToDecimetersRatio( masterScaleUnitMeters );
    result.SetTrans(meterTrans);
}
   
void ConvertAlembicMatrixToMaxMatrix( const Matrix3 &alembicMatrix, const float& masterScaleUnitMeters, Matrix3 &result)
{
    // Rotate the max matrix into an alembic reference frame, a right handed co-ordinate system
    // We set up an alembic reference frame relative to Max's coordinate system
    Matrix3 AlembicRefFrame(Point3(1,0,0), Point3(0,0,1), Point3(0,-1,0), Point3(0,0,0));
    Matrix3 AlembicRefFrameInverse = AlembicRefFrame;
    AlembicRefFrameInverse.Invert();
    result = AlembicRefFrameInverse * alembicMatrix * AlembicRefFrame;

    // Scale the translation
    Point3 inchesTrans = result.GetTrans() * GetDecimetersToInchesRatio( masterScaleUnitMeters );
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

bool IsModelTransformNode( INode *pNode )
{
    return false;
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