#include "stdafx.h"
#include "arnoldHelpers.h"

using namespace XSI; 
using namespace MATH; 

CStatus GetArnoldMotionBlurData(CDoubleArray &mbKeys, double frame)
{
   // get the arnold options property
   Pass pass = Application().GetActiveProject().GetActiveScene().GetActivePass();
   Property arnoldOptions;
   pass.GetPropertyFromName(L"Arnold_Render_Options",arnoldOptions);
   if(!arnoldOptions.IsValid())
      return CStatus::Fail;

   // check if motion blur is enabled at all
   if (!(bool)arnoldOptions.GetParameterValue(L"enable_motion_blur", DBL_MAX))
   {
      mbKeys.Add(frame);
      return CStatus::OK;
   }

   // get all of the arnold settings
   LONG stepDeform = (LONG)arnoldOptions.GetParameterValue(L"motion_step_deform", DBL_MAX);
   if (stepDeform < 0)
      stepDeform = 0;
   if (stepDeform > 255)
      stepDeform = 255;
   LONG onFrame = (LONG)arnoldOptions.GetParameterValue(L"motion_shutter_onframe", DBL_MAX);
   float frameDuration = arnoldOptions.GetParameterValue(L"motion_frame_duration", DBL_MAX);

   // compute all of the keys
   double frameKey;
   for (LONG i=0; i<stepDeform; i++)
   {
      if(onFrame==0)
         frameKey = frame - (frameDuration/2.0f) + ((frameDuration/(stepDeform-1))*i);
      else if(onFrame==1)
         frameKey = frame - ((frameDuration/(stepDeform-1))*(stepDeform-(i+1.0f)));
      else if(onFrame==2)
         frameKey = frame + ((frameDuration/(stepDeform-1))*i);

      mbKeys.Add(frameKey);
   }

   return CStatus::OK;
}