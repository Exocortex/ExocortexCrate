#include "CommonAlembic.h"
#include "CommonMeshUtilities.h"
#include "CommonUtilities.h"
#include "CommonAbcCache.h"

AbcObjectCache::AbcObjectCache( Alembic::Abc::IObject & objToCache )
		:	obj( objToCache ), isMeshTopoDynamic(false), isMeshPointCache(false), fullName(objToCache.getFullName())
{
	ESS_PROFILE_SCOPE("AbcObjectCache::AbcObjectCache");

	BasicSchemaData bsd;
	getBasicSchemaDataFromObject(objToCache, bsd);
	isConstant = bsd.isConstant;
	numSamples = bsd.nbSamples;
	bool isMesh = true;
	if ( bsd.type == bsd.__POLYMESH || !( isMesh = (bsd.type != bsd.__SUBDIV) ) )
	{
		AlembicMeshBasePtr aMesh = createAlembicMesh(&objToCache, isMesh);
		isMeshPointCache = aMesh->pointCache();
		if (!isConstant)
			isMeshTopoDynamic = aMesh->isTopoDynamic;
	}
}

AbcObjectCache* addObjectToCache( AbcArchiveCache* fullNameToObjectCache, Abc::IObject &obj, std::string parentIdentifier ) {
	ESS_PROFILE_SCOPE("addObjectToCache");
	AbcObjectCache objectCache( obj );
	objectCache.parentIdentifier = parentIdentifier;
	for( int i = 0; i < obj.getNumChildren(); i ++ ) {
		Abc::IObject child = obj.getChild( i );
		AbcObjectCache* childObjectCache = addObjectToCache( fullNameToObjectCache, child, objectCache.fullName );
		objectCache.childIdentifiers.push_back( childObjectCache->fullName );
	}
	fullNameToObjectCache->insert( AbcArchiveCache::value_type( objectCache.fullName, objectCache ) );
	return &( fullNameToObjectCache->find( objectCache.fullName )->second );
}

void createAbcArchiveCache( Abc::IArchive *pArchive, AbcArchiveCache* fullNameToObjectCache ) {
	ESS_PROFILE_SCOPE("createAbcArchiveCache");
	EC_LOG_INFO( "Creating AbcArchiveCache for archive: " << pArchive->getName() );
   Abc::IObject top = pArchive->getTop();
	addObjectToCache( fullNameToObjectCache, top, "" );
}
