plugin simpleObject AlembicMesh
	name:"Alembic Mesh"
	category:"Exocortex"
	classID:#(0xe855567c, 0xbcd73b8b)
(
	
	parameters main rollout:params
	(
		fileName type:#string ui:fileName default:"filename"
		dataPath type:#string ui:dataPath default:"path"
		normals type:#boolean ui:normals default:true
		uvs type:#boolean ui:uvs default:true
		clusters type:#boolean ui:clusters default:true
	)

	rollout params "Alembic"
	(
		edittext fileName "File Name"
		edittext dataPath "Data Path"
		checkbox normals "Normals"
		checkbox uvs "UVs"
		checkbox clusters "Clusters"
	)

	on buildMesh do
	(
		mesh = ExocortexAlembic.importMesh fileName dataPath 0 normals uvs clusters		
	)

)