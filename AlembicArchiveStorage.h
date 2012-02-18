#ifndef _ALEMBIC_ARCHIVE_STORAGE_H_
#define _ALEMBIC_ARCHIVE_STORAGE_H_

#include "Foundation.h"
#include <map>
#include <maya/MString.h>

Alembic::Abc::IArchive * getArchiveFromID(MString path);
MString addArchive(Alembic::Abc::IArchive * archive);
void deleteArchive(MString path);
void deleteAllArchives();
Alembic::Abc::IObject getObjectFromArchive(MString path, MString identifier);
MString resolvePath(MString path);

// ref counting
int addRefArchive(MString path);
int delRefArchive(MString path);
int getRefArchive(MString path);

#endif