#include "stdafx.h"
#include "AlembicGetInfo.h"
#include "CommonMeshUtilities.h"
#include <maya/MProgressWindow.h>


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
  bool isConstant;
  int parentID;
  std::vector<int> childID;
  MString data;

  infoTuple(void): valid(false), identifier(""), type(AT_UNKNOWN), name(""), nbSample(0), isConstant(false), parentID(-1), childID(), data("") {}

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
    str << "|" << (isConstant ? "1" : "0");

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
   Abc::IArchive * archive = getArchiveFromID(fileName);
   if(archive == NULL)
   {
      MGlobal::displayError("[ExocortexAlembic] FileName specified. '"+fileName+"' does not exist.");
      return MS::kFailure;
   }

   // get the root object
   std::deque<AbcObjectCache*> objects;
   std::deque<infoTuple> infoVector;
   std::set<std::string> uniqueIdentifiers;

   AbcArchiveCache *pArchiveCache = getArchiveCache( fileName.asChar() );

   objects.push_back( &(pArchiveCache->find("/")->second) );
   infoVector.push_back(infoTuple());

   MProgressWindow::reserve();
   MProgressWindow::setTitle("AlembicGetInfo");
   MProgressWindow::setInterruptable(true);
   MProgressWindow::setProgressRange(0, 500000);
   MProgressWindow::setProgress(0);


   // loop over all children and collect identifiers
   int idx = 0;
   MStringArray identifiers;
   bool processStopped = false;
   bool nameWarningShowed = false;
   MProgressWindow::startProgress();
   MProgressWindow::advanceProgress(1);
   for(size_t i=0; !processStopped && !objects.empty(); ++i)
   {
      AbcObjectCache *pObjectCache = objects.front();
      objects.pop_front();
	  const int nbChild = (int) pObjectCache->childIdentifiers.size();//iObj.getNumChildren();
      for(size_t j=0; j<nbChild; ++j)
      {
         if (idx % 50 == 0)
         {
           if (MProgressWindow::isCancelled())
           {
             processStopped = true;
             break;
           }
           MProgressWindow::advanceProgress(50);
         }
		 std::string childIdentifier = pObjectCache->childIdentifiers[j];
		 AbcObjectCache *pChildObjectCache = &(pArchiveCache->find( childIdentifier )->second);
         Abc::IObject child = pChildObjectCache->obj;
         objects.push_back(pChildObjectCache);
         ++idx;
         infoVector.push_back(infoTuple());
         infoTuple &iTuple = infoVector.back();

         // check if the name is unique!
         {
           const std::string &fullName = pChildObjectCache->fullName;
           if (uniqueIdentifiers.find(fullName) != uniqueIdentifiers.end())
             iTuple.identifier = "";
           else
           {
             uniqueIdentifiers.insert(fullName);
             iTuple.identifier = fullName.c_str();
           }
         }
         iTuple.type = getAlembicTypeFromObject(child);
         iTuple.name = child.getName().c_str();
         iTuple.nbSample = (int) pChildObjectCache->numSamples;
		 iTuple.isConstant = pChildObjectCache->isConstant;//isObjectConstant(child);
         iTuple.parentID = (int) i;
         {
           infoTuple &parentTuple = infoVector[i];
           if (iTuple.type == AT_Xform && parentTuple.type == AT_Xform)
             parentTuple.type = AT_Group;
           parentTuple.childID.push_back(idx);
         }

         // check identifier names to be unique in hierarchy
         bool nameChanged = false;
         do
         {
           nameChanged = false;
           int pid = i;
           while (pid >= 0)
           {
             const infoTuple &parent = infoVector[pid];
             if (parent.name == iTuple.name)
             {
               nameChanged = true;
               iTuple.name += "Shape";
               break;
             }
             pid = parent.parentID;
           }
         }
         while (nameChanged);    // each time there's a confusion in the name, the process will repeat again to be sure everything is unique!

         // additional data fields
         MString data;
		 if( pChildObjectCache->isMeshTopoDynamic ) {
			data += "dynamictopology=1";                 
	     }
		 if( pChildObjectCache->isMeshPointCache ) {
			data += "purepointcache=1";
		}
         if(AbcG::ICurves::matches(child.getMetaData())) {
            // check if we have topo or not
            AbcG::ICurves obj(child,Abc::kWrapExisting);
            if(obj.valid())
            {
               AbcG::ICurvesSchema::Sample sample;
			   for(size_t k=0;k<pChildObjectCache->numSamples;k++)
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

   if (!processStopped)
   {
     // remove the ref of the archive
     delRefArchive(fileName);
     int interrupt = 20;
     for (std::deque<infoTuple>::const_iterator beg = infoVector.begin(); beg != infoVector.end(); ++beg, --interrupt)
     {
       if (interrupt == 0)
       {
         interrupt = 20;
         if (MProgressWindow::isCancelled())
         {
           processStopped = true;
           break;
         }
       }
       MProgressWindow::advanceProgress(1);
       if (beg->valid)
         identifiers.append(beg->toInfo());
     }
   }

   if (processStopped)
   {
     MGlobal::displayInfo("Alembic import halted!");
     identifiers.clear();
   }
   MProgressWindow::endProgress();

   // set the return value
   setResult(identifiers);
   return status;
}


