#ifndef __XSI_SCENE_GRAPH_H
#define __XSI_SCENE_GRAPH_H

#include "CommonSceneGraph.h"
#include "CommonImport.h"

class SceneNodeXSI : public SceneNodeApp
{
public:

   XSI::CRef nodeRef;

   SceneNodeXSI(XSI::CRef ref):nodeRef(ref)
   {}

   virtual bool replaceData(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAlembicPtr& nextFileNode);
   virtual bool addChild(SceneNodeAlembicPtr fileNode, const IJobStringParser& jobParams, SceneNodeAppPtr& newAppNode);
   virtual void print();
};

typedef boost::shared_ptr<SceneNodeXSI> SceneNodeXSIPtr;

SceneNodeXSIPtr buildCommonSceneGraph(XSI::CRef xsiRoot, int& nNumNodes, bool bUnmergeNodes=true);

bool hasExtractableTransform( SceneNode::nodeTypeE type );


XSI_XformTypes::xte getXformType(AbcG::IXform& obj);

class XSIProgressBar: public CommonProgressBar
{
   XSI::ProgressBar prog;
public:
   XSIProgressBar();

   inline void init(int range) { init(0, range, 1); }
   virtual void init(int min, int max, int incr);
   virtual void start(void);
   virtual void stop(void);
   virtual void incr(int step=1);
   virtual bool isCancelled(void);
   virtual void setCaption(std::string& caption);
   virtual int getUpdateCount(){ return 2; }
};

XSI::CRef findTimeControlDccIdentifier(SceneNodeAlembicPtr fileRoot, XSI::CRef importRoot);


#endif