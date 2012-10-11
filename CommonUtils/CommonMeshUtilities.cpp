#include "CommonFoundation.h"
#include "CommonMeshUtilities.h"
#include "CommonLog.h"
#include "CommonProfiler.h"

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
 		if( objMesh.valid() ) {
			if( objMesh.getSchema().getNormalsParam().valid() ) {
				isConstant = objMesh.getSchema().getNormalsParam().isConstant();
				return true;
			}
		}
	}


	isConstant = true;
	return false;
}


bool isAlembicMeshPositions( Alembic::AbcGeom::IObject *pIObj, bool& isConstant ) {
	ESS_PROFILE_SCOPE("isAlembicMeshPositions");
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
		if( objMesh.valid() ) {
			isConstant = objMesh.getSchema().getPositionsProperty().isConstant();
			return true;
		}
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
		if( objSubD.valid() ) {
			isConstant = objSubD.getSchema().getPositionsProperty().isConstant();
			return true;
		}
	}
	isConstant = true;
	return false;
}

bool isAlembicMeshUVWs( Alembic::AbcGeom::IObject *pIObj, bool& isConstant ) {
	ESS_PROFILE_SCOPE("isAlembicMeshUVWs");
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
		if( objMesh.valid() ) {
			if( objMesh.getSchema().getUVsParam().valid() ) {
				isConstant = objMesh.getSchema().getUVsParam().isConstant();
				return true;
			}
		}
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
		if( objSubD.valid() ) {
			if( objSubD.getSchema().getUVsParam().valid() ) {
				isConstant = objSubD.getSchema().getUVsParam().isConstant();
				return true;
			}
		}
	}
	isConstant = true;

	return false;
}

bool isAlembicMeshTopoDynamic( Alembic::AbcGeom::IObject *pIObj ) {
	ESS_PROFILE_SCOPE("isAlembicMeshTopoDynamic");
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
		if(faceCountProp.valid() && ! faceCountProp.isConstant() ) {
			hasDynamicTopo = true;
		}

	}
	else if( subDSample.valid() )
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objSubD.getSchema(),".faceCounts");
		if(faceCountProp.valid() && ! faceCountProp.isConstant() ) {
			hasDynamicTopo = true;
		}
	}  
	return hasDynamicTopo;
}

bool isAlembicMeshTopology( Alembic::AbcGeom::IObject *pIObj ) {
	ESS_PROFILE_SCOPE("isAlembicMeshTopology");
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
	}

	bool isTopology = true;
	if(objMesh.valid())
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objMesh.getSchema(),".faceCounts");
		if( ! faceCountProp.valid() ) {
			isTopology = false;
		}
		else if( faceCountProp.isConstant() ) {
			Alembic::Abc::Int32ArraySamplePtr faceCounts = faceCountProp.getValue();
			if( faceCounts->size() == 0 || ( faceCounts->size() == 1 && ( faceCounts->get()[0] == 0 ) ) ) {
				isTopology = false;
			}
		}
	}
	else if( objSubD.valid() )
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objSubD.getSchema(),".faceCounts");
		if( ! faceCountProp.valid() ) {
			isTopology = false;
		}
		else if( faceCountProp.isConstant() ) {
			Alembic::Abc::Int32ArraySamplePtr faceCounts = faceCountProp.getValue();
			if( faceCounts->size() == 0 || ( faceCounts->size() == 1 && ( faceCounts->get()[0] == 0 ) ) ) {
				isTopology = false;
			}
		}
	}
	else {
		isTopology = false;
	}
	return isTopology;
}


bool isAlembicMeshPointCache( Alembic::AbcGeom::IObject *pIObj ) {
	ESS_PROFILE_SCOPE("isAlembicMeshPointCache");
	Alembic::AbcGeom::IPolyMesh objMesh;
	Alembic::AbcGeom::ISubD objSubD;

	if(Alembic::AbcGeom::IPolyMesh::matches((*pIObj).getMetaData())) {
		objMesh = Alembic::AbcGeom::IPolyMesh(*pIObj,Alembic::Abc::kWrapExisting);
	}
	else {
		objSubD = Alembic::AbcGeom::ISubD(*pIObj,Alembic::Abc::kWrapExisting);
	}

	bool isPointCache = false;
	if(objMesh.valid())
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objMesh.getSchema(),".faceCounts");
		if( ! faceCountProp.valid() ) {
			Alembic::Abc::IP3fArrayProperty positionsProp = Alembic::Abc::IP3fArrayProperty(objMesh.getSchema(),"P");
			if( positionsProp.valid() && positionsProp.getValue()->size() > 0 ) {
				isPointCache = true;
			}
		}
		else if( faceCountProp.isConstant() ) {
			Alembic::Abc::Int32ArraySamplePtr faceCounts = faceCountProp.getValue();
			// the first is our preferred method, the second check here is Helge's method that is now considered obsolete
			if( faceCounts->size() == 0 || ( faceCounts->size() == 1 && ( faceCounts->get()[0] == 0 ) ) ) {
				Alembic::Abc::IP3fArrayProperty positionsProp = Alembic::Abc::IP3fArrayProperty(objMesh.getSchema(),"P");
				if( positionsProp.valid() && positionsProp.getValue()->size() > 0 ) {
					isPointCache = true;
				}
			}
		}
	}
	else if( objSubD.valid() )
	{
		Alembic::Abc::IInt32ArrayProperty faceCountProp = Alembic::Abc::IInt32ArrayProperty(objSubD.getSchema(),".faceCounts");		
		if( ! faceCountProp.valid() ) {
			Alembic::Abc::IP3fArrayProperty positionsProp = Alembic::Abc::IP3fArrayProperty(objSubD.getSchema(),"P");
			if( positionsProp.valid() && positionsProp.getValue()->size() > 0 ) {
				isPointCache = true;
			}
		}
		else if( faceCountProp.isConstant() ) {
			Alembic::Abc::Int32ArraySamplePtr faceCounts = faceCountProp.getValue();
			// the first is our preferred method, the second check here is Helge's method that is now considered obsolete
			if( faceCounts->size() == 0 || ( faceCounts->size() == 1 && ( faceCounts->get()[0] == 0 ) ) ) {
				Alembic::Abc::IP3fArrayProperty positionsProp = Alembic::Abc::IP3fArrayProperty(objSubD.getSchema(),"P");
				if( positionsProp.valid() && positionsProp.getValue()->size() > 0 ) {
					isPointCache = true;
				}
			}
		}
	}  
	return isPointCache;
}

struct edge
{
	int a;
	int b;

	bool operator<( const edge& rEdge ) const
	{
		if(a < rEdge.a) return true;
		if(a > rEdge.a) return false;
		if(b < rEdge.b) return true;
		return false;
	}
};

struct edgeData
{
	bool bReversed;
	int count;
	int face;

	edgeData():bReversed(false), count(1), face(-1)
	{}


};


int validateAlembicMeshTopo(std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> faceCounts,
							std::vector<Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t> faceIndices,
							const std::string& meshName)
{


	std::map<edge, edgeData> edgeToCountMap;
	typedef std::map<edge, edgeData>::iterator iterator;

	int meshErrors = 0;	

	int faceOffset = 0;
	for(int i=0; i<faceCounts.size(); i++){
		int count = faceCounts[i];

		std::vector<int> occurences;
		occurences.reserve(count);

		Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS::int32_t* v = &faceIndices[faceOffset];
		for(int j=0; j<count; j++){
			
			occurences.push_back(v[j]);
		}
		std::sort(occurences.begin(), occurences.end());

		for(int j=0; j<occurences.size()-1; j++){

			if(occurences[j] == occurences[j+1]){
				ESS_LOG_WARNING("Error in mesh \""<<meshName<<"\". Vertex "<<occurences[j]<<" of face "<<i<<" is duplicated.");
				meshErrors ++;
			}
		}

		for(int j=0; j<count-1; j++){
			edge e;
			e.a = v[j];
			e.b = v[j+1];

			if( e.a == e.b ){
				//the duplicate vertices check already covers this case.
				//ESS_LOG_WARNING("edge ("<<e.a<<", "<<e.b<<") is degenerate.");
				continue;
			}

			edgeData eData;
			eData.face = i;

			if(e.b < e.a){
				std::swap(e.a, e.b);
				eData.bReversed = true;
			}

			if( edgeToCountMap.find(e) == edgeToCountMap.end() ){
				edgeToCountMap[e] = eData;
			}
			else{
				edgeData eData2 = edgeToCountMap[e];
				eData2.count++;
				
				if( (eData.bReversed && eData2.bReversed) || (!eData.bReversed && !eData2.bReversed) ){
					
					ESS_LOG_WARNING("Error in mesh \""<<meshName<<"\". Edge ("<<e.a<<", "<<e.b<<") is shared between polygons "<<eData.face<<" and "<<eData2.face<<", and these polygons have reversed orderings.");
					meshErrors ++;
				}
			}
		}
		
		faceOffset += count;
	}
	return meshErrors;

}