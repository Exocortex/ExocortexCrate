#include "AlembicTransformUtilities.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicImport_FillInXForm
///////////////////////////////////////////////////////////////////////////////////////////////////
void AlembicImport_FillInXForm(alembic_fillxform_options &options)
{
    if(!options.pIObj->valid())
        return;
    
    // Alembic::AbcGeom::IXform obj;
    Alembic::AbcGeom::IXform obj(*(options.pIObj), Alembic::Abc::kWrapExisting);

    if(!obj.valid())
        return;

   float masterScaleUnitMeters = (float)GetMasterScale(UNITS_METERS);

   double SampleTime = GetSecondsFromTimeValue(options.dTicks);

    SampleInfo sampleInfo = getSampleInfo(
        SampleTime,
        obj.getSchema().getTimeSampling(),
        obj.getSchema().getNumSamples()
        );

    Alembic::AbcGeom::XformSample sample;
    obj.getSchema().get(sample,sampleInfo.floorIndex);
    Alembic::Abc::M44d matrix = sample.getMatrix();

    const Alembic::Abc::Box3d &box3d = sample.getChildBounds();

    options.maxBoundingBox = Box3(Point3(box3d.min.x, box3d.min.y, box3d.min.z), 
        Point3(box3d.max.x, box3d.max.y, box3d.max.z));

    // blend 
    if(sampleInfo.alpha != 0.0)
    {
        obj.getSchema().get(sample,sampleInfo.ceilIndex);
        Alembic::Abc::M44d ceilMatrix = sample.getMatrix();
        matrix = (1.0 - sampleInfo.alpha) * matrix + sampleInfo.alpha * ceilMatrix;
    }

    Matrix3 objMatrix(
        Point3(matrix.getValue()[0], matrix.getValue()[1], matrix.getValue()[2]),
        Point3(matrix.getValue()[4], matrix.getValue()[5], matrix.getValue()[6]),
        Point3(matrix.getValue()[8], matrix.getValue()[9], matrix.getValue()[10]),
        Point3(matrix.getValue()[12], matrix.getValue()[13], matrix.getValue()[14]));

    ConvertAlembicMatrixToMaxMatrix(objMatrix, masterScaleUnitMeters, options.maxMatrix);

    if (options.bIsCameraTransform)
    {
        // Cameras in Max are already pointing down the negative z-axis (as is expected from Alembic).
        // So we rotate it by 90 degrees so that it is pointing down the positive y-axis.
        Matrix3 rotation(TRUE);
        rotation.RotateX(HALFPI);
        options.maxMatrix = rotation * options.maxMatrix;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// AlembicImport_XForm
///////////////////////////////////////////////////////////////////////////////////////////////////
int AlembicImport_XForm(const std::string &file, const std::string &identifier, alembic_importoptions &options)
{
    // Find the object in the archive
	Alembic::AbcGeom::IObject iObj = getObjectFromArchive(file,identifier);
	if(!iObj.valid())
		return alembic_failure;

    // Find out if we're dealing with a camera
    std::string modelfullid = getModelFullName(std::string(iObj.getFullName()));
    Alembic::AbcGeom::IObject iChildObj = getObjectFromArchive(file,modelfullid);
    bool bIsCamera = iChildObj.valid() && Alembic::AbcGeom::ICamera::matches(iChildObj.getMetaData());

    // Get the matrix for the current time 
    alembic_fillxform_options xformOptions;
    xformOptions.pIObj = &iObj;
    xformOptions.dTicks = GetCOREInterface12()->GetTime();
    xformOptions.bIsCameraTransform = bIsCamera;
    AlembicImport_FillInXForm(xformOptions);

    // Find the scene node that this transform belongs too
    // If the node does not exist, then this is just a transform, so we create
    // a dummy helper object to attach the modifier
    std::string modelid = getModelName(std::string(iObj.getName()));
    INode *pNode = options.currentSceneList.FindNodeWithName(modelid);
    if (!pNode)
    {
        Object* obj = static_cast<Object*>(CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID,0)));

        if (!obj)
            return alembic_failure;

        DummyObject *pDummy = static_cast<DummyObject*>(obj);

        // Hard code these values for now
        pDummy->SetColor(Point3(0.6f, 0.8f, 1.0f));
        pDummy->SetBox(xformOptions.maxBoundingBox);

        pDummy->EnableDisplay();

        pNode = GetCOREInterface12()->CreateObjectNode(obj, modelid.c_str());

        if (!pNode)
            return alembic_failure;

        // Add it to our current scene list
        SceneEntry *pEntry = options.sceneEnumProc.Append(pNode, obj, OBTYPE_DUMMY, &std::string(iObj.getFullName())); 
        options.currentSceneList.Append(pEntry);

        // Set up any child links for this node
        AlembicImport_SetupChildLinks(iObj, options);
    }

	// Create the xform modifier
	AlembicXFormCtrl *pCtrl = static_cast<AlembicXFormCtrl*>
		(GetCOREInterface()->CreateInstance(CTRL_MATRIX3_CLASS_ID, ALEMBIC_XFORM_CTRL_CLASSID));

	// Set the alembic id
    pCtrl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pCtrl, 0, "path" ), xformOptions.dTicks, file.c_str());
	pCtrl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pCtrl, 0, "identifier" ), xformOptions.dTicks , identifier.c_str() );
    pCtrl->GetParamBlockByID( 0 )->SetValue( GetParamIdByName( pCtrl, 0, "isCameraTransform" ), xformOptions.dTicks , xformOptions.bIsCameraTransform );

	// Add the modifier to the node
    pNode->SetTMController(pCtrl);
    pNode->InvalidateTreeTM();

    // Lock the transform
    LockNodeTransform(pNode, true);

	return alembic_success;
}

