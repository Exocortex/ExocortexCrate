plugin modifier AlembicMeshModifier
	name:"Alembic Mesh Modifier"
	classID:#(0x20730623, 0x5d650bdb)
	category:"Alembic" 
	version:1
	invisible:false
(
	parameters pblock rollout:params
	(
		fileName type:#string ui:fileName default:"filename"
		dataPath type:#string ui:dataPath default:"path"
		currentTimeHidden type:#float default:0
		timeOffset type:#float ui:timeOffset animatable:true default:0
		timeScale type:#float ui:timeScale animatable:true default:0
		faceSet type:#boolean ui:faceSet default:true
		vertices type:#boolean ui:vertices default:true
		normals type:#boolean ui:normals default:true
		uvs type:#boolean ui:uvs default:true
		clusters type:#boolean ui:clusters default:true
		mute type:#boolean ui:mute animatable:true default:false
	)

	local update_currentTimeHidden_registered = false
	
	fn update_currentTimeHidden =
	(
		currentTimeHidden = currentTime / frameRate
	)
		
	on create do
	(
		format "on create: %\n" this
		format "  current obj: %\n" $
	)

	on postCreate do 
	(
		format "on postCreate : %\n" this
		format "  current obj: %\n" $
		applied = isGameFlowModifierAlreadyApplied $
		format "  already applied: %\n" applied
	)

	on load do
	(
		format "on load : %\n" this
		format "current obj: %\n" $
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
		
		if( not update_currentTimeHidden_registered ) do
		(
			currentTimeHidden = currentTime / frameRate
			registerTimeCallback update_currentTimeHidden
			update_currentTimeHidden_registered = true
		)
		
		dataTime = timeOffset + currentTimeHidden  * timeScale
		tempMesh = mesh
		if( not mute ) do
		(
			tempMesh = ExocortexAlembic.importMesh tempMesh fileName dataPath dataTime faceSet vertices normals uvs clusters
		)
		mesh = tempMesh;
		
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