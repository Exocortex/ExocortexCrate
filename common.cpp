#include "stdafx.h"
#include "common.h"


std::string resolvePath_Internal(std::string path)
{
	return path;
}


// ----------------------------------------------------------------------------------------------------
// objectInfo constructor
objectInfo::objectInfo(float in_centroidTime):
      abc(), node(NULL), centroidTime(in_centroidTime), hide(false), ID(-1), instanceID(-1), instanceGroupID(-1), instanceCloud(NULL), suffix("_DSO")
{
}

// ----------------------------------------------------------------------------------------------------
// functions!
std::string getNameFromIdentifier(const std::string &identifier, long id, long group)
{
  std::string result;
  std::vector<std::string> parts;
  boost::split(parts, identifier, boost::is_any_of("/\\"));
  result = parts[parts.size()-1];
  for(int i=(int)parts.size()-3;i>=0;i--)
  {
    if(parts[i].empty())
      break;
    std::string suffix = parts[i].substr(parts[i].length()-3,3);
    if(suffix == "xfo" || suffix == "XFO" || suffix == "Xfo")
      result = parts[i].substr(0,parts[i].length()-3) + "." + result;
    else
      result = suffix + "." + result;
  }

  if(id >= 0)
    result += "."+boost::lexical_cast<std::string>(id);
  if(group >= 0)
    result += "."+boost::lexical_cast<std::string>(group);
  return result;
}

bool shiftedProcessing(nodeData &nodata, userData * ud)
{
  const int gInsSize = (int) ud->gInstances.size();
  std::string objFullName = nodata.object.getFullName();
  for(size_t j=0; j < gInsSize; ++j)
  {
    instanceCloudInfo &gInst = ud->gInstances[j];
    const int jInstanceInfoSize = (int) gInst.groupInfos.size();
    for(size_t k=0; k < jInstanceInfoSize; ++k)
    {
      instanceGroupInfo &gInfo = gInst.groupInfos[k];
      const int kInfIdSize = (int) gInfo.identifiers.size();
      for(size_t l=0; k < kInfIdSize; ++l)
      {
        if(gInfo.identifiers[l] == objFullName)
        {
          std::map<float,AtNode*>::iterator it = gInfo.nodes[l].find(ud->gCentroidTime);
          if(it != gInfo.nodes[l].end())
          {
            if(it->second != NULL)
              nodata.shaders = AiNodeGetArray(it->second, "shader");
          }
          return true;
        }
      }
    }
  }
  return false;
}

