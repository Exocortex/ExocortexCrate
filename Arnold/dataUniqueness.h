#ifndef _ARNOLD_ALEMBIC_UNIQUENESS_H_
#define _ARNOLD_ALEMBIC_UNIQUENESS_H_

#include "utility.h"

// uvsIdx is rewritten!
// returns a UVs array!
AtArray *removeUvsDuplicate(Alembic::AbcGeom::IV2fGeomParam &uvParam,
                            SampleInfo &sampleInfo, AtArray *uvsIdx,
                            AtArray *faceIndices);

// nIdx is rewritten!
// returns a Ns array!
void removeNormalsDuplicate(AtArray *nor, AtULong &norOffset,
                            Alembic::Abc::N3fArraySamplePtr &nParam,
                            SampleInfo &sampleInfo, AtArray *nIdx,
                            AtArray *faceIndices);

// nIdx is rewritten!
// returns a Ns array!
void removeNormalsDuplicateDynTopology(AtArray *nor, AtULong &norOffset,
                                       Alembic::Abc::N3fArraySamplePtr &nParam,
                                       Alembic::Abc::N3fArraySamplePtr &nParam2,
                                       const float alpha,
                                       SampleInfo &sampleInfo, AtArray *nIdx,
                                       AtArray *faceIndices);

#endif