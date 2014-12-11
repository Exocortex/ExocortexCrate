import _ExocortexAlembicPython as alembic
import sys
import argparse

def compareValues(a1, a2, id):
   if a1.getNbStoredSamples() != a2.getNbStoredSamples():
      print("--> VAL: " + str(id) + " doesn't have the same number of sample")
      return

   for i in xrange(0, a1.getNbStoredSamples()):
      if a1.getValues(i) != a2.getValues(i):
         print("--> VAL: " + str(id) + " value #" + str(i) + " is different")
         return
   pass

def compareProperties(a1, a2, id):
   props2 = a2.getPropertyNames()
   for prop in a1.getPropertyNames():
      if prop not in props2:
         print("--> PRO: " + str(id) + "/" + str(prop) + " doesn't exist")
         continue

      prop1 = a1.getProperty(prop)
      prop2 = a2.getProperty(prop)
      if prop1.getType() != prop2.getType():
         print("--> PRO: " + str(id) + "/" + str(prop) + " is not the same type")
         continue

      id2 = id + "/" + prop
      if prop1.isCompound():
         compareProperties(prop1, prop2, id2)
      else:
         compareValues(prop1, prop2, id2)
   pass

def compareObjects(a1, a2):
   secondIds = a2.getIdentifiers()
   for identifier in a1.getIdentifiers():
      if identifier not in secondIds:
         print("--> OBJ: " + str(identifier) + " doesn't exist")
         continue

      obj1 = a1.getObject(identifier)
      obj1_typ = obj1.getType()
      obj2 = a2.getObject(identifier)
      obj2_typ = obj1.getType()

      if obj1_typ != obj2_typ:
         print("--> OBJ: " + str(identifier) + " is not the same type")
         continue

      compareProperties(obj1, obj2, identifier)
   pass

def main(args):
   # parser args
   parser = argparse.ArgumentParser(description="Compare an alembic file to a second one and report the differences.")
   parser.add_argument("abc_in", type=str, metavar=("{file1}", "{file2}"), nargs=2, help="input alembic file to be compared")
   ns = vars(parser.parse_args(args[1:]))

   if ns["abc_in"][0] == ns["abc_in"][1]:
      print("cannot compare a file to itself!")
      return

   in1 = alembic.getIArchive(ns["abc_in"][0])
   in2 = alembic.getIArchive(ns["abc_in"][1])
   
   if in1 != None and in2 != None:
      compareObjects(in1, in2)
   in1 = None
   in2 = None

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
if __name__ == "__main__":
   main(sys.argv)
