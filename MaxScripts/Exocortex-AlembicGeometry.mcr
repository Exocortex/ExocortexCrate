plugin simpleObject AlembicMesh
	name:"Alembic Mesh"
	category:"Exocortex"
	classID:#(0xe855567c, 0xbcd73b8b)
(
	
	parameters main rollout:params
	(
		fileName type:#string ui:fileName default:"filename"
		dataPath type:#string ui:dataPath default:"path"
		dataTime type:#float ui:dataTime default:0
		normals type:#boolean ui:normals default:true
		uvs type:#boolean ui:uvs default:true
		clusters type:#boolean ui:clusters default:true
	)

	rollout params "Alembic"
	(
		edittext fileName "File Name"
		edittext dataPath "Data Path"
		spinner dataTime "Time" range:[-1000,1000,0]
		checkbox normals "Normals"
		checkbox uvs "UVs"
		checkbox clusters "Clusters"
	)

	on buildMesh do
	(
		print "This is only a test, this is not meant to be functional at this point."
		mesh = ExocortexAlembic.importMesh fileName dataPath dataTime normals uvs clusters		
	)

	tool create
	(
		on mousePoint click do
		(
			(#stop)
		)
	)
)