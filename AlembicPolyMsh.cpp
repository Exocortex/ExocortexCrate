#include "Alembic.h"
#include "AlembicPolyMsh.h"
#include "AlembicPolyMsh.h"
#include "AlembicXForm.h"
#include "SceneEntry.h"
#include <Object.h>
#include <triobj.h>
#include <IMetaData.h>
#include "Utility.h"

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

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

AlembicPolyMesh::AlembicPolyMesh(const SceneEntry &in_Ref, AlembicWriteJob *in_Job)
: AlembicObject(in_Ref, in_Job)
{
    std::string meshName = in_Ref.node->GetName();
    std::string xformName = meshName + "Xfo";

    Alembic::AbcGeom::OXform xform(GetOParent(), xformName.c_str(), GetCurrentJob()->GetAnimatedTs());
    Alembic::AbcGeom::OPolyMesh mesh(xform, meshName.c_str(), GetCurrentJob()->GetAnimatedTs());

    // JSS - I'm not sure if this is require under 3DSMAx
    // AddRef(prim.GetParent3DObject().GetKinematics().GetGlobal().GetRef());

    // create the generic properties
    mOVisibility = CreateVisibilityProperty(mesh,GetCurrentJob()->GetAnimatedTs());

    mXformSchema = xform.getSchema();
    mMeshSchema = mesh.getSchema();
}

AlembicPolyMesh::~AlembicPolyMesh()
{
    // we have to clear this prior to destruction this is a workaround for issue-171
    mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicPolyMesh::GetCompound()
{
    return mMeshSchema;
}

bool AlembicPolyMesh::Save(double time)
{
    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time);
    
    // store the metadata
    // IMetaDataManager mng;
    // mng.GetMetaData(GetRef().node, 0);
    // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

    // set the visibility
    if(GetRef().node->IsAnimated() || mNumSamples == 0)
    {
        mOVisibility.set(GetRef().node->GetPrimaryVisibility() ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);
    }

    // check if the mesh is animated (Otherwise, no need to export)
    if(mNumSamples > 0) 
    {
        if(!GetRef().node->IsAnimated())
        {
            return true;
        }
    }

    // check if we just have a pure pointcache (no surface)
    bool purePointCache = static_cast<bool>(GetCurrentJob()->GetOption("exportPurePointCache"));

    // define additional vectors, necessary for this task
    std::vector<Alembic::Abc::V3f> posVec;
    std::vector<Alembic::Abc::N3f> normalVec;
    std::vector<uint32_t> normalIndexVec;

    // Return a pointer to a TriObject given an INode or return NULL
    // if the node cannot be converted to a TriObject
    TimeValue ticks = GetTimeValueFromFrame(time);
    Object *obj = GetRef().node->EvalWorldState(ticks).obj;
    TriObject *triObj = 0;

    if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
    {
        triObj = (TriObject *) obj->ConvertToType(ticks, Class_ID(TRIOBJ_CLASS_ID, 0));

        // Make sure we have a tri object
        if (!triObj)
            return false;

        
    }
    else
    {
        return false;
    }

	Mesh &objectMesh = triObj->GetMesh();
    LONG vertCount = objectMesh.getNumVerts();

    // prepare the bounding box
    Alembic::Abc::Box3d bbox;

    // allocate the points and normals
    posVec.resize(vertCount);
    for(LONG i=0;i<vertCount;i++)
    {     
        Point3 alembicPoint; 
        ConvertMaxPointToAlembicPoint(objectMesh.getVert(i), alembicPoint);
        posVec[i].x = static_cast<float>(alembicPoint.x);
        posVec[i].y = static_cast<float>(alembicPoint.y);
        posVec[i].z = static_cast<float>(alembicPoint.z);
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
    mMeshSample.setPositions(posSample);
    mMeshSample.setSelfBounds(bbox);

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
            mMeshSample.setFaceCounts(faceCountSample);
            mMeshSample.setFaceIndices(faceIndicesSample);
        }

        mMeshSchema.set(mMeshSample);
        mNumSamples++;
        return true;
    }

	// check if we support changing topology
	bool dynamicTopology = static_cast<bool>(GetCurrentJob()->GetOption("exportDynamicTopology"));

	// Get the entire face count and index Count for the mesh
    LONG faceCount = objectMesh.getNumFaces();
	LONG sampleCount = faceCount * 3;
	
	// create an index lookup table
	std::vector<uint32_t> sampleLookup;
	sampleLookup.reserve(sampleCount);
	for(int f = 0; f < objectMesh.numFaces; f += 1)
    {
		for (int i = 0; i < 3; i += 1)
		{
			sampleLookup.push_back(objectMesh.faces[f].v[i]);
		}
	}

    // let's check if we have user normals
    size_t normalCount = 0;
    size_t normalIndexCount = 0;
    if((bool)GetCurrentJob()->GetOption("exportNormals"))
    {
        objectMesh.buildNormals();
        normalVec.resize(sampleCount);
      
        // Face and vertex normals.
        // In MAX a vertex can have more than one normal (but doesn't always have it).
        for (int i=0; i< objectMesh.getNumFaces(); i++) 
        {
            Face *f = &objectMesh.faces[i];
            for (int j = 2; j >= 0; j -= 1)
            {
                int vertexId = f->getVert(j);
                Point3 vertexNormal = GetVertexNormal(&objectMesh, i, objectMesh.getRVertPtr(vertexId));
                Point3 vertexAlembicNormal;
                ConvertMaxNormalToAlembicNormal(vertexNormal, vertexAlembicNormal);
                normalVec[normalCount].x = vertexAlembicNormal.x;
                normalVec[normalCount].y = vertexAlembicNormal.y;
                normalVec[normalCount].z = vertexAlembicNormal.z;
                normalCount += 1;
            }
        }

        // AlembicPrintFaceData(objectMesh);

        // now let's sort the normals 
        if((bool)GetCurrentJob()->GetOption("indexedNormals")) 
        {
            std::map<SortableV3f,size_t> normalMap;
            std::map<SortableV3f,size_t>::const_iterator it;
            size_t sortedNormalCount = 0;
            std::vector<Alembic::Abc::V3f> sortedNormalVec;
            normalIndexVec.resize(normalVec.size());
            sortedNormalVec.resize(normalVec.size());

            // loop over all normals
            for(size_t i=0;i<normalVec.size();i++)
            {
                it = normalMap.find(normalVec[i]);
                if(it != normalMap.end())
                    normalIndexVec[normalIndexCount++] = (uint32_t)it->second;
                else
                {
                    normalIndexVec[normalIndexCount++] = (uint32_t)sortedNormalCount;
                    normalMap.insert(std::pair<Alembic::Abc::V3f,size_t>(normalVec[i],(uint32_t)sortedNormalCount));
                    sortedNormalVec[sortedNormalCount++] = normalVec[i];
                }
            }

            // use indexed normals if they use less space
            if(sortedNormalCount * sizeof(Alembic::Abc::V3f) + 
                normalIndexCount * sizeof(uint32_t) < 
                sizeof(Alembic::Abc::V3f) * normalVec.size())
            {
                normalVec = sortedNormalVec;
                normalCount = sortedNormalCount;
            }
            else
            {
                normalIndexCount = 0;
                normalIndexVec.clear();
            }
            sortedNormalCount = 0;
            sortedNormalVec.clear();
        }
    }

	////////////////////////////////////////////////////////////////////////////////////////////////
	 // if we are the first frame!
   if(mNumSamples == 0 || (dynamicTopology))
   {
      // we also need to store the face counts as well as face indices
      if(mFaceIndicesVec.size() != sampleCount || sampleCount == 0)
      {
         mFaceCountVec.resize(faceCount);
         mFaceIndicesVec.resize(sampleCount);

         int offset = 0;
         for(LONG f=0;f<faceCount;f++)
         {
            mFaceCountVec[f] = 3;
			for (int i = 2; i >= 0; i -= 1)
            {
				mFaceIndicesVec[offset++] = objectMesh.faces[f].v[i];
            }
         }

         if(mFaceIndicesVec.size() == 0)
         {
            mFaceCountVec.push_back(0);
            mFaceIndicesVec.push_back(0);
         }
         Alembic::Abc::Int32ArraySample faceCountSample(&mFaceCountVec.front(),mFaceCountVec.size());
         Alembic::Abc::Int32ArraySample faceIndicesSample(&mFaceIndicesVec.front(),mFaceIndicesVec.size());

         mMeshSample.setFaceCounts(faceCountSample);
         mMeshSample.setFaceIndices(faceIndicesSample);
      }

      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
      if(normalVec.size() > 0 && normalCount > 0)
      {
         normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&normalVec.front(),normalCount));
         if(normalIndexCount > 0)
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&normalIndexVec.front(),normalIndexCount));
         mMeshSample.setNormals(normalSample);
      }

      // also check if we need to store UV
      if((bool)GetCurrentJob()->GetOption("exportUVs"))
      {
          if (CheckForFaceMap(GetRef().node->GetMtl(), &objectMesh)) 
          {
              mUvIndexVec.reserve(sampleCount);
              for (int i=0; i<objectMesh.getNumFaces(); i++) 
              {
                  Point3 tv[3];
                  Face* f = &objectMesh.faces[i];
                  make_face_uv(f, tv);

                  for (int j=2; j>=0; j-=1)
                  {
                      Alembic::Abc::V2f alembicUV(tv[j].x, tv[j].y);
                      mUvVec.push_back(alembicUV);
                  }
              }
          }
          else if (objectMesh.mapSupport(1))
          {
              mUvVec.reserve(sampleCount);
              MeshMap &map = objectMesh.Map(1);

              for (int findex =0; findex < map.fnum; findex += 1)
              {
                  TVFace &texFace = map.tf[findex];
                  for (int vindex = 2; vindex >= 0; vindex -= 1)
                  {
                      int vertexid = texFace.t[vindex];
                      UVVert uvVert = map.tv[vertexid];
                      Alembic::Abc::V2f alembicUV(uvVert.x, uvVert.y);
                      mUvVec.push_back(alembicUV);
                  }
              }
          }

          if (mUvVec.size() == sampleCount)
          {
              // now let's sort the uvs 
              size_t uvCount = mUvVec.size();
              size_t uvIndexCount = 0;
              if((bool)GetCurrentJob()->GetOption("indexedUVs")) 
              {
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
              mMeshSample.setUVs(uvSample);
          }
      }

      // sweet, now let's have a look at face sets (really only for first sample)
      /*
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
               Alembic::AbcGeom::OFaceSet faceSet = mMeshSchema.createFaceSet(name);
               Alembic::AbcGeom::OFaceSetSchema::Sample faceSetSample(Alembic::Abc::Int32ArraySample(&faceSetVec.front(),faceSetVec.size()));
               faceSet.getSchema().set(faceSetSample);
            }
         }
      }
	  */

      // save the sample
      mMeshSchema.set(mMeshSample);

      // check if we need to export the bindpose (also only for first frame)
	  /*
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

         Alembic::Abc::V3fArraySample sample;
         if(mBindPoseVec.size() > 0)
            sample = Alembic::Abc::V3fArraySample(&mBindPoseVec.front(),mBindPoseVec.size());
         mBindPoseProperty.set(sample);
      }   
	  */
   }
   else
   {
      Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
      if(normalVec.size() > 0 && normalCount > 0)
      {
         normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
         normalSample.setVals(Alembic::Abc::N3fArraySample(&normalVec.front(),normalCount));
         if(normalIndexCount > 0)
            normalSample.setIndices(Alembic::Abc::UInt32ArraySample(&normalIndexVec.front(),normalIndexCount));
         mMeshSample.setNormals(normalSample);
      }
      mMeshSchema.set(mMeshSample);
   }

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
            mVelocitiesVec.push_back(Alembic::Abc::V3f(0,0,0));
         Alembic::Abc::V3fArraySample sample = Alembic::Abc::V3fArraySample(&mVelocitiesVec.front(),mVelocitiesVec.size());
         mVelocityProperty.set(sample);
      }
   }
   */

   mNumSamples++;

   // Note that the TriObject should only be deleted
   // if the pointer to it is not equal to the object
   // pointer that called ConvertToType()
   if (obj != triObj)
   {
       delete triObj;
       triObj = 0;
   }

	///////////////////////////////////////////////////////////////////////////////////////////////

    return true;
}

Point3 AlembicPolyMesh::GetVertexNormal(Mesh* mesh, int faceNo, RVertex* rv)
{
	Face* f = &mesh->faces[faceNo];
	DWORD smGroup = f->smGroup;
	int numNormals = 0;
	Point3 vertexNormal;
	
	// Is normal specified
	// SPCIFIED is not currently used, but may be used in future versions.
	if (rv->rFlags & SPECIFIED_NORMAL) {
		vertexNormal = rv->rn.getNormal();
	}
	// If normal is not specified it's only available if the face belongs
	// to a smoothing group
	else if ((numNormals = rv->rFlags & NORCT_MASK) != 0 && smGroup) {
		// If there is only one vertex is found in the rn member.
		if (numNormals == 1) {
			vertexNormal = rv->rn.getNormal();
		}
		else {
			// If two or more vertices are there you need to step through them
			// and find the vertex with the same smoothing group as the current face.
			// You will find multiple normals in the ern member.
			for (int i = 0; i < numNormals; i++) {
				if (rv->ern[i].getSmGroup() & smGroup) {
					vertexNormal = rv->ern[i].getNormal();
				}
			}
		}
	}
	else {
		// Get the normal from the Face if no smoothing groups are there
		vertexNormal = mesh->getFaceNormal(faceNo);
	}
	
	return vertexNormal;
}

void AlembicPolyMesh::make_face_uv(Face *f, Point3 *tv)
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

BOOL AlembicPolyMesh::CheckForFaceMap(Mtl* mtl, Mesh* mesh)
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

