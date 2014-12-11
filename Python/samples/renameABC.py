import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# global variables
str_search  = None
str_replace = None

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# copy directly a property and its corresponding values
def copy_property(prop, outProp):
   for i in xrange(0, prop.getNbStoredSamples()):
      outProp.setValues(prop.getValues(i))

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop):
   for prop_name in cprop.getPropertyNames():
      if prop_name == ".metadata":
         continue                                                    # .metadata cause some problem
      sub_prop = cprop.getProperty(prop_name)
      out_prop = outCprop.getProperty(prop_name, sub_prop.getType())
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, out_prop)
      else:
         copy_property(sub_prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# the renaming function
def rename_id(identifier):
   global str_search
   global str_replace
   
   return identifier.replace(str_search, str_replace)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# going through each object
def copy_objects(in_data, out_data):
   for identifier in in_data.getIdentifiers():
      obj = in_data.getObject(identifier)
      obj_typ = obj.getType()
      
      identifier = rename_id(identifier)
      
      out = out_data.createObject(obj_typ, identifier, obj.getTsIndex())
      out.setMetaData(obj.getMetaData())
      for prop_name in obj.getPropertyNames():
         if prop_name == ".metadata":
            continue                                                 # .metadata cause some problem
         prop = obj.getProperty(prop_name)
         out_prop = out.getProperty(prop_name, prop.getType())
         if prop.isCompound():
            copy_compound_property(prop, out_prop)
         else:
            copy_property(prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
def main(args):
   global str_search
   global str_replace
   
   # parser args
   parser = argparse.ArgumentParser(description="Copy an alembic file into a second alembic file with the option to change to rename objects.")
   parser.add_argument("abc_in", type=str, metavar="{Alembic input file}", help="input alembic file to be copied")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   parser.add_argument("-s", type=str, metavar="{search}", help="the substring to search in the object names", required=True)
   parser.add_argument("-r", type=str, metavar="{replace}", help="the substring to replace the search substring with in the object names", required=True)
   ns = vars(parser.parse_args(args[1:]))
   
   if ns["abc_in"] == ns["o"]:
      print("Error: input and output filenames must be different")
      return
   str_search  = ns["s"]
   str_replace = ns["r"]
   
   in_data = alembic.getIArchive(ns["abc_in"])
   out_data = alembic.getOArchive(ns["o"])
   out_data.createTimeSampling(in_data.getSampleTimes())
   copy_objects(in_data, out_data)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)


