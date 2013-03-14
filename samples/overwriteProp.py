import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#copy directly a property and its corresponding values
def copy_property(prop, outProp):
   print("copy_property")
   for i in range(0, prop.getNbStoredSamples()):
      outProp.setValues(prop.getValues(i))

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
#visiting the structure, if it's a property, copy it, if it's a compound, continue the visit there
def copy_compound_property(cprop, outCprop, out_data):
   print("copy_compound_property")
   for prop_name in cprop.getPropertyNames():
      if prop_name == ".metadata":
         continue                                                    # .metadata cause some problem
      #print("--> comp pro: " + str(prop_name))

      prop = cprop.getProperty(prop_name)
      print(type(prop))

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
def copy_objects(src_data, rep_data, out_data, new_prop):
   rep_ids = rep_data.getIdentifiers()
   for identifier in src_data.getIdentifiers():
      #print("obj: " + str(identifier))
      obj_replacable = ( identifier in rep_ids )
      obj = src_data.getObject(identifier)
      rep_obj = None
      if obj_replacable:
         rep_obj = rep_data.getObject(identifier)
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
         #print("--> pro: " + str(prop_name))
         if prop_name == ".metadata":
            continue                                                 # .metadata cause some problem

         copy_src = obj
         if obj_replacable and prop_name == new_prop:                # this object is replacable and this property is the right one ? change the source
           copy_src = rep_obj
           #print("----> rep")
         
         prop = copy_src.getProperty(prop_name)
         print(type(prop))

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
   TSs = src_data.getSampleTimes()
   print(str(TSs))
   out_data.createTimeSampling(TSs)
   
   copy_objects(src_data, rep_data, out_data, ns["p"])

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   #sys.stdin.readline()
   main(sys.argv)



