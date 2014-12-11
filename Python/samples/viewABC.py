import _ExocortexAlembicPython as alembic
import sys
import argparse

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# global variables
show_time = False
show_meta = False
show_size = False
show_vals = False
show_just_obj = False
show_ts = False
obj_filter = None
typ_filter = None
noo_filter = None
not_filter = None

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# visit the hierarchy of properties and compounds
def visit_prop(prop, depth):
   if prop.isCompound():
      print(depth + "compound: " + prop.getName())
      for sub_prop in prop.getPropertyNames():
         visit_prop(prop.getProperty(sub_prop), depth+"  ")
   else:
      print(depth + "property: \"" + prop.getName() + "\", " + prop.getType())
      
      if show_size or show_vals:
         for i in xrange(0, prop.getNbStoredSamples()):
            if show_vals:
               print(depth + "-> values: " + str(prop.getValues(i)) )
            elif show_size:
               print(depth + "-> size: " + str(len(prop.getValues(i))) )

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# start the visit of the properties
def visit_object(obj):
   for prop in obj.getPropertyNames():
      visit_prop(obj.getProperty(prop), "  ")

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
# inspect the base of the archive, use the filter to discard objects if necessary
def visit_alembic(abc_archive):
   global show_just_obj
   global obj_filter
   global typ_filter
   global noo_filter
   global not_filter
   global show_ts
   
   if show_time:
      print("Time sampling: " + str(abc_archive.getSampleTimes()))
   for identifier in abc_archive.getIdentifiers():
      if (obj_filter != None and identifier.find(obj_filter) < 0) or (noo_filter != None and identifier.find(noo_filter) >= 0):
         continue                                                    # pass over this object!
      
      obj = abc_archive.getObject(identifier)
      obj_typ = obj.getType()
      if (typ_filter != None and obj_typ.find(typ_filter) < 0) or (not_filter != None and obj_typ.find(not_filter) >= 0):
         continue                                                    # pass over this object because of its type!
      
      print("OBJ: " + identifier + ", " + obj_typ)
      if show_meta:
         print("-- meta data: " + str(obj.getMetaData()))
      if show_ts:
         print("-- TS index: " + str(obj.getTsIndex()))
      
      if not show_just_obj:
         visit_object(obj)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
def main(args):
   global show_time
   global show_meta
   global show_size
   global show_vals
   global show_just_obj
   global obj_filter
   global typ_filter
   global noo_filter
   global not_filter
   global show_ts
   
   # parser args
   parser = argparse.ArgumentParser(description="Explore the structure of an Alembic file.")
   parser.add_argument("abc_in", type=str, metavar="{Alembic file}", help="input Alembic file to explore")
   parser.add_argument("-v", "--vals", action='store_true', help='show the values of the properties')
   parser.add_argument("-s", "--size", action='store_true', help='show only the number of values stored in the properties')
   parser.add_argument("-m", "--meta", action='store_true', help='show objects\' meta data')
   parser.add_argument("-t", "--time", action='store_true', help='show time sampling')
   parser.add_argument("-O", "--object", action='store_true', help='show only objects, not properties')
   parser.add_argument("-f", "--filter", type=str, metavar="{id filter}", help="only show objects containing substring {id filter} in their identifier")
   parser.add_argument("-T", "--typefilter", type=str, metavar="{type filter}", help="only show objects containing substring {type filter} in their type")
   parser.add_argument("-nf", "--NOTfilter", type=str, metavar="{id filter}", help="only copy objects NOT containing substring {id filter} in their identifier")
   parser.add_argument("-nT", "--NOTtypefilter", type=str, metavar="{type filter}", help="only copy objects NOT containing substring {type filter} in their type")
   parser.add_argument("-S", "--samp", action='store_true', help="show object's time sampling index")
   ns = vars(parser.parse_args(args[1:]))
   
   show_time = ns["time"]
   show_meta = ns["meta"]
   show_size = ns["size"]
   show_vals = ns["vals"]
   show_ts   = ns["samp"]
   obj_filter = ns["filter"]
   typ_filter = ns["typefilter"]
   noo_filter = ns["NOTfilter"]
   not_filter = ns["NOTtypefilter"]
   show_just_obj = ns["object"]
   
   abc_archive = alembic.getIArchive(ns["abc_in"])
   print("\n\nExploring " + ns["abc_in"])
   visit_alembic(abc_archive)

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)


