#ifndef _ARNOLD_ALEMBIC_COMMMON_H_
#define _ARNOLD_ALEMBIC_COMMMON_H_


#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif


  #include "utility.h"
  #include "dataUniqueness.h"

  /**
   * Declaration of data structures, constants and other used in multiple places!
   */

  struct instanceGroupInfo
  {
     std::vector<std::string> identifiers;
     std::vector<std::map<float,AtNode*> > nodes;
     std::vector<Alembic::Abc::IObject> objects;
     std::vector<Alembic::Abc::IObject> parents;
     std::vector<std::map<float,std::vector<Alembic::Abc::M44f> > > matrices;
  };

  struct instanceCloudInfo
  {
     std::vector<Alembic::Abc::P3fArraySamplePtr> pos;
     std::vector<Alembic::Abc::V3fArraySamplePtr> vel;
     std::vector<Alembic::Abc::UInt64ArraySamplePtr> id;
     std::vector<Alembic::Abc::FloatArraySamplePtr> width;
     std::vector<Alembic::Abc::V3fArraySamplePtr> scale;
     std::vector<Alembic::Abc::QuatfArraySamplePtr> rot;
     std::vector<Alembic::Abc::QuatfArraySamplePtr> ang;
     std::vector<Alembic::Abc::FloatArraySamplePtr> age;
     std::vector<Alembic::Abc::FloatArraySamplePtr> mass;
     std::vector<Alembic::Abc::C4fArraySamplePtr> color;
     std::vector<Alembic::Abc::UInt16ArraySamplePtr> shape;
     std::vector<Alembic::Abc::FloatArraySamplePtr> time;
     float timeAlpha;
     std::vector<instanceGroupInfo> groupInfos;
  };

  struct objectInfo
  {
     Alembic::Abc::IObject abc;
     AtNode * node;
     float centroidTime;
     bool hide;
     long ID;
     long instanceID;
     long instanceGroupID;
     instanceCloudInfo * instanceCloud;
     std::string suffix;

     objectInfo(float in_centroidTime);
  };

  struct userData
  {
     float gCentroidTime;
     char * gDataString;
     std::string gCurvesMode;
     std::string gPointsMode;
     AtArray * gProcShaders;
     AtArray * gProcDispMap;
     std::vector<objectInfo> gIObjects;
     std::vector<instanceCloudInfo> gInstances;
     std::vector<float> gMbKeys;
     float gTime;
     float gCurrTime;
     int proceduralDepth;

     std::vector<AtNode*> constructedNodes;
     std::vector<AtArray*> shadersToAssign;

     bool has_subdiv_settings;
     std::string subdiv_type;
     int subdiv_iterations;
     float subdiv_pixel_error;
     std::string subdiv_dicing_camera;
     bool has_disp_settings;
     float disp_zero_value;
     float disp_height;
     bool disp_autobump;
     float disp_padding;
  };

  // General Node Data
  struct nodeData
  {
    std::vector<float> samples;
    bool shifted,
         createdShifted,
         isPolyMeshNode;
    Alembic::Abc::IObject object;
    AtArray *shaders;
    AtArray *shaderIndices;
  };

  #define sampleTolerance 0.00001
  #define gCentroidPrecision 50.0f
  #define roundCentroid(x) floorf(x * gCentroidPrecision) / gCentroidPrecision

  // Reads the parameter value from data file and assign it to node
  AtVoid ReadParameterValue(AtNode* curve_node, FILE* fp, const AtChar* param_name);

  std::string getNameFromIdentifier(const std::string & identifier, long id = -1, long group = -1);

  // executed only if (nodata.shifted), same code in multiple places, in a single unique function!
  // return true if a shader has been assigned!
  bool shiftedProcessing(nodeData &nodata, userData * ud);

#endif
