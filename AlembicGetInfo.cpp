#include "AlembicGetInfo.h"
#include "CommonMeshUtilities.h"

#include <sstream>
#include <set>

using namespace std;

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

struct infoTuple
{
  bool valid;
  MString identifier;
  ALEMBIC_TYPE type;
  MString name;
  int nbSample;
  int parentID;
  std::vector<int> childID;
  MString data;

  infoTuple(void): valid(false), identifier(""), type(AT_UNKNOWN), name(""), nbSample(0), parentID(-1), childID(), data("") {}

  MString toInfo(void) const
  {
    stringstream str;
    str << "|" << nbSample << "|" << parentID << "|";

    if (childID.empty())
      str << "-1";
    else
    {
      std::vector<int>::const_iterator beg = childID.begin();
      str << (*beg);
      for (++beg; beg != childID.end(); ++beg)
        str << "." << (*beg);
    }

    MString ret = ((identifier + "|") + alembicTypeToString(type).c_str()) + "|" + name;
    ret += str.str().c_str();
    if (data.length() > 0)
      ret += "|" + data;
    return ret;
  }
};

MStatus AlembicGetInfoCommand::doIt(const MArgList & args)
{
   ESS_PROFILE_SCOPE("AlembicGetInfoCommand::doIt");
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
   std::vector<infoTuple> infoVector;
   std::set<std::string> uniqueIdentifiers;

   objects.push_back(archive->getTop());
   infoVector.push_back(infoTuple());

   // loop over all children and collect identifiers
   MStringArray identifiers;
   for(size_t i=0; i<objects.size(); ++i)
   {
      const int nbChild = objects[i].getNumChildren();
      for(size_t j=0; j<nbChild; ++j)
      {
         Alembic::Abc::IObject child = objects[i].getChild(j);
         objects.push_back(child);
         infoVector.push_back(infoTuple());
         infoTuple &iTuple = infoVector[infoVector.size()-1];

         // check if the name is unique!
         std::string fullName = child.getFullName();
         if (uniqueIdentifiers.find(fullName) != uniqueIdentifiers.end())
           iTuple.identifier = "";
         else
         {
           uniqueIdentifiers.insert(fullName);
           iTuple.identifier = fullName.c_str();
         }
         iTuple.type = getAlembicTypeFromObject(child);
         iTuple.name = child.getName().c_str();
         iTuple.nbSample = getNumSamplesFromObject(child);
         iTuple.parentID = i;

         if (iTuple.type == AT_Xform)
         {
           if (infoVector[i].type == AT_Xform)
             infoVector[i].type = AT_Group;             
         }
         infoVector[i].childID.push_back(objects.size()-1);

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
           iTuple.data = data;
         iTuple.valid = true;
      }
   }

   // remove the ref of the archive
   delRefArchive(fileName);
   for (std::vector<infoTuple>::const_iterator beg = infoVector.begin(); beg != infoVector.end(); ++beg)
   {
     if (beg->valid)
      identifiers.append(beg->toInfo());
   }

   // set the return value
   setResult(identifiers);
   return status;
}


