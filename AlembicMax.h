#ifndef __MAX_H__
#define __MAX_H__

#include "Foundation.h"

#include <Max.h>
#include <iparamb2.h>
#include <iparamm2.h>

#pragma warning( disable : 4244 )

#if MAX_PRODUCT_YEAR_NUMBER > 2010 
	#include <maxscript\maxscript.h>
#else
	#include <maxscrpt\maxscrpt.h>
#endif

#pragma warning( default : 4244 )

#include <notify.h>
#include <modstack.h>
#include <MeshNormalSpec.h>
#include <ifnpub.h> 
#include <object.h>
#include <IMetaData.h>
#include <inode.h>
#include <Dummy.h>
#include <ILockedTracks.h>

#if MAX_PRODUCT_YEAR_NUMBER == 2010 
	#ifdef GUP_EXPORTS
		#define GUPExport __declspec( dllexport )
	#else
		#define GUPExport __declspec( dllimport )
	#endif
#endif

#include <gup.h>
#include <maxapi.h>
#include <SimpObj.h>
#include <Object.h>
#include <triobj.h>
#include <MeshNormalSpec.h>
#include <iparamb.h>
#include <splshape.h>
#include <iparamm.h>
#include <simpspl.h>
#include <linshape.h>
#include <sceneapi.h>
#include <ParticleFlow/PFClassIDs.h>
#include <ParticleFlow/IPFSystem.h>

#include <assetmanagement\AssetType.h>
#include <Assetmanagement\iassetmanager.h>

typedef Interface12 MAXInterface;
#define GET_MAX_INTERFACE()	GetCOREInterface12()

#if MAX_PRODUCT_YEAR_NUMBER < 2013
	#define p_end	end 
	#define	CONST_2013
#else
	#define	CONST_2013	const
#endif

#ifdef base_type
	#undef base_type
#endif

using namespace MaxSDK;

#endif // __MAX_H__
