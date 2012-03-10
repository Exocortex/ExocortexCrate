plugin modifier AlembicMeshPointCacheModifier
	name:"Alembic Mesh Point Cache"
	classID:#(0x20730623, 0x5d650bdb)
	category:"Alembic" 
	version:1
	extends:AlembicMeshModifier
	replaceUI:true
	invisible:false
(
	parameters pblock rollout:params
	(
		fileName type:#filename ui:fileName default:"filename"
		dataPath type:#string ui:dataPath default:"path"
		currentTimeHidden type:#float default:0
		timeOffset type:#float ui:timeOffset animatable:true default:0
		timeScale type:#float ui:timeScale animatable:true default:1
		faceSet type:#boolean ui:faceSet default:true
		vertices type:#boolean ui:vertices default:true
		normals type:#boolean ui:normals default:true
		uvs type:#boolean ui:uvs default:true
		clusters type:#boolean ui:clusters default:true
		muted type:#boolean ui:muted animatable:true default:false
		
		
		on fileName set val do delegate.fileName = val
		on dataPath set val do delegate.dataPath = val
		--on currentTimeHidden set val do delegate.currentTimeHidden = val
		on timeOffset set val do delegate.timeOffset = val
		on timeScale set val do delegate.timeScale = val
		on faceSet set val do delegate.faceSet = val
		on vertices set val do delegate.vertices = val
		on normals set val do delegate.normals = val
		on uvs set val do delegate.uvs = val
		on clusters set val do delegate.clusters = val
		on muted set val do delegate.muted = val
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
		checkbox muted "Muted"
	)
	
	local update_currentTimeHidden_registered = false
	
	fn update_currentTimeHidden =
	(
		currentTimeHidden = currentTime / frameRate
	)
		
	on create do
	(
		if( not update_currentTimeHidden_registered ) do
		(
			currentTimeHidden = currentTime / frameRate
			registerTimeCallback update_currentTimeHidden
			update_currentTimeHidden_registered = true
		)
	)

)