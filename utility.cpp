#include "Foundation.h"
#include "Utility.h"
#include "SceneEntry.h"

SampleInfo getSampleInfo
(
   double iFrame,
   Alembic::AbcCoreAbstract::TimeSamplingPtr iTime,
   size_t numSamps
)
{
   SampleInfo result;
   if (numSamps == 0)
      numSamps = 1;

   std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
   iTime->getFloorIndex(iFrame, numSamps);

   result.floorIndex = floorIndex.first;
   result.ceilIndex = result.floorIndex;

   if (fabs(iFrame - floorIndex.second) < 0.0001) {
      result.alpha = 0.0f;
      return result;
   }

   std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
   iTime->getCeilIndex(iFrame, numSamps);

   if (fabs(iFrame - ceilIndex.second) < 0.0001) {
      result.floorIndex = ceilIndex.first;
      result.ceilIndex = result.floorIndex;
      result.alpha = 0.0f;
      return result;
   }

   if (result.floorIndex == ceilIndex.first) {
      result.alpha = 0.0f;
      return result;
   }

   result.ceilIndex = ceilIndex.first;

   result.alpha = (iFrame - floorIndex.second) / (ceilIndex.second - floorIndex.second);
   return result;
}

std::string buildIdentifierFromRef(const SceneEntry &in_Ref)
{
   std::string result;
   INode *pNode = in_Ref.node;
   while(pNode)
   {
       result = std::string("/")+ std::string(pNode->GetName()) + result;
       result = std::string("/")+ std::string(pNode->GetName()) + std::string("Xfo") + result;
       pNode = pNode->GetParentNode();

       if (pNode->IsRootNode())
           break;
   }

   return result;
}

std::string buildModelIdFromXFormId(const std::string &xformId)
{
    size_t start = xformId.rfind("/");
    start += 1;
    size_t end = xformId.rfind("Xfo");
    end -=1;
    std::string modelName = xformId.substr(start, end-start+1);
    modelName = xformId + std::string("/") + modelName;
    return modelName;
}

std::string getIdentifierFromRef(const SceneEntry &in_Ref)
{
    return in_Ref.fullname;
}

std::string getModelFullName( const std::string &identifier )
{
    // Max Scene nodes are also identified by their transform nodes since an INode contains
    // both the transform and the shape.  So if we find an "xfo" at the end of the identifier
    // then we extract the model name from the identifier
    std::string modelName;
    size_t pos = identifier.rfind("Xfo", identifier.length()-3, 3);
    if (pos == identifier.npos)
    {
        modelName = identifier;
    }
    else
    {
        size_t start = identifier.rfind("/");
        start += 1;
        modelName = identifier.substr(start, identifier.length()-3);
        modelName = identifier + std::string("/") + modelName;
    }

    return modelName;
}

std::string getModelName( const std::string &identifier )
{
    // Max Scene nodes are also identified by their transform nodes since an INode contains
    // both the transform and the shape.  So if we find an "xfo" at the end of the identifier
    // then we extract the model name from the identifier
    std::string modelName;
    size_t pos = identifier.rfind("Xfo", identifier.length()-3, 3);
    if (pos == identifier.npos)
        modelName = identifier;
    else
        modelName = identifier.substr(0, identifier.length()-3);

    return modelName;
}
