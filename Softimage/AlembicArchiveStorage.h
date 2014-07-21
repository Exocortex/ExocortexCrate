#ifndef _ALEMBIC_ARCHIVE_STORAGE_H_
#define _ALEMBIC_ARCHIVE_STORAGE_H_

#include "CommonUtilities.h"

inline Abc::IArchive * getArchiveFromID(XSI::CString path) {
	return getArchiveFromID( std::string( path.GetAsciiString() ) );
}
//XSI::CString addArchive(Abc::IArchive * archive);
inline void deleteArchive(XSI::CString path) {
	deleteArchive( std::string( path.GetAsciiString() ) );
}
inline Abc::IObject getObjectFromArchive(XSI::CString path, XSI::CString identifier) {
	return getObjectFromArchive( std::string( path.GetAsciiString() ), std::string( identifier.GetAsciiString() ) );
}

// ref counting
inline int addRefArchive(XSI::CString path) {
	return addRefArchive( std::string( path.GetAsciiString() ) );
}
inline int delRefArchive(XSI::CString path) {
	return delRefArchive( std::string( path.GetAsciiString() ) );
}
inline int getRefArchive(XSI::CString path) {
	return getRefArchive( std::string( path.GetAsciiString() ) );
}

#endif