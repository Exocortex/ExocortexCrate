import _ExocortexAlembicPython as alembic
import sys
import argparse

# read a line of float from the OBJ files and convert it into a tuple
def create_float_tuple(splits):
   float_list = []
   for sp in splits[1:]:
      float_list.append(float(sp))
   return tuple(float_list)

# read a triplet of data from
def create_face_triplet(splits):
   int_lst = []
   for sp in splits.split('/'):
      try:
         ii = int(sp) - 1     # OBJ files start indices at 1, not zero
         int_lst.append(ii)
      except:
         int_lst.append(-1)
   while len(int_lst) < 3:
      int_lst.append(-1)
   return tuple(int_lst)

def test_min_max(val, min_v, max_v):
   return (min(val, min_v), max(val, max_v))

#an object to hold the information about the OBJ file
class OBJ_structure:
   def __init__(self):
      self.vertices = []
      self.normals = []
      self.texcoords = []
      self.parameters = [] #not supported yet!
      self.faces = []   # a list of list of triplets  [ [(1, 0, 0) (2, 0, 0) (3, 0, 0)] ] #only one face
   
   def read_vertex(self, splits):
      self.vertices.append(create_float_tuple(splits))
   
   def read_normals(self, splits):
      self.normals.append(create_float_tuple(splits))
   
   def read_texcoords(self, splits):
      self.texcoords.append(create_float_tuple(splits))
   
   def read_parameters(self, splits):
      self.parameters.append(create_float_tuple(splits))
   
   def read_faces(self, splits):
      this_face = []
      for sp in splits[1:]:
         this_face.append(create_face_triplet(sp))
      self.faces.append(this_face)
   
   # loading the structure of the OBJ file
   def read(self, in_file):
      for line in in_file:
         sp = line.split()
         if sp[0] == "#":
            pass  #passing over a commented line
         elif sp[0] == "v":
            self.read_vertex(sp)
         elif sp[0] == "vn":
            self.read_normals(sp)
         elif sp[0] == "vt":
            self.read_texcoords(sp)
         elif sp[0] == "vp":
            self.read_parameters(sp)
         elif sp[0] == "f":
            self.read_faces(sp)
         else:
            pass #passing over an undefined line
      return self
   
   def OBJ_status(self):
      print("There's:\n\t" + str(len(self.vertices)) + " vertices\n\t" + str(len(self.normals)) + " normals\n\t" + str(len(self.texcoords)) + " texture coodinates\n\t" + str(len(self.parameters)) + " parameters")
      print("Composed of " + str(len(self.faces)) + " faces")
      return self
   
   # Creating the Alembic file from the OBJ structure
   def to_alembic(self, archive):
      archive.createTimeSampling([0,1])
      mesh = archive.createObject("AbcGeom_PolyMesh_v1", "/pFromObj")
      faceCounts = []
      faceIndices = []
      P = []
      
      #create the point list and compute the bounding box required by any PolyMesh
      pts = self.vertices[0]
      min_x = max_x = pts[0]
      min_y = max_y = pts[1]
      min_z = max_z = pts[2]
      P.append(min_x)
      P.append(min_y)
      P.append(min_z)
      for pts in self.vertices[1:]:
         P.append(pts[0])
         P.append(pts[1])
         P.append(pts[2])
         
         #test min/max
         (min_x, max_x) = test_min_max(pts[0], min_x, max_x)
         (min_y, max_y) = test_min_max(pts[1], min_y, max_y)
         (min_z, max_z) = test_min_max(pts[2], min_z, max_z)
         
      mesh.getProperty("P", "vector3farray").setValues(P)                                 #write P to the abc file
      mesh.getProperty(".selfBnds").setValues([min_x, min_y, min_z, max_x, max_y, max_z]) #write the bounding box
      
      #go through the faceCounts and faceIndices
      for face in self.faces:
         faceCounts.append(len(face))
         for indices in face:
            faceIndices.append(indices[0])
      mesh.getProperty(".faceCounts", "int32array").setValues(faceCounts)
      mesh.getProperty(".faceIndices", "int32array").setValues(faceIndices)
      
      # uv
      if len(self.texcoords) != 0:
         c_uv = mesh.getProperty("uv", "compound")
         
         #create the uv list
         uv_indices = []
         uv_vals = []
         
         #uvs
         for uv in self.texcoords:
            uv_vals.append(uv[0])
            uv_vals.append(uv[1])
         
         #uv indices
         for face in self.faces:
            for indices in face:
               uv_indices.append(indices[1])
         c_uv.getProperty(".indices", "uint32array").setValues(uv_indices)
         c_uv.getProperty(".vals", "vector2farray").setValues(uv_vals)
      
      # N
      if len(self.normals) != 0:
         c_N = mesh.getProperty("N", "compound")
         
         #create the N list
         N_indices = []
         N_vals = []
         
         #Ns
         for n in self.normals:
            N_vals.append(n[0])
            N_vals.append(n[1])
            N_vals.append(n[2])
         
         #N indices
         for face in self.faces:
            for indices in face:
               N_indices.append(indices[2])
         c_N.getProperty(".indices", "uint32array").setValues(N_indices)
         c_N.getProperty(".vals", "vector2farray").setValues(N_vals)
      return self

def main(args):
   # parser args
   parser = argparse.ArgumentParser(description="Convert an OBJ file into an Alembic file.")
   parser.add_argument("obj_in", type=str, metavar="{OBJ file}", help="input OBJ file to be converted to an alembic file")
   parser.add_argument("-o", type=str, metavar="{Alembic file name}", help="optional output file name, default is \"a.abc\"")
   ns = vars(parser.parse_args(args[1:]))
   
   archive = alembic.getOArchive(abc_out)
   
   print("Parsing " + ns["obj_in"])
   file_in = open(ns["obj_in"], "r")
   obj_st = OBJ_structure().read(file_in)
   file_in.close()
   
   obj_st.OBJ_status()
   
   abc_out = ns["o"]
   if abc_out == None:
      abc_out = "a.out"
   print("Creating " + abc_out)
   obj_st.to_alembic(archive)

if __name__ == "__main__":
   main(sys.argv)


