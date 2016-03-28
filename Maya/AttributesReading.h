//-*****************************************************************************
//
// Copyright (c) 2009-2012,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#ifndef ABCIMPORT_NODE_ITERATOR_HELPER_H_
#define ABCIMPORT_NODE_ITERATOR_HELPER_H_

#include <Alembic/Abc/IArrayProperty.h>
#include <Alembic/Abc/IScalarProperty.h>
#include <Alembic/Abc/IObject.h>

#include <Alembic/AbcGeom/ICamera.h>
#include <Alembic/AbcGeom/ICurves.h>
#include <Alembic/AbcGeom/INuPatch.h>
#include <Alembic/AbcGeom/IPoints.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/ISubD.h>
#include <Alembic/AbcGeom/IXform.h>

#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MString.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>

#include <vector>
#include <string>

// mArray or mScalar will be valid, mObj will be valid for those situations
// where the property can't be validly read, unless the object stays in scope.
struct Prop
{
    Alembic::Abc::IArrayProperty mArray;
    Alembic::Abc::IScalarProperty mScalar;
};

std::string addProps(Alembic::Abc::ICompoundProperty & iParent, MObject & iObject,
              bool iUnmarkedFaceVaryingColors, bool areWritable);

bool addArrayProp(Alembic::Abc::IArrayProperty & iProp, MObject & iParent,
    bool isWritable,
    std::ostringstream &attrsList);
bool addScalarProp(Alembic::Abc::IScalarProperty & iProp, MObject & iParent,
    bool isWritable,
    std::ostringstream &attrsList);

enum AddPropResult
{
    INVALID = 0,
    VALID_DONE = 1,
    VALID_NOTDONE = 2
};

//
// Three possible return states, invalid, valid and done (attribute
// already existed and has been updated), valid and not done (new
// attribute needs calling method to continue).
//

//
// Avoiding some duplicated code with these templated versions.
//
template<class PODTYPE>
AddPropResult
addScalarExtentOneProp(Alembic::Abc::IScalarProperty& iProp,
                       Alembic::Util::uint8_t extent,
                       PODTYPE defaultVal,
                       MPlug& plug,
                       MString& attrName,
                       MFnNumericAttribute& numAttr,
                       MObject& attrObj,
                       MFnNumericData::Type type);

template <class PODTYPE>
AddPropResult
addScalarExtentThreeProp(Alembic::Abc::IScalarProperty& iProp,
                         Alembic::Util::uint8_t extent,
                         PODTYPE defaultVal,
                         MPlug& plug,
                         MString& attrName,
                         MFnNumericAttribute& numAttr,
                         MObject& attrObj,
                         MFnNumericData::Type type1,
                         MFnNumericData::Type type2,
                         MFnNumericData::Type type3);

template <class PODTYPE>
AddPropResult
addScalarExtentFourProp(Alembic::Abc::IScalarProperty& iProp,
                        Alembic::Util::uint8_t extent,
                        PODTYPE defaultVal,
                        MPlug& plug,
                        MString& attrName,
                        MFnNumericAttribute& numAttr,
                        MObject& attrObj,
                        MFnNumericData::Type type1,
                        MFnNumericData::Type type2,
                        MFnNumericData::Type type3,
                        MFnNumericData::Type type4);

void readProp(double iFrame,
              Alembic::Abc::IArrayProperty & iProp,
              MDataHandle & iHandle);

void readProp(double iFrame,
              Alembic::Abc::IScalarProperty & iProp,
              MDataHandle & iHandle);

void readProps(double iFrame,
               Alembic::Abc::ICompoundProperty & iParent,
               MDataBlock & iDataBlock,
               const MObject & iNode);

#endif  // ABCIMPORT_NODE_ITERATOR_HELPER_H_
