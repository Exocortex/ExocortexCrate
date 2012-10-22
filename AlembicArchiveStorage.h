#ifndef _ALEMBIC_ARCHIVE_STORAGE_H_
#define _ALEMBIC_ARCHIVE_STORAGE_H_

#include "Foundation.h"
#include "CommonUtilities.h"

#include <map>
#include <maya/MString.h>

inline Abc::IArchive * getArchiveFromID(MString path) {
	return getArchiveFromID( std::string( path.asChar() ) );
}
//MString addArchive(Abc::IArchive * archive) {
//
//}
inline void deleteArchive(MString path) {
	deleteArchive( std::string( path.asChar() ) );
}
inline Abc::IObject getObjectFromArchive(MString path, MString identifier) {
	return getObjectFromArchive( std::string( path.asChar() ), std::string( identifier.asChar() ) );
}
inline MString resolvePath(MString path) {
	return MString( resolvePath( std::string( path.asChar() ) ).c_str() );
}

// ref counting
inline int addRefArchive(MString path) {
	return addRefArchive( std::string( path.asChar() ) );
}
inline int delRefArchive(MString path) {
	return delRefArchive( std::string( path.asChar() ) );
}
inline int getRefArchive(MString path) {
	return getRefArchive( std::string( path.asChar() ) );
}

#endif