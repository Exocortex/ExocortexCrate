plugin simpleObject AlembicMesh
	name:"Alembic Mesh"
	category:"Alembic"
	classID:#(0xe855567c, 0xbcd73b8b)	-- keep in sync with AlembicNames.h
(
	
	parameters main rollout:params
	(
		path type:#string ui:path default:""
		identifier type:#string ui:identifier default:""
		time type:#float ui:time default:0
		topology type:#boolean ui:topology default:true
		geometry type:#boolean ui:geometry default:true
		normals type:#boolean ui:normals default:true
		uvs type:#boolean ui:uvs default:true
		muted type:#boolean ui:muted animatable:true default:false
	)

	rollout params "Alembic"
	(
		edittext path "Path"
		edittext identifier "Identifier"
		spinner time "Time" range:[-10000,10000,0]
		checkbox topology "Topology"
		checkbox geometry "Geometry"
		checkbox normals "Normals"
		checkbox uvs "UVs"
		checkbox muted "Muted"
	)

	on buildMesh do
	(
		
		
		dataTime = timeOffset + currentTimeHidden  * timeScale
		tempMesh = TriMesh()
		if( not muted ) do
		(
			tempMesh = ExocortexAlembic.importMesh tempMesh path identifier time topology geometry normals uvs
		)
		mesh = tempMesh;
	)

	tool create
	(
		on mousePoint click do
		(
			(#stop)
		)
	)
)