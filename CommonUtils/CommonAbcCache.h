#ifndef __COMMON_ABC_CACHE_H__
#define __COMMON_ABC_CACHE_H__

class AbcObjectCache {
public:
	AbcObjectCache( Alembic::Abc::IObject & objToCache );

	Alembic::Abc::IObject obj;
	int numSamples;
	bool isConstant;
	bool isMeshPointCache;
	bool isMeshTopoDynamic;
	std::vector<std::string> childIdentifiers;
	std::string fullName;
	std::string parentIdentifier;
};

typedef std::map<std::string,AbcObjectCache> AbcArchiveCache;

void createAbcArchiveCache( Abc::IArchive *pArchive, AbcArchiveCache* fullNameToObjectCache );

#endif // __COMMON_ABC_CACHE_H__