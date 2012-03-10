/* Copyright (c) 2011, Autodesk 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  Neither the name of Autodesk or the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

/**
 * Manages the deformation of control points for the BendModifier. 
 */
class AlembicMesh2Deformer
    : public Deformer 
{
	public:
		Matrix3 tm,invtm, tmAbove, tmBelow;
		Box3 bbox;
		TimeValue time;
		float r, from, to;
		int doRegion;
		
        AlembicMesh2Deformer()
            : tm(1), invtm(1), tmAbove(1), tmBelow(1), time(0)
        { }

		AlembicMesh2Deformer(TimeValue t, ModContext &mc,
			float angle, float dir, int naxis, 
			float from, float to, int doRegion, 
			Matrix3& modmat, Matrix3& modinv)
        {
	        this->doRegion = doRegion;
	        this->from = from;
	        this->to   = to;
	        Matrix3 mat;
	        Interval valid;	
	        time   = t;	

	        tm = modmat;
	        invtm = modinv;
	        mat.IdentityMatrix();
        	
	        switch (naxis) 
            {
		        case 0: mat.RotateY( -HalfPi() );	 break; //X
		        case 1: mat.RotateX( HalfPi() );  break; //Y
		        case 2: break;  //Z
		    }
	        mat.RotateZ(DegToRad(dir));	
	        SetAxis(mat);	
	        assert (mc.box);
	        bbox = *mc.box;
	        CalcR(naxis,DegToRad(angle));
        	
	        // Turn this off for a sec.
	        this->doRegion = FALSE;
        		
	        float len  = to-from;
	        float rat1, rat2;
	        if (len==0.0f) {
		        rat1 = rat2 = 1.0f;
	        } else {
		        rat1 = to/len;
		        rat2 = from/len;
		    }
	        Point3 pt;
	        tmAbove.IdentityMatrix();
	        tmAbove.Translate(Point3(0.0f,0.0f,-to));
	        tmAbove.RotateY(DegToRad(angle * rat1));
	        tmAbove.Translate(Point3(0.0f,0.0f,to));
	        pt = Point3(0.0f,0.0f,to);
	        tmAbove.Translate((Map(0,pt*invtm)*tm)-pt);

	        tmBelow.IdentityMatrix();
	        tmBelow.Translate(Point3(0.0f,0.0f,-from));
	        tmBelow.RotateY(DegToRad(angle * rat2));	
	        tmBelow.Translate(Point3(0.0f,0.0f,from));
	        pt = Point3(0.0f,0.0f,from);
	        tmBelow.Translate((Map(0,pt*invtm)*tm)-pt);	
        	
	        this->doRegion = doRegion;
        }
		
        void SetAxis(Matrix3 &tmAxis)
        {
	        Matrix3 itm = Inverse(tmAxis);
	        tm = tm * tmAxis;
	        invtm =	itm * invtm;
        }

		void CalcR(int axis, float angle)
        {
	        float len = float(0);
	        if (!doRegion) 
            {
		        switch (axis) 
                {
			        case 0: 
                        len = bbox.pmax.x - bbox.pmin.x; 
                        break;
			        case 1:	
                        len = bbox.pmax.y - bbox.pmin.y; 
                        break;
			        case 2: 
                        len = bbox.pmax.z - bbox.pmin.z; 
                        break;
                }
		    }
	        else 
            {
		        len = to-from;
		    }

            r = SafeDivide(len, angle);
    	}
		
        // This method gets called for each point. 
		Point3 Map(int i, Point3 p)
        {
	        float x, y, c, s, yr;
	        if (r==0 && !doRegion) return p;
	        
            // The point is first transformed by the tm.  This is typical
	        // for all modifiers. 
	        p = p * tm;
	        if (doRegion) {
		        if (p.z<from) {
			        return tmBelow * p * invtm;			
		        } else 
		        if (p.z>to) {
			        return tmAbove * p * invtm;
			    }
		    }	
	        
            if (r==0) 
                return p * invtm;
	        
            x = p.x;
	        y = p.z;
	        yr = y/r;
	        
            c = cos(Pi() - yr);
	        s = sin(Pi() - yr);
	        
            p.x = r*c + r - x*c;
	        p.z = r*s - x*s;

	        // The point is finally transformed by the inverse of the tm.
	        p = p * invtm;
	        return p;
	    }
};

