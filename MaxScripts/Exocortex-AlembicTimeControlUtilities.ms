
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
	print obj
	if obj.time.isAnimated then
	(
		obj.time.controller = float_expression()
		nCtrl = obj.time.controller
		nCtrl.AddScalarTarget "current" timeCtrl.time.controller
		nCtrl.setExpression "current"
		
		print "connected"
	)
	else
	(
		print "not animated"
	)
)

fn connectTimeControlToObjects timeCtrl = (

	if selection.count == 0 then print "Cannot connect time control to objects. Selection is empty."
	
	if timeCtrl != undefined then (
		for i = 1 to selection.count do
		(
			n = selection[i]

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
	)
	else (
		print "Cannot connect to undefined time control."
	)
	
)

tc = findTimeControl()
connectTimeControlToObjects(tc)