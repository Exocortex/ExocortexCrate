#ifndef _ALEMBIC_H_
#define _ALEMBIC_H_

#include "Foundation.h"

class AlembicExporter : public SceneExport 
{
public:
	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

    bool            GetExportUV();
    bool            GetExportClusters();
    bool            GetExportEnvelopeBindPose();
    bool            GetExportDynamicTopology();
    double          GetFrameIn();
    double          GetFrameOut();
    double          GetFrameSteps();
    double          GetFrameSubSteps();
    MeshTopologyType GetTopologyType();

    void            SetExportUV(bool val);
    void            SetExportClusters(bool val);
    void            SetExportEnvelopeBindPose(bool val);
    void            SetExportDynamicTopology(bool val);
    void            SetFrameIn(double val);
    void            SetFrameOut(double val);
    void            SetFrameSteps(double val);
    void            SetFrameSubSteps(double val);
    void            SetTopologyType(MeshTopologyType val);

	int	DoExport(const TCHAR *filename,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);

	AlembicExporter();
	~AlembicExporter();

private:
    bool exportUV;
    bool exportClusters;
    bool exportEnvelopeBindPose;
    bool exportDynamicTopology;
    double frameIn;
    double frameOut;
    double frameSteps;
    double frameSubSteps;
    MeshTopologyType topologyType;
};

// Helper function to set up the node hierarchy
void AlembicImport_SetupChildLinks( Alembic::Abc::IObject &obj, alembic_importoptions &options );

#endif