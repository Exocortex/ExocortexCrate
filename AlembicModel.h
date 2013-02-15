#ifndef _ALEMBIC_MODEL_H_
#define _ALEMBIC_MODEL_H_

#include "AlembicObject.h"

class AlembicModel: public AlembicObject
{
private:
  AbcG::OXformSchema mXformSchema;
  AbcG::XformSample mXformSample;
  AbcG::OUcharProperty mXformXSINodeType;
public:

   AlembicModel(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicModel();

   virtual Abc::OCompoundProperty GetCompound();
   virtual XSI::CStatus Save(double time);
};

#endif