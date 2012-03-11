plugin modifier AlembicMeshPointCacheModifier
	name:"Alembic Mesh Point Cache"
	classID:#(0x20730623, 0x5d650bdb)	-- keep in sync with AlembicNames.h
	category:"Alembic" 
	version:1
	extends:AlembicMeshModifier
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
		format "on create : % \n" this
		format "current obj: %\n" $
		update_currentTimeHidden()
	)
	on postCreate do 
	(
		format "on postCreate : %\n" this
		format "  current obj: %\n" $
	)
	on load do
	(
		format "on load : %\n" this
		format "current obj: %\n" $
		update_currentTimeHidden()
	)
	on postLoad do 
	(
		format "on postLoad : %\n" this
		format "current obj: %\n" $
	)
	on clone orig do 
	(
		format "on clone: % : % : % : %\n" this orig
		format "current obj: %\n" $
	)
	on update do 
	(
		format "on update : %\n" this
		format "current obj: %\n" $
		update_currentTimeHidden()
	)
	on attachedToNode node do
	(
		format "on attachedToNode: % %\n" this node
		format "current obj: %\n" $
	)
	on detachedFromNode node do
	(
		format "on detachedFromNode : % %\n" this node
		format "current obj: %\n" $
	)
	on deleted do 
	(
		format "on deleted : % \n" this
		format "current obj: %\n" $
	)
)