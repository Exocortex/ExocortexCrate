import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
def create_new_TS(tsampling, scale, offset):
   new_ts = []
   for sampling in tsampling:
      sub_ts = []
      K = sampling[0] * (1.0 - scale) + offset
      for s in sampling:
         sub_ts.append(s*scale + K)
      new_ts.append(sub_ts)
   return new_ts

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
      sub_prop = cprop.getProperty(prop_name)
      out_prop = outCprop.getProperty(prop_name, sub_prop.getType())
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, out_prop)
      else:
         copy_property(sub_prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# going through each object
def copy_objects(in_data, out_data):
   for identifier in in_data.getIdentifiers():
      obj = in_data.getObject(identifier)
      obj_typ = obj.getType()
      
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
   # parser args
   parser = argparse.ArgumentParser(description="Copy an alembic file into a new one by changing it's time sampling.\n\nNote that this program doesn't validate the time sampling. It's possible to create time samplings with negative values.")
   parser.add_argument("abc_in", type=str,   metavar="{Alembic input file}",  help="input alembic file to be copied and retimed")
   parser.add_argument("-o",     type=str,   metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   parser.add_argument("-s",     type=float, metavar="{scaling factor}",      help="scaling factor, default is 1.0",                  default=1.0)
   parser.add_argument("-a",     type=float, metavar="{offset}",              help="offset for the time sampling, default is 0.0",    default=0.0)
   ns = vars(parser.parse_args(args[1:]))
   
   if ns["abc_in"] == ns["o"]:
      print("Error: input and output filenames must be different")
      return
   
   scale  = ns["s"]
   offset = ns["a"]
   
   if scale <= 0.0:
      print("Error: the scale factor cannot be less or equal to zero")
      return
   
   in_data  = alembic.getIArchive(ns["abc_in"])
   out_data = alembic.getOArchive(ns["o"])
   out_data.createTimeSampling(create_new_TS(in_data.getSampleTimes(), scale, offset))
   copy_objects(in_data, out_data)
   
   print("\n\n")

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)



