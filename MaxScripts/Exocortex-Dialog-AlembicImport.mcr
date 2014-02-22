---------------------------------------------------------------------------------------------------------
-- Custom import/export dialog with settings

rollout AlembicImportSettings "Alembic Import Settings" width:288 height:260
(
    local filename

	GroupBox geoGroup "Geometry" pos:[8,8] width:272 height:244
	checkbox normalCheckbox "Normals" pos:[48,32] width:200 height:15 checked:true
	checkbox uvCheckbox "UVs" pos:[48,48] width:200 height:15 checked:true
    --checkbox clustersCheckbox "Clusters" pos:[48,64] width:200 height:15 checked:true
	checkbox attachCheckbox "Attach to existing objects" pos:[48,64] width:200 height:15 checked:false

	button importButton "Import" pos:[16,228] width:64 height:24
	button cancelButton "Cancel" pos:[208,228] width:64 height:24
	--GroupBox grpVisibility "Visibility" pos:[9,121] width:272 height:46
	--dropdownList dropDownVis "" pos:[20,140] width:252 height:21 items:#("Just Import Value", "Connected Controllers") selection:1
	checkbox materialIdsCheckbox "Material Ids" pos:[48,82] width:145 height:16 checked:true
    checkbox geoFromTopoCheckbox "Load Geometry From Topology Modifier" pos:[48,100] width:220 height:16 checked:false
    checkbox loadTimeControlCheckbox "Use Time Control" pos:[48,118] width:220 height:16 checked:true
    checkbox loadCurvesAsNurbsCheckbox "Load Curves as NURBS" pos:[48,136] width:220 height:16 checked:true
    checkbox meshSmoothModifiersCheckbox "Mesh Smooth Modifiers" pos:[48,152] width:220 height:16 checked:true
    checkbox userPropertiesCheckbox "UserProperties" pos:[48,168] width:200 height:16 checked:false
    dropdownList objectDuplication "Object Duplication" pos:[48,184] width:200 height:40 items:#("Copy", "Instance", "Reference") selection:1

	on importButton pressed do
	(
    	jobString = "filename=" + (filename as string)
    	jobString += ";normals=" 
    	jobString += (normalCheckbox.checked as string)
    	jobString += ";uvs=" 
    	jobString += (uvCheckbox.checked as string)
    	jobString += ";attachToExisting=" 
    	jobString += (attachCheckbox.checked as string)
        jobString += ";loadGeometryInTopologyModifier="
        jobString += (geoFromTopoCheckbox.checked as string)
        jobString += ";loadTimeControl="
        jobString += (loadTimeControlCheckbox.checked as string)
        jobString += ";loadCurvesAsNurbs="
        jobString += (loadCurvesAsNurbsCheckbox.checked as string)
        jobString += ";loadUserProperties="
        jobString += (userPropertiesCheckbox.checked as string)
        jobString += ";meshSmoothModifiers="
        jobString += (meshSmoothModifiersCheckbox.checked as string)
        jobString += ";materialIds="
        jobString += (materialIdsCheckbox.checked as string)

        if(objectDuplication.selection == 1) do
        (
            jobString += ";objectDuplication=copy"
        )
        if(objectDuplication.selection == 2) do
        (
            jobString += ";objectDuplication=instance"
        )
        if(objectDuplication.selection == 3) do
        (
            jobString += ";objectDuplication=reference"
        )

    	result = ExocortexAlembic.createImportJob(jobString)

	    --result = ExocortexAlembic.import filename normalCheckbox.checked uvCheckbox.checked materialIdsCheckbox.checked attachCheckbox.checked 0
	    if( result != 0 ) do
	    (
	    	messageBox "Failure - See Maxscript Listener for details." title:"Exocortex Alembic Import"
	    )
		destroyDialog AlembicImportSettings
	)
	on cancelButton pressed do
	(
	    destroyDialog AlembicImportSettings
	)
)
---------------------------------------------------------------------------------------------------------
-- MAXScript functions to bring up the options dialogs

fn alembicImportDialog =
(
    filename = getOpenFileName caption:"Import from Alembic File:" types:"Alembic(*.abc)|*.abc|All(*.*)|*.*" historyCategory:"Alembic"
    if (filename != undefined) do
    (
        createDialog AlembicImportSettings
        AlembicImportSettings.filename = filename
    )
)

---------------------------------------------------------------------------------------------------------
-- MacroScript to link the menu items to MAXScript functions

macroScript AlembicImportUI
    category:"Alembic" 
    buttonText:"Alembic Import..."
    tooltip:"Alembic Import..."
(
    on execute do  
    (
        alembicImportDialog()
    )
)
