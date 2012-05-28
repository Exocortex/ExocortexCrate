import _ExocortexAlembicPython as alembic
import imp
import sys

#import _ExocortexAlembicPython as alembic
alembic = imp.load_dynamic("_ExocortexAlembicPython", "/home/BuildUser/Work/ExocortexAlembicShared/install/Linux/Python26/_ExocortexAlembicPython.so")

def copy_property(prop, outProp):
   print("Copy property " + prop.getName() + " of type " + prop.getType() + " with " + str(prop.getNbStoredSamples()) + " stored samples")
   print("--- nature of the out property: " + str(type(outProp)) + " and its name is " + outProp.getName())
   for i in xrange(0, prop.getNbStoredSamples()):
      outProp.setValues(prop.getValues(i))

def copy_compound_property(cprop, outCprop):
   #print("Copy compound property " + cprop.getName())
   for prop_name in cprop.getPropertyNames():
      sub_prop = cprop.getProperty(prop_name)
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, outCprop.getProperty(prop_name, sub_prop.getType()))
      else:
         copy_property(sub_prop, outCprop.getProperty(prop_name, sub_prop.getType()))
   pass

def start_copy(in_data, out_data):
   out_data.createTimeSampling(in_data.getSampleTimes()[1])
   #out_data.createTimeSampling( [1] )
   for identifier in in_data.getIdentifiers():
      obj = in_data.getObject(identifier)
      print("Copy object " + identifier + " of type " + obj.getType())
      out = out_data.createObject(obj.getType(), identifier)
      for prop_name in obj.getPropertyNames():
         prop = obj.getProperty(prop_name)
         if prop.isCompound():
            copy_compound_property(prop, out.getProperty(prop_name, prop.getType()))
         else:
            copy_property(prop, out.getProperty(prop_name, prop.getType()))

def main(args):
   if len(args) == 1:
      print("Error, no input:\n\t" + args[0] + " [alembic_file.abc]")
      return
   else:
      in_data = alembic.getIArchive(args[1])
      out_data = alembic.getOArchive(args[1] + ".output.abc")
      start_copy(in_data, out_data)

if __name__ == "__main__":
   main(sys.argv)

