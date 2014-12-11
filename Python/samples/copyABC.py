import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# global variables

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#copy directly a property and its corresponding values
def copy_property(prop, outProp):
   for i in xrange(0, prop.getNbStoredSamples()):
      outProp.setValues(prop.getValues(i))

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop):
   for prop_name in cprop.getPropertyNames():
      if prop_name == ".metadata":
         continue                                                    # .metadata cause some problem

      if prop.isCompound():
         out_prop = outCprop.getProperty(prop_name, prop.getType())
         copy_compound_property(prop, out_prop, out_data)
      else:
         curTS = prop.getSampleTimes()

         out_prop = None
         if len(curTS.getTimeSamples()) == 0:
            out_prop = outCprop.getProperty(prop_name, prop.getType())
         else:
            tsSampling = out_data.createTimeSampling([curTS])
            out_prop = outCprop.getProperty(prop_name, prop.getType(), tsSampling[0])

         copy_property(prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# going through each object
def copy_objects(in_data, out_data):
   for identifier in in_data.getIdentifiers():
      obj = in_data.getObject(identifier)
      obj_typ = obj.getType()

      curTS = obj.getSampleTimes() 
      out = None
      if len(curTS.getTimeSamples()) == 0:
         out = out_data.createObject(obj_typ, identifier)
      else:
         tsSampling = out_data.createTimeSampling([curTS])
         out = out_data.createObject(obj_typ, identifier, tsSampling[0])      

      out.setMetaData(obj.getMetaData())
      for prop_name in obj.getPropertyNames():
         if prop_name == ".metadata":
            continue                                              # .metadata cause some problem

         prop = obj.getProperty(prop_name)

         if prop.isCompound():
            out_prop = out.getProperty(prop_name, prop.getType())
            copy_compound_property(prop, out_prop, out_data)
         else:
            curTS = prop.getSampleTimes()

            out_prop = None
            if len(curTS.getTimeSamples()) == 0:
               out_prop = out.getProperty(prop_name, prop.getType())
            else:
               tsSampling = out_data.createTimeSampling([curTS])
               out_prop = out.getProperty(prop_name, prop.getType(), tsSampling[0])

            copy_property(prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
def main(args):
   # parser args
   parser = argparse.ArgumentParser(description="Copy integrally an alembic file into a second alembic file")
   parser.add_argument("abc_in", type=str, metavar="{Alembic input file}", help="input alembic file to be copied")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   ns = vars(parser.parse_args(args[1:]))
   
   if ns["abc_in"] == ns["o"]:
      print("Error: input and output filenames must be different")
      return
   
   in_data = alembic.getIArchive(ns["abc_in"])
   out_data = alembic.getOArchive(ns["o"])
   out_data.createTimeSampling(in_data.getSampleTimes())
   copy_objects(in_data, out_data)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)


