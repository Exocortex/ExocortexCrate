---------------------------------------------------------------------------------------------------------
-- Custom import/export dialog with settings

rollout AlembicExportSettings "Alembic Export Settings" width:288 height:304
(
    GroupBox animGroup "Animation" pos:[8,8] width:272 height:96
    label inLabel "Frame In" pos:[16,32] width:128 height:21
    label outLabel "Frame Out" pos:[16,48] width:128 height:21
    label stepsLabel "Frame Steps" pos:[16,64] width:128 height:21
	label subStepsLabel "Frame Sub-Steps" pos:[16,80] width:128 height:21
    spinner inSpinner "" pos:[112,32] width:160 height:16 range:[1,1000000,1] type:#integer
    spinner outSpinner "" pos:[112,48] width:160 height:16 range:[1,1000000,100] type:#integer
    spinner stepsSpinner "" pos:[112,64] width:160 height:16 range:[1,1000000,1] type:#integer
	spinner subStepsSpinner "" pos:[112,80] width:160 height:16 range:[1,1000000,1] type:#integer
    
    GroupBox geoGroup "Geometry" pos:[8,112] width:272 height:144
    dropdownList meshTopologyDropDown "Mesh Topology" pos:[16,136] width:256 height:40 items:#("Just Surfaces (No Normals)", "Point Cache (No Surfaces)", "Surface + Normals (For Interchange)") selection:3
    checkbox uvCheckbox "UVs" pos:[32,184] width:128 height:15 checked:true
    --checkbox clustersCheckbox "Clusters" pos:[32,200] width:128 height:15 checked:true
    checkbox envelopeCheckbox "Envelope BindPose" pos:[32,200] width:150 height:15 checked:true
    checkbox dynamicTopologyCheckbox "Dynamic Topology" pos:[32,216] width:128 height:15 checked:true

    button exportButton "Export" pos:[16,272] width:64 height:24
    button cancelButton "Cancel" pos:[208,272] width:64 height:24

    on exportButton pressed do
    (
        filename = getSaveFileName caption:"Export to Alembic File:" types:"Alembic(*.abc)|*.abc|All(*.*)|*.*" historyCategory:"Alembic"
        if (filename != undefined) do
        (
            ExocortexAlembic.export filename inSpinner.value outSpinner.value stepsSpinner.value subStepsSpinner.value meshTopologyDropDown.selection uvCheckbox.checked false envelopeCheckbox.checked dynamicTopologyCheckbox.checked
            destroyDialog AlembicExportSettings
        )
    )
    on cancelButton pressed do
    (
        destroyDialog AlembicExportSettings
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
    category:"Exocortex" 
    buttonText:"Alembic Export..."
    tooltip:"Alembic Export..."
(
    on execute do 
    (
        alembicExportDialog()
    )
)
