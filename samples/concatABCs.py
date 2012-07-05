import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# global variables
upper_ts = [0]                                                       # keep the maximum TS value in each file, initialized with [0],
                                                                     # because no need to add on the first file, just need to add it to recreate the right one!
                                                                     # the last value is a dummy value!

xforms_dict = {}                                                     # keep the transformation matrix
vertex_def  = {}                                                     # identify vertex transform properties in all alembic files

vertex_tss = {}                                                      # keep all the TS index used by the objects amongst the different input files
all_tss = []                                                         # keep all TS to access them later!
final_tss = []                                                       # the final TS once concatenate
tss_indices = {}                                                     # associate the TS indices tuple with the final TS index

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# extract maximum TS values
def extract_max_TS(sampleTimes):
   global upper_ts
   f_max = 0.0
   start_0 = False
   for st in sampleTimes:
      if st[0] == 0.0:
         start_0 = True
      if st[-1] > f_max:
         f_max = st[-1]
   
   if start_0:
      f_max = f_max + (1.0/24.0)                                     # if at least one of the time samples starts at time zero, need an extra padding!
   upper_ts.append(f_max)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# visit the object's properties and identify the vertex deforms one
def extract_vertex_deform_info(prop, fullname, nb_ts):
   global vertex_def
   if prop.isCompound():
      for sub_prop in prop.getPropertyNames():
         extract_vertex_deform_info(prop.getProperty(sub_prop), fullname + "/" + sub_prop, nb_ts)
   else:
      if fullname not in vertex_def.keys():                          # fullname not registered yet in vertex_def
         vertex_def[fullname] = False
      if not vertex_def[fullname]:                                   # this property needs to be rechecked
         vertex_def[fullname] = (1 != prop.getNbStoredSamples())     # this property will need to be traited as a vertex transformation.

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# get the information about the Xforms and identify which properties are vertex deforms
def extract_ts_xforms(archive):
   global xforms_dict
   global vertex_tss
   global all_tss
   
   sampleTimes = archive.getSampleTimes()
   all_tss.append(sampleTimes)
   extract_max_TS(sampleTimes)

   for identifier in archive.getIdentifiers():
      obj = archive.getObject(identifier)
      o_type = obj.getType()
      
      vertex_tss[identifier] = [obj.getTsIndex()]                    # get the TS index of this object (first file)
      nb_ts = len(sampleTimes[obj.getTsIndex()])
      
      if o_type.find("Xform") > -1:                                  # is an Xform
         xkey = identifier
         xvalues = []
         xfm = obj.getProperty(".xform")
         
         if xfm == None:
            raise Exception("no .xform in " + identifier)
         
         last_i = xfm.getNbStoredSamples()                           # need to be sure that theirs one matrices per sampling
         for i in xrange(0, last_i):
            xvalues.append(xfm.getValues(i))
         
         if nb_ts != last_i:                                         # duplicate the last matrix if necessary
            last_v = xfm.getValues(last_i-1)
            for i in xrange(last_i, nb_ts):
               xvalues.append(last_v)
         xforms_dict[xkey] = xvalues
         
      else:                                                          # still have to visit the object to identify vertex transform properties
         for prop in obj.getPropertyNames():
            extract_vertex_deform_info(obj.getProperty(prop), identifier + "/" + prop, nb_ts)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# identical to previous function, but check if the structure is identical
def concat_ts_xforms(archive):
   global xforms_dict
   global vertex_tss
   
   sampleTimes = archive.getSampleTimes()
   all_tss.append(sampleTimes)
   extract_max_TS(sampleTimes)
   
   nb_xforms = len(xforms_dict)                                      # need to be sure there's the same amount of Xforms in each file and that they're the same
   
   for identifier in archive.getIdentifiers():
      # is this object already known ?
      if identifier not in vertex_tss.keys():
         raise Exception(identifier + " doesn't exist in previous files")
      
      obj = archive.getObject(identifier)
      o_type = obj.getType()
      
      # append the TS index
      vertex_tss[identifier].append(obj.getTsIndex())
      nb_ts = len(sampleTimes[obj.getTsIndex()])
      
      if o_type.find("Xform") > -1:
         xfm = obj.getProperty(".xform")
         xvalues = xforms_dict[identifier]                           # get the values for this identifier
         
         last_i = xfm.getNbStoredSamples()                           # need to be sure that theirs one matrices per sampling
         for i in xrange(0, last_i):
            xvalues.append(xfm.getValues(i))
         
         if nb_ts != last_i:                                         # duplicate the last matrix if necessary
            last_v = xfm.getValues(last_i-1)
            for i in xrange(last_i, nb_ts):
               xvalues.append(last_v)
         
         nb_xforms = nb_xforms - 1                                   # one more Xform done!
         
      else:                                                          # still have to visit the object to identify vertex transform properties
         for prop in obj.getPropertyNames():
            extract_vertex_deform_info(obj.getProperty(prop), identifier + "/" + prop, nb_ts)
   
   if nb_xforms != 0:
      raise Exception("The file is missing some xforms")

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# create the final Time Sampling information for the concatenation!
def create_concat_ts():
   global upper_ts
   global vertex_tss
   global all_tss
   global final_tss
   global tss_indices
   
   # get all unique indices and create the right TS and associate them with the right index
   final_tss = [[0.0]]
   xx = 1
   for k in vertex_tss.keys():
      t_idx = tuple(vertex_tss[k])
      if t_idx not in tss_indices.keys():
         out_ts = []
         ii = 0
         for ti in t_idx:
            u_ts = upper_ts[ii]                                      # the upper_ts value for this file, file #ii!
            for a in all_tss[ii][ti]:
               out_ts.append(a + u_ts)                               # add u_ts to a, so the time sampling stays in chronological order
            ii = ii + 1
         final_tss.append(out_ts)
         tss_indices[t_idx] = xx                                     # all object with indices t_idx, are now associated with TS number xx
         xx = xx + 1
   
   pass

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# copy directly a property and its corresponding values
def copy_property(prop, outProp, full_name, firstFile):
   global vertex_def
   full_name = full_name + "/" + prop.getName()
   
   # if it's the first file, then needs to copy it... or if it's a vertex deform, needs to append the values of the other files
   if firstFile or vertex_def[full_name]:
      for i in xrange(0, prop.getNbStoredSamples()):
         outProp.setValues(prop.getValues(i))

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop, full_name, firstFile):
   full_name = full_name + "/" + cprop.getName()
   for prop_name in cprop.getPropertyNames():
      if prop_name == ".metadata":                                   # cause some problems, removed them
         continue
      sub_prop = cprop.getProperty(prop_name)
      out_prop = outCprop.getProperty(prop_name, sub_prop.getType())
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, out_prop, full_name, firstFile)
      else:
         copy_property(sub_prop, out_prop, full_name, firstFile)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# similar to function "copy_objects" copyABC.py... only difference is that it replaces the Xforms with the ones accumulated
def create_output(out_arch, in_arch, firstFile):
   global concat_ts
   global xforms_dict
   global vertex_tss
   global tss_indices
   
   for identifier in in_arch.getIdentifiers():
      obj = in_arch.getObject(identifier)
      o_type = obj.getType()

      # get the TS tuple of this object to retreive the right TS index
      tup_idx = tuple(vertex_tss[identifier])

      out = out_arch.createObject(o_type, identifier, tss_indices[tup_idx])
      if firstFile:
         out.setMetaData(obj.getMetaData())
      
      if o_type.find("Xform") == -1:                                 # not an Xform, copy it integrally
         for prop_name in obj.getPropertyNames():
            if prop_name == ".metadata":                             # cause some problems, removed them
               continue
            prop = obj.getProperty(prop_name)
            out_prop = out.getProperty(prop_name, prop.getType())
            if prop.isCompound():
               copy_compound_property(prop, out_prop, identifier, firstFile)
            else:
               copy_property(prop, out_prop, identifier, firstFile)
         
      elif firstFile:                                                # an Xform, use data collected above, but only for the first file, no need to repeat those
         xfm = out.getProperty(".xform", "matrix4d")
         xvalues = xforms_dict[identifier]
         for xval in xvalues:
            xfm.setValues(xval)
         
# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
def main(args):
   global concat_ts
   global vertex_def
   global final_tss
   
   # parser args
   parser = argparse.ArgumentParser(description="Concatenate multiple alembic files only if their geometries are identical.\n\nCurrently, it only supports Xforms. Issue #16 on GitHub is about adding vertex/normal deformations.")
   parser.add_argument("abc_files", metavar="{Alembic file}", type=str, nargs="+", help="alembic file to concatenate")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   ns = vars(parser.parse_args(args[1:]))
   
   abc_files = ns["abc_files"]
   if len(abc_files) < 2:
      print("Error: need at least two files for the concatenation")
      return
   
   for abc in abc_files:
      if abc == ns["o"]:
         print("Error: the output filename must be distinct from all the input files")
         return
   
   #read the first file and extract the time sampling, xforms and identifying vertex deforms
   print("\n\nScanning: " + abc_files[0])
   extract_ts_xforms(alembic.getIArchive(abc_files[0]))
   
   # go through the remaining files and accumulate the data about the time sampling, xforms and identifying more if necessary vertex deforms
   try:
      for abc in abc_files[1:]:
         print("Scanning: " + abc)
         concat_ts_xforms(alembic.getIArchive(abc))
   except Exception as abc_error:
      print("Error: not able to concatenate file " + abc_error)
      return
   
   # compute the new TSs
   create_concat_ts()
   
   # create the concatenate archive
   print("\nCreating:  " + ns["o"])
   out_arch = alembic.getOArchive(ns["o"])
   out_arch.createTimeSampling(final_tss)
   firstFile = True
   
   # going through each file to append their xforms
   for abc in abc_files:
      print("\t>> " + abc)
      create_output(out_arch, alembic.getIArchive(abc), firstFile)
      firstFile = False
   
   out_arch = None
   print("\n\n")

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)



