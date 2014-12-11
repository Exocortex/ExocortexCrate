import _ExocortexAlembicPython as alembic
import sys
import argparse


# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# create a new identifier name unique also for the file ID!
def merge_identifier_name(file_id, identifier):
   nb_slash = 0
   while identifier[nb_slash] == '/':
      nb_slash = nb_slash + 1
   return ('/'*nb_slash) + str(file_id) + "_" + identifier[nb_slash:]

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# classes to handle conflict resolution!

# use the first instance
class conflict_first:
   def __init__(self):
      self.ids = {}                             # { identifier => file_id }
   
   def add_identifier(self, identifier, file_id):
      if identifier not in self.ids.keys():     # first time this identifier is seen, add it!
         self.ids[identifier] = file_id
   
   def get_identifier(self, identifier, file_id):
      if identifier not in self.ids.keys():
         return None
      elif self.ids[identifier] == file_id:
         return identifier
      return None

class conflict_last:
   def __init__(self):
      self.ids = {}                             # { identifier => file_id }
   
   def add_identifier(self, identifier, file_id):
      self.ids[identifier] = file_id            # make sure the last file with this object is the one that will be used for the final merging
   
   def get_identifier(self, identifier, file_id):
      if identifier not in self.ids.keys():
         return None
      elif self.ids[identifier] == file_id:
         return identifier
      return None

class conflict_rename:
   def __init__(self):
      self.ids = {}                             # { identifier => { file_1 => "file1name", file_3 => "file3name" } }
   
   def add_identifier(self, identifier, file_id):
      if identifier not in self.ids.keys():     # new ID, just add it
         tmp = {}
         tmp[file_id] = identifier
         self.ids[identifier] = tmp
      else:
         tmp = self.ids[identifier]
         if len(tmp) == 1:
            f_id = tmp.keys()[0]                # only 1, need to rename it first to avoid the conflict
            tmp[f_id] = merge_identifier_name(f_id, tmp[f_id])
         tmp[file_id] = merge_identifier_name(file_id, identifier)   # add this file_id reference with the renamed name!
   
   def get_identifier(self, identifier, file_id):
      if identifier not in self.ids.keys():
         return None
      tmp = self.ids[identifier]
      if file_id not in tmp.keys():
         return None
      return tmp[file_id]

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# manage the time sampling amongst the different alembic files
class ts_manager:
   def __init__(self, conflict="first"):
      self.tsampling = [[0.0]]                  # all the time sampling in one list, initialize with the first one, always [0.0]
      self.dict = {}                            # dictonary to hold the various TS indices
      self.conflict = None
      if conflict == "first":
         self.conflict = conflict_first()
      elif conflict == "last":
         self.conflict = conflict_last()
      elif conflict == "rename":
         self.conflict = conflict_rename()
      
      if self.conflict == None:
         raise Exception("unknown conflict resolution")
   
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
      # scan the identifiers to resolve name conflicts!
      for identifier in in_archive.getIdentifiers():
         self.conflict.add_identifier(identifier, file_id)
   
   def get_time_sampling(self):
      return self.tsampling
   
   def get_file_indices(self, file_id):
      return self.dict[file_id]
   
   def use_this_object(self, file_id, identifier): # if this object is ready to be explored or not, if it is, it returns the new name it should have. If it's not, returns None
      return self.conflict.get_identifier(identifier, file_id)

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
def copy_objects(ts_manager, in_data, out_data, file_id, indices):
   for identifier in in_data.getIdentifiers():
      obj = in_data.getObject(identifier)
      obj_typ = obj.getType()
      
      new_id = ts_manager.use_this_object(file_id, identifier)       # is it the right file for this object!
      if new_id != None:
         ts_index = indices[obj.getTsIndex()]                           # get the real index in the merged file!
         out = out_data.createObject(obj_typ, new_id, ts_index)
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
   conflict_string = "how to deal with objects with the same name in different files. Option --first-- keeps the first instance found, --last-- keeps the last instance found, and --rename-- renames the object with the file ID (order in which they're specified as parameter) to be unique."
   
   # parser args
   parser = argparse.ArgumentParser(description="Merge multiple alembic files in one file.\n\nNote that to avoid any confusion, the objects' names are altered in the output file by adding the file ID (the number in which the file is specified as a parameter) at the beginning of the name.")
   parser.add_argument("abc_files", metavar="{Alembic files}", type=str, nargs="+", help="alembic file to merge")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   parser.add_argument("-c", choices=("first", "last", "rename"), help=conflict_string, default="first")
   ns = vars(parser.parse_args(args[1:]))
   
   abc_files = ns["abc_files"]
   if len(abc_files) < 2:
      print("Error: need at least two files to merge")
      return
   
   for abc in abc_files:
      if abc == ns["o"]:
         print("Error: the output filename must be distinct from all the input files")
         return
   
   # 1. scan the files
   ts_man = ts_manager(ns["c"])
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
      copy_objects(ts_man, alembic.getIArchive(abcf), out_data, idx, ts_man.get_file_indices(idx))
      idx = idx + 1
   print("\n\n")

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)

