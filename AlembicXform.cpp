#include "AlembicXform.h"

#include <maya/MFnTransform.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicXform::AlembicXform(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   MString xformName = node.name()+"Xfo";
   Alembic::AbcGeom::OXform xform(GetOParent(),xformName.asChar(),GetJob()->GetAnimatedTs());

   mXformSchema = xform.getSchema();
}

AlembicXform::~AlembicXform()
{
}

Alembic::Abc::OCompoundProperty AlembicXform::GetCompound()
{
   return mXformSchema;
}

MStatus AlembicXform::Save(double time)
{
   // access the xform
   MFnTransform node(GetRef());

   // TODO: implement storage of metadata
   // SaveMetaData(prim.GetParent3DObject().GetRef(),this);

   Alembic::AbcGeom::XformSample sample;

   MTransformationMatrix xf = node.transformation();
   MMatrix matrix = xf.asMatrix();
   Alembic::Abc::M44d abcMatrix;
   matrix.get(abcMatrix.x);
   sample.setMatrix(abcMatrix);

   // save the sample
   mXformSchema.set(sample);

   return MStatus::kSuccess;
}
