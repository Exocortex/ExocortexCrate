#include "Alembic.h"
#include "stdafx.h"

#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
//#include "MeshMtlList.h"
#include "ObjectList.h"
#include "SceneEnumProc.h"

#include "AlembicNames.h"
#include "AlembicXformUtilities.h"
#include "resource.h"
#include "utility.h"

/*void AlembicImport_SetupChildLinks( Abc::IObject &obj, alembic_importoptions
&options )
{
        std::string pName = obj.getName();
    INode *pParentNode = options.currentSceneList.FindNodeWithName(pName,
false);

    if (pParentNode)
    {
                std::string name;

        for (size_t i = 0; i < obj.getNumChildren(); i += 1)
        {
            Abc::IObject childObj = obj.getChild(i);


                        if(childObj.getNumChildren() == 1 &&
!AbcG::IXform::matches(childObj.getChild(0).getMetaData()))
                        {
                                name = childObj.getChild(0).getName();
                        }
                        else
                        {
                                name = childObj.getName();
                        }

            INode *pChildNode = options.currentSceneList.FindNodeWithName(name,
false);

            // PeterM: This will need to be rethought out on how this works
                        if (pChildNode){
                pParentNode->AttachChild(pChildNode, 0);
                        }
        }
    }
}*/
