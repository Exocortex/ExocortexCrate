#include "Alembic.h"
#include "AlembicArchiveStorage.h"
#include "AlembicDefinitions.h"
#include "AlembicSplineGeomModifier.h"
#include "AlembicVisibilityController.h"
#include "AlembicXForm.h"
#include "stdafx.h"
#include "utility.h"

using namespace MaxSDK::AssetManagement;

IObjParam *AlembicSplineGeomModifier::ip = NULL;
AlembicSplineGeomModifier *AlembicSplineGeomModifier::editMod = NULL;

static AlembicSplineGeomModifierClassDesc AlembicSplineGeomModifierDesc;
ClassDesc2 *GetAlembicSplineGeomModifierClassDesc()
{
  return &AlembicSplineGeomModifierDesc;
}

//--- Properties block -------------------------------

static ParamBlockDesc2 AlembicSplineGeomModifierParams(
    0, _T(ALEMBIC_SPLINE_GEOM_MODIFIER_SCRIPTNAME), 0,
    GetAlembicSplineGeomModifierClassDesc(), P_AUTO_CONSTRUCT | P_AUTO_UI, 0,

    // rollout description
    IDD_ALEMBIC_SPLINE_GEOM_PARAMS, IDS_ALEMBIC, 0, 0, NULL,

    // params
    AlembicSplineGeomModifier::ID_PATH, _T("path"), TYPE_FILENAME,
    P_RESET_DEFAULT, IDS_PATH, p_default, "", p_ui, TYPE_EDITBOX, IDC_PATH_EDIT,
    p_assetTypeID, kExternalLink, p_end,

    AlembicSplineGeomModifier::ID_IDENTIFIER, _T("identifier"), TYPE_STRING,
    P_RESET_DEFAULT, IDS_IDENTIFIER, p_default, "", p_ui, TYPE_EDITBOX,
    IDC_IDENTIFIER_EDIT, p_end,

    AlembicSplineGeomModifier::ID_TIME, _T("time"), TYPE_FLOAT, P_ANIMATABLE,
    IDS_TIME, p_default, 0.0f, p_range, 0.0f, 1000.0f, p_ui, TYPE_SPINNER,
    EDITTYPE_FLOAT, IDC_TIME_EDIT, IDC_TIME_SPIN, 0.01f, p_end,

    AlembicSplineGeomModifier::ID_MUTED, _T("muted"), TYPE_BOOL, P_ANIMATABLE,
    IDS_MUTED, p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_MUTED_CHECKBOX,
    p_end,

    p_end);

//--- Modifier methods -------------------------------

AlembicSplineGeomModifier::AlembicSplineGeomModifier()
{
  pblock = NULL;
  m_CachedAbcFile = "";

  GetAlembicSplineGeomModifierClassDesc()->MakeAutoParamBlocks(this);
}

AlembicSplineGeomModifier::~AlembicSplineGeomModifier()
{
  delRefArchive(m_CachedAbcFile);
}

RefTargetHandle AlembicSplineGeomModifier::Clone(RemapDir &remap)
{
  AlembicSplineGeomModifier *mod = new AlembicSplineGeomModifier();

  mod->ReplaceReference(0, remap.CloneRef(pblock));

  BaseClone(this, mod, remap);
  return mod;
}

void AlembicSplineGeomModifier::EnumAuxFiles(AssetEnumCallback &nameEnum,
                                             DWORD flags)
{
  if ((flags & FILE_ENUM_CHECK_AWORK1) && TestAFlag(A_WORK1)) {
    return;  // LAM - 4/11/03
  }

  if (!(flags & FILE_ENUM_INACTIVE)) {
    return;  // not needed by renderer
  }

  if (flags & FILE_ENUM_ACCESSOR_INTERFACE) {
    IEnumAuxAssetsCallback *callback =
        static_cast<IEnumAuxAssetsCallback *>(&nameEnum);
    callback->DeclareAsset(AlembicPathAccessor(this));
  }
  // else {
  //	IPathConfigMgr::GetPathConfigMgr()->RecordInputAsset(
  // this->GetParamBlockByID( 0 )->GetAssetUser( GetParamIdByName( this, 0,
  //"path" ), 0 ), nameEnum, flags);
  //}

  ReferenceTarget::EnumAuxFiles(nameEnum, flags);
}

void AlembicSplineGeomModifier::ModifyObject(TimeValue t, ModContext &mc,
                                             ObjectState *os, INode *node)
{
  ESS_CPP_EXCEPTION_REPORTING_START
  ESS_PROFILE_FUNC();

  Interval interval = FOREVER;  // os->obj->ObjectValidity(t);
  // ESS_LOG_INFO( "Interval Start: " << interval.Start() << " End: " <<
  // interval.End() );

  MCHAR const *strPath = NULL;
  this->pblock->GetValue(AlembicSplineGeomModifier::ID_PATH, t, strPath,
                         interval);

  MCHAR const *strIdentifier = NULL;
  this->pblock->GetValue(AlembicSplineGeomModifier::ID_IDENTIFIER, t,
                         strIdentifier, interval);

  float fTime;
  this->pblock->GetValue(AlembicSplineGeomModifier::ID_TIME, t, fTime,
                         interval);

  BOOL bTopology = false;
  BOOL bGeometry = true;
  float fGeoAlpha = 1.0f;
  BOOL bNormals = false;
  BOOL bUVs = true;

  BOOL bMuted;
  this->pblock->GetValue(AlembicSplineGeomModifier::ID_MUTED, t, bMuted,
                         interval);

  // ESS_LOG_INFO( "AlembicSplineGeomModifier::ModifyObject strPath: " <<
  // strPath << " strIdentifier: " << strIdentifier << " fTime: " << fTime <<
  //	" bTopology: " << bTopology << " bGeometry: " << bGeometry << "
  // bNormals: " << bNormals << " bUVs: " << bUVs << " bMuted: " << bMuted );

  if (bMuted || !strPath || !strIdentifier) {
    return;
  }

  std::string szPath = EC_MCHAR_to_UTF8(strPath);
  std::string szIdentifier = EC_MCHAR_to_UTF8(strIdentifier);

  AbcG::IObject iObj;
  try {
    iObj = getObjectFromArchive(szPath, szIdentifier);
  }
  catch (std::exception exp) {
    extern bool g_hasModifierErrorOccurred;
    g_hasModifierErrorOccurred = true;
    ESS_LOG_ERROR("Can not open Alembic data stream.  Path: "
                  << szPath << " identifier: " << szIdentifier
                  << " reason: " << exp.what());
    return;
  }

  if (!iObj.valid()) {
    extern bool g_hasModifierErrorOccurred;
    g_hasModifierErrorOccurred = true;
    ESS_LOG_ERROR("Not a valid Alembic data stream.  Path: "
                  << szPath << " identifier: " << szIdentifier);
    return;
  }

  ShapeObject *shape = (ShapeObject *)os->obj;

  BezierShape bezierShape;
  PolyShape polyShape;

  alembic_fillshape_options options;
  options.pIObj = &iObj;
  options.pShapeObject = shape;
  options.dTicks = GetTimeValueFromSeconds(fTime);
  options.nDataFillFlags = 0;
  options.nDataFillFlags |= ALEMBIC_DATAFILL_VERTEX;
  options.nDataFillFlags |= ALEMBIC_DATAFILL_BOUNDINGBOX;

  SClass_ID superClassID = os->obj->SuperClassID();
  Class_ID classID = os->obj->ClassID();
  if (superClassID != SHAPE_CLASS_ID) {
    extern bool g_hasModifierErrorOccurred;
    g_hasModifierErrorOccurred = true;
    ESS_LOG_ERROR(
        "Can not convert internal object data into a ShapeObject, confused. "
        "(1)");
    return;
  }

  if (classID == Class_ID(SPLINESHAPE_CLASS_ID, 0)) {
    SplineShape *pSplineShape = (SplineShape *)os->obj;
    options.pBezierShape = &pSplineShape->shape;
  }
  else if (classID == Class_ID(LINEARSHAPE_CLASS_ID, 0)) {
    LinearShape *pLinearShape = (LinearShape *)os->obj;
    options.pPolyShape = &pLinearShape->shape;
  }
  else {
    extern bool g_hasModifierErrorOccurred;
    g_hasModifierErrorOccurred = true;
    ESS_LOG_ERROR(
        "Can not convert internal object data into a ShapeObject, confused. "
        "(2)");
  }

  try {
    AlembicImport_FillInShape(options);
  }
  catch (std::exception exp) {
    extern bool g_hasModifierErrorOccurred;
    g_hasModifierErrorOccurred = true;
    ESS_LOG_ERROR("Error reading shape from Alembic data stream.  Path: "
                  << strPath << " identifier: " << strIdentifier
                  << " reason: " << exp.what());
    return;
  }

  // update the validity channel
  if (bTopology) {
    os->obj->UpdateValidity(TOPO_CHAN_NUM, interval);
    os->obj->UpdateValidity(GEOM_CHAN_NUM, interval);
  }
  if (bGeometry) {
    os->obj->UpdateValidity(GEOM_CHAN_NUM, interval);
  }

  ESS_CPP_EXCEPTION_REPORTING_END
}
#if (crate_Max_Version >= 2015)
RefResult AlembicSplineGeomModifier::NotifyRefChanged(const Interval &changeInt,
                                                      RefTargetHandle hTarget,
                                                      PartID &partID,
                                                      RefMessage message,
                                                      BOOL propagate)
{
#else
RefResult AlembicSplineGeomModifier::NotifyRefChanged(Interval changeInt,
                                                      RefTargetHandle hTarget,
                                                      PartID &partID,
                                                      RefMessage message)
{
#endif
  ESS_CPP_EXCEPTION_REPORTING_START

  switch (message) {
    case REFMSG_CHANGE:
      if (hTarget == pblock) {
        ParamID changing_param = pblock->LastNotifyParamID();
        switch (changing_param) {
          case ID_PATH: {
            delRefArchive(m_CachedAbcFile);
            MCHAR const *strPath = NULL;
            TimeValue t = GetCOREInterface()->GetTime();
            Interval v;
            pblock->GetValue(AlembicSplineGeomModifier::ID_PATH, t, strPath, v);
            m_CachedAbcFile = EC_MCHAR_to_UTF8(strPath);
            addRefArchive(m_CachedAbcFile);
          } break;
          default:
            break;
        }

        AlembicSplineGeomModifierParams.InvalidateUI(changing_param);
      }
      break;

    case REFMSG_WANT_SHOWPARAMLEVEL:

      break;
  }

  ESS_CPP_EXCEPTION_REPORTING_END

  return REF_SUCCEED;
}

void AlembicSplineGeomModifier::BeginEditParams(IObjParam *ip, ULONG flags,
                                                Animatable *prev)
{
  this->ip = ip;
  editMod = this;

  AlembicSplineGeomModifierDesc.BeginEditParams(ip, this, flags, prev);
}

void AlembicSplineGeomModifier::EndEditParams(IObjParam *ip, ULONG flags,
                                              Animatable *next)
{
  AlembicSplineGeomModifierDesc.EndEditParams(ip, this, flags, next);

  this->ip = NULL;
  editMod = NULL;
}