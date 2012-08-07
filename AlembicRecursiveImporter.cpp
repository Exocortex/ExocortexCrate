#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicArchiveStorage.h"
#include "AlembicMeshUtilities.h"
#include "AlembicSplineUtilities.h"
#include "AlembicXformUtilities.h"
#include "AlembicXformController.h"
#include "AlembicCameraUtilities.h"
#include "AlembicParticles.h"
#include "SceneEnumProc.h"
#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
#include "AlembicRecursiveImporter.h"
#include "Utility.h"

#include "AlembicRecursiveImporter.h"



nodeCategory getNodeCategory(Alembic::AbcGeom::IObject& iObj)
{
	if( Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()) ||
		Alembic::AbcGeom::ICamera::matches(iObj.getMetaData()) ||
		Alembic::AbcGeom::IPoints::matches(iObj.getMetaData()) ||
		Alembic::AbcGeom::ICurves::matches(iObj.getMetaData()) ||
		Alembic::AbcGeom::ISubD::matches(iObj.getMetaData())) {
		return NODECAT_GEOMETRY;
	}
	else if(Alembic::AbcGeom::IXform::matches(iObj.getMetaData())){
		return NODECAT_XFORM;
	}
	else {
		return NODECAT_UNSUPPORTED;
	}
}

int createAlembicObject(Alembic::AbcGeom::IObject& iObj, INode **pMaxNode, alembic_importoptions &options, std::string& file)
{
	int ret = alembic_success;
	//if(Alembic::AbcGeom::IXform::matches(iObj.getMetaData())) //Transform
	//{
	//	ESS_LOG_INFO( "AlembicImport_XForm: " << objects[j].getFullName() );
	//	int ret = AlembicImport_PolyMesh(file, iObj, options, pMaxNode); 
	//} 	
	if (Alembic::AbcGeom::IPolyMesh::matches(iObj.getMetaData()) || 
		Alembic::AbcGeom::ISubD::matches(iObj.getMetaData()) ) // PolyMesh / SubD
	{
		ESS_LOG_INFO( "AlembicImport_PolyMesh: " << iObj.getFullName() );
		ret = AlembicImport_PolyMesh(file, iObj, options, pMaxNode); 
	}
	else if (Alembic::AbcGeom::ICamera::matches(iObj.getMetaData())) // Camera
	{
		ESS_LOG_INFO( "AlembicImport_Camera: " << iObj.getFullName() );
		ret = AlembicImport_Camera(file, iObj, options, pMaxNode);
	}
	else if (Alembic::AbcGeom::IPoints::matches(iObj.getMetaData())) // Points
	{
		ESS_LOG_INFO( "AlembicImport_Points: " << iObj.getFullName() );
		ret = AlembicImport_Points(file, iObj, options, pMaxNode);
	}
	else if (Alembic::AbcGeom::ICurves::matches(iObj.getMetaData())) // Curves
	{
		ESS_LOG_INFO( "AlembicImport_Shape: " << iObj.getFullName() );
		ret = AlembicImport_Shape(file, iObj, options, pMaxNode);
	}
	return ret;
}

int recurseOnAlembicObject(Alembic::AbcGeom::IObject& iObj, INode *pParentMaxNode, 
						   bool bNodeWasMerged, alembic_importoptions &options, std::string& file, progressUpdate& progress)
{
	nodeCategory cat = getNodeCategory(iObj);	//TODO: there other children that are supported but handled elsewhere, have collapsed category?
	if( cat == NODECAT_UNSUPPORTED ){		
		return alembic_success;
	}

	if(!iObj.valid()){
		return alembic_failure;
	}

	const char* fullname = iObj.getFullName().c_str();
	const char* pname = (pParentMaxNode) ? pParentMaxNode->GetName() : "";
	const char* name = iObj.getName().c_str();

	INode* pMaxNode = NULL;
	size_t mergedGeomNodeIndex = -1;

	if(!bNodeWasMerged)
	{
		bool bCreateDummyNode = false;
		
		Alembic::AbcGeom::IObject* mergedGeomChild = NULL;
		if(cat == NODECAT_XFORM)
		{	//if a transform node, decide whether or not use a dummy node OR merge this dummy node with geometry node child

			size_t mergeIndex = -1;
			size_t geomNodeCount = 0;
			for(size_t j=0; j<iObj.getNumChildren(); j++)
			{
				nodeCategory cat = getNodeCategory(iObj.getChild(j));
				if( cat == NODECAT_GEOMETRY ){
					mergedGeomChild = &iObj.getChild(j);
					mergeIndex = j;
					geomNodeCount++;
				}
			} 

			if(geomNodeCount == 0 ){//create dummy node
				bCreateDummyNode = true;
			}
			else if(geomNodeCount == 1){ //create geometry node
				mergedGeomNodeIndex = mergeIndex;
			}
			else if(geomNodeCount > 1){ //create dummy node
				bCreateDummyNode = true;
			}
		}

		INode* pExistingNode = NULL;
		if(bCreateDummyNode){

			std::string importName = iObj.getName();

			size_t found = importName.find("Xfo");
			if(found == std::string::npos){
				found = importName.find("xfo");
			}
			if(found != std::string::npos){
				importName = importName.substr(0, found);
			}
			
			pExistingNode = GetChildNodeFromName(importName, pParentMaxNode);
			if(options.attachToExisting && pExistingNode){
				pMaxNode = pExistingNode;

				//see if a controller already exists, and then delete it
				

				int ret = AlembicImport_XForm(pMaxNode, iObj, file, options);
				if(ret != 0) return ret;
			}//only create node if either attachToExisting is false or it is true and the object does not already exist
			else{
				int ret = AlembicImport_DummyNode(iObj, options, &pMaxNode, importName);
				if(ret != 0) return ret;

				ret = AlembicImport_XForm(pMaxNode, iObj, file, options);
				if(ret != 0) return ret;
			}
		}
		else{
			if(mergedGeomNodeIndex != -1){//we are merging, so look at the child geometry node
				pExistingNode = GetChildNodeFromName(mergedGeomChild->getName(), pParentMaxNode);
				if(options.attachToExisting && pExistingNode){
					pMaxNode = pExistingNode;
				}//only create node if either attachToExisting is false or it is true and the object does not already exist
				
				int ret = createAlembicObject(*mergedGeomChild, &pMaxNode, options, file);
				if(ret != 0) return ret;
				ret = AlembicImport_XForm(pMaxNode, iObj, file, options);
				if(ret != 0) return ret;
				
			}
			else{ //multiple geometry nodes under a dummy node (in pParentMaxNode)
				pExistingNode = GetChildNodeFromName(iObj.getName(), pParentMaxNode);
				if(options.attachToExisting && pExistingNode){
					pMaxNode = pExistingNode;
				}//only create node if either attachToExisting is false or it is true and the object does not already exist
				
				int ret = createAlembicObject(iObj, &pMaxNode, options, file);
				if(ret != 0) return ret;

				//import identity matrix, since more than goemetry node share the same transform
				//Should we just list MAX put a default position/scale/rotation controller on?

				//	int ret = AlembicImport_XForm(pMaxNode, *piParentObj, file, options);
				
			}

			if(!pMaxNode){
				return alembic_failure;
			}

			//printControllers(pMaxNode);
		}

		if(pParentMaxNode && !pExistingNode){
			pParentMaxNode->AttachChild(pMaxNode, 0);
		}
	}

	progress.increment();
	progress.update();

	for(size_t j=0; j<iObj.getNumChildren(); j++)
	{
		//if(getNodeCategory(iObj.getChild(j)) != NODECAT_UNSUPPORTED) continue;// skip over unsupported types
		//TODO: warning if geometry node has geometry node child

		INode *pNextParent = bNodeWasMerged ? pParentMaxNode : pMaxNode;
		bool bMerged = (mergedGeomNodeIndex == j);
		int ret = recurseOnAlembicObject(iObj.getChild(j), pNextParent, bMerged, options, file, progress);
		if(ret != 0) return ret;
	} 

	return alembic_success;
}