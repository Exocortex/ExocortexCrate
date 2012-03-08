// alembicPlugin
#include "Alembic.h"
#include "AlembicDefinitions.h"
#include "AlembicWriteJob.h"
#include "iparamm2.h"
#include "MeshMtlList.h"
#include "SceneEnumProc.h"
#include "SceneEntry.h"
#include "ObjectList.h"
#include "ObjectEntry.h"
#include "resource.h"
#include "utility.h"

// Dummy function for progress bar
DWORD WINAPI fn(LPVOID arg)
{
	return(0);
}

// Dialog proc
static INT_PTR CALLBACK ExportDlgProc(HWND hWnd, UINT msg,	WPARAM wParam, LPARAM lParam)
{
    AlembicExporter *exp = reinterpret_cast<AlembicExporter *>(lParam);
    switch (msg) {
    case WM_INITDIALOG:
        {
            char str[32];
            HWND hwndEdit = NULL;

            // The the dialogue with values from exporter            
            CenterWindow(hWnd, GetParent(hWnd)); 
            CheckDlgButton(hWnd, IDC_CHECK_UV, exp->GetExportUV()); 
            CheckDlgButton(hWnd, IDC_CHECK_CLUSTER, exp->GetExportClusters()); 
            CheckDlgButton(hWnd, IDC_CHECK_BINDPOSE, exp->GetExportEnvelopeBindPose());
            CheckDlgButton(hWnd, IDC_CHECK_TOPO, exp->GetExportDynamicTopology()); 

            hwndEdit = GetDlgItem(hWnd, IDC_EDIT_IN);
            sprintf_s(str, 32, "%d" , exp->GetFrameIn());
            Edit_SetText(hwndEdit, str);

            hwndEdit = GetDlgItem(hWnd, IDC_EDIT_OUT);
            sprintf_s(str, 32, "%d" , exp->GetFrameOut());
            Edit_SetText(hwndEdit, str);

            hwndEdit = GetDlgItem(hWnd, IDC_EDIT_FRAME_STEP);
            sprintf_s(str, 32, "%d" , exp->GetFrameSteps());
            Edit_SetText(hwndEdit, str);

            hwndEdit = GetDlgItem(hWnd, IDC_EDIT_SUB_STEP);
            sprintf_s(str, 32, "%d" , exp->GetFrameSubSteps());
            Edit_SetText(hwndEdit, str);

            MeshTopologyType topoType = exp->GetTopologyType();
            HWND hwndCombo = GetDlgItem(hWnd, IDC_COMBO_MESH);

            const TCHAR* m_sArray[] = 
            {                 
                _T( "Just Surfaces (No Normals)" ), 
                _T( "Point Cache (No Surfaces)" ),
                _T( "Surface + Normals (For Interchange)" )
            };

            for(int iCount = 0; iCount < 3; iCount++ )
            {
                ::SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM)m_sArray[iCount]);
            }

            ComboBox_SetCurSel(hwndCombo, topoType);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDC_OK:
            {
                char str[32];
                HWND hwndEdit = NULL;

                // Set back the parameter to the exporter
                hwndEdit = GetDlgItem(hWnd, IDC_CHECK_UV);
                exp->SetExportUV(Button_GetState(hwndEdit) == BST_CHECKED);

                hwndEdit = GetDlgItem(hWnd, IDC_CHECK_CLUSTER);
                exp->SetExportClusters(Button_GetState(hwndEdit) == BST_CHECKED);

                hwndEdit = GetDlgItem(hWnd, IDC_CHECK_BINDPOSE);
                exp->SetExportEnvelopeBindPose(Button_GetState(hwndEdit) == BST_CHECKED);

                hwndEdit = GetDlgItem(hWnd, IDC_CHECK_TOPO);
                exp->SetExportDynamicTopology(Button_GetState(hwndEdit) == BST_CHECKED);

                hwndEdit = GetDlgItem(hWnd, IDC_EDIT_IN);                
                Edit_GetText(hwndEdit, str, 32);
                double frameIn = atoi(str);

                hwndEdit = GetDlgItem(hWnd, IDC_EDIT_OUT);                
                Edit_GetText(hwndEdit, str, 32);
                double frameOut = atoi(str);                

                hwndEdit = GetDlgItem(hWnd, IDC_EDIT_FRAME_STEP);                
                Edit_GetText(hwndEdit, str, 32);
                double frameSteps = atoi(str);               

                hwndEdit = GetDlgItem(hWnd, IDC_EDIT_SUB_STEP);                
                Edit_GetText(hwndEdit, str, 32);
                double frameSubSteps = atoi(str);

                exp->SetFrameIn(frameIn);
                exp->SetFrameOut(frameOut);
                exp->SetFrameSteps(frameSteps);
                exp->SetFrameSubSteps(frameSubSteps);

                HWND hwndCombo = GetDlgItem(hWnd, IDC_COMBO_MESH);
                exp->SetTopologyType(static_cast<MeshTopologyType>(ComboBox_GetCurSel(hwndCombo)));

                EndDialog(hWnd, 1);
            }
            break;
        case IDC_CANCEL:
            EndDialog(hWnd, 0);
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}       

#define ALEMBICEXPORTER_CLASS_ID	Class_ID(0x79d613a4, 0x4f21c3ad)

class AlembicExporterClassDesc:public ClassDesc2 
{
public:
    int 			IsPublic() { return TRUE; }
    void *			Create(BOOL loading = FALSE) { return new AlembicExporter(); }
    const TCHAR *	ClassName() { return _T("AlembicExporter"); }
    SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
    Class_ID		ClassID() { return ALEMBICEXPORTER_CLASS_ID; }
    const TCHAR* 	Category() { return _T("Sparks"); }

    const TCHAR*	InternalName() { return _T("AlembicExporter"); }	// returns fixed parsable name (scripter-visible name)
    HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle
};

static AlembicExporterClassDesc AlembicExporterDesc;
ClassDesc2* GetAlembicExporterDesc() { return &AlembicExporterDesc; }


int	AlembicExporter::ExtCount()
{
    return 1;
}

const TCHAR *AlembicExporter::Ext(int n)
{
    return _T("abc");
}

const TCHAR *AlembicExporter::LongDesc()
{
    return _T("Alembic exporter for 3DSMax scene");
}

const TCHAR *AlembicExporter::ShortDesc()
{
    return _T("Alembic Exporter");
}

const TCHAR *AlembicExporter::AuthorName()
{
    return _T("Exocortex");
}

const TCHAR *AlembicExporter::CopyrightMessage()
{
    return _T("Copyright Exocortex");
}

const TCHAR *AlembicExporter::OtherMessage1()
{
    return _T("");
}

const TCHAR *AlembicExporter::OtherMessage2()
{
    return _T("");
}

unsigned int AlembicExporter::Version()
{
    return 100;
}

void AlembicExporter::ShowAbout(HWND hWnd)
{

}

int	AlembicExporter::DoExport(const TCHAR *filename, ExpInterface *ei, Interface *i, BOOL suppressPrompts, DWORD options)
{
    ESS_CPP_EXCEPTION_REPORTING_START
		
	// Prompt the user with our dialogbox, and get all the options.
    /*
    if (!DialogBoxParam(GetAlembicExporterDesc()->HInstance(), MAKEINTRESOURCE(IDD_ALEMBIC_EXPORT_DLG), i->GetMAXHWnd(), ExportDlgProc, (LPARAM)this)) 
    {
    // User cancelled
    return 1;
    }
    */

    // Startup the progress bar.
	i->ProgressStart("Exporting Alembic File", TRUE, fn, NULL);

    MeshMtlList allMtls;
    SceneEnumProc currentScene(ei->theScene, i->GetTime(), i, &allMtls);
    ObjectList allSceneObjects(currentScene);
    Object *currentObject = NULL;

    double minFrame = 1000000.0;
    double maxFrame = -1000000.0;
    double maxSteps = 1;
    double maxSubsteps = 1;

    // remember the min and max values for the frames
    if(frameIn < minFrame)
    {
        minFrame = frameIn;
    }

    if(frameOut > maxFrame)
    {
        maxFrame = frameOut;
    }

    if(frameSteps > maxSteps)
    {
        maxSteps = frameSteps;
    }

    if(frameSteps > 1.0)
    {
        frameSubSteps = 1.0;
    }

    if(frameSubSteps > maxSubsteps)
    {
        maxSubsteps = frameSubSteps;
    }

    if (strlen(filename) <= 0)
    {
        MessageBox(GetActiveWindow(), "[alembic] No filename specified.", "Error", MB_OK);
        return 1;
    }

    std::vector<double> frames;
    for(double frame=frameIn; frame<=frameOut; frame+=frameSteps / frameSubSteps)
    {
        // Adding frames
        frames.push_back(frame);
    }

    AlembicWriteJob *job = new AlembicWriteJob(filename, allSceneObjects, frames, i);
    job->SetOption("exportNormals",topologyType != SURFACE);
    job->SetOption("exportUVs", exportUV);
    job->SetOption("exportFaceSets",topologyType != NORMAL);
    job->SetOption("exportBindPose",exportEnvelopeBindPose);
    job->SetOption("exportPurePointCache",exportClusters);
    job->SetOption("exportDynamicTopology",exportDynamicTopology);
    job->SetOption("indexedNormals",true);
    job->SetOption("indexedUVs",true);

    // check if the job is satifsied
    if(job->PreProcess() != true)
    {
        MessageBox(GetActiveWindow(), "[alembic] Job skipped. Not satisfied.", "Error", MB_OK);
        delete(job);
        return 1;
    }

    // now, let's run through all frames, and process the jobs
    for(double frame = minFrame; frame<=maxFrame; frame += maxSteps / maxSubsteps)
    {
        i->ProgressUpdate(static_cast<int>(frame/maxFrame*100.0f));
        int ticks = GetTimeValueFromFrame(frame);
        i->SetTime(ticks);
        job->Process(frame);
    }

    delete(job);
    i->ProgressEnd();

    return TRUE;

	ESS_CPP_EXCEPTION_REPORTING_END
}

AlembicExporter::AlembicExporter()
{
    exportUV = false;
    exportClusters = false;
    exportEnvelopeBindPose = false;
    exportDynamicTopology = false;
    frameIn = 1;
    frameOut = 100;
    frameSteps = 1;
    frameSubSteps = 1;
    topologyType = NORMAL_SURFACE;
}

AlembicExporter::~AlembicExporter()
{
}

bool AlembicExporter::GetExportUV()
{
    return exportUV;
}

bool AlembicExporter::GetExportClusters()
{
    return exportClusters;
}

bool AlembicExporter::GetExportEnvelopeBindPose()
{
    return exportEnvelopeBindPose;
}

bool AlembicExporter::GetExportDynamicTopology()
{
    return exportDynamicTopology;
}

double AlembicExporter::GetFrameIn()
{
    return frameIn;
}

double AlembicExporter::GetFrameOut()
{
    return frameOut;
}

double AlembicExporter::GetFrameSteps()
{
    return frameSteps;
}

double AlembicExporter::GetFrameSubSteps()
{
    return frameSubSteps;
}

MeshTopologyType AlembicExporter::GetTopologyType()
{
    return topologyType;
}

void AlembicExporter::SetExportUV(bool val)
{
    exportUV = val;
}

void AlembicExporter::SetExportClusters(bool val)
{
    exportClusters = val;
}

void AlembicExporter::SetExportEnvelopeBindPose(bool val)
{
    exportEnvelopeBindPose = val;
}

void AlembicExporter::SetExportDynamicTopology(bool val)
{
    exportDynamicTopology = val;
}

void AlembicExporter::SetFrameIn(double val)
{
    frameIn = val;
}

void AlembicExporter::SetFrameOut(double val)
{
    frameOut = val;
}

void AlembicExporter::SetFrameSteps(double val)
{
    frameSteps = val;
}

void AlembicExporter::SetFrameSubSteps(double val)
{
    frameSubSteps = val;
}

void AlembicExporter::SetTopologyType(MeshTopologyType val)
{
    topologyType = val;
}

void AlembicImport_SetupChildLinks( Alembic::Abc::IObject &obj, alembic_importoptions &options )
{
    INode *pParentNode = options.currentSceneList.FindNodeWithName(std::string(obj.getName()));

    if (pParentNode)
    {
        for (size_t i = 0; i < obj.getNumChildren(); i += 1)
        {
            Alembic::Abc::IObject childObj = obj.getChild(i);
            INode *pChildNode = options.currentSceneList.FindNodeWithName(std::string(childObj.getName()));

            // PeterM: This will need to be rethought out on how this works
            if (pChildNode)
                pParentNode->AttachChild(pChildNode, 0);
        }
    }
}
