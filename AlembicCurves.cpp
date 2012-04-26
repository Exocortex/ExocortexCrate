#include "AlembicCurves.h"
#include <maya/MFnNurbsCurveData.h>
#include "MetaData.h"
#include <maya/MPoint.h>
#include <maya/MPointArray.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

AlembicCurves::AlembicCurves(const MObject & in_Ref, AlembicWriteJob * in_Job)
: AlembicObject(in_Ref, in_Job)
{
   MFnDependencyNode node(in_Ref);
   MString name = GetUniqueName(truncateName(node.name()));
   mObject = Alembic::AbcGeom::OCurves(GetParentObject(),name.asChar(),GetJob()->GetAnimatedTs());

   mSchema = mObject.getSchema();

   // create all properties
   mRadiusProperty = Alembic::Abc::OFloatArrayProperty(mSchema, ".radius", mSchema.getMetaData(), GetJob()->GetAnimatedTs() );
}

AlembicCurves::~AlembicCurves()
{
   mObject.reset();
   mSchema.reset();
}

MStatus AlembicCurves::Save(double time)
{
   // access the geometry
   MFnNurbsCurve node(GetRef());

   // save the metadata
   SaveMetaData(this);

   // prepare the bounding box
   Alembic::Abc::Box3d bbox;

   // check if we have the global cache option
   bool globalCache = GetJob()->GetOption(L"exportInGlobalSpace").asInt() > 0;
   Alembic::Abc::M44f globalXfo;
   if(globalCache)
      globalXfo = GetGlobalMatrix(GetRef());

   MPointArray positions;
   node.getCVs(positions);

   mPosVec.resize(positions.length());
   for(unsigned int i=0;i<positions.length();i++)
   {
      mPosVec[i].x = (float)positions[i].x;
      mPosVec[i].y = (float)positions[i].y;
      mPosVec[i].z = (float)positions[i].z;
      if(globalCache)
         globalXfo.multVecMatrix(mPosVec[i],mPosVec[i]);
      bbox.extendBy(mPosVec[i]);
   }

   // store the positions to the samples
   mSample.setPositions(Alembic::Abc::P3fArraySample(&mPosVec.front(),mPosVec.size()));
   mSample.setSelfBounds(bbox);

   if(mNumSamples == 0)
   {
      mNbVertices.push_back(node.numCVs());
      mSample.setCurvesNumVertices(Alembic::Abc::Int32ArraySample(mNbVertices));

      if (node.form() == MFnNurbsCurve::kOpen)
         mSample.setWrap(Alembic::AbcGeom::kNonPeriodic);
      else
         mSample.setWrap(Alembic::AbcGeom::kPeriodic);
      if (node.degree() == 3)
         mSample.setType(Alembic::AbcGeom::kCubic);
      else
         mSample.setType(Alembic::AbcGeom::kLinear);

      MPlug widthPlug = node.findPlug("width");
      if (!widthPlug.isNull())
         mRadiusVec.push_back(widthPlug.asFloat());
      else
         mRadiusVec.push_back(1.0);

      mRadiusProperty.set(Alembic::Abc::FloatArraySample(&mRadiusVec.front(),mRadiusVec.size()));
   }

   // save the sample
   mSchema.set(mSample);
   mNumSamples++;

   return MStatus::kSuccess;
}

void AlembicCurvesNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicCurvesNode::~AlembicCurvesNode()
{
   PreDestruction();
}

MObject AlembicCurvesNode::mTimeAttr;
MObject AlembicCurvesNode::mFileNameAttr;
MObject AlembicCurvesNode::mIdentifierAttr;
MObject AlembicCurvesNode::mOutGeometryAttr;

MStatus AlembicCurvesNode::initialize()
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

MStatus AlembicCurvesNode::compute(const MPlug & plug, MDataBlock & dataBlock)
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
      Alembic::Abc::IObject iObj = getObjectFromArchive(mFileName,identifier);
      if(!iObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      Alembic::AbcGeom::ICurves obj(iObj,Alembic::Abc::kWrapExisting);
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
   Alembic::AbcGeom::ICurvesSchema::Sample sample;
   Alembic::AbcGeom::ICurvesSchema::Sample sample2;
   mSchema.get(sample,sampleInfo.floorIndex);
   if(sampleInfo.alpha != 0.0)
      mSchema.get(sample2,sampleInfo.ceilIndex);

   // create the output subd
   if(mCurvesData.isNull())
   {
      MFnNurbsCurveData curveDataFn;
      mCurvesData = curveDataFn.create();
   }

   Alembic::Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   if(sample.getNumCurves() > 1)
   {
      MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' in archive '"+mFileName+"' contains more than one curve.");
      return MStatus::kFailure;
   }

   Alembic::Abc::Int32ArraySamplePtr nbVertices = sample.getCurvesNumVertices();
   unsigned int nbCVs = (unsigned int)nbVertices->get()[0];
   int degree = 1;
   if(sample.getType() == Alembic::AbcGeom::kCubic)
      degree = 3;
   bool closed = sample.getWrap() == Alembic::AbcGeom::kPeriodic;
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
         Alembic::Abc::P3fArraySamplePtr samplePos2 = sample2.getPositions();
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

void AlembicCurvesDeformNode::PreDestruction()
{
   mSchema.reset();
   delRefArchive(mFileName);
   mFileName.clear();
}

AlembicCurvesDeformNode::~AlembicCurvesDeformNode()
{
   PreDestruction();
}

MObject AlembicCurvesDeformNode::mTimeAttr;
MObject AlembicCurvesDeformNode::mFileNameAttr;
MObject AlembicCurvesDeformNode::mIdentifierAttr;

MStatus AlembicCurvesDeformNode::initialize()
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

MStatus AlembicCurvesDeformNode::deform(MDataBlock & dataBlock, MItGeometry & iter, const MMatrix & localToWorld, unsigned int geomIndex)
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
      Alembic::Abc::IObject iObj = getObjectFromArchive(mFileName,identifier);
      if(!iObj.valid())
      {
         MGlobal::displayWarning("[ExocortexAlembic] Identifier '"+identifier+"' not found in archive '"+mFileName+"'.");
         return MStatus::kFailure;
      }
      Alembic::AbcGeom::ICurves obj(iObj,Alembic::Abc::kWrapExisting);
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
   Alembic::AbcGeom::ICurvesSchema::Sample sample;
   Alembic::AbcGeom::ICurvesSchema::Sample sample2;
   mSchema.get(sample,sampleInfo.floorIndex);
   if(sampleInfo.alpha != 0.0)
      mSchema.get(sample2,sampleInfo.ceilIndex);

   Alembic::Abc::P3fArraySamplePtr samplePos = sample.getPositions();
   Alembic::Abc::P3fArraySamplePtr samplePos2;
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
      //MFloatPoint pt = iter.position();
      MPoint pt = iter.position();
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
