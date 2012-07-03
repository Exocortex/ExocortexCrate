import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# manage the time sampling amongst the different alembic files
class ts_manager:
   def __init__(self):
      self.tsampling = [[0.0]]                  # all the time sampling in one list, initialize with the first one, always [0.0]
      self.dict = {}                            # dictonary to hold the various TS indices
   
   def scan_alembic(self, in_archive, file_id):
      stimes = in_archive.getSampleTimes()[1:]  # remove the first one, always [0.0], anyway!
      
      # avoid TS duplicate, because Alembic won't allow them to exist and the exact index is required!
      indices = [0]
      g_idx = len(self.tsampling)
      for ts in stimes:
         idx = 0
         for test in self.tsampling:
            if ts == test:
               break
            idx = idx + 1
         
         if idx == len(self.tsampling):         # new index
            self.tsampling = self.tsampling + [ts]
            indices = indices + [g_idx]
            g_idx = g_idx + 1
         else:                                  # reuse this index 
            indices = indices + [idx]
      
      self.dict[file_id] = indices              # associate the ts indices of the file with the real ts indices of the merged file
   
   def get_time_sampling(self):
      return self.tsampling
   
   def get_file_indices(self, file_id):
      return self.dict[file_id]

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# create a new identifier name unique also for the file ID!
def merge_identifier_name(file_id, identifier):
   nb_slash = 0
   while identifier[nb_slash] == '/':
      nb_slash = nb_slash + 1
   return ('/'*nb_slash) + str(file_id) + "_" + identifier[nb_slash:]

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# copy directly a property and its corresponding values
def copy_property(prop, outProp):
   for i in xrange(0, prop.getNbStoredSamples()):
      outProp.setValues(prop.getValues(i))

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop):
   for prop_name in cprop.getPropertyNames():
      if prop_name == ".metadata":                                   # .metadata cause some problem
         continue
      sub_prop = cprop.getProperty(prop_name)
      out_prop = outCprop.getProperty(prop_name, sub_prop.getType())
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, out_prop)
      else:
         copy_property(sub_prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# going through each object
def copy_objects(in_data, out_data, file_id, indices):
   for identifier in in_data.getIdentifiers():
      obj = in_data.getObject(identifier)
      obj_typ = obj.getType()
      
      identifier = merge_identifier_name(file_id, identifier)        # change the object identifier
      ts_index = indices[obj.getTsIndex()]                           # get the real index in the merged file!
      out = out_data.createObject(obj_typ, identifier, ts_index)
      out.setMetaData(obj.getMetaData())
      
      for prop_name in obj.getPropertyNames():
         if prop_name == ".metadata":                                # .metadata cause some problem
            continue
         prop = obj.getProperty(prop_name)
         out_prop = out.getProperty(prop_name, prop.getType())
         if prop.isCompound():
            copy_compound_property(prop, out_prop)
         else:
            copy_property(prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
def main(args):
   # parser args
   parser = argparse.ArgumentParser(description="Merge multiple alembic files in one file.\n\nNote that to avoid any confusion, the objects' names are altered in the output file by adding the file ID (the number in which the file is specified as a parameter) at the beginning of the name.")
   parser.add_argument("abc_files", metavar="{Alembic files}", type=str, nargs="+", help="alembic file to merge")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   ns = vars(parser.parse_args(args[1:]))
   
   abc_files = ns["abc_files"]
   if len(abc_files) < 2:
      print("Error: need at least two files to merge")
      return
   
   # 1. scan the files
   ts_man = ts_manager()
   idx = 0
   for abcf in abc_files:
      ts_man.scan_alembic(alembic.getIArchive(abcf), idx)
      idx = idx + 1
   
   # 2. create output archive and assign the time sampling
   out_data = alembic.getOArchive(ns["o"])
   out_data.createTimeSampling(ts_man.get_time_sampling())
   
   # 3. copy the object in the output file
   idx = 0
   for abcf in abc_files:
      copy_objects(alembic.getIArchive(abcf), out_data, idx, ts_man.get_file_indices(idx))
      idx = idx + 1
   print("\n\n")

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)

