#include "AlembicGetInfo.h"

AlembicGetInfoCommand::AlembicGetInfoCommand()
{
}

AlembicGetInfoCommand::~AlembicGetInfoCommand()
{
}

MSyntax AlembicGetInfoCommand::createSyntax()
{
   MSyntax syntax;
   syntax.addFlag("-h", "-help");
   syntax.addFlag("-f", "-fileNameArg", MSyntax::kString);
   syntax.enableQuery(false);
   syntax.enableEdit(false);

   return syntax;
}

void* AlembicGetInfoCommand::creator()
{
   return new AlembicGetInfoCommand();
}

MStatus AlembicGetInfoCommand::doIt(const MArgList & args)
{
   MStatus status = MS::kSuccess;
   MArgParser argData(syntax(), args, &status);

   if (argData.isFlagSet("help"))
   {
      MGlobal::displayInfo("[ExocortexAlembic]: ExocortexAlembic_getInfo command:");
      MGlobal::displayInfo("                    -f : provide a fileName (string)");
      return MS::kSuccess;
   }

   if(!argData.isFlagSet("fileNameArg"))
   {
      // TODO: display dialog
      MGlobal::displayError("[ExocortexAlembic] No fileName specified.");
      return status;
   }

   // get the filename arg
   MString fileName = argData.flagArgumentString("fileNameArg",0);

   // create an archive and get it
   addRefArchive(fileName);
   Alembic::Abc::IArchive * archive = getArchiveFromID(fileName);

   // get the root object
   std::vector<Alembic::Abc::IObject> objects;
   objects.push_back(archive->getTop());

   // loop over all children and collect identifiers
   MStringArray identifiers;
   for(size_t i=0;i<objects.size();i++)
   {
      for(size_t j=0;j<objects[i].getNumChildren();j++)
      {
         Alembic::Abc::IObject child = objects[i].getChild(j);
         objects.push_back(child);

         MString identifier = child.getFullName().c_str();
         MString type = getTypeFromObject(child);
         if(type.length() == 0)
            continue;
         identifier += "|"+type;
         MString name = truncateName(child.getName().c_str());
         if(type != "Xform")
            name = injectShapeToName(name);
         identifier += "|"+name;
         MString numSamples;
         numSamples.set((double)getNumSamplesFromObject(child));
         identifier += "|"+numSamples;

         // check the metadata
         if(getCompoundFromObject(child).getPropertyHeader(".metadata") != NULL)
         {
            Alembic::Abc::IStringArrayProperty metaDataProp = Alembic::Abc::IStringArrayProperty( getCompoundFromObject(child), ".metadata" );
            Alembic::Abc::StringArraySamplePtr metaDataPtr = metaDataProp.getValue(0);
            MString metadata;
            for(size_t i=0;i<metaDataPtr->size();i++)
            {
               metadata += metaDataPtr->get()[i].c_str();
               if(i != metaDataPtr->size()-1)
                  metadata += ";";
            }
            identifier += "|"+metadata;
         }

         identifiers.append(identifier);
      }
   }

   // remove the ref of the archive
   delRefArchive(fileName);

   // set the return value
   setResult(identifiers);

   return status;
}
