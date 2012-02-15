
---------------------------------------------------------------------------------------------------------
-- Custom import/export dialog with settings
rollout AlembicImportSettings "Alembic Import Settings" width:180 height:180
(
    local filename

    label animLabel "Settings (none at this time)" pos:[10,7] width:160 height:21
    button importButton "Import" pos:[20,144] width:64 height:24
    button cancelButton "Cancel" pos:[100,144] width:64 height:24

    on importButton pressed do
    (
        ExocortexAlembic.import filename
        destroyDialog AlembicImportSettings
    )

    on cancelButton pressed do
    (
        destroyDialog AlembicImportSettings
    )
)

rollout AlembicExportSettings "Alembic Export Settings" width:288 height:288
(
    GroupBox animGroup "Animation" pos:[5,8] width:272 height:80
    label inLabel "Frame In" pos:[16,32] width:128 height:21
    label outLabel "Frame Out" pos:[16,48] width:128 height:21
    label stepsLabel "Frame Steps" pos:[16,64] width:128 height:21
    spinner inSpinner "" pos:[112,32] width:160 height:16 range:[1,1000000,1] type:#integer
    spinner outSpinner "" pos:[112,48] width:160 height:16 range:[1,1000000,100] type:#integer
    spinner stepsSpinner "" pos:[112,64] width:160 height:16 range:[1,1000000,1] type:#integer
    
    GroupBox geoGroup "Geometry" pos:[8,96] width:272 height:136
    dropdownList meshTopologyDropDown "Mesh Topology" pos:[16,112] width:248 height:40 items:#("Just Surfaces (No Normals)", "Point Cache (No Surfaces)", "Surface + Normals (For Interchange)") selection:3
    checkbox uvCheckbox "UVs" pos:[48,160] width:128 height:15 checked:true
    checkbox clustersCheckbox "Clusters" pos:[48,176] width:128 height:15 checked:true
    checkbox envelopeCheckbox "Envelope BindPose" pos:[48,192] width:150 height:15 checked:true
    checkbox dynamicTopologyCheckbox "Dynamic Topology" pos:[48,208] width:128 height:15 checked:true

    button exportButton "Export" pos:[16,248] width:64 height:24
    button cancelButton "Cancel" pos:[208,248] width:64 height:24

    on exportButton pressed do
    (
        filename = getSaveFileName caption:"Export to Alembic File:" types:"Alembic(*.abc)|*.abc|All(*.*)|*.*" historyCategory:"Alembic"
        if (filename != undefined) do
        (
            ExocortexAlembic.export filename inSpinner.value outSpinner.value stepsSpinner.value meshTopologyDropDown.selection uvCheckbox.checked clustersCheckbox.checked envelopeCheckbox.checked dynamicTopologyCheckbox.checked
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
fn alembicImportDialog =
(
    filename = getOpenFileName caption:"Import from Alembic File:" types:"Alembic(*.abc)|*.abc|All(*.*)|*.*" historyCategory:"Alembic"
    if (filename != undefined) do
    (
        createDialog AlembicImportSettings
        AlembicImportSettings.filename = filename
    )
)

fn alembicExportDialog =
(
    createDialog AlembicExportSettings
)


---------------------------------------------------------------------------------------------------------
-- MacroScripts the link the menu items to MAXScript functions
macroScript AlembicImportUI
    category:"Alembic UI" 
    buttonText:"Alembic Import..."
    tooltip:"Alembic Import..."
(
    on execute do  
    (
        alembicImportDialog()
    )
)

macroScript AlembicExportUI
    category:"Alembic UI" 
    buttonText:"Alembic Export..."
    tooltip:"Alembic Export..."
(
    on execute do 
    (
        alembicExportDialog()
    )
)


---------------------------------------------------------------------------------------------------------
-- Function to create the menu items in the main menu bar
fn initializeAlembicMenus =
(
    -- Get the main menu bar
    mainMenuBar = menuMan.getMainMenuBar()
        
    -- Create a new menu
    alembicMenu = menuMan.findMenu "Alembic"
    if (alembicMenu != undefined) do 
    (
        menuMan.unRegisterMenu alembicMenu
    )
    alembicMenu = menuMan.createMenu "Alembic"

    -- Create a menu items that brings up the import dialog
    alembicImportItem = menuMan.createActionItem "AlembicImportUI" "Alembic UI"

    -- Create a menu item that brings up the export dialog
    alembicExportItem = menuMan.createActionItem "AlembicExportUI" "Alembic UI"

    -- Add the items to the menu
    alembicMenu.addItem alembicImportItem 1
    alembicMenu.addItem alembicExportItem 2

    -- Create a new menu item with the menu as it's sub-menu
    alembicMainMenuItem = menuMan.createSubMenuItem "Alembic" alembicMenu

    -- compute the index of the next-to-last menu item in the main menu bar
    alembicMenuItemIndex = mainMenuBar.numItems()

    -- Add the sub-menu just at the second to last slot
    mainMenuBar.addItem alembicMainMenuItem alembicMenuItemIndex

    -- redraw the menu bar with the new item
    menuMan.updateMenuBar()
)


initializeAlembicMenus()
