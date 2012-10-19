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
   //MString name = GetUniqueName(truncateName(node.name()));
   MString name = GetUniqueName(node.name());
   mObject = Alembic::AbcGeom::OPoints(GetParentObject(),name.asChar(),GetJob()->GetAnimatedTs());

   mSchema = mObject.getSchema();

   mAgeProperty = Alembic::Abc::OFloatArrayProperty(mSchema.getArbGeomParams(), ".age", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mMassProperty = Alembic::Abc::OFloatArrayProperty(mSchema.getArbGeomParams(), ".mass", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mColorProperty = Alembic::Abc::OC4fArrayProperty(mSchema.getArbGeomParams(), ".color", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );

   /*
   // create all properties
   mInstancenamesProperty = OStringArrayProperty(mSchema.getArbGeomParams(), ".instancenames", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );

   // particle attributes
   mScaleProperty = OV3fArrayProperty(mSchema.getArbGeomParams(), ".scale", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mOrientationProperty = OQuatfArrayProperty(mSchema.getArbGeomParams(), ".orientation", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mAngularVelocityProperty = OQuatfArrayProperty(mSchema.getArbGeomParams(), ".angularvelocity", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeTypeProperty = OUInt16ArrayProperty(mSchema.getArbGeomParams(), ".shapetype", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeTimeProperty = OFloatArrayProperty(mSchema.getArbGeomParams(), ".shapetime", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mShapeInstanceIDProperty = OUInt16ArrayProperty(mSchema.getArbGeomParams(), ".shapeinstanceid", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
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

   mSample.setSelfBounds(bbox);

   // setup the sample
   mSample.setPositions(Alembic::Abc::P3fArraySample(posVec));
   mSample.setVelocities(Alembic::Abc::V3fArraySample(velVec));
   mSample.setWidths(Alembic::AbcGeom::OFloatGeomParam::Sample(Alembic::Abc::FloatArraySample(widthVec), Alembic::AbcGeom::kVertexScope));
   mSample.setIds(Alembic::Abc::UInt64ArraySample(idVec));
   mAgeProperty.set(Alembic::Abc::FloatArraySample(ageVec));
   mMassProperty.set(Alembic::Abc::FloatArraySample(massVec));
   mColorProperty.set(Alembic::Abc::C4fArraySample(colorVec));

   // save the sample
   mSchema.set(mSample);
   mNumSamples++;

   return MStatus::kSuccess;
}


static AlembicPointsNodeList alembicPointsNodeList;

void AlembicPointsNode::PostConstructor(void)
{
  alembicPointsNodeList.push_front(this);
  listPosition = alembicPointsNodeList.begin();
}

void AlembicPointsNode::PreDestruction()
{
  for (AlembicPointsNodeListIter beg = alembicPointsNodeList.begin(); beg != alembicPointsNodeList.end(); ++beg)
  {
    if (beg == listPosition)
    {
      alembicPointsNodeList.erase(listPosition);
      break;
    }
  }

  mSchema.reset();
  delRefArchive(mFileName);
  mFileName.clear();
}

AlembicPointsNode::AlembicPointsNode(void)
{
  PostConstructor();
}

AlembicPointsNode::~AlembicPointsNode(void)
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

MStatus AlembicPointsNode::init(const MString &fileName, const MString &identifier)
{
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
    obj = Alembic::AbcGeom::IPoints(iObj,Alembic::Abc::kWrapExisting);
    if(!obj.valid())
    {
      MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a Points.");
      return MStatus::kFailure;
    }
    mSchema = obj.getSchema();

    if(!mSchema.valid())
      return MStatus::kFailure;
  }
  return MS::kSuccess;
}

static MVector quaternionToVector(const Imath::Quatf &qf)
{
  const float deg = (float)( acos(qf.r) * (360.0f / M_PI) );
  Imath::V3f v = qf.v.normalized();
  return MVector(v.x * deg, v.y * deg, v.z * deg);
}

MStatus AlembicPointsNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   ESS_PROFILE_SCOPE("AlembicPointsNode::compute");
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

   // update the frame number to be imported
   double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
   const MString &fileName = dataBlock.inputValue(mFileNameAttr).asString();
   const MString &identifier = dataBlock.inputValue(mIdentifierAttr).asString();

   // check if we have the file
   if (!(status = init(fileName, identifier)))
     return status;

   // get the sample
   SampleInfo sampleInfo = getSampleInfo
   (
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

   // get all of the data from alembic
   Alembic::Abc::UInt64ArraySamplePtr sampleIds = sample.getIds();
   Alembic::Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   Alembic::Abc::V3fArraySamplePtr sampleVel = sample.getVelocities();

   Alembic::Abc::C4fArraySamplePtr sampleColor;
   {
     Alembic::Abc::IC4fArrayProperty propColor;
     if( getArbGeomParamPropertyAlembic( obj, "color", propColor ) )
	     sampleColor = propColor.getValue(sampleInfo.floorIndex);
   }
   Alembic::Abc::FloatArraySamplePtr sampleAge;
   {
     Alembic::Abc::IFloatArrayProperty propAge;
     if( getArbGeomParamPropertyAlembic( obj, "age", propAge ) )
	     sampleAge = propAge.getValue(sampleInfo.floorIndex);
   }
   Alembic::Abc::FloatArraySamplePtr sampleMass;
   {
     Alembic::Abc::IFloatArrayProperty propMass;
     if( getArbGeomParamPropertyAlembic( obj, "mass", propMass ) )
       sampleMass = propMass.getValue(sampleInfo.floorIndex);
   }
   Alembic::Abc::UInt16ArraySamplePtr sampleShapeInstanceID;
   {
     Alembic::Abc::IUInt16ArrayProperty propShapeInstanceID;
     if( getArbGeomParamPropertyAlembic( obj, "shapeinstanceid", propShapeInstanceID ) )
       sampleShapeInstanceID = propShapeInstanceID.getValue(sampleInfo.floorIndex);   
   }
   Alembic::Abc::QuatfArraySamplePtr sampleOrientation;
   {
     Alembic::Abc::IQuatfArrayProperty propOrientation;
     if( getArbGeomParamPropertyAlembic( obj, "orientation", propOrientation ) )
       sampleOrientation = propOrientation.getValue(sampleInfo.floorIndex);
   }

   // get the current values from the particle cloud
   MIntArray ids;
   part.particleIds(ids);
   MVectorArray positions;
   part.position(positions);
   MVectorArray velocities;
   part.velocity(velocities);
   MVectorArray rgbs;
   part.rgb(rgbs);
   MDoubleArray opacities;
   part.opacity(opacities);
   MDoubleArray ages;
   part.age(ages);
   MDoubleArray masses;
   part.mass(masses);
   MDoubleArray shapeInstId;
   part.getPerParticleAttribute("shapeInstanceIdPP", shapeInstId);
   MVectorArray orientationPP;
   part.getPerParticleAttribute("orientationPP", orientationPP);

   // check if this is a valid sample
   unsigned int particleCount = (unsigned int)samplePos->size();
   if(sampleIds->size() == 1)
   {
      if(sampleIds->get()[0] == (uint64_t)-1)
      {
         particleCount = 0;
      }
   }

   // ensure to have the right amount of particles
   if(positions.length() > particleCount)
   {
      part.setCount(particleCount);
   }
   else
   {
      MPointArray emitted;
      emitted.setLength(particleCount - positions.length());
      part.emit(emitted);
   }

   positions.setLength(particleCount);
   velocities.setLength(particleCount);
   rgbs.setLength(particleCount);
   opacities.setLength(particleCount);
   ages.setLength(particleCount);
   masses.setLength(particleCount);
   shapeInstId.setLength(particleCount);
   orientationPP.setLength(particleCount);

   // if we need to emit new particles, do that now
   if(particleCount > 0)
   {
      for(unsigned int i=0;i<particleCount;i++)
      {
         positions[i].x = samplePos->get()[i].x;
         positions[i].y = samplePos->get()[i].y;
         positions[i].z = samplePos->get()[i].z;

         if(sampleVel && sampleVel->get())
         {
            velocities[i].x = sampleVel->get()[i].x;
            velocities[i].y = sampleVel->get()[i].y;
            velocities[i].z = sampleVel->get()[i].z;
         }

         if(sampleInfo.alpha != 0.0)
         {
            positions[i] += velocities[i] * sampleInfo.alpha;
         }

         if(sampleColor && sampleColor->get())
         {
            rgbs[i].x = sampleColor->get()[i].r;
            rgbs[i].y = sampleColor->get()[i].g;
            rgbs[i].z = sampleColor->get()[i].b;
            opacities[i] = sampleColor->get()[i].a;
         }
         else
         {
            rgbs[i] = MVector(0.0,0.0,0.0);
            opacities[i] = 1.0;
         }
         
         ages[i] = (sampleAge && sampleAge->get()) ? sampleAge->get()[i] : 0.0;
         masses[i] = (sampleMass && sampleMass->get()) ? sampleMass->get()[i] : 1.0;
         shapeInstId[i] = (sampleShapeInstanceID && sampleShapeInstanceID->get()) ? sampleShapeInstanceID->get()[i] : 0.0;
         orientationPP[i] = (sampleOrientation && sampleOrientation->get()) ? quaternionToVector( sampleOrientation->get()[i] ) : MVector::zero;
      }
   }

   // take care of the remaining attributes
   part.setPerParticleAttribute("position", positions);
   part.setPerParticleAttribute("velocity", velocities);
   part.setPerParticleAttribute("rgbPP", rgbs);
   part.setPerParticleAttribute("opacityPP", opacities);
   part.setPerParticleAttribute("agePP", ages);
   part.setPerParticleAttribute("massPP", masses);
   part.setPerParticleAttribute("shapeInstanceIdPP", shapeInstId);
   part.setPerParticleAttribute("orientationPP", orientationPP);

   hOut.set( dOutput );
	 dataBlock.setClean( plug );

   return MStatus::kSuccess;
}

void AlembicPointsNode::instanceInitialize(void)
{
  {
    const MString getAttrCmd = "getAttr " + name();
    MString fileName, identifier;

    MGlobal::executeCommand(getAttrCmd + ".fileName", fileName);
    MGlobal::executeCommand(getAttrCmd + ".identifier", identifier);

    if (!init(fileName, identifier))
      return;
  }

  const size_t lastSample = mSchema.getNumSamples()-1;
  {
    Alembic::Abc::IUInt16ArrayProperty propShapeT;
    if( !getArbGeomParamPropertyAlembic( obj, "shapetype", propShapeT ) )
      return;

    Alembic::Abc::UInt16ArraySamplePtr sampleShapeT = propShapeT.getValue(lastSample);
    if (!sampleShapeT || sampleShapeT->size() <= 0 || sampleShapeT->get()[0] != 7)
      return;
  }

  // check if there's instance names! If so, load the names!... if there's any!
  Alembic::Abc::IStringArrayProperty propInstName;
  if(!getArbGeomParamPropertyAlembic( obj, "instancenames", propInstName ) )
    return;

  Alembic::Abc::StringArraySamplePtr instNames = propInstName.getValue(lastSample);
  const int nbNames = (int) instNames->size();
  if (nbNames < 1)
    return;

  MString addObjectCmd = " -addObject";
  {
    const MString _obj = " -object ";
    for (int i = 0; i < nbNames; ++i)
    {
      const std::string res = instNames->get()[i];
      if (res.length() > 0)
        addObjectCmd += _obj + removeInvalidCharacter(res).c_str();
    }
    addObjectCmd += " ";
  }

  // create the particle instancer and assign shapeInstanceIdPP as the object index!
  MString particleShapeName, mInstancerName;
  MGlobal::executeCommand("string $___connectInfo[] = `connectionInfo -dfs " + name() + ".output[0]`;\nplugNode $___connectInfo[0];", particleShapeName);

  MGlobal::executeCommand("particleInstancer " + particleShapeName, mInstancerName);
  const MString partInstCmd = "particleInstancer -e -name " + mInstancerName;
  MGlobal::executeCommand(partInstCmd + " -rotationUnits Degrees -objectIndex shapeInstanceIdPP -rotation orientationPP " + particleShapeName);
  MGlobal::executeCommand(partInstCmd + addObjectCmd + particleShapeName);
}

AlembicPostImportPointsCommand::AlembicPostImportPointsCommand(void)
{}
AlembicPostImportPointsCommand::~AlembicPostImportPointsCommand(void)
{}

MStatus AlembicPostImportPointsCommand::doIt(const MArgList& args)
{
  AlembicPointsNodeListIter beg = alembicPointsNodeList.begin(), end = alembicPointsNodeList.end();
  for (; beg != end; ++beg)
    (*beg)->instanceInitialize();
  return MS::kSuccess;
}

MSyntax AlembicPostImportPointsCommand::createSyntax(void)
{
  return MSyntax();
}

void* AlembicPostImportPointsCommand::creator(void)
{
  return new AlembicPostImportPointsCommand();
}


