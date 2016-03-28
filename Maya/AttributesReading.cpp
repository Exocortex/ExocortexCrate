//-*****************************************************************************
//
// Copyright (c) 2009-2013,
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
#include "stdafx.h"

#include "AttributesReading.h"

#include <Alembic/AbcCoreFactory/IFactory.h>

#include <maya/MDoubleArray.h>
#include <maya/MIntArray.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MUint64Array.h>
#include <maya/MStringArray.h>
#include <maya/MFnData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObjectArray.h>
#include <maya/MDGModifier.h>
#include <maya/MSelectionList.h>
#include <maya/MFnLight.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnCamera.h>
#include <maya/MTime.h>

double getWeightAndIndex(double iFrame,
    Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps,
    Alembic::AbcCoreAbstract::index_t & oIndex,
    Alembic::AbcCoreAbstract::index_t & oCeilIndex)
{
  if (numSamps == 0)
    numSamps = 1;

  std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
    iTime->getFloorIndex(iFrame, numSamps);

  oIndex = floorIndex.first;
  oCeilIndex = oIndex;

  if (fabs(iFrame - floorIndex.second) < 0.0001)
    return 0.0;

  std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
    iTime->getCeilIndex(iFrame, numSamps);

  if (oIndex == ceilIndex.first)
    return 0.0;

  oCeilIndex = ceilIndex.first;

  double alpha = (iFrame - floorIndex.second) /
    (ceilIndex.second - floorIndex.second);

  // we so closely match the ceiling so we'll just use it
  if (fabs(1.0 - alpha) < 0.0001)
  {
    oIndex = oCeilIndex;
    return 0.0;
  }

  return alpha;
}

template<typename T>
T simpleLerp(double alpha, T val1, T val2)
{
  double dv = static_cast<double>( val1 );
  return static_cast<T>( dv + alpha * (static_cast<double>(val2) - dv) );
}

template <class T>
void unsupportedWarning(T & iProp)
{
    MString warn = "Unsupported attr, skipping: ";
    warn += iProp.getName().c_str();
    warn += " ";
    warn += PODName(iProp.getDataType().getPod());
    warn += "[";
    warn += iProp.getDataType().getExtent();
    warn += "]";

    MGlobal::displayWarning(warn);
}

void addString(MObject & iParent, const std::string & iAttrName,
    const std::string & iValue)
{
    MFnStringData fnStringData;
    MString attrValue(iValue.c_str());
    MString attrName(iAttrName.c_str());
    MObject strAttrObject = fnStringData.create("");

    MFnTypedAttribute attr;
    MObject attrObj = attr.create(attrName, attrName, MFnData::kString,
        strAttrObject);
    MFnDependencyNode parentFn(iParent);
    parentFn.addAttribute(attrObj, MFnDependencyNode::kLocalDynamicAttr);

    // work around bug where this string wasn't getting saved to a file when
    // it is the default value
    MPlug plug = parentFn.findPlug(attrName);
    if (!plug.isNull())
    {
        plug.setString(attrValue);
    }
}

void addArbAttrAndScope(MObject & iParent, const std::string & iAttrName,
    const std::string & iScope, const std::string & iInterp,
    Alembic::Util::uint8_t iExtent)
{

    std::string attrStr;

    // constant scope colors can use setUsedAsColor
    if (iInterp == "rgb" && iScope != "" && iScope != "con")
    {
        attrStr = "rgb";
    }
    else if (iInterp == "rgba" && iScope != "" && iScope != "con")
    {
        attrStr = "rgba";
    }
    else if (iInterp == "vector")
    {
        if (iExtent == 2)
            attrStr = "vector2";
        // the data type makes it intrinsically a vector3
    }
    else if (iInterp == "point")
    {
        if (iExtent == 2)
            attrStr = "point2";
        // the data type is treated intrinsically as a point3
    }
    else if (iInterp == "normal")
    {
        if (iExtent == 2)
            attrStr = "normal2";
        else if (iExtent == 3)
            attrStr = "normal3";
    }

    if (attrStr != "")
    {
        std::string attrName = iAttrName + "_AbcType";
        addString(iParent, attrName, attrStr);
    }

    if (iScope != "" && iScope != "con")
    {
        std::string attrName = iAttrName + "_AbcGeomScope";
        addString(iParent, attrName, iScope);
    }
}

bool addArrayProp(Alembic::Abc::IArrayProperty & iProp, MObject & iParent,
        bool isWritable,
        std::ostringstream &attrsList)
{
    MFnDependencyNode parentFn(iParent);
    MString attrName(iProp.getName().c_str());
    MPlug plug = parentFn.findPlug(attrName);

    MFnTypedAttribute typedAttr;
    MFnNumericAttribute numAttr;

    MObject attrObj;
    Alembic::AbcCoreAbstract::DataType dtype = iProp.getDataType();
    Alembic::Util::uint8_t extent = dtype.getExtent();
    std::string interp = iProp.getMetaData().get("interpretation");

    bool isScalarLike = iProp.isScalarLike() &&
        iProp.getMetaData().get("isArray") != "1";

    // the first sample is read only when the property is constant
    switch (dtype.getPod())
    {
        case Alembic::Util::kBooleanPOD:
        {
            if (extent != 1 || !isScalarLike)
            {
                return false;
            }

            bool bval = 0;

            if (iProp.isConstant())
            {
                Alembic::AbcCoreAbstract::ArraySamplePtr val;
                iProp.get(val);
                bval =
                    ((Alembic::Util::bool_t *)(val->getData()))[0] != false;
            }

            if (plug.isNull())
            {
                attrObj = numAttr.create(attrName, attrName,
                    MFnNumericData::kBoolean, bval);
            }
            else
            {
                plug.setValue(bval);
            }
        }
        break;

        case Alembic::Util::kUint8POD:
        case Alembic::Util::kInt8POD:
        {
            if (extent != 1 || !isScalarLike)
            {
                return false;
            }

            // default is 1 just to accomodate visiblitity
            Alembic::Util::int8_t val = 1;

            if (iProp.isConstant())
            {
                Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                iProp.get(samp);
                val = ((Alembic::Util::int8_t *) samp->getData())[0];
            }

            if (plug.isNull())
            {
                attrObj = numAttr.create(attrName, attrName,
                    MFnNumericData::kByte, val);
            }
            else
            {
                plug.setValue(val);
                attrsList << ";" << attrName;

                return true;
            }
        }
        break;

        case Alembic::Util::kInt16POD:
        case Alembic::Util::kUint16POD:
        {
            // MFnNumericData::kShort or k2Short or k3Short
            if (extent > 3 || !isScalarLike)
            {
                return false;
            }

            Alembic::Util::int16_t val[3] = {0, 0, 0};

            if (iProp.isConstant())
            {
                Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                iProp.get(samp);
                const Alembic::Util::int16_t * sampData =
                    (const Alembic::Util::int16_t *) samp->getData();

                for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                {
                    val[i] = sampData[i];
                }
            }

            if (!plug.isNull())
            {
                unsigned int numChildren = plug.numChildren();
                if (numChildren == 0)
                {
                    plug.setValue(val[0]);
                }
                else
                {
                    if (numChildren > extent)
                        numChildren = extent;

                    for (unsigned int i = 0; i < numChildren; ++i)
                    {
                        plug.child(i).setValue(val[i]);
                    }
                }
                attrsList << ";" << attrName;

                return true;
            }
            else if (extent == 1)
            {
                attrObj = numAttr.create(attrName, attrName,
                    MFnNumericData::kShort);
                numAttr.setDefault(val[0]);
            }
            else if (extent == 2)
            {
                attrObj = numAttr.create(attrName, attrName,
                    MFnNumericData::k2Short);
                numAttr.setDefault(val[0], val[1]);
            }
            else if (extent == 3)
            {
                attrObj = numAttr.create(attrName, attrName,
                    MFnNumericData::k3Short);
                numAttr.setDefault(val[0], val[1], val[2]);
            }
        }
        break;

        case Alembic::Util::kUint32POD:
        case Alembic::Util::kInt32POD:
        {
            if (!isScalarLike)
            {
                MFnIntArrayData fnData;
                MObject arrObj;

                if (iProp.isConstant())
                {
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    iProp.get(samp);

                    MIntArray arr((int *) samp->getData(),
                        static_cast<unsigned int>(samp->size()));
                    arrObj = fnData.create(arr);
                    if (!plug.isNull())
                    {
                        plug.setValue(arrObj);
                        attrsList << ";" << attrName;

                        return true;
                    }
                }
                else
                {
                    MIntArray arr;
                    arrObj = fnData.create(arr);
                }

                attrObj = typedAttr.create(attrName, attrName,
                    MFnData::kIntArray, arrObj);
            }
            // isScalarLike
            else
            {
                if (extent > 3)
                {
                    return false;
                }

                Alembic::Util::int32_t val[3] = {0, 0, 0};

                if (iProp.isConstant())
                {
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    iProp.get(samp);
                    const Alembic::Util::int32_t * sampData =
                        (const Alembic::Util::int32_t *)samp->getData();
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                        val[i] = sampData[i];
                    }
                }

                if (!plug.isNull())
                {
                    unsigned int numChildren = plug.numChildren();
                    if (numChildren == 0)
                    {
                        plug.setValue(val[0]);
                    }
                    else
                    {
                        if (numChildren > extent)
                            numChildren = extent;

                        for (unsigned int i = 0; i < numChildren; ++i)
                        {
                            plug.child(i).setValue(val[i]);
                        }
                    }
                    attrsList << ";" << attrName;

                    return true;
                }
                else if (extent == 1)
                {
                    attrObj = numAttr.create(attrName, attrName,
                        MFnNumericData::kInt);
                    numAttr.setDefault(val[0]);
                }
                else if (extent == 2)
                {
                    attrObj = numAttr.create(attrName, attrName,
                        MFnNumericData::k2Int);
                    numAttr.setDefault(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    attrObj = numAttr.create(attrName, attrName,
                        MFnNumericData::k3Int);
                    numAttr.setDefault(val[0], val[1], val[2]);
                }
            }
        }
        break;

        // look for MFnVectorArrayData?
        case Alembic::Util::kFloat32POD:
        {
            if (!isScalarLike)
            {
                if ((extent == 2 || extent == 3) && (interp == "normal" ||
                    interp == "vector" || interp == "rgb"))
                {
                    MFnVectorArrayData fnData;
                    MObject arrObj;

                    if (iProp.isConstant())
                    {
                        Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                        iProp.get(samp);

                        unsigned int sampSize = (unsigned int)samp->size();
                        MVectorArray arr(sampSize);
                        MVector vec;
                        const Alembic::Util::float32_t * sampData =
                            (const Alembic::Util::float32_t *) samp->getData();

                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            vec.x = sampData[extent*i];
                            vec.y = sampData[extent*i+1];

                            if (extent == 3)
                            {
                                vec.z = sampData[extent*i+2];
                            }
                            arr[i] = vec;
                        }

                        arrObj = fnData.create(arr);
                        if (!plug.isNull())
                        {
                            plug.setValue(arrObj);
                            attrsList << ";" << attrName;

                            return true;
                        }
                    }
                    else
                    {
                        MVectorArray arr;
                        arrObj = fnData.create(arr);
                    }

                    attrObj = typedAttr.create(attrName, attrName,
                        MFnData::kVectorArray, arrObj);
                }
                else if (interp == "point" && (extent == 2 || extent == 3))
                {
                    MFnPointArrayData fnData;
                    MObject arrObj;

                    if (iProp.isConstant())
                    {
                        Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                        iProp.get(samp);

                        unsigned int sampSize = (unsigned int)samp->size();
                        MPointArray arr(sampSize);
                        MPoint pt;

                        const Alembic::Util::float32_t * sampData =
                            (const Alembic::Util::float32_t *) samp->getData();

                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            pt.x = sampData[extent*i];
                            pt.y = sampData[extent*i+1];

                            if (extent == 3)
                            {
                                pt.z = sampData[extent*i+2];
                            }
                            arr[i] = pt;
                        }

                        arrObj = fnData.create(arr);
                        if (!plug.isNull())
                        {
                            plug.setValue(arrObj);
                            attrsList << ";" << attrName;

                            return true;
                        }
                    }
                    else
                    {
                        MPointArray arr;
                        arrObj = fnData.create(arr);
                    }

                    attrObj = typedAttr.create(attrName, attrName,
                        MFnData::kPointArray, arrObj);
                }
                else
                {
                    MFnDoubleArrayData fnData;
                    MObject arrObj;

                    if (iProp.isConstant())
                    {
                        Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                        iProp.get(samp);

                        MDoubleArray arr((float *) samp->getData(),
                            static_cast<unsigned int>(samp->size()));
                        arrObj = fnData.create(arr);
                        if (!plug.isNull())
                        {
                            plug.setValue(arrObj);
                            attrsList << ";" << attrName;

                            return true;
                        }
                    }
                    else
                    {
                        MDoubleArray arr;
                        arrObj = fnData.create(arr);
                    }

                    attrObj = typedAttr.create(attrName, attrName,
                        MFnData::kDoubleArray, arrObj);
                }

            }
            // isScalarLike
            else
            {
                if (extent > 3)
                {
                    return false;
                }

                float val[3] = {0, 0, 0};

                if (iProp.isConstant())
                {
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    iProp.get(samp);
                    const float * sampData = (const float *) samp->getData();
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                        val[i] = sampData[i];
                    }
                }

                if (!plug.isNull())
                {
                    unsigned int numChildren = plug.numChildren();
                    if (numChildren == 0)
                    {
                        plug.setValue(val[0]);
                    }
                    else
                    {
                        if (numChildren > extent)
                            numChildren = extent;

                        for (unsigned int i = 0; i < numChildren; ++i)
                        {
                            plug.child(i).setValue(val[i]);
                        }
                    }
                    attrsList << ";" << attrName;

                    return true;
                }
                else if (extent == 1)
                {
                    attrObj = numAttr.create(attrName, attrName,
                        MFnNumericData::kFloat);
                    numAttr.setDefault(val[0]);
                }
                else if (extent == 2)
                {
                    attrObj = numAttr.create(attrName, attrName,
                        MFnNumericData::k2Float);
                    numAttr.setDefault(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    if (interp == "rgb")
                    {
                        attrObj = numAttr.createColor(attrName, attrName);
                    }
                    else if (interp == "point")
                    {
                        attrObj = numAttr.createPoint(attrName, attrName);
                    }
                    else
                    {
                        attrObj = numAttr.create(attrName, attrName,
                            MFnNumericData::k3Float);
                    }
                    numAttr.setDefault(val[0], val[1], val[2]);
                }
            }
        }
        break;

        case Alembic::Util::kFloat64POD:
        {
            if (!isScalarLike)
            {
                if ((extent == 2 || extent == 3) && (interp == "normal" ||
                    interp == "vector" || interp == "rgb"))
                {
                    MFnVectorArrayData fnData;
                    MObject arrObj;

                    if (iProp.isConstant())
                    {
                        Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                        iProp.get(samp);

                        unsigned int sampSize = (unsigned int)samp->size();
                        MVectorArray arr(sampSize);
                        MVector vec;
                        const Alembic::Util::float64_t * sampData =
                            (const Alembic::Util::float64_t *) samp->getData();

                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            vec.x = sampData[extent*i];
                            vec.y = sampData[extent*i+1];

                            if (extent == 3)
                            {
                                vec.z = sampData[extent*i+2];
                            }

                            arr[i] = vec;
                        }

                        arrObj = fnData.create(arr);
                        if (!plug.isNull())
                        {
                            plug.setValue(arrObj);
                            attrsList << ";" << attrName;

                            return true;
                        }
                    }
                    else
                    {
                        MVectorArray arr;
                        arrObj = fnData.create(arr);
                    }

                    attrObj = typedAttr.create(attrName, attrName,
                        MFnData::kVectorArray, arrObj);
                }
                else if (interp == "point" && (extent == 2 || extent == 3))
                {
                    MFnPointArrayData fnData;
                    MObject arrObj;

                    if (iProp.isConstant())
                    {
                        Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                        iProp.get(samp);

                        unsigned int sampSize = (unsigned int)samp->size();
                        MPointArray arr(sampSize);
                        MPoint pt;
                        const Alembic::Util::float64_t * sampData =
                            (const Alembic::Util::float64_t *) samp->getData();

                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            pt.x = sampData[extent*i];
                            pt.y = sampData[extent*i+1];

                            if (extent == 3)
                            {
                                pt.z = sampData[extent*i+2];
                            }

                            arr[i] = pt;
                        }

                        arrObj = fnData.create(arr);
                        if (!plug.isNull())
                        {
                            plug.setValue(arrObj);
                            attrsList << ";" << attrName;

                            return true;
                        }
                    }
                    else
                    {
                        MPointArray arr;
                        arrObj = fnData.create(arr);
                    }

                    attrObj = typedAttr.create(attrName, attrName,
                        MFnData::kPointArray, arrObj);
                }
                else
                {
                    MFnDoubleArrayData fnData;
                    MObject arrObj;

                    if (iProp.isConstant())
                    {
                        Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                        iProp.get(samp);

                        MDoubleArray arr((double *) samp->getData(),
                            static_cast<unsigned int>(samp->size()));
                        arrObj = fnData.create(arr);
                        if (!plug.isNull())
                        {
                            plug.setValue(arrObj);
                            attrsList << ";" << attrName;

                            return true;
                        }
                    }
                    else
                    {
                        MDoubleArray arr;
                        arrObj = fnData.create(arr);
                    }

                    attrObj = typedAttr.create(attrName, attrName,
                        MFnData::kDoubleArray, arrObj);
                }

            }
            else
            {
                if (extent > 4)
                {
                    return false;
                }

                attrsList << ";" << attrName;

                double val[4] = {0, 0, 0, 0};

                if (iProp.isConstant())
                {
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    iProp.get(samp);
                    const double * sampData = (const double *) samp->getData();
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                        val[i] = sampData[i];
                    }
                }

                if (!plug.isNull())
                {
                    unsigned int numChildren = plug.numChildren();
                    if (numChildren == 0)
                    {
                        plug.setValue(val[0]);
                    }
                    else
                    {
                        if (numChildren > extent)
                            numChildren = extent;

                        for (unsigned int i = 0; i < numChildren; ++i)
                        {
                            plug.child(i).setValue(val[i]);
                        }
                    }
                    attrsList << ";" << attrName;

                    return true;
                }
                else if (extent == 1)
                {
                    if (plug.isNull())
                    {
                        attrObj = numAttr.create(attrName, attrName,
                            MFnNumericData::kDouble);
                        numAttr.setDefault(val[0]);
                    }
                    else
                    {
                        plug.setValue(val[0]);
                        attrsList << ";" << attrName;

                        return true;
                    }
                }
                else if (extent == 2)
                {
                    attrObj = numAttr.create(attrName, attrName,
                        MFnNumericData::k2Double);
                    numAttr.setDefault(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    attrObj = numAttr.create(attrName, attrName,
                        MFnNumericData::k3Double);
                    numAttr.setDefault(val[0], val[1], val[2]);
                }
                else if (extent == 4)
                {
                    attrObj = numAttr.create(attrName, attrName,
                        MFnNumericData::k4Double);
                    numAttr.setDefault(val[0], val[1], val[2], val[4]);
                }
            }
        }
        break;

        // MFnStringArrayData
        case Alembic::Util::kStringPOD:
        {
            if (!isScalarLike)
            {
                MFnStringArrayData fnData;
                MObject arrObj;

                if (iProp.isConstant())
                {
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    iProp.get(samp);

                    unsigned int sampSize = (unsigned int)samp->size();
                    MStringArray arr;
                    arr.setLength(sampSize);

                    Alembic::Util::string * strData =
                        (Alembic::Util::string *) samp->getData();

                    for (unsigned int i = 0; i < sampSize; ++i)
                    {
                        arr[i] = strData[i].c_str();
                    }
                    arrObj = fnData.create(arr);
                    if (!plug.isNull())
                    {
                        plug.setValue(arrObj);
                        attrsList << ";" << attrName;

                        return true;
                    }
                }
                else
                {
                    MStringArray arr;
                    arrObj = fnData.create(arr);
                }

                attrObj = typedAttr.create(attrName, attrName,
                    MFnData::kStringArray, arrObj);
            }
            // isScalarLike
            else
            {
                if (extent != 1)
                {
                    return false;
                }

                MFnStringData fnStringData;
                MObject strAttrObject;

                if (iProp.isConstant())
                {
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    iProp.get(samp);
                    MString attrValue(
                        ((Alembic::Util::string *) samp->getData())[0].c_str());
                    strAttrObject = fnStringData.create(attrValue);
                    if (!plug.isNull())
                    {
                        plug.setValue(strAttrObject);
                        attrsList << ";" << attrName;

                        return true;
                    }
                }
                else
                {
                    MString attrValue;
                    strAttrObject = fnStringData.create(attrValue);
                }

                attrObj = typedAttr.create(attrName, attrName, MFnData::kString,
                        MObject::kNullObj);

                parentFn.addAttribute(attrObj,
                    MFnDependencyNode::kLocalDynamicAttr);

                plug = parentFn.findPlug(attrName);
                if (!plug.isNull())
                {
                    plug.setValue(strAttrObject);
                }
            }
        }
        break;

        // MFnStringArrayData
        case Alembic::Util::kWstringPOD:
        {
            if (!isScalarLike)
            {
                MFnStringArrayData fnData;
                MObject arrObj;

                if (iProp.isConstant())
                {
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    iProp.get(samp);

                    unsigned int sampSize = (unsigned int)samp->size();
                    MStringArray arr;
                    arr.setLength(sampSize);

                    Alembic::Util::wstring * strData =
                        (Alembic::Util::wstring *) samp->getData();

                    for (unsigned int i = 0; i < sampSize; ++i)
                    {
                        arr[i] = (wchar_t *)(strData[i].c_str());
                    }
                    arrObj = fnData.create(arr);

                    if (!plug.isNull())
                    {
                        plug.setValue(arrObj);
                        attrsList << ";" << attrName;

                        return true;
                    }
                }
                else
                {
                    MStringArray arr;
                    arrObj = fnData.create(arr);
                }

                attrObj = typedAttr.create(attrName, attrName,
                    MFnData::kStringArray, arrObj);
            }
            // isScalarLike
            else
            {
                if (extent != 1)
                {
                    return false;
                }

                MFnStringData fnStringData;
                MObject strAttrObject;

                if (iProp.isConstant())
                {
                    Alembic::AbcCoreAbstract::ArraySamplePtr samp;
                    iProp.get(samp);
                    MString attrValue(
                        ((Alembic::Util::wstring *)samp->getData())[0].c_str());
                    strAttrObject = fnStringData.create(attrValue);
                    if (!plug.isNull())
                    {
                        plug.setValue(strAttrObject);
                        attrsList << ";" << attrName;

                        return true;
                    }
                }
                else
                {
                    MString attrValue;
                    strAttrObject = fnStringData.create(attrValue);
                }

                attrObj = typedAttr.create(attrName, attrName, MFnData::kString,
                        MObject::kNullObj);

                parentFn.addAttribute(attrObj,  MFnDependencyNode::kLocalDynamicAttr);

                plug = parentFn.findPlug(attrName);
                if (!plug.isNull())
                {
                    plug.setValue(strAttrObject);
                }
            }
        }
        break;

        default:
        {
            // Not sure what to do with kFloat16POD, kInt64POD, kUInt64POD
            // so we'll just skip them for now
            return false;
        }
        break;
    }

    typedAttr.setKeyable(true);
    numAttr.setKeyable(true);
    typedAttr.setWritable(isWritable);
    numAttr.setWritable(isWritable);
    typedAttr.setReadable(true);
    numAttr.setReadable(true);

    if (isScalarLike && interp == "rgb")
    {
        typedAttr.setUsedAsColor(true);
        numAttr.setUsedAsColor(true);
    }

    if ( ! parentFn.hasAttribute( attrName ) )
    {
        parentFn.addAttribute(attrObj,  MFnDependencyNode::kLocalDynamicAttr);
    }

    addArbAttrAndScope(iParent, iProp.getName(),
        iProp.getMetaData().get("geoScope"), interp, extent);

    plug = parentFn.findPlug(attrName);
    if (!plug.isNull()) {
        attrsList << ";" << attrName;
    }

    return true;
}

template <class PODTYPE>
AddPropResult
addScalarExtentOneProp(Alembic::Abc::IScalarProperty& iProp,
                       Alembic::Util::uint8_t extent,
                       PODTYPE defaultVal,
                       MPlug& plug,
                       MString& attrName,
                       MFnNumericAttribute& numAttr,
                       MObject& attrObj,
                       MFnNumericData::Type type)
{
    if (extent != 1)
        return INVALID;

    static const Alembic::Abc::ISampleSelector iss((Alembic::Abc::index_t)0);

    PODTYPE val = defaultVal;
    if (iProp.isConstant())
        iProp.get(&val, iss);

    if (plug.isNull())
    {
        attrObj = numAttr.create(attrName, attrName, type, val);
    }
    else
    {
        plug.setValue(val);
        return VALID_DONE;
    }

    return VALID_NOTDONE;
}

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
                         MFnNumericData::Type type3)
{
    if (extent > 3)
        return INVALID;

    static const Alembic::Abc::ISampleSelector iss((Alembic::Abc::index_t)0);

    PODTYPE val[3] = {defaultVal, defaultVal, defaultVal};

    if (iProp.isConstant())
        iProp.get(&val, iss);

    if (!plug.isNull())
    {
        unsigned int numChildren = plug.numChildren();
        if (numChildren == 0)
        {
            plug.setValue(val[0]);
        }
        else
        {
            if (numChildren > extent)
                numChildren = extent;

            for (unsigned int i = 0; i < numChildren; ++i)
                plug.child(i).setValue(val[i]);
        }
        return VALID_DONE;
    }
    else if (extent == 1)
    {
        attrObj = numAttr.create(attrName, attrName, type1);
        numAttr.setDefault(val[0]);
    }
    else if (extent == 2)
    {
        attrObj = numAttr.create(attrName, attrName, type2);
        numAttr.setDefault(val[0], val[1]);
    }
    else if (extent == 3)
    {
        attrObj = numAttr.create(attrName, attrName, type3);
        numAttr.setDefault(val[0], val[1], val[2]);
    }

    return VALID_NOTDONE;
}

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
                        MFnNumericData::Type type4)
{
    if (extent > 4)
        return INVALID;

    static const Alembic::Abc::ISampleSelector iss((Alembic::Abc::index_t)0);

    PODTYPE val[4] = {defaultVal, defaultVal, defaultVal, defaultVal};

    if (iProp.isConstant())
        iProp.get(&val, iss);

    if (!plug.isNull())
    {
        unsigned int numChildren = plug.numChildren();
        if (numChildren == 0)
        {
            plug.setValue(val[0]);
        }
        else
        {
            if (numChildren > extent)
                numChildren = extent;

            for (unsigned int i = 0; i < numChildren; ++i)
            {
                plug.child(i).setValue(val[i]);
            }
        }

        return VALID_DONE;
    }
    else if (extent == 1)
    {
        attrObj = numAttr.create(attrName, attrName, type1);
        numAttr.setDefault(val[0]);
    }
    else if (extent == 2)
    {
        attrObj = numAttr.create(attrName, attrName, type2);
        numAttr.setDefault(val[0], val[1]);
    }
    else if (extent == 3)
    {
        attrObj = numAttr.create(attrName, attrName, type3);
        numAttr.setDefault(val[0], val[1], val[2]);
    }
    else if (extent == 4)
    {
        attrObj = numAttr.create(attrName, attrName, type4);
        numAttr.setDefault(val[0], val[1], val[2], val[4]);
    }

    return VALID_NOTDONE;
}

bool addScalarProp(Alembic::Abc::IScalarProperty & iProp, MObject & iParent,
        bool isWritable,
        std::ostringstream &attrsList)
{
    MFnDependencyNode parentFn(iParent);
    MString attrName(iProp.getName().c_str());
    MPlug plug = parentFn.findPlug(attrName);

    MFnTypedAttribute typedAttr;
    MFnNumericAttribute numAttr;

    MObject attrObj;
    Alembic::AbcCoreAbstract::DataType dtype = iProp.getDataType();
    Alembic::Util::uint8_t extent = dtype.getExtent();
    std::string interp = iProp.getMetaData().get("interpretation");

    switch (dtype.getPod())
    {
      case Alembic::Util::kBooleanPOD:
      {
          AddPropResult result = addScalarExtentOneProp<bool>
              (iProp, extent, false, plug, attrName, numAttr, attrObj,
               MFnNumericData::kBoolean);

          if (result == INVALID) {
              return false;
          }

          if (result == VALID_DONE) {
              attrsList << ";" << attrName;
              return true;
          }
      }
      break;

      case Alembic::Util::kUint8POD:
      case Alembic::Util::kInt8POD:
      {
          AddPropResult result = addScalarExtentOneProp<Alembic::Util::int8_t>
              (iProp, extent, 1, plug, attrName, numAttr, attrObj,
               MFnNumericData::kByte);

          if (result == INVALID) {
              return false;
          }

          if (result == VALID_DONE) {
              attrsList << ";" << attrName;
              return true;
          }
      }
      break;

      case Alembic::Util::kInt16POD:
      case Alembic::Util::kUint16POD:
      {
          AddPropResult result = addScalarExtentThreeProp<Alembic::Util::int16_t>
              (iProp, extent, 0, plug, attrName, numAttr, attrObj,
               MFnNumericData::kShort,
               MFnNumericData::k2Short,
               MFnNumericData::k3Short);

          if (result == INVALID) {
              return false;
          }

          if (result == VALID_DONE) {
              attrsList << ";" << attrName;
              return true;
          }
      }
      break;

      case Alembic::Util::kUint32POD:
      case Alembic::Util::kInt32POD:
      {
          AddPropResult result = addScalarExtentThreeProp<Alembic::Util::int32_t>
              (iProp, extent, 0, plug, attrName, numAttr, attrObj,
               MFnNumericData::kInt,
               MFnNumericData::k2Int,
               MFnNumericData::k3Int);

          if (result == INVALID) {
              return false;
          }

          if (result == VALID_DONE) {
              attrsList << ";" << attrName;
              return true;
          }
      }
      break;

      case Alembic::Util::kFloat32POD:
      {
          AddPropResult result = addScalarExtentThreeProp<float>
              (iProp, extent, 0.f, plug, attrName, numAttr, attrObj,
               MFnNumericData::kFloat,
               MFnNumericData::k2Float,
               MFnNumericData::k3Float);

          if (result == INVALID) {
              return false;
          }

          if (result == VALID_DONE) {
              attrsList << ";" << attrName;
              return true;
          }
      }
      break;

      case Alembic::Util::kFloat64POD:
      {
          AddPropResult result = addScalarExtentFourProp<double>
              (iProp, extent, 0.f, plug, attrName, numAttr, attrObj,
               MFnNumericData::kDouble,
               MFnNumericData::k2Double,
               MFnNumericData::k3Double,
               MFnNumericData::k4Double);

          if (result == INVALID) {
              return false;
          }

          if (result == VALID_DONE) {
              attrsList << ";" << attrName;
              return true;
          }
      }
      break;

      case Alembic::Util::kStringPOD:
      {
          if (extent != 1)
              return false;

          MFnStringData fnStringData;
          MObject strAttrObject;

          if (iProp.isConstant())
          {
              Alembic::Abc::IStringProperty strProp( iProp.getPtr(),
                                                     Alembic::Abc::kWrapExisting );
              if (!strProp.valid())
                  return false;

              if (strProp.getNumSamples() == 0)
                  return false;

              static const Alembic::Abc::ISampleSelector iss((Alembic::Abc::index_t)0);
              std::string val = strProp.getValue(iss);

              MString attrValue(val.c_str());
              strAttrObject = fnStringData.create(attrValue);
              if (!plug.isNull())
              {
                  plug.setValue(strAttrObject);
                  attrsList << ";" << attrName;
                  return true;
              }
          }

          attrObj = typedAttr.create(attrName, attrName, MFnData::kString,
                        MObject::kNullObj);

          parentFn.addAttribute(attrObj,  MFnDependencyNode::kLocalDynamicAttr);

          plug = parentFn.findPlug(attrName);
          if (!plug.isNull())
          {
             plug.setValue(strAttrObject);
          }
      }
      break;

      default:
          std::cout << "Type not yet supported.\n";
          break;
    }

    typedAttr.setKeyable(true);
    numAttr.setKeyable(true);
    typedAttr.setWritable(isWritable);
    numAttr.setWritable(isWritable);
    typedAttr.setReadable(true);
    numAttr.setReadable(true);

    if (interp == "rgb")
    {
        typedAttr.setUsedAsColor(true);
        numAttr.setUsedAsColor(true);
    }

    if ( ! parentFn.hasAttribute( attrName ) )
    {
        parentFn.addAttribute(attrObj,  MFnDependencyNode::kLocalDynamicAttr);
    }

    addArbAttrAndScope(iParent, iProp.getName(),
        iProp.getMetaData().get("geoScope"), interp, extent);

    plug = parentFn.findPlug(attrName);
    if (!plug.isNull()) {
        attrsList << ";" << attrName;
    }

    return true;
}

//=============================================================================


std::string addProps(Alembic::Abc::ICompoundProperty & iParent, MObject & iObject,
    bool iUnmarkedFaceVaryingColors, bool areWritable)
{
    // if the params CompoundProperty (.arbGeomParam or .userProperties)
    // aren't valid, then skip
    if (!iParent)
        return "";

    std::ostringstream attrsList;
    std::size_t numProps = iParent.getNumProperties();
    for (std::size_t i = 0; i < numProps; ++i)
    {
        const Alembic::Abc::PropertyHeader & propHeader =
            iParent.getPropertyHeader(i);

        const std::string & propName = propHeader.getName();

        if (propName.empty() || propName.find('[') != std::string::npos)
        {
            MString warn = "Skipping oddly named property: ";
            warn += propName.c_str();

            MGlobal::displayWarning(warn);
        }
        // Skip attributes that we deal with elsewhere
        else if (propName[0] != '.')
        {
            if (propHeader.isArray())
            {
                Alembic::Abc::IArrayProperty prop(iParent, propName);
                if (prop.getNumSamples() == 0)
                {
                    MString warn = "Skipping property with no samples: ";
                    warn += propName.c_str();

                    MGlobal::displayWarning(warn);
                }

                if (!addArrayProp(prop, iObject, areWritable, attrsList))
                {
                    unsupportedWarning<Alembic::Abc::IArrayProperty>(prop);
                }
            }
            else if (propHeader.isScalar())
            {
                Alembic::Abc::IScalarProperty prop(iParent, propName);
                if (prop.getNumSamples() == 0)
                {
                    MString warn = "Skipping property with no samples: ";
                    warn += propName.c_str();

                    MGlobal::displayWarning(warn);
                }

                if (!addScalarProp(prop, iObject, areWritable, attrsList))
                {
                    unsupportedWarning<Alembic::Abc::IScalarProperty>(prop);
                }
            }
        }
    }

    std::string result = attrsList.str();
    // Remove the leading ';'
    result.erase(0, 1);
    return result;
}

//=============================================================================

void readProp(double iFrame,
              Alembic::Abc::IArrayProperty & iProp,
              MDataHandle & iHandle)
{
    MObject attrObj;
    Alembic::AbcCoreAbstract::DataType dtype = iProp.getDataType();
    Alembic::Util::uint8_t extent = dtype.getExtent();

    Alembic::AbcCoreAbstract::ArraySamplePtr samp, ceilSamp;

    const SampleInfo &sampleInfo =
      getSampleInfo(iFrame, iProp.getTimeSampling(), iProp.getNumSamples());
    const Alembic::AbcCoreAbstract::index_t index = sampleInfo.floorIndex;
    const Alembic::AbcCoreAbstract::index_t ceilIndex = sampleInfo.ceilIndex;
    const double alpha = sampleInfo.alpha;

    bool isScalarLike = iProp.isScalarLike() &&
        iProp.getMetaData().get("isArray") != "1";

    switch (dtype.getPod())
    {
        case Alembic::Util::kBooleanPOD:
        {
            if (!isScalarLike || extent != 1)
            {
                return;
            }

            iProp.get(samp, index);
            Alembic::Util::bool_t val =
                ((Alembic::Util::bool_t *) samp->getData())[0];
            iHandle.setBool(val != false);
        }
        break;

        case Alembic::Util::kUint8POD:
        case Alembic::Util::kInt8POD:
        {
            if (!isScalarLike || extent != 1)
            {
                return;
            }

            Alembic::Util::int8_t val;

            if (index != ceilIndex && alpha != 0.0)
            {
                iProp.get(samp, index);
                iProp.get(ceilSamp, ceilIndex);
                Alembic::Util::int8_t lo =
                    ((Alembic::Util::int8_t *) samp->getData())[0];
                Alembic::Util::int8_t hi =
                    ((Alembic::Util::int8_t *) ceilSamp->getData())[0];
                val = simpleLerp<Alembic::Util::int8_t>(alpha, lo, hi);
            }
            else
            {
                iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                val = ((Alembic::Util::int8_t *) samp->getData())[0];
            }

            iHandle.setChar(val);
        }
        break;

        case Alembic::Util::kInt16POD:
        case Alembic::Util::kUint16POD:
        {
            Alembic::Util::int16_t val[3];

            if (index != ceilIndex && alpha != 0.0)
            {
                iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                iProp.get(ceilSamp, Alembic::Abc::ISampleSelector(ceilIndex));
                for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                {
                     val[i] = simpleLerp<Alembic::Util::int16_t>(alpha,
                        ((Alembic::Util::int16_t *)samp->getData())[i],
                        ((Alembic::Util::int16_t *)ceilSamp->getData())[i]);
                }
            }
            else
            {
                iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                {
                     val[i] = ((Alembic::Util::int16_t *)samp->getData())[i];
                }
            }

            if (extent == 1)
            {
                iHandle.setShort(val[0]);
            }
            else if (extent == 2)
            {
                iHandle.set2Short(val[0], val[1]);
            }
            else if (extent == 3)
            {
                iHandle.set3Short(val[0], val[1], val[2]);
            }
        }
        break;

        case Alembic::Util::kUint32POD:
        case Alembic::Util::kInt32POD:
        {
            if (isScalarLike && extent < 4)
            {
                Alembic::Util::int32_t val[3];

                if (index != ceilIndex && alpha != 0.0)
                {
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                    iProp.get(ceilSamp,
                        Alembic::Abc::ISampleSelector(ceilIndex));
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                         val[i] = simpleLerp<Alembic::Util::int32_t>(alpha,
                            ((Alembic::Util::int32_t *)samp->getData())[i],
                            ((Alembic::Util::int32_t *)ceilSamp->getData())[i]);
                    }
                }
                else
                {
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                         val[i] =
                            ((Alembic::Util::int32_t *) samp->getData())[i];
                    }
                }

                if (extent == 1)
                {
                    iHandle.setInt(val[0]);
                }
                else if (extent == 2)
                {
                    iHandle.set2Int(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    iHandle.set3Int(val[0], val[1], val[2]);
                }
            }
            else
            {
                MFnIntArrayData fnData;
                iProp.get(samp, Alembic::Abc::ISampleSelector(index));

                if (alpha != 0.0 && index != ceilIndex)
                {
                    iProp.get(ceilSamp,
                        Alembic::Abc::ISampleSelector(ceilIndex));

                    MIntArray arr((int *) samp->getData(),
                        static_cast<unsigned int>(samp->size()));
                    std::size_t sampSize = samp->size();

                    // size is different don't lerp
                    if (sampSize != ceilSamp->size())
                    {
                        attrObj = fnData.create(arr);
                    }
                    else
                    {
                        int * hi = (int *) ceilSamp->getData();
                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            arr[i] = simpleLerp<int>(alpha, arr[i], hi[i]);
                        }
                    }
                    attrObj = fnData.create(arr);
                }
                else
                {
                    MIntArray arr((int *) samp->getData(),
                        static_cast<unsigned int>(samp->size()));
                    attrObj = fnData.create(arr);
                }
            }
        }
        break;

        case Alembic::Util::kFloat32POD:
        {
            if (isScalarLike && extent < 4)
            {
                float val[3];

                if (index != ceilIndex && alpha != 0.0)
                {
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                    iProp.get(ceilSamp,
                        Alembic::Abc::ISampleSelector(ceilIndex));
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                        val[i] = simpleLerp<float>(alpha,
                            ((float *)samp->getData())[i],
                            ((float *)ceilSamp->getData())[i]);
                    }
                }
                else
                {
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                        val[i] = ((float *)samp->getData())[i];
                    }
                }

                if (extent == 1)
                {
                    iHandle.setFloat(val[0]);
                }
                else if (extent == 2)
                {
                    iHandle.set2Float(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    iHandle.set3Float(val[0], val[1], val[2]);
                }
            }
            else
            {
                std::string interp = iProp.getMetaData().get("interpretation");

                if ((extent == 2 || extent == 3) && (interp == "normal" ||
                    interp == "vector" || interp == "rgb"))
                {
                    MFnVectorArrayData fnData;
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                    unsigned int sampSize = (unsigned int)samp->size();
                    MVectorArray arr(sampSize);
                    MVector vec;

                    if (alpha != 0.0 && index != ceilIndex)
                    {
                        iProp.get(ceilSamp,
                            Alembic::Abc::ISampleSelector(ceilIndex));

                        // size is different don't lerp
                        if (sampSize != ceilSamp->size())
                        {
                            float * vals = (float *) samp->getData();
                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                vec.x = vals[extent*i];
                                vec.y = vals[extent*i+1];

                                if (extent == 3)
                                {
                                    vec.z = vals[extent*i+2];
                                }
                                arr[i] = vec;
                            }

                            attrObj = fnData.create(arr);
                        }
                        else
                        {
                            float * lo = (float *) samp->getData();
                            float * hi = (float *) ceilSamp->getData();

                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                vec.x = simpleLerp<double>(alpha,
                                    lo[extent*i], hi[extent*i]);

                                vec.y = simpleLerp<double>(alpha,
                                    lo[extent*i+1], hi[extent*i+1]);

                                if (extent == 3)
                                {
                                    vec.z = simpleLerp<double>(alpha,
                                        lo[extent*i+2], hi[extent*i+2]);
                                }
                                arr[i] = vec;
                            }
                        }
                        attrObj = fnData.create(arr);
                    }
                    else
                    {
                        float * vals = (float *) samp->getData();
                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            vec.x = vals[extent*i];
                            vec.y = vals[extent*i+1];

                            if (extent == 3)
                            {
                                vec.z = vals[extent*i+2];
                            }
                            arr[i] = vec;
                        }
                        attrObj = fnData.create(arr);
                    }
                }
                else if (interp == "point" && (extent == 2 || extent == 3))
                {
                    MFnPointArrayData fnData;
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));

                    unsigned int sampSize = (unsigned int)samp->size();
                    MPointArray arr(sampSize);
                    MPoint pt;

                    if (alpha != 0.0 && index != ceilIndex)
                    {
                        iProp.get(ceilSamp,
                            Alembic::Abc::ISampleSelector(ceilIndex));

                        // size is different don't lerp
                        if (sampSize != ceilSamp->size())
                        {
                            float * vals = (float *) samp->getData();
                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                pt.x = vals[extent*i];
                                pt.y = vals[extent*i+1];

                                if (extent == 3)
                                {
                                    pt.z = vals[extent*i+2];
                                }
                                arr[i] = pt;
                            }

                            attrObj = fnData.create(arr);
                        }
                        else
                        {
                            float * lo = (float *) samp->getData();
                            float * hi = (float *) ceilSamp->getData();

                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                pt.x = simpleLerp<double>(alpha,
                                    lo[extent*i], hi[extent*i]);

                                pt.y = simpleLerp<double>(alpha,
                                    lo[extent*i+1], hi[extent*i+1]);

                                if (extent == 3)
                                {
                                    pt.z = simpleLerp<double>(alpha,
                                        lo[extent*i+2], hi[extent*i+2]);
                                }
                                arr[i] = pt;
                            }
                            attrObj = fnData.create(arr);
                        }
                    }
                    else
                    {
                        float * vals = (float *) samp->getData();
                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            pt.x = vals[extent*i];
                            pt.y = vals[extent*i+1];

                            if (extent == 3)
                            {
                                pt.z = vals[extent*i+2];
                            }
                            arr[i] = pt;
                        }
                        attrObj = fnData.create(arr);
                    }
                }
                else
                {
                    MFnDoubleArrayData fnData;
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));

                    if (alpha != 0.0 && index != ceilIndex)
                    {
                        iProp.get(ceilSamp,
                            Alembic::Abc::ISampleSelector(ceilIndex));

                        MDoubleArray arr((float *) samp->getData(),
                            static_cast<unsigned int>(samp->size()));
                        std::size_t sampSize = samp->size();

                        // size is different don't lerp
                        if (sampSize != ceilSamp->size())
                        {
                            attrObj = fnData.create(arr);
                        }
                        else
                        {
                            float * hi = (float *) ceilSamp->getData();
                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                arr[i] = simpleLerp<double>(alpha, arr[i],
                                    hi[i]);
                            }
                        }
                        attrObj = fnData.create(arr);
                    }
                    else
                    {
                        MDoubleArray arr((float *) samp->getData(),
                            static_cast<unsigned int>(samp->size()));
                        attrObj = fnData.create(arr);
                    }
                }
            }
        }
        break;

        case Alembic::Util::kFloat64POD:
        {
            // need to differentiate between vectors, points, and color array?
            if (isScalarLike && extent < 5)
            {
                double val[4];

                if (index != ceilIndex && alpha != 0.0)
                {
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));

                    iProp.get(ceilSamp,
                        Alembic::Abc::ISampleSelector(ceilIndex));
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                         val[i] = simpleLerp<double>(alpha,
                            ((double *)(samp->getData()))[i],
                            ((double *)(ceilSamp->getData()))[i]);
                    }
                }
                else
                {
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                         val[i] = ((double *)(samp->getData()))[i];
                    }
                }

                if (extent == 1)
                {
                    iHandle.setDouble(val[0]);
                }
                else if (extent == 2)
                {
                    iHandle.set2Double(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    iHandle.set3Double(val[0], val[1], val[2]);
                }
                else if (extent == 4)
                {
                    MFnNumericData numData;
                    numData.create(MFnNumericData::k4Double);
                    numData.setData4Double(val[0], val[1], val[2], val[3]);
                    iHandle.setMObject(numData.object());
                }
            }
            else
            {
                std::string interp = iProp.getMetaData().get("interpretation");

                if ((extent == 2 || extent == 3) && (interp == "normal" ||
                    interp == "vector" || interp == "rgb"))
                {
                    MFnVectorArrayData fnData;
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                    unsigned int sampSize = (unsigned int)samp->size();
                    MVectorArray arr(sampSize);
                    MVector vec;

                    if (alpha != 0.0 && index != ceilIndex)
                    {
                        iProp.get(ceilSamp,
                            Alembic::Abc::ISampleSelector(ceilIndex));

                        // size is different don't lerp
                        if (sampSize != ceilSamp->size())
                        {
                            double * vals = (double *) samp->getData();
                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                vec.x = vals[extent*i];
                                vec.y = vals[extent*i+1];

                                if (extent == 3)
                                {
                                    vec.z = vals[extent*i+2];
                                }
                                arr[i] = vec;
                            }

                            attrObj = fnData.create(arr);
                        }
                        else
                        {
                            double * lo = (double *) samp->getData();
                            double * hi = (double *) ceilSamp->getData();

                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                vec.x = simpleLerp<double>(alpha,
                                    lo[extent*i], hi[extent*i]);

                                vec.y = simpleLerp<double>(alpha,
                                    lo[extent*i+1], hi[extent*i+1]);

                                if (extent == 3)
                                {
                                    vec.z = simpleLerp<double>(alpha,
                                        lo[extent*i+2], hi[extent*i+2]);
                                }
                                arr[i] = vec;
                            }
                        }
                        attrObj = fnData.create(arr);
                    }
                    else
                    {
                        double * vals = (double *) samp->getData();
                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            vec.x = vals[extent*i];
                            vec.y = vals[extent*i+1];

                            if (extent == 3)
                            {
                                vec.z = vals[extent*i+2];
                            }
                            arr[i] = vec;
                        }
                        attrObj = fnData.create(arr);
                    }
                }
                else if (interp == "point" && (extent == 2 || extent == 3))
                {
                    MFnPointArrayData fnData;
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                    unsigned int sampSize = (unsigned int)samp->size();
                    MPointArray arr(sampSize);
                    MPoint pt;

                    if (alpha != 0.0 && index != ceilIndex)
                    {
                        iProp.get(ceilSamp,
                            Alembic::Abc::ISampleSelector(ceilIndex));

                        // size is different don't lerp
                        if (sampSize != ceilSamp->size())
                        {
                            double * vals = (double *) samp->getData();
                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                pt.x = vals[extent*i];
                                pt.y = vals[extent*i+1];

                                if (extent == 3)
                                {
                                    pt.z = vals[extent*i+2];
                                }
                                arr[i] = pt;
                            }

                            attrObj = fnData.create(arr);
                        }
                        else
                        {
                            double * lo = (double *) samp->getData();
                            double * hi = (double *) ceilSamp->getData();

                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                pt.x = simpleLerp<double>(alpha,
                                    lo[extent*i], hi[extent*i]);

                                pt.y = simpleLerp<double>(alpha,
                                    lo[extent*i+1], hi[extent*i+1]);

                                if (extent == 3)
                                {
                                    pt.z = simpleLerp<double>(alpha,
                                        lo[extent*i+2], hi[extent*i+2]);
                                }
                                arr[i] = pt;
                            }
                            attrObj = fnData.create(arr);
                        }
                    }
                    else
                    {
                        double * vals = (double *) samp->getData();
                        for (unsigned int i = 0; i < sampSize; ++i)
                        {
                            pt.x = vals[extent*i];
                            pt.y = vals[extent*i+1];

                            if (extent == 3)
                            {
                                pt.z = vals[extent*i+2];
                            }
                            arr[i] = pt;
                        }
                        attrObj = fnData.create(arr);
                    }
                }
                else
                {
                    MFnDoubleArrayData fnData;
                    iProp.get(samp, Alembic::Abc::ISampleSelector(index));

                    if (alpha != 0.0 && index != ceilIndex)
                    {
                        iProp.get(ceilSamp,
                            Alembic::Abc::ISampleSelector(ceilIndex));

                        MDoubleArray arr((double *) samp->getData(),
                            static_cast<unsigned int>(samp->size()));
                        std::size_t sampSize = samp->size();

                        // size is different don't lerp
                        if (sampSize != ceilSamp->size())
                        {
                            attrObj = fnData.create(arr);
                        }
                        else
                        {
                            double * hi = (double *) ceilSamp->getData();
                            for (unsigned int i = 0; i < sampSize; ++i)
                            {
                                arr[i] = simpleLerp<double>(alpha, arr[i],
                                    hi[i]);
                            }
                        }
                        attrObj = fnData.create(arr);
                    }
                    else
                    {
                        MDoubleArray arr((double *) samp->getData(),
                            static_cast<unsigned int>(samp->size()));
                        attrObj = fnData.create(arr);
                    }
                }
            }
        }
        break;

        // MFnStringArrayData
        case Alembic::Util::kStringPOD:
        {
            if (isScalarLike && extent == 1)
            {
                iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                iHandle.setString(
                    ((Alembic::Util::string *)samp->getData())[0].c_str());
            }
            else
            {
                MFnStringArrayData fnData;
                iProp.get(samp, Alembic::Abc::ISampleSelector(index));

                unsigned int sampSize = (unsigned int)samp->size();
                MStringArray arr;
                arr.setLength(sampSize);
                attrObj = fnData.create(arr);
                Alembic::Util::string * strData =
                    (Alembic::Util::string *) samp->getData();

                for (unsigned int i = 0; i < sampSize; ++i)
                {
                    arr[i] = strData[i].c_str();
                }
            }
        }
        break;

        // MFnStringArrayData
        case Alembic::Util::kWstringPOD:
        {
            if (isScalarLike && extent == 1)
            {
                iProp.get(samp, Alembic::Abc::ISampleSelector(index));
                iHandle.setString(
                    ((Alembic::Util::wstring *)samp->getData())[0].c_str());
            }
            else
            {
                MFnStringArrayData fnData;
                iProp.get(samp, Alembic::Abc::ISampleSelector(index));

                unsigned int sampSize = (unsigned int)samp->size();
                MStringArray arr;
                arr.setLength(sampSize);
                attrObj = fnData.create(arr);
                Alembic::Util::wstring * strData =
                    (Alembic::Util::wstring *) samp->getData();

                for (unsigned int i = 0; i < sampSize; ++i)
                {
                    arr[i] = (wchar_t *)strData[i].c_str();
                }
            }
        }
        break;

        default:
        break;
    }

    if (!attrObj.isNull())
        iHandle.set(attrObj);
}

void readProp(double iFrame,
              Alembic::Abc::IScalarProperty & iProp,
              MDataHandle & iHandle)
{
    MObject attrObj;
    Alembic::AbcCoreAbstract::DataType dtype = iProp.getDataType();
    Alembic::Util::uint8_t extent = dtype.getExtent();

    const SampleInfo &sampleInfo =
      getSampleInfo(iFrame, iProp.getTimeSampling(), iProp.getNumSamples());
    const Alembic::AbcCoreAbstract::index_t index = sampleInfo.floorIndex;
    const Alembic::AbcCoreAbstract::index_t ceilIndex = sampleInfo.ceilIndex;
    const double alpha = sampleInfo.alpha;

    switch (dtype.getPod())
    {
        case Alembic::Util::kBooleanPOD:
        {
            if (extent != 1)
                return;

            Alembic::Util::bool_t val;
            iProp.get(&val, index);

            iHandle.setBool(val != false);
        }
        break;

        case Alembic::Util::kUint8POD:
        case Alembic::Util::kInt8POD:
        {
            if (extent != 1)
                return;

            Alembic::Util::int8_t val;

            if (index != ceilIndex && alpha != 0.0)
            {
                Alembic::Util::int8_t lo;
                Alembic::Util::int8_t hi;

                iProp.get(&lo, index);
                iProp.get(&hi, ceilIndex);
                val = simpleLerp<Alembic::Util::int8_t>(alpha, lo, hi);
            }
            else
            {
                iProp.get(&val, index);
            }

            iHandle.setChar(val);
        }
        break;

        case Alembic::Util::kInt16POD:
        case Alembic::Util::kUint16POD:
        {
            Alembic::Util::int16_t val[3];

            if (index != ceilIndex && alpha != 0.0)
            {
                Alembic::Util::int16_t lo[3];
                Alembic::Util::int16_t hi[3];

                iProp.get(lo, index);
                iProp.get(hi, ceilIndex);

                for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                {
                    val[i] = simpleLerp<Alembic::Util::int16_t>(alpha,
                                                                lo[i],
                                                                hi[i]);
                }
            }
            else
            {
                iProp.get(val, index);
            }

            if (extent == 1)
            {
                iHandle.setShort(val[0]);
            }
            else if (extent == 2)
            {
                iHandle.set2Short(val[0], val[1]);
            }
            else if (extent == 3)
            {
                iHandle.set3Short(val[0], val[1], val[2]);
            }
        }
        break;

        case Alembic::Util::kUint32POD:
        case Alembic::Util::kInt32POD:
        {
            if (extent < 4)
            {
                Alembic::Util::int32_t val[3];

                if (index != ceilIndex && alpha != 0.0)
                {
                    Alembic::Util::int32_t lo[3];
                    Alembic::Util::int32_t hi[3];

                    iProp.get(lo, index);
                    iProp.get(hi, ceilIndex);

                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                    {
                         val[i] = simpleLerp<Alembic::Util::int32_t>(alpha,
                                                                     lo[i],
                                                                     hi[i]);
                    }
                }
                else
                {
                    iProp.get(val, index);
                }

                if (extent == 1)
                {
                    iHandle.setInt(val[0]);
                }
                else if (extent == 2)
                {
                    iHandle.set2Int(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    iHandle.set3Int(val[0], val[1], val[2]);
                }
            }
        }
        break;

        case Alembic::Util::kFloat32POD:
        {
            if (extent < 4)
            {
                float val[3];

                if (index != ceilIndex && alpha != 0.0)
                {
                    float lo[3];
                    float hi[3];

                    iProp.get(lo, index);
                    iProp.get(hi, ceilIndex);

                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                        val[i] = simpleLerp<float>(alpha, lo[i], hi[i]);
                }
                else
                {
                    iProp.get(val, index);
                }

                if (extent == 1)
                {
                    iHandle.setFloat(val[0]);
                }
                else if (extent == 2)
                {
                    iHandle.set2Float(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    iHandle.set3Float(val[0], val[1], val[2]);
                }
            }
        }
        break;

        case Alembic::Util::kFloat64POD:
        {
            // need to differentiate between vectors, points, and color array?
            if (extent < 5)
            {
                double val[4];

                if (index != ceilIndex && alpha != 0.0)
                {
                    double lo[4];
                    double hi[4];

                    iProp.get(lo, index);
                    iProp.get(hi, index);

                    for (Alembic::Util::uint8_t i = 0; i < extent; ++i)
                        val[i] = simpleLerp<double>(alpha, lo[i], hi[i]);
                }
                else
                {
                    iProp.get(val, index);
                }

                if (extent == 1)
                {
                    iHandle.setDouble(val[0]);
                }
                else if (extent == 2)
                {
                    iHandle.set2Double(val[0], val[1]);
                }
                else if (extent == 3)
                {
                    iHandle.set3Double(val[0], val[1], val[2]);
                }
                else if (extent == 4)
                {
                    MFnNumericData numData;
                    numData.create(MFnNumericData::k4Double);
                    numData.setData4Double(val[0], val[1], val[2], val[3]);
                    iHandle.setMObject(numData.object());
                }
            }
        }
        break;

        // MFnStringArrayData
        case Alembic::Util::kStringPOD:
        {
            if (extent == 1)
            {
                Alembic::Abc::IStringProperty strProp( iProp.getPtr(),
                                                       Alembic::Abc::kWrapExisting );
                iHandle.setString(strProp.getValue(index).c_str());
            }
        }
        break;

        // MFnStringArrayData
        case Alembic::Util::kWstringPOD:
        {
            if (extent == 1)
            {
                Alembic::Abc::IWstringProperty strProp( iProp.getPtr(),
                                                        Alembic::Abc::kWrapExisting );
                iHandle.setString(strProp.getValue(index).c_str());
            }
        }
        break;

        default:
        break;
    }

    if (!attrObj.isNull())
        iHandle.set(attrObj);
}

void readProps(double iFrame,
              Alembic::Abc::ICompoundProperty & iParent,
              MDataBlock & iDataBlock,
              const MObject & iNode)
{
    // if the params CompoundProperty (.arbGeomParam or .userProperties)
    // aren't valid, then skip
    if (!iParent)
        return;

    MStatus status;
    MFnDependencyNode depNode(iNode, &status);
    if (status != MStatus::kSuccess) {
        MGlobal::displayWarning("Unable to read properties");
        return;
    }

    std::size_t numProps = iParent.getNumProperties();
    for (std::size_t i = 0; i < numProps; ++i)
    {
        const Alembic::Abc::PropertyHeader & propHeader =
            iParent.getPropertyHeader(i);

        const std::string & propName = propHeader.getName();

        if (propName.empty() || propName.find('[') != std::string::npos)
        {
            MString warn = "Skipping oddly named property: ";
            warn += propName.c_str();

            MGlobal::displayWarning(warn);
            continue;
        }

        // Skip attributes that we deal with elsewhere
        if (propName[0] == '.') {
          continue;
        }

        MPlug plug = depNode.findPlug(propName.c_str(), true, &status);
        if (status != MStatus::kSuccess) {
            MGlobal::displayWarning("Skipping new property " + depNode.name() + "." + MString(propName.c_str()));
            continue;
        }

        MDataHandle handle = iDataBlock.outputValue(plug, &status);
        if (status != MStatus::kSuccess) {
            MGlobal::displayWarning("Unable to get data block!");
            continue;
        }

        if (propHeader.isArray())
        {
            Alembic::Abc::IArrayProperty prop(iParent, propName);
            if (prop.getNumSamples() == 0)
            {
                MString warn = "Skipping property with no samples: ";
                warn += propName.c_str();

                MGlobal::displayWarning(warn);
            }

            readProp(iFrame, prop, handle);
        }
        else if (propHeader.isScalar())
        {
            Alembic::Abc::IScalarProperty prop(iParent, propName);
            if (prop.getNumSamples() == 0)
            {
                MString warn = "Skipping property with no samples: ";
                warn += propName.c_str();

                MGlobal::displayWarning(warn);
            }

            readProp(iFrame, prop, handle);
        }
    }
}
