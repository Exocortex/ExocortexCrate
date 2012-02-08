#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "Foundation.h"

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

struct ArchiveInfo
{
   std::string path;
};


SampleInfo getSampleInfo(double iFrame,Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps);
std::string getIdentifierFromRef(const MObject & in_Ref);
MString truncateName(const MString & in_Name);
MString getFullNameFromRef(const MObject & in_Ref);
MString getFullNameFromIdentifier(std::string in_Identifier);
MObject getRefFromFullName(const MString & in_Path);
MObject getRefFromIdentifier(std::string in_Identifier);
bool isRefAnimated(const MObject & in_Ref);
bool returnIsRefAnimated(const MObject & in_Ref, bool animated);
void clearIsRefAnimatedCache();

// remapping imported names
void nameMapAdd(MString identifier, MString name);
MString nameMapGet(MString identifier);
void nameMapClear();

// utility mappings
Alembic::Abc::ICompoundProperty getCompoundFromObject(Alembic::Abc::IObject object);
Alembic::Abc::TimeSamplingPtr getTimeSamplingFromObject(Alembic::Abc::IObject object);
size_t getNumSamplesFromObject(Alembic::Abc::IObject object);

#endif  // _FOUNDATION_H_
