#ifndef __XSI_SCENE_GRAPH_H
#define __XSI_SCENE_GRAPH_H

#include "CommonImport.h"
#include "CommonSceneGraph.h"

class SceneNodeXSI : public SceneNodeApp {
 public:
  XSI::CRef nodeRef;
  bool bMergedSubtreeNodeParent;

  SceneNodeXSI(XSI::CRef ref) : nodeRef(ref), bMergedSubtreeNodeParent(false) {}
  SceneNodeXSI(const SceneNodeXSI& n, bool mergedSubtreeNodeParent)
      : nodeRef(n.nodeRef), bMergedSubtreeNodeParent(mergedSubtreeNodeParent)
  {
  }

  virtual bool replaceData(SceneNodeAlembicPtr fileNode,
                           const IJobStringParser& jobParams,
                           SceneNodeAlembicPtr& nextFileNode);
  virtual bool addChild(SceneNodeAlembicPtr fileNode,
                        const IJobStringParser& jobParams,
                        SceneNodeAppPtr& newAppNode);
  virtual void print();

  virtual Imath::M44f getGlobalTransFloat(double time);
  virtual Imath::M44d getGlobalTransDouble(double time);
  virtual bool getVisibility(double time);
};

typedef boost::shared_ptr<SceneNodeXSI> SceneNodeXSIPtr;

SceneNodeXSIPtr buildCommonSceneGraph(XSI::CRef xsiRoot, int& nNumNodes,
                                      bool bUnmergeNodes, bool bSelectAll);

XSI_XformTypes::xte getXformType(AbcG::IXform& obj);

class XSIProgressBar : public CommonProgressBar {
  XSI::ProgressBar prog;
  int nProgress;

 public:
  XSIProgressBar();

  inline void init(int range) { init(0, range, 1); }
  virtual void init(int min, int max, int incr);
  virtual void start(void);
  virtual void stop(void);
  virtual void incr(int step = 1);
  virtual bool isCancelled(void);
  virtual void setCaption(std::string& caption);
  virtual int getUpdateCount() { return 2; }
};

XSI::CRef findTimeControlDccIdentifier(SceneNodeAlembicPtr fileRoot,
                                       XSI::CRef importRoot,
                                       XSI_XformTypes::xte in_xte);

#endif