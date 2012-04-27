#include "AlembicPoints.h"
#include "MetaData.h"
#include <maya/MVectorArray.h>
#include <maya/MFnArrayAttrsData.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicPoints::AlembicPoints(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   MString name = GetUniqueName(truncateName(node.name()));
   mObject = Alembic::AbcGeom::OPoints(GetParentObject(),name.asChar(),GetJob()->GetAnimatedTs());

   mSchema = mObject.getSchema();

   mAgeProperty = Alembic::Abc::OFloatArrayProperty(mSchema, ".age", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mMassProperty = Alembic::Abc::OFloatArrayProperty(mSchema, ".mass", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mColorProperty = Alembic::Abc::OC4fArrayProperty(mSchema, ".color", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );

   /*
   // create all properties
   mInstancenamesProperty = OStringArrayProperty(mSchema, ".instancenames", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );

   // particle attributes
   mScaleProperty = OV3fArrayProperty(mSchema, ".scale", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mOrientationProperty = OQuatfArrayProperty(mSchema, ".orientation", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mAngularVelocityProperty = OQuatfArrayProperty(mSchema, ".angularvelocity", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeTypeProperty = OUInt16ArrayProperty(mSchema, ".shapetype", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeTimeProperty = OFloatArrayProperty(mSchema, ".shapetime", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeInstanceIDProperty = OUInt16ArrayProperty(mSchema, ".shapeinstanceid", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   */
}

AlembicPoints::~AlembicPoints()
{
   mObject.reset();
   mSchema.reset();
}

MStatus AlembicPoints::Save(double time)
{
   // access the geometry
   MFnParticleSystem node(GetRef());

   // save the metadata
   SaveMetaData(this);

   // prepare the bounding box
   Alembic::Abc::Box3d bbox;

   // access the points
   MVectorArray vectors;
   node.position(vectors);

   // check if we have the global cache option
   bool globalCache = GetJob()->GetOption(L"exportInGlobalSpace").asInt() > 0;
   Alembic::Abc::M44f globalXfo;
   if(globalCache)
      globalXfo = GetGlobalMatrix(GetRef());

   // push the positions to the bbox
   size_t particleCount = vectors.length();
   std::vector<Alembic::Abc::V3f> posVec(particleCount);
   for(unsigned int i=0;i<vectors.length();i++)
   {
      posVec[i].x = (float)vectors[i].x;
      posVec[i].y = (float)vectors[i].y;
      posVec[i].z = (float)vectors[i].z;
      if(globalCache)
         globalXfo.multVecMatrix(posVec[i],posVec[i]);
      bbox.extendBy(posVec[i]);
   }
   vectors.clear();

   // get the velocities
   node.velocity(vectors);
   std::vector<Alembic::Abc::V3f> velVec(particleCount);
   for(unsigned int i=0;i<vectors.length();i++)
   {
      velVec[i].x = (float)vectors[i].x;
      velVec[i].y = (float)vectors[i].y;
      velVec[i].z = (float)vectors[i].z;
      if(globalCache)
         globalXfo.multDirMatrix(velVec[i],velVec[i]);
   }
   vectors.clear();

   // get the widths
   MDoubleArray doubles;
   node.radius(doubles);
   std::vector<float> widthVec(particleCount);
   for(unsigned int i=0;i<doubles.length();i++)
      widthVec[i] = (float)doubles[i];
   doubles.clear();

   // get the ids
   std::vector<uint64_t> idVec(particleCount);
   MIntArray ints;
   node.particleIds(ints);
   for(unsigned int i=0;i<ints.length();i++)
      idVec[i] = (uint64_t)ints[i];
   ints.clear();

   // get the age
   node.age(doubles);
   std::vector<float> ageVec(particleCount);
   for(unsigned int i=0;i<doubles.length();i++)
      ageVec[i] = (float)doubles[i];
   doubles.clear();

   // get the mass
   node.mass(doubles);
   std::vector<float> massVec(particleCount);
   for(unsigned int i=0;i<doubles.length();i++)
      massVec[i] = (float)doubles[i];
   doubles.clear();

   // get the color
   std::vector<Alembic::Abc::C4f> colorVec;
   if(node.hasOpacity() || node.hasRgb())
   {
      colorVec.resize(particleCount);
      node.rgb(vectors);
      node.opacity(doubles);
      for(unsigned int i=0;i<doubles.length();i++)
      {
         colorVec[i].r = (float)vectors[i].x;
         colorVec[i].g = (float)vectors[i].y;
         colorVec[i].b = (float)vectors[i].z;
         colorVec[i].a = (float)doubles[i];
      }

      vectors.clear();
      doubles.clear();
   }
   if(colorVec.size() == 0)
      colorVec.push_back(Alembic::Abc::C4f(0.0f,0.0f,0.0f,0.0f));

   // resize the sample to use at least one particle
   if(particleCount == 0)
   {
      posVec.push_back(Alembic::Abc::V3f(FLT_MAX,FLT_MAX,FLT_MAX));
      velVec.push_back(Alembic::Abc::V3f(0.0f,0.0f,0.0f));
      widthVec.push_back(0.0f);
      idVec.push_back((uint64_t)-1);
      ageVec.push_back(0.0f);
      massVec.push_back(0.0f);
   }
   else
      mSample.setSelfBounds(bbox);

   // setup the sample
   mSample.setPositions(Alembic::Abc::P3fArraySample(&posVec.front(),posVec.size()));
   mSample.setVelocities(Alembic::Abc::V3fArraySample(&velVec.front(),velVec.size()));
   mSample.setWidths(Alembic::AbcGeom::OFloatGeomParam::Sample(Alembic::Abc::FloatArraySample(&widthVec.front(),widthVec.size()),Alembic::AbcGeom::kVertexScope));
   mSample.setIds(Alembic::Abc::UInt64ArraySample(&idVec.front(),idVec.size()));
   mAgeProperty.set(Alembic::Abc::FloatArraySample(&ageVec.front(),ageVec.size()));
   mMassProperty.set(Alembic::Abc::FloatArraySample(&massVec.front(),massVec.size()));
   mColorProperty.set(Alembic::Abc::C4fArraySample(&colorVec.front(),colorVec.size()));

   // save the sample
   mSchema.set(mSample);
   mNumSamples++;

   return MStatus::kSuccess;
}

void AlembicPointsNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicPointsNode::~AlembicPointsNode()
{
   PreDestruction();
}

MObject AlembicPointsNode::mTimeAttr;
MObject AlembicPointsNode::mFileNameAttr;
MObject AlembicPointsNode::mIdentifierAttr;

MStatus AlembicPointsNode::initialize()
{
   MStatus status;

   MFnUnitAttribute uAttr;
   MFnTypedAttribute tAttr;
   MFnNumericAttribute nAttr;
   MFnGenericAttribute gAttr;
   MFnStringData emptyStringData;
   MObject emptyStringObject = emptyStringData.create("");

   // input time
   mTimeAttr = uAttr.create("inTime", "tm", MFnUnitAttribute::kTime, 0.0);
   status = uAttr.setStorable(true);
   status = uAttr.setKeyable(true);
   status = addAttribute(mTimeAttr);

   // input file name
   mFileNameAttr = tAttr.create("fileName", "fn", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setUsedAsFilename(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mFileNameAttr);

   // input identifier
   mIdentifierAttr = tAttr.create("identifier", "if", MFnData::kString, emptyStringObject);
   status = tAttr.setStorable(true);
   status = tAttr.setKeyable(false);
   status = addAttribute(mIdentifierAttr);

   // create a mapping
   status = attributeAffects(mTimeAttr, mOutput);
   status = attributeAffects(mFileNameAttr, mOutput);
   status = attributeAffects(mIdentifierAttr, mOutput);

   return status;
}

MStatus AlembicPointsNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   if ( !( plug == mOutput ) )
      return ( MS::kUnknownParameter );

   MStatus status;

   // from the maya api examples (ownerEmitter.cpp)
	int multiIndex = plug.logicalIndex( &status );
	MArrayDataHandle hOutArray = dataBlock.outputArrayValue( mOutput, &status);
	MArrayDataBuilder bOutArray = hOutArray.builder( &status );
	MDataHandle hOut = bOutArray.addElement(multiIndex, &status);
	MFnArrayAttrsData fnOutput;
	MObject dOutput = fnOutput.create ( &status );

   MPlugArray  connectionArray;
   plug.connectedTo(connectionArray, false, true, &status);
   if(connectionArray.length() == 0)
      return status;

   MPlug particleShapeOutPlug = connectionArray[0];
   MObject particleShapeNode = particleShapeOutPlug.node(&status);
   MFnParticleSystem part(particleShapeNode, &status);
   part.setCount(0);

   // update the frame number to be imported
   double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
   MString & fileName = dataBlock.inputValue(mFileNameAttr).asString();
   MString & identifier = dataBlock.inputValue(mIdentifierAttr).asString();

   // check if we have the file
   if(fileName != mFileName || identifier != mIdentifier)
   {
      mSchema.reset();
      if(fileName != mFileName)
      {
         delRefArchive(mFileName);
         mFileName = fileName;
         addRefArchive(mFileName);
      }
      mIdentifier = identifier;

      // get the object from the archive
      Alembic::Abc::IObject iObj = getObjectFromArchive(mFileName,identifier);
      if(!iObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      Alembic::AbcGeom::IPoints obj(iObj,Alembic::Abc::kWrapExisting);
      if(!obj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a Points.");
         return MStatus::kFailure;
      }
      mSchema = obj.getSchema();
   }

   if(!mSchema.valid())
      return MStatus::kFailure;

   // get the sample
   SampleInfo sampleInfo = getSampleInfo(
      inputTime,
      mSchema.getTimeSampling(),
      mSchema.getNumSamples()
   );

   // check if we have to do this at all
   if(mLastSampleInfo.floorIndex == sampleInfo.floorIndex && mLastSampleInfo.ceilIndex == sampleInfo.ceilIndex)
      return MStatus::kSuccess;

   mLastSampleInfo = sampleInfo;

   // access the points values
   Alembic::AbcGeom::IPointsSchema::Sample sample;
   mSchema.get(sample,sampleInfo.floorIndex);

   // if we are in the first sample, remove all known particles
   if(sampleInfo.floorIndex == 0)
   {
      mLookup.clear();
      mMaxId = UINT_MAX;
   }

   // first time around we figure out which particles are known
   Alembic::Abc::UInt64ArraySamplePtr sampleIds = sample.getIds();
   std::vector<unsigned int> idToRemove, idToUpdate, idToEmit;
   idToRemove.reserve(sampleIds->size());
   idToUpdate.reserve(sampleIds->size());
   idToEmit.reserve(sampleIds->size());
   unsigned int newMaxId = UINT_MAX;
   unsigned int lastId = UINT_MAX;
   lookupIt it;

   // check if we need to remove all particles
   if(sampleIds->size() == 0)
   {
      idToRemove.reserve(mLookup.size());
      for(it = mLookup.begin(); it != mLookup.end(); it++)
         idToRemove.push_back(it->second);
   }
   else
   {
      for(unsigned int i=0;i<sampleIds->size();i++)
      {
         unsigned int id = (unsigned int)sampleIds->get()[i];
         if(id > newMaxId || newMaxId == UINT_MAX)
            newMaxId = id;

         // if this is a higher id than what we have in the map
         if(id > mMaxId || mMaxId == UINT_MAX)
         {
            mLookup.insert(lookupPair(id,(unsigned int)mLookup.size()));
            idToEmit.push_back(id);
            continue;
         }

         // check if we skipped some ids, which means we need to remove them
         if(lastId != UINT_MAX && lastId != id - 1)
         {
            for(unsigned int j=lastId+1; j<id; j++)
            {
               it = mLookup.find(j);
               if(it != mLookup.end())
               {
                  idToRemove.push_back(it->second);
                  mLookup.erase(it);
               }
            }
         }

         // now check if we have this id, if not, insert it
         it = mLookup.find(id);
         if(it != mLookup.end())
         {
            idToUpdate.push_back(it->second);
         }
         else
         {
            mLookup.insert(lookupPair(id,(unsigned int)mLookup.size()));
            idToEmit.push_back(id);
         }

         lastId = id;
      }
   }

   // update the lookup with continuous indices
   if(mLookup.size() > 0)
   {
      unsigned int lookupIndex = 0;
      for(it = mLookup.begin(); it != mLookup.end(); it++, lookupIndex++)
         it->second = lookupIndex;
   }

   mMaxId = newMaxId;

   Alembic::Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   unsigned int particleCount = 0;
   if(samplePos)
      if(samplePos->size() > 0)
         if(samplePos->get()[0].x != FLT_MAX)
            particleCount = (unsigned int)samplePos->size();

   if(particleCount > 0)
   {
      // positions
   	MPointArray outPos;// = fnOutput.vectorArray("position", &status);
   	MDoubleArray outLifeSpan;// = fnOutput.doubleArray("lifespanPP", &status);
      //outIds.setLength(particleCount);
      outPos.setLength(particleCount);
      outLifeSpan.setLength(particleCount);
      for(unsigned int i=0;i<particleCount;i++)
      {
         //outIds[i] = (int)sampleIds->get()[i];
         outPos[i].x = samplePos->get()[i].x;
         outPos[i].y = samplePos->get()[i].y;
         outPos[i].z = samplePos->get()[i].z;
         outLifeSpan[i] = 0.0;
      }
      
      part.emit(outPos);

      //part.setPerParticleAttribute("particleId",outIds);
      //part.setPerParticleAttribute("position",outPos);
      part.setPerParticleAttribute("lifespanPP",outLifeSpan);

      // velocities
      Alembic::Abc::V3fArraySamplePtr sampleVel = sample.getVelocities();
      if(sampleVel)
      {
         if(sampleVel->size() == particleCount)
         {
         	MVectorArray outVel;// = fnOutput.vectorArray("velocity", &status);
            outVel.setLength(particleCount);
            for(unsigned int i=0;i<particleCount;i++)
            {
               outVel[i].x = sampleVel->get()[i].x;
               outVel[i].y = sampleVel->get()[i].y;
               outVel[i].z = sampleVel->get()[i].z;
            }
            //part.setPerParticleAttribute("velocity",outVel);
         }
      }
   }

   hOut.set( dOutput );
	dataBlock.setClean( plug );

   return MStatus::kSuccess;
}
