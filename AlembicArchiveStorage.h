#ifndef _ALEMBIC_ARCHIVE_STORAGE_H_
#define _ALEMBIC_ARCHIVE_STORAGE_H_

#include "Foundation.h"
#include "AlembicMax.h"
#include <map>

Alembic::Abc::IArchive * getArchiveFromID(std::string path);
std::string addArchive(Alembic::Abc::IArchive * archive);
void deleteArchive(std::string path);
void deleteAllArchives();
Alembic::Abc::IObject getObjectFromArchive(std::string path, std::string identifier);

// ref counting
int addRefArchive(std::string path);
int delRefArchive(std::string path);
int getRefArchive(std::string path);

#endif