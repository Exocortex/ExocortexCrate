import imp
import sys

#import _ExocortexAlembicPython as alembic
alembic = imp.load_dynamic("_ExocortexAlembicPython", "/home/BuildUser/Work/ExocortexAlembicShared/install/Linux/Python26/_ExocortexAlembicPython.so")


archive = alembic.getOArchive(sys.argv[1])
archive.createTimeSampling([0,1])
xform = archive.createObject("AbcGeom_Xform_2", "/xform")
mesh = archive.createObject("AbcGeom_PolyMesh_2", "/xform/mesh")
compound = mesh.getProperty("comp1", "compound")
compound = compound.getProperty("comp1", "compound")
compound.getProperty("comp1", "vector3farray")

xform = None
mesh = None
archive = None

archive = alembic.getIArchive(sys.argv[1])
for id in archive.getIdentifiers():
	obj = archive.getObject(id)
	print obj.getIdentifier()
	for compoundName in obj.getPropertyNames():
		compound = obj.getProperty(compoundName)
		if compound.isCompound():
			print "compound: "+compound.getName()
			for propName in compound.getPropertyNames():
				prop = compound.getProperty(propName)
				print "--> "+prop.getName()
				if prop.isCompound():
					for name in prop.getPropertyNames():
						prop2 = prop.getProperty(name)
						print "----> "+prop2.getName()
				
		else:
			print compound.getName()
		
