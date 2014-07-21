include "Exocortex-Dialog-AlembicImport.mcr"
include "Exocortex-Dialog-AlembicExport.mcr"
include "Exocortex-Helper-AlembicTimeControl_Mesh.mcr"
include "Exocortex-Helper-AlembicTimeControl.mcr"
include "Exocortex-Metadata.mcr"
include "Exocortex-AlembicTimeControlUtilities.mcr"

macroScript AlembicProfileStats
    category:"Alembic" 
    buttonText:"Profile Stats"
    tooltip:"Output profiling statistics to the log window"
(
    on execute do 
    (
		ExocortexAlembic.profileStats()
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
    alembicImportItem = menuMan.createActionItem "AlembicImportUI" "Alembic"

    -- Create a menu item that brings up the export dialog
    alembicExportItem = menuMan.createActionItem "AlembicExportUI" "Alembic"

    -- Create a menu item that add Metadata display modifier to selected objects
    alembicAddMetadata = menuMan.createActionItem "AlembicCreateMetadataModifier" "Alembic"

    -- Create a menu item that add Metadata display modifier to selected objects
    alembicProfileStats = menuMan.createActionItem "AlembicProfileStats" "Alembic"

    alembicTimeControlItem = menuMan.createActionItem "NewTimeControlUI" "Alembic"

    newTimeControlForSelectionItem = menuMan.createActionItem "NewTimeControlForSelectionUI" "Alembic"

    setTimeControlForSelectionItem = menuMan.createActionItem "SetTimeControlForSelectionUI" "Alembic"
    
    -- Add the items to the menu
    alembicMenu.addItem alembicImportItem 1
    alembicMenu.addItem alembicExportItem 2
    alembicMenu.addItem alembicAddMetadata 3
    alembicMenu.addItem alembicProfileStats 4
    alembicMenu.addItem alembicTimeControlItem 5
    alembicMenu.addItem newTimeControlForSelectionItem 6
    alembicMenu.addItem setTimeControlForSelectionItem 7

    -- Create a new menu item with the menu as it's sub-menu
    alembicMainMenuItem = menuMan.createSubMenuItem "Alembic" alembicMenu

    -- compute the index of the next-to-last menu item in the main menu bar
    alembicMenuItemIndex = mainMenuBar.numItems()

    -- Add the sub-menu just at the second to last slot
    mainMenuBar.addItem alembicMainMenuItem alembicMenuItemIndex

    -- redraw the menu bar with the new item
    menuMan.updateMenuBar()
    
    -- Initialize the exocortex alembic libraries
    ExocortexAlembic.init()
)


initializeAlembicMenus()
