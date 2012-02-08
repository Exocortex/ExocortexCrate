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
   double scale[3];
   xf.getScale(scale,MSpace::kTransform);
   MQuaternion rot = xf.rotation();
   MVector axis;
   double angle;
   rot.getAxisAngle(axis,angle);
   MVector trans = xf.getTranslation(MSpace::kTransform);
   sample.setScale(Alembic::Abc::V3d(scale[0],scale[1],scale[2]));
   sample.setRotation(Alembic::Abc::V3d(axis.x,axis.y,axis.z),angle);
   sample.setTranslation(Alembic::Abc::V3d(trans.x,trans.y,trans.z));

   // save the sample
   mXformSchema.set(sample);

   return MStatus::kSuccess;
}
