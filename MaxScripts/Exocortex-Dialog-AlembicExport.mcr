--------------------------------------------------------------------------------------------------------
-- Custom import/export dialog with settings

rollout AlembicExportSettings "Alembic Export Settings" width:288 height:420
(
	GroupBox animGroup "Animation" pos:[8,64] width:272 height:96
	label inLabel "Frame In" pos:[16,88] width:128 height:21
	label outLabel "Frame Out" pos:[16,104] width:128 height:21
	label stepsLabel "Frame Steps" pos:[16,120] width:128 height:21
	label subStepsLabel "Frame Sub-Steps" pos:[16,136] width:128 height:21
	spinner inSpinner "" pos:[112,88] width:160 height:16 range:[0,1e+006,0] type:#integer
	spinner outSpinner "" pos:[112,104] width:160 height:16 range:[1,1e+006,100] type:#integer
	spinner stepsSpinner "" pos:[112,120] width:160 height:16 range:[1,1e+006,1] type:#integer
	spinner subStepsSpinner "" pos:[112,136] width:160 height:16 range:[1,1e+006,1] type:#integer
    
	GroupBox geoGroup "Geometry" pos:[8,168] width:272 height:208
	
	dropdownList meshTopologyDropDown "Mesh Topology" pos:[16,192] width:256 height:40 items:#("Just Surfaces (No Normals)", "Point Cache (No Surfaces)", "Surface + Normals (For Interchange)") selection:3
	
	dropdownList particleSystemExportMethod "Particle System Export Method" pos:[16,240] width:256 height:40 items:#("Shape Node Instancing", "Automatic Instancing", "Merged Mesh") selection:1

	checkbox uvCheckbox "UVs" pos:[32,288] width:128 height:15 checked:true
	checkbox envelopeCheckbox "Envelope BindPose" pos:[32,304] width:150 height:15 checked:true
	checkbox materialIdsCheckbox "Material Ids" pos:[32,320] width:107 height:14 checked:true
	checkbox flattenHierarchyCheckbox "Flatten Hierarchy" pos:[32,336] width:107 height:14 checked:true
	checkbox transformCacheCheckbox "Transform Cache" pos:[32,352] width:200 height:14 checked:false

	button exportButton "Export" pos:[16,386] width:64 height:24
	button cancelButton "Cancel" pos:[208,386] width:64 height:24
	GroupBox selectGroup "Selection" pos:[8,16] width:272 height:40
	checkbox exportSelectedCheckbox "Export Selected Objects" pos:[32,32] width:232 height:16 checked:true

	on exportButton pressed do
	(
	    filename = getSaveFileName caption:"Export to Alembic File:" types:"Alembic(*.abc)|*.abc|All(*.*)|*.*" historyCategory:"Alembic"
	    if (filename != undefined) do
	    (


	    	jobString = "filename=" + (filename as string)
	    	jobString += ";in=" + (inSpinner.value as string)
	    	jobString += ";out=" + (outSpinner.value as string)
	    	jobString += ";step=" + (stepsSpinner.value as string)
	    	jobString += ";substep=" + (subStepsSpinner.value as string)
	    	if(meshTopologyDropDown.selection == 2) do jobString += ";purepointcache=true"
	    	if(meshTopologyDropDown.selection == 3) do jobString += ";normals=true"
	    	jobString += ";uvs=" 
	    	jobString += (uvCheckbox.checked as string)
	    	jobString += ";materialids=" 
	    	jobString += (materialIdsCheckbox.checked as string)
	    	jobString += ";bindpose=" 
	    	jobString += (envelopeCheckbox.checked as string)
	    	jobString += ";exportselected=" 
	    	jobString += (exportSelectedCheckbox.checked as string)
	    	jobString += ";flattenhierarchy=" 
	    	jobString += (flattenHierarchyCheckbox.checked as string)
	    	if(particleSystemExportMethod.selection == 2) do jobString += ";automaticinstancing=true" 
	    	if(particleSystemExportMethod.selection == 3) do jobString += ";particlesystemtomeshconversion=true" 
	    	jobString += ";transformCache="
	    	jobString += (transformCacheCheckbox.checked as string)

	    	result = ExocortexAlembic.createExportJobs(jobString)
	        if( result != 0 ) do
	        (
	            	messageBox "Failure - See Maxscript Listener for details." title:"Exocortex Alembic Export"
		    )
	        destroyDialog AlembicExportSettings
	    )
	)
	on cancelButton pressed do
	(
	    destroyDialog AlembicExportSettings
	)
	
	on AlembicExportSettings open do
    (
		inSpinner.value = animationRange.start
		outSpinner.value = animationRange.end
    )
)
---------------------------------------------------------------------------------------------------------
-- MAXScript functions to bring up the options dialogs

fn alembicExportDialog =
(
    createDialog AlembicExportSettings
)

---------------------------------------------------------------------------------------------------------
-- MacroScript to link the menu items to MAXScript functions

macroScript AlembicExportUI
    category:"Alembic" 
    buttonText:"Alembic Export..."
    tooltip:"Alembic Export..."
(
    on execute do 
    (
        alembicExportDialog()
    )
)
