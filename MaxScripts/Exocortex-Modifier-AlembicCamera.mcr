plugin modifier AlembicCameraBaseModifier
	name:"Alembic Camera"
	classID:#(0x20730624, 0x3d650bdb)	-- keep in sync with AlembicNames.h
	category:"Alembic" 
	version:1
	extends:AlembicCameraBaseModifier
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
		--muted type:#boolean ui:muted animatable:true default:false
		
		on path set val do delegate.path = val
		on identifier set val do delegate.identifier = val
		on currentTimeHidden set val do delegate.currentTimeHidden = val
		on timeOffset set val do delegate.timeOffset = val
		on timeScale set val do delegate.timeScale = val		
		--on muted set val do delegate.muted = val
	)

	rollout params "Alembic"
	(
		edittext path "Path"
		edittext identifier "Identifier"
		spinner timeOffset "Time Offset" range:[-10000,10000,0]
		spinner timeScale "Time Scale" range:[-10000,10000,0]		
		-- checkbox muted "Muted"
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