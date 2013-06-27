#ifndef _ALEMBIC_POINTS_H_
#define _ALEMBIC_POINTS_H_

#include "AlembicObject.h"
#include <maya/MFnParticleSystem.h>
#include <maya/MFnInstancer.h>
#include <list>

class AlembicPoints: public AlembicObject
{
private:
   bool hasInstancer;
   MString instName;

   AbcG::OPoints mObject;
   AbcG::OPointsSchema mSchema;
   AbcG::OPointsSchema::Sample mSample;

   Abc::OFloatArrayProperty mAgeProperty;
   Abc::OFloatArrayProperty mMassProperty;
   Abc::OC4fArrayProperty mColorProperty;

   Abc::OQuatfArrayProperty mAngularVelocityProperty;
   Abc::OStringArrayProperty mInstanceNamesProperty;    // only written once, when mNumSample == 0
   Abc::OQuatfArrayProperty mOrientationProperty;
   Abc::OV3fArrayProperty mScaleProperty;
   Abc::OUInt16ArrayProperty mShapeInstanceIdProperty; 
   Abc::OFloatArrayProperty mShapeTimeProperty;
   Abc::OUInt16ArrayProperty mShapeTypeProperty;

   // instancing functions
   bool listIntanceNames(std::vector<std::string> &names);
   bool sampleInstanceProperties(std::vector<Abc::Quatf> angularVel, std::vector<Abc::Quatf> orientation, std::vector<Abc::uint16_t> shapeId, std::vector<Abc::uint16_t> shapeType, std::vector<Abc::float32_t> shapeTime);

public:
   AlembicPoints(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent);
   ~AlembicPoints();

   virtual Abc::OObject GetObject() { return mObject; }
   virtual Abc::OCompoundProperty GetCompound() { return mSchema; }
   virtual MStatus Save(double time);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// import

class BasePropertyManager
{
public:
	std::string propName, attrName;
	Abc::ICompoundProperty comp;

	BasePropertyManager(const std::string &name, const Abc::ICompoundProperty &cmp): propName(name), attrName(name), comp(cmp)
	{
		if (attrName[0] == '.')
			attrName = attrName.substr(1);
		attrName += "PP";
	}

	virtual void readFromParticle(const MFnParticleSystem &part) = 0;
	virtual void readFromAbc(Alembic::AbcCoreAbstract::index_t floorIndex, const unsigned int particleCount) = 0;
	virtual void setParticleProperty(MFnParticleSystem &part) = 0;
	virtual void initPerParticle(const MString &partName) = 0;
};
typedef boost::shared_ptr<BasePropertyManager> BasePropertyManagerPtr;

class ArbGeomProperties
{
public:
	ArbGeomProperties(const Abc::ICompoundProperty &comp);

	// functions
	inline bool isValid(void) const { return valid; }

	void initPerParticle(const MString &partName);
	void readFromParticle(const MFnParticleSystem &part);
	void readFromAbc(Alembic::AbcCoreAbstract::index_t floorIndex, const unsigned int particleCount);
	void setParticleProperty(MFnParticleSystem &part);

private:
	// private functions
	void constructData(const Abc::ICompoundProperty &comp);

	// private data
	bool valid;
	std::vector<BasePropertyManagerPtr> baseProperties;
};

class AlembicPointsNode;

typedef std::list<AlembicPointsNode*> AlembicPointsNodeList;
typedef AlembicPointsNodeList::iterator AlembicPointsNodeListIter;

class AlembicPointsNode : public AlembicObjectEmitterNode
{
public:
  AlembicPointsNode();
   virtual ~AlembicPointsNode();

   void instanceInitialize(void);

   // override virtual methods from MPxNode
   virtual void PreDestruction();
   virtual void PostConstructor(void);
   virtual MStatus compute(const MPlug & plug, MDataBlock & dataBlock);
   static void* creator() { return (new AlembicPointsNode()); }
   static MStatus initialize();

private:
   // input attributes
   static MObject mTimeAttr;
   static MObject mFileNameAttr;
   static MObject mIdentifierAttr;
   MString mFileName;
   MString mIdentifier;
   AbcG::IPointsSchema mSchema;
   AbcG::IPoints obj;
   AlembicPointsNodeListIter listPosition;

   MStatus init(const MString &filename, const MString &identifier);
   MStatus initPerParticles(const MString &partName);

   // members
   SampleInfo mLastSampleInfo;
   double mLastInputTime;
   ArbGeomProperties *arbGeomProperties;
};

MStatus AlembicPostImportPoints(void);


// TODO: remove this command! from the this cpp and alembic.cpp too!
class AlembicPostImportPointsCommand : public MPxCommand
{
public:
  AlembicPostImportPointsCommand(void);
  virtual ~AlembicPostImportPointsCommand(void);

  virtual bool isUndoable(void) const { return false; }
  MStatus doIt(const MArgList& args);

  static MSyntax createSyntax(void);
  static void* creator(void);
};

#endif