#ifndef _FOUNDATION_H_
#define _FOUNDATION_H_

#ifdef NOMINMAX
#undef NOMINMAX
#endif

#include "CommonAlembic.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#include <Max.h>
#undef max
#undef min

#include <Object.h>
#include <iparamb2.h>
#include <iparamm2.h>
#pragma warning(disable : 4244)
#if MAX_PRODUCT_YEAR_NUMBER > 2010
#include <maxscript\maxscript.h>
#else
#include <maxscrpt\maxscrpt.h>
#endif
#pragma warning(default : 4244)
#include <Dummy.h>
#include <ILockedTracks.h>
#include <IMetaData.h>
#include <MeshNormalSpec.h>
#include <ifnpub.h>
#include <inode.h>
#include <modstack.h>
#include <notify.h>
#include <object.h>

#if MAX_PRODUCT_YEAR_NUMBER == 2010
#ifdef GUP_EXPORTS
#define GUPExport __declspec(dllexport)
#else
#define GUPExport __declspec(dllimport)
#endif
#endif

#include <AssetManagement\IAssetAccessor.h>
#include <Assetmanagement\iassetmanager.h>
#include <MeshNormalSpec.h>
#include <Object.h>
#include <ParticleFlow/IPFSystem.h>
#include <ParticleFlow/PFClassIDs.h>
#include <SimpObj.h>
#include <assetmanagement\AssetType.h>
#include <color.h>
#include <custattrib.h>
#include <gup.h>
#include <icustattribcontainer.h>
#include <iparamb.h>
#include <iparamm.h>
#include <linshape.h>
#include <maxapi.h>
#include <sceneapi.h>
#include <simpspl.h>
#include <splshape.h>
#include <triobj.h>

#ifdef THINKING_PARTICLES
#define IDS_CLASS_NAME 3
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#include <matterwaves.h>
#undef max
#undef min
#endif

#include <IParticleObjectExt.h>
#include <ParticleFlow/IChannelContainer.h>
#include <ParticleFlow/IPFActionList.h>
#include <ParticleFlow/IPFRender.h>
#include <ParticleFlow/IPFSystem.h>
#include <ParticleFlow/IPFTest.h>
#include <ParticleFlow/IParticleChannelID.h>
#include <ParticleFlow/IParticleChannelLifespan.h>
#include <ParticleFlow/IParticleChannelShape.h>
#include <ParticleFlow/IParticleChannels.h>
#include <ParticleFlow/IParticleContainer.h>
#include <ParticleFlow/IParticleGroup.h>
#include <ParticleFlow/PFSimpleOperator.h>
#include <ifnpub.h>

#if MAX_PRODUCT_YEAR_NUMBER < 2013
typedef Interface12 MAXInterface;
#define GET_MAX_INTERFACE() GetCOREInterface12()
#else
typedef Interface14 MAXInterface;
#define GET_MAX_INTERFACE() GetCOREInterface14()
#endif

#if MAX_PRODUCT_YEAR_NUMBER < 2013
#define p_end end
#define CONST_2013
#else
#define CONST_2013 const
#endif

#ifdef base_type
#undef base_type
#endif

using namespace MaxSDK;

#ifndef _EC_WSTR
#define _EC_WSTR(s) L##s
#endif
#ifndef EC_WSTR
#define EC_WSTR(s) _EC_WSTR(s)
#endif

#ifndef _EC_QUOTE
#define _EC_QUOTE(x) #x
#endif
#ifndef EC_QUOTE
#define EC_QUOTE(x) _EC_QUOTE(x)
#endif

#ifndef EC_WQUOTE
#define EC_WQUOTE(x) EC_WSTR(EC_QUOTE(x))
#endif

#if MAX_PRODUCT_YEAR_NUMBER < 2013
#define EC_UTF8_to_MCHAR(x) x
#define EC_UTF8_to_TSTR(x) TSTR(x)
#define EC_UTF8_to_TCHAR(x) x
#define EC_MCHAR_to_UTF8(x) std::string(x)
#define EC_MSTR_to_UTF8(x) std::string(x.data())
#else

inline std::string ec_toUtf8(std::wstring const& src)
{
  WStr wStr(src.c_str());
  CStr cStr = wStr.ToCStr();
  std::string result(cStr.data());
  return result;

  /*char *szTemp = new char[ (int)(src.size() * 6 + 1) ];

  // This assumes that wcstombs is actually using UTF-8 for its character
  conversions, which isn't guaranteed.
  // TODO: replace wcstombs with something more reliable that doesn't depend on
  the current locale
  int length = (int)wcstombs(szTemp, src.c_str(), (int)src.size());
  szTemp[length] = 0;
  std::string convertedString( szTemp );
  delete[] szTemp;
  return convertedString;*/
}

#define EC_UTF8_to_MCHAR(x) EC_WSTR(x)
#define EC_UTF8_to_TSTR(x) TSTR::FromUTF8(x)
#define EC_UTF8_to_TCHAR(x) TSTR::FromUTF8(x).data()
#define EC_MCHAR_to_UTF8(x) ec_toUtf8(x)
#define EC_MSTR_to_UTF8(x) ec_toUtf8(x.data())
#endif

#endif  // _FOUNDATION_H_
