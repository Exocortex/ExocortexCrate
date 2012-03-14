plugin modifier AlembicMeshModifier
	name:"Alembic Mesh Modifier"
	classID:#(0x20730623, 0x5d650bdb)	-- keep in sync with AlembicNames.h
	category:"Alembic" 
	version:1
	extends:AlembicMeshBaseModifier
	replaceUI:true
	invisible:false
(
		parameters main rollout:params
	(
		path type:#string ui:path default:""
		identifier type:#string ui:identifier default:""
		time type:#float ui:time animatable:true default:0
		topology type:#boolean ui:topology default:true
		geometry type:#boolean ui:geometry default:true
		normals type:#boolean ui:normals default:true
		uvs type:#boolean ui:uvs default:true
		muted type:#boolean ui:muted animatable:true default:false
	
		on path set val do delegate.path = val
		on identifier set val do delegate.identifier = val
		on time set val do delegate.time = val
		on topology set val do delegate.topology = val
		on geometry set val do delegate.geometry = val
		on normals set val do delegate.normals = val
		on uvs set val do delegate.uvs = val
		on muted set val do delegate.muted = val
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
)