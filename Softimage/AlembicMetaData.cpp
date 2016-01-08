#include "AlembicMetaData.h"
#include "AlembicObject.h"
#include "stdafx.h"

using namespace XSI;
using namespace MATH;

void SaveMetaData(XSI::CRef x3dRef, AlembicObject* object)
{
  if (object == NULL) return;
  if (object->GetNumSamples() > 0) return;
  X3DObject x3d(x3dRef);
  if (!x3d.IsValid()) return;

  // search for properties
  CRefArray props = x3d.GetLocalProperties();
  for (LONG i = 0; i < props.GetCount(); i++) {
    CustomProperty prop(props[i]);
    if (!prop.IsValid()) continue;
    if (!prop.GetType().IsEqualNoCase(L"alembic_metadata")) continue;

    // we found a metadata property
    Abc::OStringArrayProperty metaDataProperty = Abc::OStringArrayProperty(
        object->GetCompound(), ".metadata", object->GetCompound().getMetaData(),
        object->GetJob()->GetAnimatedTs());

    std::vector<std::string> metaData(20);
    size_t offset = 0;
    for (LONG j = 0; j < 10; j++) {
      metaData[offset++] = prop.GetParameterValue(L"name" + CString(j))
                               .GetAsText()
                               .GetAsciiString();
      metaData[offset++] = prop.GetParameterValue(L"value" + CString(j))
                               .GetAsText()
                               .GetAsciiString();
    }

    Abc::StringArraySample metaDataSample(&metaData.front(), metaData.size());
    metaDataProperty.set(metaDataSample);
    break;
  }
}

SICALLBACK alembic_metadata_Define(CRef& in_ctxt)
{
  Context ctxt(in_ctxt);
  CustomProperty oCustomProperty;
  Parameter oParam;
  oCustomProperty = ctxt.GetSource();

  // get the current frame in an out
  for (LONG i = 0; i < 10; i++) {
    oCustomProperty.AddParameter(L"name" + CString(i), CValue::siString,
                                 siReadOnly | siPersistable, L"", L"", L"", L"",
                                 L"", L"", L"", oParam);
    oCustomProperty.AddParameter(L"value" + CString(i), CValue::siString,
                                 siReadOnly | siPersistable, L"", L"", L"", L"",
                                 L"", L"", L"", oParam);
  }
  return CStatus::OK;
}

SICALLBACK alembic_metadata_DefineLayout(CRef& in_ctxt)
{
  Context ctxt(in_ctxt);
  PPGLayout oLayout;
  oLayout = ctxt.GetSource();
  oLayout.Clear();
  for (LONG i = 0; i < 10; i++) {
    oLayout.AddRow();
    oLayout.AddItem(L"name" + CString(i)).PutAttribute(siUINoLabel, true);
    oLayout.AddItem(L"value" + CString(i)).PutAttribute(siUINoLabel, true);
    oLayout.EndRow();
  }
  return CStatus::OK;
}

SICALLBACK alembic_MenuMetaData_Init(CRef& in_ctxt)
{
  Context ctxt(in_ctxt);
  Menu oMenu;
  oMenu = ctxt.GetSource();
  MenuItem oNewItem;
  oMenu.AddCommandItem(L"Alembic MetaData", L"alembic_attach_metadata",
                       oNewItem);
  return CStatus::OK;
}

SICALLBACK alembic_attach_metadata_Init(CRef& in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;
  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"");
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs;
  oArgs = oCmd.GetArguments();
  oArgs.AddWithHandler(L"objects", L"Collection");
  return CStatus::OK;
}

SICALLBACK alembic_attach_metadata_Execute(CRef& in_ctxt)
{
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // get the objects
  CValue objectsAsValue = args[0];
  CRefArray objects = objectsAsValue;
  if (objects.GetCount() == 0) {
    Selection sel = Application().GetSelection();
    for (LONG i = 0; i < sel.GetCount(); i++) {
      objects.Add(sel.GetItem(i));
    }
  }
  if (objects.GetCount() == 0) {
    Application().LogMessage(
        L"[ExocortexAlembic] No objects passed / selected.", siErrorMsg);
    return CStatus::InvalidArgument;
  }

  for (LONG i = 0; i < objects.GetCount(); i++) {
    X3DObject object(objects[i]);
    if (!object.IsValid()) continue;
    CRefArray props = object.GetLocalProperties();
    bool hasMetaData = false;
    for (LONG j = 0; j < props.GetCount(); j++) {
      CustomProperty prop(props[j]);
      if (!prop.IsValid()) continue;
      if (prop.GetType().IsEqualNoCase(L"alembic_metadata")) {
        hasMetaData = true;
        break;
      }
    }
    if (hasMetaData) {
      Application().LogMessage(L"[ExocortexAlembic] Skipping 'L" +
                                   object.GetFullName() +
                                   "', already has metadata attached.",
                               siWarningMsg);
      continue;
    }
    object.AddProperty(L"alembic_metadata");
  }

  return CStatus::OK;
}
