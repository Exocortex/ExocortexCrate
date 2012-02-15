
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

rollout AlembicExportSettings "Alembic Export Settings" width:180 height:180
(
	label animLabel "Settings (none at this time)" pos:[10,7] width:160 height:21
	button exportButton "Export" pos:[20,144] width:64 height:24
	button cancelButton "Cancel" pos:[100,144] width:64 height:24

	-- checkbox normalsCheckbox "Normals" pos:[57,87] width:62 height:15 checked:true
	-- checkbox chk2 "UVs" pos:[57,110] width:62 height:15 checked:true
	-- label animLabel "Animation" pos:[10,7] width:78 height:21
	-- editText animationInBox "In" pos:[34,25] width:64 height:16
	-- editText edt2 "Out" pos:[35,44] width:64 height:16
	-- editText edt3 "Frame Steps" pos:[36,64] width:128 height:16

	on exportButton pressed do
	(
		filename = getSaveFileName caption:"Export to Alembic File:" types:"Alembic(*.abc)|*.abc|All(*.*)|*.*" historyCategory:"Alembic"
		if (filename != undefined) do
		(
			ExocortexAlembic.export filename
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
		createDialog AlembicImportSettings -1 -1
		AlembicImportSettings.filename = filename
	)
)

fn alembicExportDialog =
(
	createDialog AlembicExportSettings -1 -1
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
