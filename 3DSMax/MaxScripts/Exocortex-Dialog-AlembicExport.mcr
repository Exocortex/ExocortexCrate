--------------------------------------------------------------------------------------------------------
-- Custom import/export dialog with settings

rollout AlembicExportSettings "Alembic Export Settings" width:288 height:528
(
	GroupBox selectGroup "Selection" pos:[8,16] width:272 height:40
	checkbox exportSelectedCheckbox "Export Selected Objects" pos:[32,32] width:232 height:16 checked:true

	GroupBox animGroup "Animation" pos:[8,64] width:272 height:96
	label inLabel "Frame In" pos:[16,88] width:128 height:21
	label outLabel "Frame Out" pos:[16,104] width:128 height:21
	label stepsLabel "Frame Steps" pos:[16,120] width:128 height:21
	label subStepsLabel "Frame Sub-Steps" pos:[16,136] width:128 height:21
	spinner inSpinner "" pos:[112,88] width:160 height:16 range:[0,1e+006,0] type:#integer
	spinner outSpinner "" pos:[112,104] width:160 height:16 range:[1,1e+006,100] type:#integer
	spinner stepsSpinner "" pos:[112,120] width:160 height:16 range:[1,1e+006,1] type:#integer
	spinner subStepsSpinner "" pos:[112,136] width:160 height:16 range:[1,1e+006,1] type:#integer
    
	GroupBox geoGroup "Geometry" pos:[8,168] width:272 height:256
	
	dropdownList meshTopologyDropDown "Mesh Topology" pos:[16,186] width:256 height:40 items:#("Just Surfaces (No Normals)", "Point Cache (No Surfaces)", "Surface + Normals (Everything)") selection:3
	dropdownList particleSystemExportMethod "Particle System Export Method" pos:[16,228] width:256 height:40 items:#("Automatic Instancing", "Merged Mesh") selection:1
	dropdownList facesetExportMethod "Faceset Export Method" pos:[16,270] width:256 height:40 items:#("Partitioning Facesets only", "All") selection:1

	checkbox materialIdsCheckbox "Material Ids" pos:[32,310] width:107 height:14 checked:true
	checkbox uvCheckbox "UVs" pos:[32,326] width:128 height:15 checked:true
	checkbox flattenHierarchyCheckbox "Flatten Hierarchy" pos:[32,342] width:107 height:14 checked:true
	checkbox transformCacheCheckbox "Transform Cache" pos:[32,358] width:200 height:14 checked:false
	checkbox validateMeshTopology "Validate Mesh Topology" pos:[32,374]
	checkbox renameConflictingNodes "Rename Conflicting Nodes" pos:[32,390]
	checkbox mergeSelectedPolymeshSubtree "Merge Selected Polymesh Subtree" pos:[32,406]
	checkbox includeParentNodes "Include Parent Nodes" pos:[32,432]

	dropdownList storageFormat "Storage Format" pos:[16,454] width:256 height:40 items:#("HDF5", "Ogawa") selection:1

	button exportButton "Export" pos:[16,500] width:64 height:24
	button cancelButton "Cancel" pos:[208,500] width:64 height:24

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
	    	if(meshTopologyDropDown.selection == 1) do
	    	(
	    		jobString += ";normals=false"
	    		jobString += ";purepointcache=false"
	    	)
	    	if(meshTopologyDropDown.selection == 2) do
	    	(
	    		jobString += ";normals=false"
	    		jobString += ";purepointcache=true"
	    	)
	    	if(meshTopologyDropDown.selection == 3) do 
	    	(
	    		jobString += ";purepointcache=false"
	    		jobString += ";normals=true"
	    	)
	    	jobString += ";uvs=" 
	    	jobString += (uvCheckbox.checked as string)
	    	jobString += ";materialids=" 
	    	jobString += (materialIdsCheckbox.checked as string)
	    	jobString += ";exportselected=" 
	    	jobString += (exportSelectedCheckbox.checked as string)
	    	jobString += ";flattenhierarchy=" 
	    	jobString += (flattenHierarchyCheckbox.checked as string)
	    	if(particleSystemExportMethod.selection == 1) do jobString += ";automaticinstancing=true" 
	    	if(particleSystemExportMethod.selection == 2) do jobString += ";particlesystemtomeshconversion=true" 
	    	jobString += ";transformCache="
	    	jobString += (transformCacheCheckbox.checked as string)
	    	jobString += ";validateMeshTopology="
	    	jobString += (validateMeshTopology.checked as string)
	    	jobString += ";renameConflictingNodes="
	    	jobString += (renameConflictingNodes.checked as string)
	    	jobString += ";mergePolyMeshSubtree="
	    	jobString += (mergeSelectedPolymeshSubtree.checked as string)
 	jobString += ";mergePolyMeshSubtree="
	    	jobString += (mergeSelectedPolymeshSubtree.checked as string)

	    	if(storageFormat.selection == 1) do jobString += ";storageFormat=hdf5" 
	    	if(storageFormat.selection == 2) do jobString += ";storageFormat=ogawa" 

	    	if(facesetExportMethod.selection == 1) do jobString += ";facesets=partitioningFacesetsOnly"
	    	if(facesetExportMethod.selection == 2) do jobString += ";facesets=all"

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
