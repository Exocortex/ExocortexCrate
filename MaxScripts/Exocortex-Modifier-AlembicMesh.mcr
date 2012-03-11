plugin modifier AlembicMeshModifier
	name:"Alembic Mesh"
	classID:#(0x20730623, 0x5d650bdb)	-- keep in sync with AlembicNames.h
	category:"Alembic" 
	version:1
	extends:AlembicMeshBaseModifier
	replaceUI:true
	invisible:false
(
	parameters pblock rollout:params
	(
		path type:#filename ui:path default:""
		identifier type:#string ui:identifier default:""
		currentTimeHidden type:#float default:0
		timeOffset type:#float ui:timeOffset animatable:true default:0
		timeScale type:#float ui:timeScale animatable:true default:1
		faceSet type:#boolean ui:faceSet default:true
		vertices type:#boolean ui:vertices default:true
		normals type:#boolean ui:normals default:true
		uvs type:#boolean ui:uvs default:true
		clusters type:#boolean ui:clusters default:true
		muted type:#boolean ui:muted animatable:true default:false
		
		on path set val do delegate.path = val
		on identifier set val do delegate.identifier = val
		on currentTimeHidden set val do delegate.currentTimeHidden = val
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
		edittext path "Path"
		edittext identifier "Identifier"
		spinner timeOffset "Time Offset" range:[-10000,10000,0]
		spinner timeScale "Time Scale" range:[-10000,10000,0]		
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
		if( not update_currentTimeHidden_registered ) do
		(
			print "currentTimeHidden registered"
			registerTimeCallback update_currentTimeHidden
			update_currentTimeHidden_registered = true
		)
		currentTimeHidden = currentTime / frameRate
		delegate.currentTimeHidden = currentTimeHidden
	)
		
	on create do
	(
		update_currentTimeHidden()
	)
	on load do
	(
		update_currentTimeHidden()
	)
	on update do 
	(
		update_currentTimeHidden()
	)

)