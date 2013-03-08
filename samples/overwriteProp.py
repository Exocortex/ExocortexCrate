import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#copy directly a property and its corresponding values
def copy_property(prop, outProp):
   for i in xrange(0, prop.getNbStoredSamples()):
      outProp.setValues(prop.getValues(i))

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop, out_data):
   for prop_name in cprop.getPropertyNames():
      if prop_name == ".metadata":
         continue                                                    # .metadata cause some problem
      print("--> comp pro: " + str(prop_name))
      sub_prop = cprop.getProperty(prop_name)
      curTS = sub_prop.getSampleTimes();
      tsSampling = out_data.createTimeSampling([curTS])
      out_prop = outCprop.getProperty(prop_name, sub_prop.getType(), tsSampling[0])
      if sub_prop.isCompound():
         copy_compound_property(sub_prop, out_prop)
      else:
         copy_property(sub_prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# going through each object
def copy_objects(src_data, rep_data, out_data, new_prop):
   rep_ids = rep_data.getIdentifiers()
   for identifier in src_data.getIdentifiers():
      print("obj: " + str(identifier))
      obj_replacable = ( identifier in rep_ids )
      obj = src_data.getObject(identifier)
      rep_obj = None
      if obj_replacable:
         rep_obj = rep_data.getObject(identifier)
      obj_typ = obj.getType()

      #curTS = obj.getSampleTimes()
      #out = None
      #if len(curTS.getSampleTimes()) == 0:
      #   out = out_data.createObject(obj_typ, identifier)
      #else:
      #   tsSampling = out_data.createTimeSampling([curTS])
      #   out = out_data.createObject(obj_typ, identifier, tsSampling[0])
      out = out_data.createObject(obj_typ, identifier)
      out.setMetaData(obj.getMetaData())
      propList = list(obj.getPropertyNames())
      print(propList)
      #for prop_name in propList:         
      ii = 0
      while ii < len(propList):
         print(ii)
         prop_name = propList[ii]
         ii = ii + 1
         print("--> pro: " + str(prop_name))
         if prop_name == ".metadata":
            continue                                                 # .metadata cause some problem

         copy_src = obj
         if obj_replacable and prop_name == new_prop:                # this object is replacable and this property is the right one ? change the source
           copy_src = rep_obj
           print("----> rep")
         
         prop = copy_src.getProperty(prop_name)
         #curTS = prop.getSampleTimes();
         #out_prop = None;
         #if len(curTS.getSampleTimes()) == 0:
         #   out_prop = out.getProperty(prop_name, prop.getType())
         #else:
         #   tsSampling = out_data.createTimeSampling([curTS])
         #   out_prop = out.getProperty(prop_name, prop.getType(), tsSampling[0])
         out_prop = out.getProperty(prop_name, prop.getType())
         
         if prop.isCompound():
            copy_compound_property(prop, out_prop, out_data)
         else:
            copy_property(prop, out_prop)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
def main(args):
   # parser args
   parser = argparse.ArgumentParser(description="Overwrite specific properties in objects with corresponding properties in a second file (with a similar structure).")
   parser.add_argument("abc_files", metavar="{Alembic files}", type=str, nargs="+", help="alembic file to merge")
   parser.add_argument("-o", type=str, metavar="{Alembic output file}", help="optional output file name, default is \"a.abc\"", default="a.abc")
   parser.add_argument("-p", type=str, metavar="{Property to replace}", help="Property to replace. Must be a property of the object. Not in a compound.")
   ns = vars(parser.parse_args(args[1:]))
   
   abc_files = ns["abc_files"]
   if len(abc_files) != 2:
      print("Error: need two files to merge")
      return
   
   if ns["o"] in abc_files:
      print("Error: the output filename must be distinct from all the input files")
      return
   
   ins = ns["abc_files"]
   src_data = alembic.getIArchive(ins[0])
   rep_data = alembic.getIArchive(ins[1])
   out_data = alembic.getOArchive(ns["o"])
   out_data.createTimeSampling(src_data.getSampleTimes())
   
   copy_objects(src_data, rep_data, out_data, ns["p"])

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)



