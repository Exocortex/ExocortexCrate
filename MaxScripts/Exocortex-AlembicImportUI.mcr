---------------------------------------------------------------------------------------------------------
-- Custom import/export dialog with settings

rollout AlembicImportSettings "Alembic Import Settings" width:288 height:236
(
    local filename

    GroupBox geoGroup "Geometry" pos:[8,8] width:272 height:104
    checkbox normalCheckbox "Normals" pos:[48,32] width:200 height:15 checked:true
    checkbox uvCheckbox "UVs" pos:[48,48] width:200 height:15 checked:true
    checkbox clustersCheckbox "Clusters" pos:[48,64] width:200 height:15 checked:true
    checkbox attachCheckbox "Attach to existing objects" pos:[48,80] width:200 height:15 checked:true

    button importButton "Import" pos:[16,200] width:64 height:24
    button cancelButton "Cancel" pos:[208,200] width:64 height:24

    on importButton pressed do
    (
        ExocortexAlembic.import filename normalCheckbox.checked uvCheckbox.checked clustersCheckbox.checked attachCheckbox.checked
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
    category:"Exocortex" 
    buttonText:"Alembic Import..."
    tooltip:"Alembic Import..."
(
    on execute do  
    (
        alembicImportDialog()
    )
)
