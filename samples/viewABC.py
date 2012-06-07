import _ExocortexAlembicPython as alembic
import sys
import argparse

show_time = False
show_meta = False
show_size = False
show_vals = False
obj_filter = None

def visit_prop(prop, depth):
   if prop.isCompound():
      print(depth + "compound: " + prop.getName())
      for sub_prop in prop.getPropertyNames():
         visit_prop(prop.getProperty(sub_prop), depth+"  ")
   else:
      print(depth + "property: " + prop.getName() + ", " + prop.getType())
      
      if show_size or show_vals:
         for i in xrange(0, prop.getNbStoredSamples()):
            if show_vals:
               print(depth + "-> values: " + str(prop.getValues(i)) )
            elif show_size:
               print(depth + "-> size: " + str(len(prop.getValues(i))) )

def visit_object(obj):
   for prop in obj.getPropertyNames():
      visit_prop(obj.getProperty(prop), "  ")

def visit_alembic(abc_archive):
   global obj_filter
   if show_time:
      print("Time sampling: " + str(abc_archive.getSampleTimes()))
   for id in abc_archive.getIdentifiers():
      if obj_filter != None and id.find(obj_filter) < 0:
         continue          # pass over this object!
      
      obj = abc_archive.getObject(id)
      print("OBJ: " + id + ", " + obj.getType())
      if show_meta:
         print("-- meta data: " + str(obj.getMetaData()))
      visit_object(obj)

def main(args):
   global show_time
   global show_meta
   global show_size
   global show_vals
   global obj_filter
   
   # parser args
   parser = argparse.ArgumentParser(description="Explore the structure of an Alembic file.")
   parser.add_argument("abc_in", type=str, metavar="{Alembic file}", help="input Alembic file to explore")
   parser.add_argument("-v", "--vals", action='store_true', help='show the values of the properties')
   parser.add_argument("-s", "--size", action='store_true', help='show only the number of values stored in the properties')
   parser.add_argument("-m", "--meta", action='store_true', help='show objects\' meta data')
   parser.add_argument("-t", "--time", action='store_true', help='show time sampling')
   parser.add_argument("-f", "--filter", type=str, help="only show objects containing this substring in their identifier")
   ns = vars(parser.parse_args(args[1:]))
   
   show_time = ns["time"]
   show_meta = ns["meta"]
   show_size = ns["size"]
   show_vals = ns["vals"]
   obj_filter = ns["filter"]
   
   abc_archive = alembic.getIArchive(ns["abc_in"])
   print("\n\nExploring " + ns["abc_in"])
   visit_alembic(abc_archive)

if __name__ == "__main__":
   main(sys.argv)


