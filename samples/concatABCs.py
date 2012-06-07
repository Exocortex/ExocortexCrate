import _ExocortexAlembicPython as alembic
import sys
import argparse

class ABCException(Exception):
   def __init__(self, value):
      self.value = value
   def __str__(self):
      return repr(self.value)

concat_ts = []          # keep the time samples over all alembic files
xforms_dict = {}        # keep the transformation matrix
vertex_def = {}       # identify vertex transform properties in all alembic files

# concat the time samples by adding add the end of the list and adding the 
def concat_time_samples(append_ts):
   global concat_ts
   last = concat_ts[-1] # pick the last time sample currently known
   for ap in append_ts:
      concat_ts.append(ap + last)   # the time stamps must be added at the end

# visit the object's properties and identify the vertex deforms one
def extract_vertex_deform_info(prop, fullname, nb_ts):
   global vertex_def
   if prop.isCompound():
      for sub_prop in prop.getPropertyNames():
         extract_vertex_deform_info(prop.getProperty(sub_prop), fullname + "/" + sub_prop, nb_ts)
   else:
      if fullname not in vertex_def.keys():
         vertex_def[fullname] = False
      if not vertex_def[fullname]:                                      # this property needs to be rechecked
         vertex_def[fullname] = (1 != prop.getNbStoredSamples())    # this property will need to be traited as a vertex transformation.

# get the information about the Xforms and identify which properties are vertex deforms
def extract_ts_xforms(archive):
   global xforms_dict
   global concat_ts
   concat_ts = concat_ts + list(archive.getSampleTimes()[1])   #get the first TS
   nb_ts = len(archive.getSampleTimes()[1])
   print("\t\tsample times: " + str(nb_ts))
   for identifier in archive.getIdentifiers():
      obj = archive.getObject(identifier)
      o_type = obj.getType()
      
      if o_type.find("Xform") > -1:    # is an Xform
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
         
      else:                            # still have to visit the object to identify vertex transform properties
         for prop in obj.getPropertyNames():
            extract_vertex_deform_info(obj.getProperty(prop), identifier + "/" + prop, nb_ts)

def concat_ts_xforms(archive):
   global xforms_dict
   global concat_ts
   arch_ts = archive.getSampleTimes()[1]  # archive's TS
   concat_time_samples(arch_ts)# add the TS right at the end of the list
   nb_ts = len(arch_ts)
   print("\t\tsample times: " + str(nb_ts))
   nb_xforms = len(xforms_dict)           # need to be sure there's the same amount of Xforms in each file and that they're the same
   
   for identifier in archive.getIdentifiers():
      obj = archive.getObject(identifier)
      o_type = obj.getType()
      if o_type.find("Xform") > -1:
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
         
      else:                            # still have to visit the object to identify vertex transform properties
         for prop in obj.getPropertyNames():
            extract_vertex_deform_info(obj.getProperty(prop), identifier + "/" + prop, nb_ts)
   
   if nb_xforms != 0:
      raise ABCException("The file is missing some xforms")
   pass

# copy directly a property and its corresponding values
def copy_property(prop, outProp, full_name, firstFile):
   global vertex_def
   full_name = full_name + "/" + prop.getName()
   
   # if it's the first file, then needs to copy it... or if it's a vertex deform, needs to append the values
   if firstFile or vertex_def[full_name]:
      for i in xrange(0, prop.getNbStoredSamples()):
         outProp.setValues(prop.getValues(i))

# visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop, full_name, firstFile):
   full_name = full_name + "/" + cprop.getName()
   for prop_name in cprop.getPropertyNames():
      if prop_name == ".metadata":  # cause some problems, removed them
         continue
      sub_prop = cprop.getProperty(prop_name)
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, outCprop.getProperty(prop_name, sub_prop.getType()), full_name, firstFile)
      else:
         copy_property(sub_prop, outCprop.getProperty(prop_name, sub_prop.getType()), full_name, firstFile)

# similar to function "copy_objects" copyABC.py... only difference is that it replaces the Xforms with the ones accumulated
def create_output(out_arch, in_arch, firstFile):
   global concat_ts
   global xforms_dict
   
   nb_ts = len(in_arch.getSampleTimes())  # get the number of sample times
   if firstFile:
      out_arch.createTimeSampling(concat_ts) # assign the time sampling only once to the output archive
   
   for identifier in in_arch.getIdentifiers():
      obj = in_arch.getObject(identifier)
      o_type = obj.getType()
      out = out_arch.createObject(o_type, identifier)
      if firstFile:
         out.setMetaData(obj.getMetaData())
      
      if o_type.find("Xform") == -1:   # not an Xform, copy it integrally
         for prop_name in obj.getPropertyNames():
            if prop_name == ".metadata":  # cause some problems, removed them
               continue
            prop = obj.getProperty(prop_name)
            if prop.isCompound():
               copy_compound_property(prop, out.getProperty(prop_name, prop.getType()), identifier, firstFile)
            else:
               copy_property(prop, out.getProperty(prop_name, prop.getType()), identifier, firstFile)
         
      elif firstFile:                            # an Xform, use data collected above, but only for the first file, no need to repeat those
         xfm = out.getProperty(".xform", "matrix4d")
         xvalues = xforms_dict[identifier]
         for xval in xvalues:
            xfm.setValues(xval)
         

def main(args):
   global concat_ts
   global vertex_def
   # parser args
   parser = argparse.ArgumentParser(description="Concatenate multiple alembic files only if their geometries are identical.\n\nCurrently, it only supports Xforms. Issue #16 on GitHub is about adding vertex/normal deformations.")
   parser.add_argument("abc_files", metavar="{Alembic files}", type=str, nargs="+", help="alembic files to concatenate")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   ns = vars(parser.parse_args(args[1:]))
   
   if len(ns["abc_files"]) < 2:
      print("Error: need at least two files for the concatenation")
      return
   
   #read the first file and extract the time sampling, xforms and identifying vertex deforms
   abc_files = ns["abc_files"]
   in_arch = alembic.getIArchive(abc_files[0])
   print("\n\nScanning " + abc_files[0])
   extract_ts_xforms(in_arch)
   in_arch = None
   
   # go through the remaining files and accumulate the data about the time sampling, xforms and identifying vertex deforms
   try:
      for abc in abc_files[1:]:
         print("Scanning " + abc)
         concat_ts_xforms(alembic.getIArchive(abc))
   except ABCException as abc_error:
      print("Error: not able to concatenate files")
      print("\t" + abc_error)
      return
   
   # create the concatenate archive
   print("\nCreating " + ns["o"])
   print("Archive with " + str(len(concat_ts)) + " time sampling")
   out_arch = alembic.getOArchive(ns["o"])
   
   firstFile = True
   for abc in abc_files:
      print("\t>> " + abc)
      in_arch = alembic.getIArchive(abc)
      create_output(out_arch, in_arch, firstFile)
      firstFile = False # not the first file anymore
      in_arch = None    # better close it
   
   out_arch = None   # finally close the output archive
   print("\n\n")

if __name__ == "__main__":
   main(sys.argv)


