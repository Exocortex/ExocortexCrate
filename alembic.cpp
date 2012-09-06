// alembicPlugin
#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
//#include "MeshMtlList.h"
#include "SceneEnumProc.h"
#include "ObjectList.h"

#include "resource.h"
#include "utility.h"
#include "AlembicNames.h"
#include "AlembicXformUtilities.h"
  
/*void AlembicImport_SetupChildLinks( Alembic::Abc::IObject &obj, alembic_importoptions &options )
{
	std::string pName = obj.getName();
    INode *pParentNode = options.currentSceneList.FindNodeWithName(pName, false);

    if (pParentNode)
    {
		std::string name; 

        for (size_t i = 0; i < obj.getNumChildren(); i += 1)
        {
            Alembic::Abc::IObject childObj = obj.getChild(i);


			if(childObj.getNumChildren() == 1 && !Alembic::AbcGeom::IXform::matches(childObj.getChild(0).getMetaData()))
			{
				name = childObj.getChild(0).getName();
			}
			else 
			{
				name = childObj.getName(); 
			}

            INode *pChildNode = options.currentSceneList.FindNodeWithName(name, false);

            // PeterM: This will need to be rethought out on how this works
			if (pChildNode){
                pParentNode->AttachChild(pChildNode, 0);
			}
        }
    }
}*/
