
fn findTimeControl = (
	if selection.count == 0 then print "Cannot find time control. Selection is empty."
	
	for i = 1 to selection.count do
	(
		if classof selection[i] == AlembicTimeControl then
		(
			return selection[i]
		)
	)
)

fn connectTimeControl obj timeCtrl = (
	--print obj
	if obj.time.isAnimated then
	(
		obj.time.controller = float_expression()
		local nCtrl = obj.time.controller
		nCtrl.AddScalarTarget "current" timeCtrl.time.controller
		nCtrl.setExpression "current"
		
		--print "connected"
	)
	else
	(
		--print "not animated"
	)
 )

fn createTimeControl name = (
	cntl = AlembicTimeControl name: name
)

fn connectTimeControlToObjects timeCtrl = (

	if selection.count == 0 then print "Cannot connect time control to objects. Selection is empty."

	for i = 1 to selection.count do
	(
		local n = selection[i]
		
		if classof n == Dummy then
		(
			if n.transform.isAnimated and \
				classof n.transform.controller == Alembic_Xform then
			(	
				connectTimeControl n.transform.controller timeCtrl
			)
		)

		if classof n == PolyMeshObject or \
		classof n == Freecamera or \
		classof n == Alembic_Particles or \
		classof n == NURBSCurveshape or \
		classof n == SplineShape then
		(
		
			if n.transform.isAnimated and \
				classof n.transform.controller == Alembic_Xform then
			(	
				connectTimeControl n.transform.controller timeCtrl
			)
			
			if n.visibility.isAnimated and \
				classof n.visibility.controller == Alembic_Visibility then 
			(
				connectTimeControl n.visibility.controller timeCtrl
			)
		)

		if classof n == PolyMeshObject then
		(
		
			
			for j = 1 to n.modifiers.count do
			(
				modj = n.modifiers[j]
				case (classof modj) of
				(
					Alembic_Mesh_Normals: 
					(
						connectTimeControl modj timeCtrl
					)
					Alembic_Mesh_Geometry: 
					(
						connectTimeControl modj timeCtrl
					)
					Alembic_Mesh_UVW: 
					(
						connectTimeControl modj timeCtrl
					)
					Alembic_Mesh_Topology:
					(
						connectTimeControl modj timeCtrl
					)
				)
			)
		
		)
		else if classof n == Freecamera then
		(
			--connectTimeControl n.FOV timeCtrl
			--connectTimeControl n.MultiPass_Effect.focalDepth timeCtrl
			--connectTimeControl n.nearclip timeCtrl
			--connectTimeControl n.farclip timeCtrl
			
			--local camMod = n.modifiers["Alembic Camera Properties"] 
			--if camMod != undefined then
			--(
			--	connectTimeControl camMod.hfov timeCtrl
			--	connectTimeControl camMod.vfov timeCtrl
			--	connectTimeControl camMod.focallength timeCtrl
			--	connectTimeControl camMod.haperature timeCtrl
			--	connectTimeControl camMod.vaperature timeCtrl
			--	connectTimeControl camMod.hfilmoffset timeCtrl
			--	connectTimeControl camMod.vfilmoffset timeCtrl
			--	connectTimeControl camMod.lsratio timeCtrl
			--	connectTimeControl camMod.loverscan timeCtrl
			--	connectTimeControl camMod.roverscan timeCtrl
			--	connectTimeControl camMod.toverscan timeCtrl
			--	connectTimeControl camMod.boverscan timeCtrl
			--	connectTimeControl camMod.fstop timeCtrl
			--	connectTimeControl camMod.focusdistance timeCtrl
			--	connectTimeControl camMod.shutteropen timeCtrl
			--	connectTimeControl camMod.shutterclose timeCtrl
			--	connectTimeControl camMod.nclippingplane timeCtrl
			--	connectTimeControl camMod.fclippingplane timeCtrl
			--)
		)
		else if classof n == Alembic_Particles then
		(
			connectTimeControl n timeCtrl
		)
		else if classof n == NURBSCurveshape then
		(
			for j = 1 to n.modifiers.count do
			(
				modj = n.modifiers[j]
				case (classof modj) of
				(
					Alembic_NURBS: 
					(
						connectTimeControl modj timeCtrl
					)
				)
			)
		)
		else if classof n == SplineShape then
		(
			for j = 1 to n.modifiers.count do
			(
				modj = n.modifiers[j]
				case (classof modj) of
				(
					Alembic_Spline_Geometry: 
					(
						connectTimeControl modj timeCtrl
					)
					Alembic_Spline_Topology: 
					(
						connectTimeControl modj timeCtrl
					)
				)
			)
		)
	)

)



macroScript NewTimeControlUI
    category:"Alembic" 
    buttonText:"Create Time Control"
    tooltip:"Create Time Control"
(
    on execute do  
    (
         AlembicTimeControl()
    )
)

macroScript NewTimeControlForSelectionUI
    category:"Alembic" 
    buttonText:"New Time Control For Selection"
    tooltip:"New Time Control For Selection"
(
    on execute do  
    (
    	local timeCtrl = AlembicTimeControl()
        connectTimeControlToObjects(timeCtrl)
    )
)

macroScript SetTimeControlForSelectionUI
    category:"Alembic" 
    buttonText:"Set Time Control For Selection"
    tooltip:"Set Time Control For Selection"
(
    on execute do  
    (
    	local timeCtrl = findTimeControl()
    	if timeCtrl != undefined then
    	(
    		connectTimeControlToObjects(timeCtrl)
    	)
    	else
    	(
    		print "Could not find time control in selection."
    	)
    )
)