#ifndef _ALEMBIC_CURVES_H_
#define _ALEMBIC_CURVES_H_

#include <maya/MFnNurbsCurve.h>
#include <maya/MUint64Array.h>
#include "AlembicObject.h"
#include "AttributesWriter.h"

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class AlembicCurveAccumulator;

typedef boost::shared_ptr<AlembicCurveAccumulator> AlembicCurveAccumulatorPtr;

/**
 * For the case where curves should be merged together. This class handles the
 * accumulation of data for each frame and writes it as a single curve object in
 * Alembic.
 */
class AlembicCurveAccumulator {
 private:
  MObject m_ref;       // a reference to the first curve found in this group!
  Abc::M44f mInverse;  // the inverse matrix of the first curve (changed for
  // every frame) so that other curves will have their
  // coordinates into the first curve's space

  AbcG::OCurves mObject;
  AbcG::OCurvesSchema mSchema;
  AbcG::OCurvesSchema::Sample mSample;
  Abc::Box3d bbox;
  bool firstSample;  // when it's the first sample recorded, more stuff are
  // written to the alembic file
  bool useGlobalCache;

  std::vector<Abc::V3f> mPosVec;
  std::vector<AbcA::int32_t> mNbVertices;
  std::vector<float> mRadiusVec;  // TODO: add support for radius and knots.
  // Need to find a way to express the knot
  // vector as parts in the alembic file because
  // it's a different size than the nbVertices!
  std::vector<float> mKnotVec;

  Abc::OFloatArrayProperty mRadiusProperty;
  Abc::OC4fArrayProperty mColorProperty;

 public:
  AlembicCurveAccumulator(const MObject &ref, SceneNodePtr eNode,
                          AlembicWriteJob *in_Job, Abc::OObject oParent);

  // methods
  void startRecording(void);
  void stopRecording(void);
  void save(const MObject &ref, double time);

  // statics
  static void Initialize(void);
  static void StartRecordingFrame(void);
  static void StopRecordingFrame(void);
  static void Destroy(void);
  static AlembicCurveAccumulatorPtr GetAccumulator(int id, const MObject &ref,
                                                   SceneNodePtr eNode,
                                                   AlembicWriteJob *in_Job,
                                                   Abc::OObject oParent);
};

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class AlembicCurves : public AlembicObject {
 private:
  AlembicCurveAccumulatorPtr accumRef;

  AbcG::OCurves mObject;
  AbcG::OCurvesSchema mSchema;
  AbcG::OCurvesSchema::Sample mSample;

  AttributesWriterPtr mAttrs;

  std::vector<Abc::V3f> mPosVec;
  std::vector<AbcA::int32_t> mNbVertices;
  std::vector<float> mRadiusVec;
  std::vector<float> mKnotVec;

  Abc::OV3fArrayProperty mVelocityProperty;
  Abc::OFloatArrayProperty mRadiusProperty;
  Abc::OC4fArrayProperty mColorProperty;
  Abc::OInt32ArrayProperty mFaceIndexProperty;
  Abc::OInt32ArrayProperty mVertexIndexProperty;
  Abc::OFloatArrayProperty mKnotVectorProperty;

 public:
  AlembicCurves(SceneNodePtr eNode, AlembicWriteJob *in_Job,
                Abc::OObject oParent);
  ~AlembicCurves();

  virtual Abc::OObject GetObject() { return mObject; }
  virtual Abc::OCompoundProperty GetCompound() { return mSchema; }
  virtual MStatus Save(double time, unsigned int timeIndex,
      bool isFirstFrame);
};

class AlembicCurvesNode : public AlembicObjectNode {
 public:
  AlembicCurvesNode() {}
  virtual ~AlembicCurvesNode();

  // override virtual methods from MPxNode
  virtual void PreDestruction();
  virtual MStatus compute(const MPlug &plug, MDataBlock &dataBlock);
  static void *creator() { return (new AlembicCurvesNode()); }
  static MStatus initialize();

 private:
  // input attributes
  static MObject mTimeAttr;
  static MObject mFileNameAttr;
  static MObject mIdentifierAttr;
  MString mFileName;
  MString mIdentifier;
  AbcG::ICurves mObj;
  AbcG::ICurvesSchema mSchema;

  // output attributes
  static MObject mOutGeometryAttr;

  // members
  SampleInfo mLastSampleInfo;
  MObject mCurvesData;
  MFnNurbsCurve mCurves;
};

class AlembicCurvesDeformNode : public AlembicObjectDeformNode {
 public:
  virtual ~AlembicCurvesDeformNode();
  // override virtual methods from MPxDeformerNode
  virtual void PreDestruction();
  virtual MStatus deform(MDataBlock &dataBlock, MItGeometry &iter,
                         const MMatrix &localToWorld, unsigned int geomIndex);
  static void *creator() { return (new AlembicCurvesDeformNode()); }
  static MStatus initialize();

 private:
  // input attributes
  static MObject mTimeAttr;
  static MObject mFileNameAttr;
  static MObject mIdentifierAttr;
  MString mFileName;
  MString mIdentifier;
  AbcG::ICurvesSchema mSchema;

  // members
  SampleInfo mLastSampleInfo;
};

class AlembicCurvesLocatorNode : public AlembicObjectLocatorNode {
 public:
  AlembicCurvesLocatorNode();
  virtual ~AlembicCurvesLocatorNode();

  // override virtual methods from MPxDeformerNode
  virtual void PreDestruction();
  virtual MStatus compute(const MPlug &plug, MDataBlock &dataBlock);
  virtual void draw(M3dView &view, const MDagPath &path,
                    M3dView::DisplayStyle style, M3dView::DisplayStatus status);
  static void *creator() { return (new AlembicCurvesLocatorNode()); }
  static MStatus initialize();
  virtual bool isBounded() const { return true; }
  MBoundingBox boundingBox() const;

 private:
  // private methods
  bool updated();

  // input attributes
  static MObject mTimeAttr;
  static MObject mFileNameAttr;
  static MObject mIdentifierAttr;
  static MObject mSentinelAttr;
  MString mFileName;
  MString mIdentifier;
  AbcG::ICurvesSchema mSchema;
  int mSent;
  unsigned int mNbCurves;
  unsigned int mNbVertices;
  std::vector<unsigned int> mIndices;
  std::vector<Abc::V3f> mPositions;
  std::vector<Abc::C4f> mColors;
  MBoundingBox mBoundingBox;

  // members
  SampleInfo mLastSampleInfo;
};

#endif
