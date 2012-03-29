// alembicPlugin
#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
//#include "MeshMtlList.h"
#include "SceneEnumProc.h"
#include "ObjectList.h"
#include "ObjectEntry.h"
#include "resource.h"
#include "utility.h"
#include "AlembicNames.h"
  
void AlembicImport_SetupChildLinks( Alembic::Abc::IObject &obj, alembic_importoptions &options )
{
    INode *pParentNode = options.currentSceneList.FindNodeWithName(std::string(obj.getName()));

    if (pParentNode)
    {
        for (size_t i = 0; i < obj.getNumChildren(); i += 1)
        {
            Alembic::Abc::IObject childObj = obj.getChild(i);
            INode *pChildNode = options.currentSceneList.FindNodeWithName(std::string(childObj.getName()));

            // PeterM: This will need to be rethought out on how this works
            if (pChildNode)
                pParentNode->AttachChild(pChildNode, 0);
        }
    }
}
