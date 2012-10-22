#ifndef __ALEMBIC_TRANSFORM_UTILITIES_H
#define __ALEMBIC_TRANSFORM_UTILITIES_H

#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "AlembicXformController.h"
#include "Utility.h"
#include "dummy.h"
#include <ILockedTracks.h>
#include "iparamb2.h"
#include "AlembicNames.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// alembic_fillxform_options
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _alembic_fillxform_options
{
    AbcG::IObject *pIObj;
    Matrix3 maxMatrix;
    Box3    maxBoundingBox;
    TimeValue dTicks;
    bool bIsCameraTransform;

    _alembic_fillxform_options()
    {
        pIObj = 0;
        dTicks = 0;
        maxMatrix.IdentityMatrix();
        bIsCameraTransform = false;
    }
} alembic_fillxform_options;


void AlembicImport_FillInXForm(alembic_fillxform_options &options);
int AlembicImport_XForm(INode* pParentNode, INode* pMaxNode, AbcG::IObject& iObjXform, AbcG::IObject* p_iObjGeom, const std::string &file, alembic_importoptions &options);
int AlembicImport_DummyNode(AbcG::IObject& iObj, alembic_importoptions &options, INode** pMaxNode, const std::string& importName);

size_t getNumXformChildren( AbcG::IObject& iObj );

#endif	// __ALEMBIC_TRANSFORM_UTILITIES_H