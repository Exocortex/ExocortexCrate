import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
def create_new_TS(tsampling, scale, offset):
   new_tss = []
   for sampling in tsampling:
      ts_type = sampling.getType()
      samples = sampling.getTimeSamples()

      if ts_type == "uniform":
         samples = samples[0] + offset
      else:
         tmp_ts = []
         K = samples[0] * (1.0 - scale) + offset
         for s in samples:
            tmp_ts.append( s*scale + K )
         samples = tmp_ts

      new_tss.append( alembic.createTimeSampling(ts_type, samples) )
   return new_tss

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#copy directly a property and its corresponding values
def copy_property(prop, outProp):
   for i in xrange(0, prop.getNbStoredSamples()):
      vals = prop.getValues(i)
      outProp.setValues(vals)

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
   ori_ts = in_data.getSampleTimes()
   new_ts = create_new_TS(ori_ts, scale, offset)
   out_data.createTimeSampling(new_ts)
   copy_objects(in_data, out_data)
   
   print("\n\n")

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)



