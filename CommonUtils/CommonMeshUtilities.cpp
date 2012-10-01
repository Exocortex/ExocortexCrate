#include "CommonFoundation.h"
#include "CommonMeshUtilities.h"

bool isAlembicMeshValid( Alembic::AbcGeom::IObject *pIObj ) {
	//ESS_PROFILE_FUNC();
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
	//ESS_PROFILE_FUNC();
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
	//ESS_PROFILE_FUNC();
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
	//ESS_PROFILE_FUNC();
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
	//ESS_PROFILE_FUNC();
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


bool isAlembicMeshPointCache( Alembic::AbcGeom::IObject *pIObj ) {
		//ESS_PROFILE_FUNC();
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

	bool isPointCache = false;
	if(objMesh.valid())
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objMesh.getSchema(),".faceCounts");
		if( ! faceCountProp.valid() ) {
			Alembic::Abc::IP3fArrayProperty positionsProp = Alembic::Abc::IP3fArrayProperty(objMesh.getSchema(),".P");
			if( positionsProp.valid() && positionsProp.getValue()->size() > 0 ) {
				isPointCache = true;
			}
		}
		else if( faceCountProp.isConstant() ) {
			Alembic::Abc::Int32ArraySamplePtr faceCounts = faceCountProp.getValue();
			// the first is our preferred method, the second check here is Helge's method that is now considered obsolete
			if( faceCounts->size() == 0 || ( faceCounts->size() == 1 && ( faceCounts->get()[0] == 0 ) ) ) {
				Alembic::Abc::IP3fArrayProperty positionsProp = Alembic::Abc::IP3fArrayProperty(objMesh.getSchema(),".P");
				if( positionsProp.valid() && positionsProp.getValue()->size() > 0 ) {
					isPointCache = true;
				}
			}
		}
	}
	else
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objSubD.getSchema(),".faceCounts");		
		if( ! faceCountProp.valid() ) {
			Alembic::Abc::IP3fArrayProperty positionsProp = Alembic::Abc::IP3fArrayProperty(objMesh.getSchema(),".P");
			if( positionsProp.valid() && positionsProp.getValue()->size() > 0 ) {
				isPointCache = true;
			}
		}
		else if( faceCountProp.isConstant() ) {
			Alembic::Abc::Int32ArraySamplePtr faceCounts = faceCountProp.getValue();
			// the first is our preferred method, the second check here is Helge's method that is now considered obsolete
			if( faceCounts->size() == 0 || ( faceCounts->size() == 1 && ( faceCounts->get()[0] == 0 ) ) ) {
				Alembic::Abc::IP3fArrayProperty positionsProp = Alembic::Abc::IP3fArrayProperty(objSubD.getSchema(),".P");
				if( positionsProp.valid() && positionsProp.getValue()->size() > 0 ) {
					isPointCache = true;
				}
			}
		}
	}  
	return isPointCache;
}
