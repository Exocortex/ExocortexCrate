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
 * Simplifies node picking by merging the responsibilities for both
 * PickNodeCallback and PickModeCallback, into a single virtual function 
 * that must be implemented (OnObjectPicked).
 */
class SingleObjectNodePicker : 
      public PickModeCallback,
      public PickNodeCallback 
{
private: 

    INode* hit;

public:     

    SingleObjectNodePicker() 
        : hit(NULL)
    { }

    ~SingleObjectNodePicker()
    { }

    BOOL HitTest(IObjParam *ip, HWND hWnd, ViewExp *vpt, IPoint2 m, int flags) { 
        return ip->PickNode(hWnd, m, this) ? TRUE : FALSE;
    }

    BOOL Pick(IObjParam *ip, ViewExp *vpt) {
        hit = vpt->GetClosestHit();
        OnObjectPicked(hit);
        return TRUE;
    }

    void EnterMode(IObjParam *ip) {
        hit = NULL;
		GetCOREInterface()->PushPrompt(_T("Select target"));
    }

    void ExitMode(IObjParam *ip) {
		GetCOREInterface()->PopPrompt();
    }

    BOOL RightClick(IObjParam *ip,ViewExp *vpt)  {
        return TRUE;
    }

    BOOL Filter(INode *node) {
        return node != NULL && node->GetObjectRef() != NULL;
    }

    PickNodeCallback *GetFilter() {
        return this;
    }

    BOOL AllowMultiSelect() {
        return FALSE;
    }

    INode* GetHitNode() {
        return hit;
    }

protected:

    virtual void OnObjectPicked(INode* hit) = 0;
};
