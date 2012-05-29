import _ExocortexAlembicPython as alembic
import sys
import argparse

b_verbose = False

#copy directly a property and its corresponding values
def copy_property(prop, outProp, full_name):
   if b_verbose:
      print("PROP: \"" + full_name + "/" + prop.getName() + "\" of type " + prop.getType())
   for i in xrange(0, prop.getNbStoredSamples()):
      vals = prop.getValues(i)
      outProp.setValues(vals)

#visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop, full_name):
   full_name = full_name + "/" + cprop.getName()
   if b_verbose:
      print("COMP: \"" + full_name + "\"")
   for prop_name in cprop.getPropertyNames():
      sub_prop = cprop.getProperty(prop_name)
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, outCprop.getProperty(prop_name, sub_prop.getType()), full_name)
      else:
         copy_property(sub_prop, outCprop.getProperty(prop_name, sub_prop.getType()), full_name)

#going through each object
def copy_objects(in_data, out_data):
   for identifier in in_data.getIdentifiers():
      obj = in_data.getObject(identifier)
      if b_verbose:
         print("OBJ : \"" + identifier + "\" of type " + obj.getType())
      out = out_data.createObject(obj.getType(), identifier)
      out.setMetaData(obj.getMetaData())
      for prop_name in obj.getPropertyNames():
         prop = obj.getProperty(prop_name)
         if prop.isCompound():
            copy_compound_property(prop, out.getProperty(prop_name, prop.getType()), identifier)
         else:
            copy_property(prop, out.getProperty(prop_name, prop.getType()), identifier)

def main(args):
   global b_verbose
   
   # parser args
   parser = argparse.ArgumentParser(description="Copy integrally an alembic file into a second alembic file")
   parser.add_argument("abc_in", type=str, metavar="{Alembic input file}", help="input alembic file to be copied")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   parser.add_argument("-v", "--verbose", action='store_true', help='show the details of the copy')
   ns = vars(parser.parse_args(args[1:]))
   
   b_verbose = ns["verbose"]
   
   in_data = alembic.getIArchive(ns["abc_in"])
   out_data = alembic.getOArchive(ns["o"])
   out_data.createTimeSampling(in_data.getSampleTimes()[1]) #copy first the time sampling
   copy_objects(in_data, out_data)

if __name__ == "__main__":
   main(sys.argv)


