plugin simpleObject AlembicMesh
	name:"Alembic Mesh"
	category:"Alembic Objects"
	classID:#(0xe855567c, 0xbcd73b8b)
(
	
	parameters main rollout:params
	(
		fileName type:#string ui:fileName default:"filename"
		dataPath type:#string ui:dataPath default:"path"
		currentTimeHidden type:#float default:0
		timeOffset type:#float ui:timeOffset animatable:true default:0
		timeScale type:#float ui:timeScale animatable:true default:1
		faceSet type:#boolean ui:faceSet default:true
		vertices type:#boolean ui:vertices default:true
		normals type:#boolean ui:normals default:true
		uvs type:#boolean ui:uvs default:true
		clusters type:#boolean ui:clusters default:true
		mute type:#boolean ui:mute animatable:true default:false
	)

	rollout params "Alembic"
	(
		edittext fileName "File Name"
		edittext dataPath "Data Path"
		spinner timeOffset "Time Offset" range:[-1000,1000,0]
		spinner timeScale "Time Scale" range:[-1000,1000,0]
		checkbox faceSet "Topology"
		checkbox vertices "Vertices"
		checkbox normals "Normals"
		checkbox uvs "UVs"
		checkbox clusters "Clusters"
		checkbox mute "Mute"
	)

	local update_currentTimeHidden_registered = false
	
	fn update_currentTimeHidden =
	(
		currentTimeHidden = currentTime / frameRate
	)
	
	on buildMesh do
	(
		if( not update_currentTimeHidden_registered ) do
		(
			currentTimeHidden = currentTime / frameRate
			registerTimeCallback update_currentTimeHidden
			update_currentTimeHidden_registered = true
		)
		
		dataTime = timeOffset + currentTimeHidden  * timeScale
		tempMesh = TriMesh()
		if( not mute ) do
		(
			tempMesh = ExocortexAlembic.importMesh tempMesh fileName dataPath dataTime faceSet vertices normals uvs clusters
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