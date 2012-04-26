#ifndef __ALEMBIC_SPLINE_UTILITIES_H
#define __ALEMBIC_SPLINE_UTILITIES_H

#include "Foundation.h"
#include "AlembicMax.h"
#include "resource.h"
#include "AlembicDefinitions.h"

// Alembic Functions

//////////////////////////////////////////////////////////////////////////////////////////
// Import options struct containing the information necessary to fill the Shape object
typedef struct _alembic_fillshape_options
{
public:
    _alembic_fillshape_options() {
		pIObj = NULL;
		pShapeObject = NULL;
		pBezierShape = NULL;
		pPolyShape = NULL;
		dTicks = 0;
	}

    Alembic::AbcGeom::IObject  *pIObj;
	ShapeObject				   *pShapeObject;
    BezierShape                *pBezierShape;
    PolyShape                  *pPolyShape;
    TimeValue                   dTicks;
    Interval                    validInterval;
    AlembicDataFillFlags        nDataFillFlags;
} alembic_fillshape_options;

//////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
void AlembicImport_FillInShape(alembic_fillshape_options &options);

typedef struct _alembic_importoptions alembic_importoptions;

extern int AlembicImport_Shape(const std::string &path, Alembic::AbcGeom::IObject& iObj, alembic_importoptions &options, INode** pMaxNode);

#endif // __ALEMBIC_SPLINE_UTILITIES_H