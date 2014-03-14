#ifndef _ALEMBIC_XFORM_H_
#define _ALEMBIC_XFORM_H_


#include "AlembicObject.h"
#include "AlembicPropertyUtils.h"
// #include "Point3.h"

void SaveXformSample(const SceneEntry &in_Ref, AbcG::OXformSchema &schema, AbcG::XformSample &sample, double time, bool bFlatten);
void SaveCameraXformSample(const SceneEntry &in_Ref, AbcG::OXformSchema &schema, AbcG::XformSample &sample, double time, bool bFlatten);

class AlembicXForm : public AlembicObject
{
private:
   AbcG::OXformSchema mXformSchema;
   AbcG::XformSample mXformSample;
   AlembicCustomAttributesEx customAttributes;
public:
   AlembicXForm(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicXForm();
   virtual bool Save(double time, bool bLastFrame);
   virtual Abc::OCompoundProperty GetCompound();
};

#endif