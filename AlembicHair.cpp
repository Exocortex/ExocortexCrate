#include "AlembicHair.h"
#include "MetaData.h"
#include <maya/MPoint.h>
#include <maya/MRenderLine.h>
#include <maya/MRenderLineArray.h>

AlembicHair::AlembicHair(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   //MString name = GetUniqueName(truncateName(node.name()));
   MString name = GetUniqueName(node.name());
   mObject = AbcG::OCurves(GetParentObject(),name.asChar(),GetJob()->GetAnimatedTs());

   mSchema = mObject.getSchema();

   // create all properties
   mRadiusProperty = Abc::OFloatArrayProperty(mSchema, ".radius", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
   mColorProperty = Abc::OC4fArrayProperty(mSchema, ".color", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
}

AlembicHair::~AlembicHair()
{
   mObject.reset();
   mSchema.reset();
}

MStatus AlembicHair::Save(double time)
{
  ESS_PROFILE_SCOPE("AlembicHair::Save");
   MStatus status;

   // access the geometry
   MFnPfxGeometry node(GetRef());

   // save the metadata
   SaveMetaData(this);

   // prepare the bounding box
   Abc::Box3d bbox;

   // check if we have the global cache option
   bool globalCache = GetJob()->GetOption(L"exportInGlobalSpace").asInt() > 0;

   MRenderLineArray mainLines, leafLines, flowerLines;
   node.getLineData(mainLines,leafLines,flowerLines, true, false, mNumSamples == 0, false, false, mNumSamples == 0, false, true, globalCache);

   // first we need to count the number of points
   unsigned int vertexCount = 0;
   for(unsigned int i=0;i<(unsigned int)mainLines.length();i++)
      vertexCount += mainLines.renderLine(i,&status).getLine().length();

   mPosVec.resize(vertexCount);
   unsigned int offset = 0;
   if(mNumSamples==0)
   {
      mNbVertices.resize((size_t)mainLines.length());
      mRadiusVec.resize(vertexCount);
      mColorVec.resize(vertexCount);
      for(unsigned int i=0;i<(unsigned int)mainLines.length();i++)
      {
         const MRenderLine & line = mainLines.renderLine(i,&status);
         const MVectorArray & positions = line.getLine();
         const MDoubleArray & radii = line.getWidth();
         const MVectorArray & colors = line.getColor();
         const MVectorArray & transparencies = line.getTransparency();
         mNbVertices[i] = positions.length();

         for(unsigned int j=0;j<positions.length();j++)
         {
            mPosVec[offset].x = (float)positions[j].x;
            mPosVec[offset].y = (float)positions[j].y;
            mPosVec[offset].z = (float)positions[j].z;
            mRadiusVec[offset] = (float)radii[j];
            mColorVec[offset].r = (float)colors[j].x;
            mColorVec[offset].g = (float)colors[j].y;
            mColorVec[offset].b = (float)colors[j].z;
            mColorVec[offset].a = 1.0f - (float)transparencies[j].x;
            offset++;
         }
      }
   }
   else
   {
      for(unsigned int i=0;i<(unsigned int)mainLines.length();i++)
      {
         const MRenderLine & line = mainLines.renderLine(i,&status);
         const MVectorArray & positions = line.getLine();
         for(unsigned int j=0;j<positions.length();j++)
         {
            mPosVec[offset].x = (float)positions[j].x;
            mPosVec[offset].y = (float)positions[j].y;
            mPosVec[offset].z = (float)positions[j].z;
            offset++;
         }
      }
   }

   // store the positions to the samples
   mSample.setPositions(Abc::P3fArraySample(&mPosVec.front(),mPosVec.size()));
   mSample.setSelfBounds(bbox);

   if(mNumSamples == 0)
   {
      mSample.setCurvesNumVertices(Abc::Int32ArraySample(mNbVertices));
      mSample.setWrap(AbcG::kNonPeriodic);
      mSample.setType(AbcG::kLinear);

      mRadiusProperty.set(Abc::FloatArraySample(&mRadiusVec.front(),mRadiusVec.size()));
      mColorProperty.set(Abc::C4fArraySample(&mColorVec.front(),mColorVec.size()));
   }

   // save the sample
   mSchema.set(mSample);
   mNumSamples++;

   return MStatus::kSuccess;
}

/*
void AlembicHairNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicHairNode::~AlembicHairNode()
{
   PreDestruction();
}

MObject AlembicHairNode::mTimeAttr;
MObject AlembicHairNode::mFileNameAttr;
MObject AlembicHairNode::mIdentifierAttr;
MObject AlembicHairNode::mOutGeometryAttr;

MStatus AlembicHairNode::initialize()
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

   // output curve
   mOutGeometryAttr = tAttr.create("outCurve", "os", MFnData::kNurbsCurve);
   status = tAttr.setStorable(false);
   status = tAttr.setWritable(false);
   status = tAttr.setKeyable(false);
   status = tAttr.setHidden(false);
   status = addAttribute(mOutGeometryAttr);

   // create a mapping
   status = attributeAffects(mTimeAttr, mOutGeometryAttr);
   status = attributeAffects(mFileNameAttr, mOutGeometryAttr);
   status = attributeAffects(mIdentifierAttr, mOutGeometryAttr);
   return status;
}

MStatus AlembicHairNode::compute(const MPlug & plug, MDataBlock & dataBlock)
{
   MStatus status;

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
      Abc::IObject iObj = getObjectFromArchive(mFileName,identifier);
      if(!iObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      AbcG::ICurves obj(iObj,Abc::kWrapExisting);
      if(!obj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a Curves.");
         return MStatus::kFailure;
      }
      mSchema = obj.getSchema();
      mCurvesData = MObject::kNullObj;
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
   if(!mCurvesData.isNull() && mLastSampleInfo.floorIndex == sampleInfo.floorIndex && mLastSampleInfo.ceilIndex == sampleInfo.ceilIndex)
      return MStatus::kSuccess;

   mLastSampleInfo = sampleInfo;

   // access the camera values
   AbcG::ICurvesSchema::Sample sample;
   AbcG::ICurvesSchema::Sample sample2;
   mSchema.get(sample,sampleInfo.floorIndex);
   if(sampleInfo.alpha != 0.0)
      mSchema.get(sample2,sampleInfo.ceilIndex);

   // create the output subd
   if(mCurvesData.isNull())
   {
      MFnNurbsCurveData curveDataFn;
      mCurvesData = curveDataFn.create();
   }

   Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   if(sample.getNumCurves() > 1)
   {
      MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' contains more than one curve.");
      return MStatus::kFailure;
   }

   Abc::Int32ArraySamplePtr nbVertices = sample.getCurvesNumVertices();
   unsigned int nbCVs = (unsigned int)nbVertices->get()[0];
   int degree = 1;
   if(sample.getType() == AbcG::kCubic)
      degree = 3;
   bool closed = sample.getWrap() == AbcG::kPeriodic;
   int nbSpans = (int)nbCVs - degree;

   MDoubleArray knots;
   for(int span = 0; span <= nbSpans; span++)
   {
      knots.append(double(span));
      if(span == 0 || span == nbSpans)
      {
         for(int m=1; m<degree; m++)
            knots.append(double(span));
      }
   }

   MPointArray points;
   if(samplePos->size() > 0)
   {
      points.setLength((unsigned int)samplePos->size());
      bool done = false;
      if(sampleInfo.alpha != 0.0)
      {
         Abc::P3fArraySamplePtr samplePos2 = sample2.getPositions();
         if(points.length() == (unsigned int)samplePos2->size())
         {
            float blend = (float)sampleInfo.alpha;
            float iblend = 1.0f - blend;
            for(unsigned int i=0;i<points.length();i++)
            {
               points[i].x = samplePos->get()[i].x * iblend + samplePos2->get()[i].x * blend;
               points[i].y = samplePos->get()[i].y * iblend + samplePos2->get()[i].y * blend;
               points[i].z = samplePos->get()[i].z * iblend + samplePos2->get()[i].z * blend;
            }
            done = true;
         }
      }

      if(!done)
      {
         for(unsigned int i=0;i<points.length();i++)
         {
            points[i].x = samplePos->get()[i].x;
            points[i].y = samplePos->get()[i].y;
            points[i].z = samplePos->get()[i].z;
         }
      }
   }
   
   // create a subd either with or without uvs
   mCurves.create(points,knots,degree,closed ? MFnNurbsCurve::kClosed : MFnNurbsCurve::kOpen, false, false, mCurvesData);

   // output all channels
   dataBlock.outputValue(mOutGeometryAttr).set(mCurvesData);

   return MStatus::kSuccess;
}

void AlembicHairDeformNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicHairDeformNode::~AlembicHairDeformNode()
{
   PreDestruction();
}

MObject AlembicHairDeformNode::mTimeAttr;
MObject AlembicHairDeformNode::mFileNameAttr;
MObject AlembicHairDeformNode::mIdentifierAttr;

MStatus AlembicHairDeformNode::initialize()
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
   status = attributeAffects(mTimeAttr, outputGeom);
   status = attributeAffects(mFileNameAttr, outputGeom);
   status = attributeAffects(mIdentifierAttr, outputGeom);

   return status;
}

MStatus AlembicHairDeformNode::deform(MDataBlock & dataBlock, MItGeometry & iter, const MMatrix & localToWorld, unsigned int geomIndex)
{
   // get the envelope data
   float env = dataBlock.inputValue( envelope ).asFloat();
   if(env == 0.0f) // deformer turned off
      return MStatus::kSuccess;

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
      Abc::IObject iObj = getObjectFromArchive(mFileName,identifier);
      if(!iObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      AbcG::ICurves obj(iObj,Abc::kWrapExisting);
      if(!obj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' is not a Curves.");
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

   // access the camera values
   AbcG::ICurvesSchema::Sample sample;
   AbcG::ICurvesSchema::Sample sample2;
   mSchema.get(sample,sampleInfo.floorIndex);
   if(sampleInfo.alpha != 0.0)
      mSchema.get(sample2,sampleInfo.ceilIndex);

   Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   Abc::P3fArraySamplePtr samplePos2;
   if(sampleInfo.alpha != 0.0)
      samplePos2 = sample2.getPositions();

   // iteration should not be necessary. the iteration is only 
   // required if the same mesh is attached to the same deformer
   // several times
   float blend = (float)sampleInfo.alpha;
   float iblend = 1.0f - blend;
   unsigned int index = 0;
   for(iter.reset();!iter.isDone(); iter.next())
   {
      index = iter.index();
      MFloatPoint pt = iter.position();
      MPoint abcPos = pt;
      float weight = weightValue(dataBlock,geomIndex,index) * env;
      if(weight == 0.0f)
         continue;
      float iweight = 1.0f - weight;
      if(index >= samplePos->size())
         continue;
      bool done = false;
      if(sampleInfo.alpha != 0.0)
      {
         if(samplePos2->size() == samplePos->size())
         {
            abcPos.x = iweight * pt.x + weight * (samplePos->get()[index].x * iblend + samplePos2->get()[index].x * blend);
            abcPos.y = iweight * pt.y + weight * (samplePos->get()[index].y * iblend + samplePos2->get()[index].y * blend);
            abcPos.z = iweight * pt.z + weight * (samplePos->get()[index].z * iblend + samplePos2->get()[index].z * blend);
            done = true;
         }
      }
      if(!done)
      {
         abcPos.x = iweight * pt.x + weight * samplePos->get()[index].x;
         abcPos.y = iweight * pt.y + weight * samplePos->get()[index].y;
         abcPos.z = iweight * pt.z + weight * samplePos->get()[index].z;
      }
      iter.setPosition(abcPos);
   }
   return MStatus::kSuccess;
}
*/
