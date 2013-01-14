#include "stdafx.h"
#include "AlembicPoints.h"
#include "MetaData.h"

bool AlembicPoints::listIntanceNames(std::vector<std::string> &names)
{
	MStatus status;
  MDagPathArray allPaths;
  {
    MMatrixArray allMatrices;
    MIntArray pathIndices;
    MIntArray pathStartIndices;

    MSelectionList sl;
    sl.add(instName);

    MDagPath dagp;
    sl.getDagPath(0, dagp);

    status = MFnInstancer(dagp).allInstances( allPaths, allMatrices, pathStartIndices, pathIndices );
	if (status != MS::kSuccess)
	{
		MGlobal::displayWarning("[ExocortexAlembic] Instancer error: " + status.errorString());
		return false;
	}
  }

  names.resize(allPaths.length());
  for (int i = 0; i < allPaths.length(); ++i)
  {
	  std::string nm = allPaths[i].fullPathName().asChar();
	  size_t pos = nm.find("|");
	  while (pos != std::string::npos)
	  {
			nm[pos] = '/';
			pos = nm.find("|", pos);
	  }
	  names[i] = nm;
  }
  mInstanceNamesProperty.set(Abc::StringArraySample(names));
  return true;
}

bool AlembicPoints::sampleInstanceProperties( std::vector<Abc::Quatf> angularVel,     std::vector<Abc::Quatf> orientation,
                                              std::vector<Abc::v4::uint16_t> shapeId, std::vector<Abc::v4::uint16_t> shapeType,
                                              std::vector<Abc::float32_t> shapeTime)
{
  MMatrixArray allMatrices;
  MIntArray pathIndices;
  MIntArray pathStartIndices;
  int nbParticles;
  {
    MDagPathArray allPaths;

    MSelectionList sl;
    sl.add(instName);

    MDagPath dagp;
    sl.getDagPath(0, dagp);

    MFnInstancer inst(dagp);
    inst.allInstances( allPaths, allMatrices, pathStartIndices, pathIndices );
    nbParticles = inst.particleCount();
  }

  // resize vectors!
  angularVel.resize(nbParticles);
  orientation.resize(nbParticles);
  shapeId.resize(nbParticles);
  shapeType.resize(nbParticles, 7);
  shapeTime.resize(nbParticles, mNumSamples);

  float matrix_data[4][4];
  for (int i = 0; i < nbParticles; ++i)
  {
    angularVel[i] = Abc::Quatf();     // don't know how to access this one yet!
    allMatrices[i].get(matrix_data);
	orientation[i] = Imath::extractQuat(Imath::M44f(matrix_data));
    shapeId[i] = pathIndices[pathStartIndices[i]];  // only keep the first one...
  }

  mAngularVelocityProperty.set(Abc::QuatfArraySample(angularVel));
  mOrientationProperty.set(Abc::QuatfArraySample(orientation));
  mShapeInstanceIdProperty.set(Abc::UInt16ArraySample(shapeId));
  mShapeTimeProperty.set(Abc::FloatArraySample(shapeTime));
  mShapeTypeProperty.set(Abc::UInt16ArraySample(shapeType));
  return true;
}

AlembicPoints::AlembicPoints(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
	: AlembicObject(eNode, in_Job, oParent), hasInstancer(false)
{
	mObject = AbcG::OPoints(GetMyParent(), eNode->name, GetJob()->GetAnimatedTs());
	mSchema = mObject.getSchema();

	Abc::OCompoundProperty arbGeomParam = mSchema.getArbGeomParams();
	mAgeProperty   = Abc::OFloatArrayProperty(arbGeomParam, ".age", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
	mMassProperty  = Abc::OFloatArrayProperty(arbGeomParam, ".mass", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
	mColorProperty = Abc::OC4fArrayProperty  (arbGeomParam, ".color", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );

	mAngularVelocityProperty = Abc::OQuatfArrayProperty  (arbGeomParam, ".angularvelocity", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
	mInstanceNamesProperty   = Abc::OStringArrayProperty (arbGeomParam, ".instancenames", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
	mOrientationProperty     = Abc::OQuatfArrayProperty  (arbGeomParam, ".orientation", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
	mScaleProperty           = Abc::OV3fArrayProperty    (arbGeomParam, ".scale", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
	mShapeInstanceIdProperty = Abc::OUInt16ArrayProperty (arbGeomParam, ".shapeinstanceid", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
	mShapeTimeProperty       = Abc::OFloatArrayProperty  (arbGeomParam, ".shapetime", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
	mShapeTypeProperty       = Abc::OUInt16ArrayProperty (arbGeomParam, ".shapetype", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );

	MStringArray instancers;
	MGlobal::executeCommand(("listConnections -t instancer " + eNode->name).c_str(), instancers);
	switch(instancers.length())
	{
	case 0:
		break;
	case 1:
		hasInstancer = true;
		instName = instancers[0];
		break;
	default:
		MGlobal::displayWarning(("[ExocortexAlembic] More than one instancer associated to " + eNode->name + ", can only use one.\nParticle instancing ignore.").c_str());
		break;
	}
}

AlembicPoints::~AlembicPoints()
{
   mObject.reset();
   mSchema.reset();
}

MStatus AlembicPoints::Save(double time)
{
  ESS_PROFILE_SCOPE("AlembicPoints::Save");
   // access the geometry
   MFnParticleSystem node(GetRef());

   // save the metadata
   SaveMetaData(this);

   // prepare the bounding box
   Abc::Box3d bbox;

   // access the points
   MVectorArray vectors;
   node.position(vectors);

   // check if we have the global cache option
   const bool globalCache = GetJob()->GetOption(L"exportInGlobalSpace").asInt() > 0;
   Abc::M44f globalXfo;
   if(globalCache)
      globalXfo = GetGlobalMatrix(GetRef());

   // instance names, scale,
   std::vector<Abc::V3f> scales;
   std::vector<std::string> instanceNames;
   if (hasInstancer && mNumSamples == 0)
   {
     scales.push_back(Abc::V3f(1.0f, 1.0f, 1.0f));
     mScaleProperty.set(Abc::V3fArraySample(scales));

     listIntanceNames(instanceNames);
   }

   // push the positions to the bbox
   size_t particleCount = vectors.length();
   std::vector<Abc::V3f> posVec(particleCount);
   for(unsigned int i=0;i<vectors.length();i++)
   {
      const MVector &out = vectors[i];
      Alembic::Abc::v4::V3f  &in  = posVec[i];
      in.x = (float)out.x;
      in.y = (float)out.y;
      in.z = (float)out.z;
      if(globalCache)
         globalXfo.multVecMatrix(in, in);
      bbox.extendBy(in);
   }
   vectors.clear();

   // get the velocities
   node.velocity(vectors);
   std::vector<Abc::V3f> velVec(particleCount);
   for(unsigned int i=0;i<vectors.length();i++)
   {
      const MVector &out = vectors[i];
      Alembic::Abc::v4::V3f &in  = velVec[i];
      in.x = (float)out.x;
      in.y = (float)out.y;
      in.z = (float)out.z;
      if(globalCache)
         globalXfo.multDirMatrix(in, in);
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
   std::vector<Abc::uint64_t> idVec(particleCount);
   MIntArray ints;
   node.particleIds(ints);
   for(unsigned int i=0;i<ints.length();i++)
      idVec[i] = (Abc::uint64_t)ints[i];
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
   std::vector<Abc::C4f> colorVec;
   if(node.hasOpacity() || node.hasRgb())
   {
      colorVec.resize(particleCount);
      node.rgb(vectors);
      node.opacity(doubles);
      for(unsigned int i=0;i<doubles.length();i++)
      {
         const MVector &out = vectors[i];
         Imath::C4f &in = colorVec[i];
         in.r = (float)out.x;
         in.g = (float)out.y;
         in.b = (float)out.z;
         in.a = (float)doubles[i];
      }
      vectors.clear();
      doubles.clear();
   }

   mSample.setSelfBounds(bbox);

   // setup the sample
   mSample.setPositions(Abc::P3fArraySample(posVec));
   mSample.setVelocities(Abc::V3fArraySample(velVec));
   mSample.setWidths(AbcG::OFloatGeomParam::Sample(Abc::FloatArraySample(widthVec), AbcG::kVertexScope));
   mSample.setIds(Abc::UInt64ArraySample(idVec));
   mAgeProperty.set(Abc::FloatArraySample(ageVec));
   mMassProperty.set(Abc::FloatArraySample(massVec));
   mColorProperty.set(Abc::C4fArraySample(colorVec));

   //--- instancing!!
   std::vector<Abc::Quatf> angularVel, orientation;
   std::vector<Abc::v4::uint16_t> shapeId, shapeType;
   std::vector<Abc::float32_t> shapeTime;
   if (hasInstancer)
     sampleInstanceProperties(angularVel, orientation, shapeId, shapeType, shapeTime);

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

AlembicPointsNode::AlembicPointsNode(void): mLastInputTime(-1.0)
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
    Abc::IObject iObj = getObjectFromArchive(mFileName,identifier);
    if(!iObj.valid())
    {
      MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
      return MStatus::kFailure;
    }
    obj = AbcG::IPoints(iObj,Abc::kWrapExisting);
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

static MVector quaternionToVector(const Abc::Quatf &qf, const Abc::Quatf &angVel)
{
	const Abc::Quatf qfVel = (angVel.r != 0.0f) ? (angVel * qf).normalize() : qf;

	Imath::V3f vec;
	extractEulerXYZ(qfVel.toMatrix44(), vec);
	return MVector(vec.x, vec.y, vec.z);
}

static bool useAngularVelocity(Abc::QuatfArraySamplePtr &velPtr, const AbcG::IPoints &obj, Alembic::AbcCoreAbstract::index_t floorIndex)
{
	Abc::IQuatfArrayProperty velProp;
	if (getArbGeomParamPropertyAlembic(obj, "angularvelocity", velProp))
	{
		velPtr = velProp.getValue(floorIndex);
		return (velPtr != NULL && velPtr->size() != 0);
	}
	return false;
}

MStatus AlembicPointsNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   ESS_PROFILE_SCOPE("AlembicPointsNode::compute");
   if ( !( plug == mOutput ) )
      return ( MS::kUnknownParameter );

   MStatus status;

   // from the maya api examples (ownerEmitter.cpp)
	 const int multiIndex = plug.logicalIndex( &status );
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
   if (status != MS::kSuccess)
	   return status;

   // update the frame number to be imported
   const double inputTime = dataBlock.inputValue(mTimeAttr).asTime().as(MTime::kSeconds);
   const MString &fileName   = dataBlock.inputValue(mFileNameAttr).asString();
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
   if(mLastInputTime == inputTime && mLastSampleInfo.floorIndex == sampleInfo.floorIndex && mLastSampleInfo.ceilIndex == sampleInfo.ceilIndex)
      return MStatus::kSuccess;

   mLastInputTime = inputTime;
   mLastSampleInfo = sampleInfo;

   // access the points values
   AbcG::IPointsSchema::Sample sample;
   mSchema.get(sample,sampleInfo.floorIndex);

   // get all of the data from alembic
   Abc::UInt64ArraySamplePtr sampleIds = sample.getIds();
   Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   Abc::V3fArraySamplePtr sampleVel = sample.getVelocities();

   Abc::C4fArraySamplePtr sampleColor;
   {
     Abc::IC4fArrayProperty propColor;
     if( getArbGeomParamPropertyAlembic( obj, "color", propColor ) )
	     sampleColor = propColor.getValue(sampleInfo.floorIndex);
   }
   Abc::FloatArraySamplePtr sampleAge;
   {
     Abc::IFloatArrayProperty propAge;
     if( getArbGeomParamPropertyAlembic( obj, "age", propAge ) )
	     sampleAge = propAge.getValue(sampleInfo.floorIndex);
   }
   Abc::FloatArraySamplePtr sampleMass;
   {
     Abc::IFloatArrayProperty propMass;
     if( getArbGeomParamPropertyAlembic( obj, "mass", propMass ) )
       sampleMass = propMass.getValue(sampleInfo.floorIndex);
   }
   Abc::UInt16ArraySamplePtr sampleShapeInstanceID;
   {
     Abc::IUInt16ArrayProperty propShapeInstanceID;
     if( getArbGeomParamPropertyAlembic( obj, "shapeinstanceid", propShapeInstanceID ) )
       sampleShapeInstanceID = propShapeInstanceID.getValue(sampleInfo.floorIndex);   
   }
   Abc::QuatfArraySamplePtr sampleOrientation;
   {
     Abc::IQuatfArrayProperty propOrientation;
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
      if(sampleIds->get()[0] == (Abc::uint64_t)-1)
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
      const bool validVel = sampleVel && sampleVel->get();
      const bool validCol = sampleColor && sampleColor->get();
      const bool validAge = sampleAge && sampleAge->get();
      const bool validMas = sampleMass && sampleMass->get();
      const bool validSid = sampleShapeInstanceID && sampleShapeInstanceID->get();
      const bool validOri = sampleOrientation && sampleOrientation->get();
      const float timeAlpha = getTimeOffsetFromSchema( mSchema, sampleInfo );

	  const bool velUseFirstSample = validVel && (sampleVel->size() == 1);
	  const bool colUseFirstSample = validCol && (sampleColor->size() == 1);
	  const bool ageUseFirstSample = validAge && (sampleAge->size() == 1);
	  const bool masUseFirstSample = validMas && (sampleMass->size() == 1);
	  const bool sidUseFirstSample = validSid && (sampleShapeInstanceID->size() == 1);
	  const bool oriUseFirstSample = validOri && (sampleOrientation->size() == 1);
      for(unsigned int i=0; i<particleCount; ++i)
      {
         const Alembic::Abc::V3f &pout = samplePos->get()[i];
         MVector &pin = positions[i];
         pin.x = pout.x;
         pin.y = pout.y;
         pin.z = pout.z;

         if(validVel)
         {
			 const Alembic::Abc::V3f &out = sampleVel->get()[velUseFirstSample ? 0 : i];
            MVector &in = velocities[i];
            in.x = out.x;
            in.y = out.y;
            in.z = out.z;

            pin += in * timeAlpha;
         }

         if(validCol)
         {
			 const Alembic::Abc::C4f &out = sampleColor->get()[colUseFirstSample ? 0 : i];
            MVector &in = rgbs[i];
            in.x = out.r;
            in.y = out.g;
            in.z = out.b;
            opacities[i] = out.a;
         }
         else
         {
            rgbs[i] = MVector(0.0,0.0,0.0);
            opacities[i] = 1.0;
         }

		 ages[i]          = validAge ? sampleAge->get()[ageUseFirstSample ? 0 : i] : 0.0;
		 masses[i]        = validMas ? sampleMass->get()[masUseFirstSample ? 0 : i] : 1.0;
		 shapeInstId[i]   = validSid ? sampleShapeInstanceID->get()[sidUseFirstSample ? 0 : i] : 0.0;
      }

		// compute the right orientation with the angular velocity if necessary!
		if (validOri)
		{
			Abc::QuatfArraySamplePtr velPtr;
			const bool useAngVel = timeAlpha != 0.0f && useAngularVelocity(velPtr, obj, sampleInfo.floorIndex);
			const bool angUseFirstSample = useAngVel && (velPtr->size() == 1);
			for(unsigned int i = 0; i < particleCount; ++i)
			{
				const Abc::Quatf angVel = ( useAngVel ? velPtr->get()[angUseFirstSample ? 0 : i] * timeAlpha : Abc::Quatf() );
				orientationPP[i] = quaternionToVector(sampleOrientation->get()[oriUseFirstSample ? 0 : i], angVel);
			}
		}
		else
		{
			for(unsigned int i = 0; i < particleCount; ++i)
				orientationPP[i] = MVector::zero;
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

  {
    Abc::IUInt16ArrayProperty propShapeT;
    if( !getArbGeomParamPropertyAlembic( obj, "shapetype", propShapeT ) )
      return;

    Abc::UInt16ArraySamplePtr sampleShapeT = propShapeT.getValue(propShapeT.getNumSamples()-1);
    if (!sampleShapeT || sampleShapeT->size() <= 0 || sampleShapeT->get()[0] != 7)
      return;
  }

  // check if there's instance names! If so, load the names!... if there's any!
  Abc::IStringArrayProperty propInstName;
  if(!getArbGeomParamPropertyAlembic( obj, "instancenames", propInstName ) )
    return;

  Abc::StringArraySamplePtr instNames = propInstName.getValue(propInstName.getNumSamples()-1);
  const int nbNames = (int) instNames->size();
  if (nbNames < 1 || (nbNames == 1 && instNames->get()[0].length() == 0) )
    return;

  MString addObjectCmd = " -addObject";
  {
    const MString _obj = " -object ";
    for (int i = 0; i < nbNames; ++i)
    {
      std::string res = instNames->get()[i];
	  if (res.length() == 0)
		  continue;

	  size_t pos = res.find("/");
	  int repl = 0;
	  while (pos != std::string::npos)
	  {
			++repl;
			res[pos] = '|';
			pos = res.find("/", pos);
	  }
	  res = removeInvalidCharacter(res, true);

	  if (repl == 1)	// repl == 1 means it might be just a shape... need this for backward compatibility!
	  {
		MSelectionList sl;
		sl.add(res.substr(1).c_str());
		MDagPath dag;
		sl.getDagPath(0,dag);
		res = dag.fullPathName().asChar();
	  }
      addObjectCmd += _obj + res.c_str();
    }
    addObjectCmd += " ";
  }

  // create the particle instancer and assign shapeInstanceIdPP as the object index!
  MString particleShapeName, mInstancerName;
  MGlobal::executeCommand("string $___connectInfo[] = `connectionInfo -dfs " + name() + ".output[0]`;\nplugNode $___connectInfo[0];", particleShapeName);

  MGlobal::executeCommand("particleInstancer " + particleShapeName, mInstancerName);
  const MString partInstCmd = "particleInstancer -e -name " + mInstancerName;
  MGlobal::executeCommand(partInstCmd + " -rotationUnits Radians -rotationOrder XYZ -objectIndex shapeInstanceIdPP -rotation orientationPP " + particleShapeName);
  MGlobal::executeCommand(partInstCmd + addObjectCmd + particleShapeName);
}

MStatus AlembicPostImportPoints(void)
{
  ESS_PROFILE_SCOPE("AlembicPostImportPoints");
  AlembicPointsNodeListIter beg = alembicPointsNodeList.begin(), end = alembicPointsNodeList.end();
  for (; beg != end; ++beg)
    (*beg)->instanceInitialize();
  return MS::kSuccess;
}

AlembicPostImportPointsCommand::AlembicPostImportPointsCommand(void)
{}
AlembicPostImportPointsCommand::~AlembicPostImportPointsCommand(void)
{}

MStatus AlembicPostImportPointsCommand::doIt(const MArgList& args)
{
  return AlembicPostImportPoints();
}

MSyntax AlembicPostImportPointsCommand::createSyntax(void)
{
  return MSyntax();
}

void* AlembicPostImportPointsCommand::creator(void)
{
  return new AlembicPostImportPointsCommand();
}


