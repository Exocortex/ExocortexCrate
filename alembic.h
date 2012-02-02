#ifndef _ALEMBIC_H_
#define _ALEMBIC_H_

#ifdef NOMINMAX
	#undef NOMINMAX
#endif

#include "Max.h"
#include "Foundation.h"

// Alembic Data Fill Bit Flags
typedef unsigned int AlembicDataFillFlags;
const unsigned int ALEMBIC_DATAFILL_VERTEX = 1;
const unsigned int ALEMBIC_DATAFILL_FACELIST = 2; 

enum MeshTopologyType
{
    NORMAL,
    SURFACE,
    NORMAL_SURFACE
};

enum alembic_return_code
{
	alembic_success = 0,
	alembic_invalidarg,
	alembic_failure,
};

typedef struct _alembic_importoptions
{
   bool importNormals;
   bool importUVs;
   bool importClusters;
   bool importVisibility;
   bool importStandins;
   bool importBboxes;
   bool attachToExisting;

   _alembic_importoptions() : importNormals(false)
	, importUVs(false)
	, importClusters(false)
	, importVisibility(false)
	, importStandins(false)
	, importBboxes(false)
	, attachToExisting(false)
   {
   }
} alembic_importoptions;

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

SampleInfo getSampleInfo(double iFrame,Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps);

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

#endif