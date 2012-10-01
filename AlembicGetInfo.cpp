#include "AlembicGetInfo.h"
#include "CommonMeshUtilities.h"

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
      return MS::kFailure;
   }

   // get the filename arg
   MString fileName = argData.flagArgumentString("fileNameArg",0);

   // create an archive and get it
   addRefArchive(fileName);
   Alembic::Abc::IArchive * archive = getArchiveFromID(fileName);
   if(archive == NULL)
   {
      MGlobal::displayError("[ExocortexAlembic] FileName specified. '"+fileName+"' does not exist.");
      return MS::kFailure;
   }

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
         //MString name = truncateName(child.getName().c_str());
         MString name = child.getName().c_str();
         //if(type != "Xform")
            //name = injectShapeToName(name);
         identifier += "|"+name;
         MString numSamples;
         numSamples.set((double)getNumSamplesFromObject(child));
         identifier += "|"+numSamples;

         // additional data fields
         MString data;
         if(Alembic::AbcGeom::IPolyMesh::matches(child.getMetaData())) {
            // check if we have topo or not
            if( isAlembicMeshTopoDynamic( & child ) ) {
				data += "dynamictopology=1";                 
			}
			if( isAlembicMeshPointCache( & child ) ) {
				data += "purepointcache=1";
			}
         } else if(Alembic::AbcGeom::ISubD::matches(child.getMetaData())) {
            // check if we have topo or not
			if( isAlembicMeshTopoDynamic( & child ) ) {
				data += "dynamictopology=1";                 
			}
			if( isAlembicMeshPointCache( & child ) ) {
				data += "purepointcache=1";
			}
         } else if(Alembic::AbcGeom::ICurves::matches(child.getMetaData())) {
            // check if we have topo or not
            Alembic::AbcGeom::ICurves obj(child,Alembic::Abc::kWrapExisting);
            if(obj.valid())
            {
               Alembic::AbcGeom::ICurvesSchema::Sample sample;
               for(size_t k=0;k<obj.getSchema().getNumSamples();k++)
               {
                  obj.getSchema().get(sample,k);
                  if(sample.getNumCurves() != 1)
                  {
                     data += "hair=1";
                     break;
                  }
               }
            }
         }
         if(data.length() > 0)
            identifier += "|"+data;

         identifiers.append(identifier);
      }
   }

   // remove the ref of the archive
   delRefArchive(fileName);

   // set the return value
   setResult(identifiers);

   return status;
}
