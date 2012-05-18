#import _ExocortexAlembicPython as alembic
import imp
import sys


alembic = imp.load_dynamic("_ExocortexAlembicPython", "/home/jcaron/Work/ExocortexAlembicShared/install/Linux/Python26/_ExocortexAlembicPython.so")

#car = ap.getIArchive("/home/jcaron/alembic_data/car.abc")
#car_obj = car.getObject('/AI20_011_15Xfo/AI20_011_15')
#car_obj_uv = car_obj.getProperty('uv')
#car_obj_uv_vals = car_obj_uv.getProperty('.vals')

def visit_prop(prop, depth):
   #print((depth*"  ") + "PROP: " + prop.getName() + ", " + prop.getType())
   if prop.isCompound():
      print((depth*"  ") + "COMP: " + prop.getName())
      for sub_prop in prop.getPropertyNames():
         visit_prop(prop.getProperty(sub_prop), depth+1)
   else:
      print((depth*"  ") + "PROP: " + prop.getName() + ", " + prop.getType() + ", " + str(type(prop)))
   #pass

def visit_object(obj):
   for prop in obj.getPropertyNames():
      visit_prop(obj.getProperty(prop), 1)
   pass

def visit_alembic(abc_archive):
   print("Time sampling: " + str(abc_archive.getSampleTimes()))
   for id in abc_archive.getIdentifiers():
      obj = abc_archive.getObject(id)
      print("OBJ: " + id + ", " + obj.getType())
      print("-- meta: " + str(obj.getMetaData()))
      visit_object(obj)
   pass

def main(args):
   if len(args) == 1:
      print("Error, no input:\n\t" + args[0] + " [alembic_file.abc]")
      return
   else:
      abc_archive = alembic.getIArchive(args[1])
      print("\n\n\nScanning: " + args[1])
      visit_alembic(abc_archive)

if __name__ == "__main__":
   main(sys.argv)
