import win32com.client
from win32com.client import constants

#=====================================================================================
def XSILoadPlugin( in_reg ):
    in_reg.Author = "Exocortex Technologies, Inc and Helge Mathee"
    in_reg.Name = "ExocortexAlembicSoftimage_UI"
    in_reg.Major = 1
    in_reg.Minor = 0
    in_reg.Email = "support@exocortex.com"
    in_reg.URL = "http://www.exocortex.com/alembic"
    
    in_reg.RegisterMenu(constants.siMenuMainFileExportID,"alembic_MenuExport", False, False)
    in_reg.RegisterMenu(constants.siMenuMainFileImportID,"alembic_MenuImport", False, False)
    in_reg.RegisterMenu(constants.siMenuMainFileProjectID,"alembic_MenuPathManager", False, False)
    in_reg.RegisterMenu(constants.siMenuMainFileProjectID,"alembic_ProfileStats",False, False)
    in_reg.RegisterMenu(constants.siMenuTbGetPropertyID,"alembic_MenuMetaData", False, False)

    Application.LogMessage(in_reg.Name + " has been loaded.",constants.siVerbose)

    return True


#=====================================================================================
def XSIUnloadPlugin( in_reg ):
    strPluginName = in_reg.Name
    Application.LogMessage(str(strPluginName) + str(" has been unloaded."),constants.siVerbose)
    return True


#=====================================================================================
def alembic_MenuExport_Init(ctx):    
    ctx.Source.AddCommandItem("Alembic 1.1", "alembic_export_jobs")

#=====================================================================================
def alembic_MenuImport_Init(ctx):    
    ctx.Source.AddCommandItem("Alembic 1.1", "alembic_import_jobs")

#=====================================================================================
def alembic_MenuPathManager_Init(ctx):    
    ctx.Source.AddCommandItem("Alembic Path Manager", "alembic_path_manager")

#=====================================================================================
def alembic_ProfileStats_Init(ctx):    
    ctx.Source.AddCommandItem("Alembic Profile Stats", "alembic_profile_stats")

#=====================================================================================
def alembic_MenuMetaData_Init(ctx):    
    ctx.Source.AddCommandItem("Alembic MetaData", "alembic_attach_metadata")
