import _ExocortexAlembicPython as alembic
import sys
import argparse

class ABCException(Exception):
   def __init__(self, value):
      self.value = value
   def __str__(self):
      return repr(self.value)

concat_ts = []
xforms_dict = {}

def concat_time_samples(append_ts):
   global concat_ts
   last = concat_ts[-1] # pick the last time sample currently known
   for ap in append_ts:
      concat_ts.append(ap + last)   # the time stamps must be added at the end

def extract_ts_xforms(archive):
   global xforms_dict
   global concat_ts
   concat_ts = concat_ts + list(archive.getSampleTimes()[1])   #get the first TS
   nb_ts = len(concat_ts)
   for identifier in archive.getIdentifiers():
      obj = archive.getObject(identifier)
      o_type = obj.getType()
      if o_type.find("Xform") == -1:
         continue
      
      xkey = identifier
      xvalues = []
      xfm = obj.getProperty(".xform")
      
      if xfm == None:
         raise ABCException("no .xform in " + identifier)
      
      last_i = xfm.getNbStoredSamples()   # need to be sure that theirs one matrices per sampling
      for i in xrange(0, last_i):
         xvalues.append(xfm.getValues(i))
      
      if nb_ts != last_i:                 # duplicate the last matrix if necessary
         last_v = xfm.getValues(last_i-1)
         for i in xrange(last_i, nb_ts):
            xvalues.append(last_v)
      xforms_dict[xkey] = xvalues
   pass

def concat_ts_xforms(archive):
   global xforms_dict
   global concat_ts
   arch_ts = archive.getSampleTimes()[1]  # archive's TS
   concat_time_samples(arch_ts)# add the TS right at the end of the list
   nb_ts = len(arch_ts)
   nb_xforms = len(xforms_dict)           # need to be sure there's the same amount of Xforms in each file and that they're the same
   
   for identifier in archive.getIdentifiers():
      obj = archive.getObject(identifier)
      o_type = obj.getType()
      if o_type.find("Xform") == -1:
         continue
      
      xfm = obj.getProperty(".xform")
      xvalues = xforms_dict[identifier]   # get the values for this identifier and test if it's a valid one!
      if xvalues == None:
         raise ABCException(identifier + " doesn't exist in previous files")
      
      last_i = xfm.getNbStoredSamples()   # need to be sure that theirs one matrices per sampling
      for i in xrange(0, last_i):
         xvalues.append(xfm.getValues(i))
      
      if nb_ts != last_i:                 # duplicate the last matrix if necessary
         last_v = xfm.getValues(last_i-1)
         for i in xrange(last_i, nb_ts):
            xvalues.append(last_v)
      
      nb_xforms = nb_xforms - 1           # one more Xform done!
   
   if nb_xforms != 0:
      raise ABCException("The file is missing some xforms")
   pass


# copy directly a property and its corresponding values
def copy_property(prop, outProp, full_name):
   for i in xrange(0, prop.getNbStoredSamples()):
      vals = prop.getValues(i)
      outProp.setValues(vals)

# visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop, full_name):
   full_name = full_name + "/" + cprop.getName()
   for prop_name in cprop.getPropertyNames():
      sub_prop = cprop.getProperty(prop_name)
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, outCprop.getProperty(prop_name, sub_prop.getType()), full_name)
      else:
         copy_property(sub_prop, outCprop.getProperty(prop_name, sub_prop.getType()), full_name)

# similar to function "copy_objects" copyABC.py... only difference is that it replaces the Xforms with the ones accumulated
def create_output(out_arch, in_arch):
   global concat_ts
   global xforms_dict
   out_arch.createTimeSampling(concat_ts)
   
   for identifier in in_arch.getIdentifiers():
      obj = in_arch.getObject(identifier)
      o_type = obj.getType()
      out = out_arch.createObject(o_type, identifier)
      out.setMetaData(obj.getMetaData())
      
      if o_type.find("Xform") == -1:   # not an Xform, copy it integrally
         
         for prop_name in obj.getPropertyNames():
            prop = obj.getProperty(prop_name)
            if prop.isCompound():
               copy_compound_property(prop, out.getProperty(prop_name, prop.getType()), identifier)
            else:
               copy_property(prop, out.getProperty(prop_name, prop.getType()), identifier)
         
      else:                            # an Xform, use data collected above
         
         xfm = out.getProperty(".xform", "matrix4d")
         xvalues = xforms_dict[identifier]
         for xval in xvalues:
            xfm.setValues(xval)
         
   pass

def main(args):
   # parser args
   parser = argparse.ArgumentParser(description="Concatenate multiple alembic files only if their geometries are identical.")
   parser.add_argument("abc_files", metavar="{Alembic files}", type=str, nargs="+", help="alembic files to concatenate")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   ns = vars(parser.parse_args(args[1:]))
   
   if len(ns["abc_files"]) < 2:
      print("Error: need at least two files for the concatenation")
      return
   
   #read the first file and extract the time sampling and xforms
   in_arch = alembic.getIArchive(ns["abc_files"][0])
   print("\n\nScanning " + ns["abc_files"][0])
   extract_ts_xforms(in_arch)
   in_arch = None
   
   # go through the remaining files and accumulate the data about the xforms
   try:
      for abc in ns["abc_files"][1:]:
         print("Scanning " + abc)
         concat_ts_xforms(alembic.getIArchive(abc))
   except ABCException as abc_error:
      print("Error: not able to concatenate files")
      print("\t" + abc_error)
      return
   
   # create the concatenate archive
   print("Creating " + ns["o"])
   in_arch = alembic.getIArchive(ns["abc_files"][0])
   out_arch = alembic.getOArchive(ns["o"])
   create_output(out_arch, in_arch)
   in_arch  = None
   out_arch = None

if __name__ == "__main__":
   main(sys.argv)


