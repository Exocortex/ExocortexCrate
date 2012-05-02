#include "Alembic.h"
#include "AlembicMax.h"
#include "AlembicPoints.h"
#include "AlembicXform.h"
#include "SceneEnumProc.h"
#include "Utility.h"
#include <IParticleObjectExt.h>
#include <ParticleFlow/IParticleChannelID.h>
#include <ParticleFlow/IParticleChannelShape.h>
#include <ParticleFlow/IParticleContainer.h>
#include <ParticleFlow/IParticleGroup.h>
#include <ParticleFlow/IPFSystem.h>
#include <ParticleFlow/IPFActionList.h>
#include <ParticleFlow/PFSimpleOperator.h>
#include <ParticleFlow/IParticleChannels.h>
#include <ParticleFlow/IChannelContainer.h>
#include <ParticleFlow/IParticleChannelLifespan.h>
#include <ifnpub.h>
#include <ImathMatrixAlgo.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

#define PARTICLECHANNELLOCALOFFSETR_INTERFACE Interface_ID(0x12ec5d1d, 0x1eb34500) 
#define GetParticleChannelLocalOffsetRInterface(obj) ((IParticleChannelIntR*)obj->GetInterface(PARTICLECHANNELLOCALOFFSETR_INTERFACE)) 

AlembicPoints::AlembicPoints(const SceneEntry &in_Ref, AlembicWriteJob *in_Job)
    : AlembicObject(in_Ref, in_Job)
{
    std::string pointsName = in_Ref.node->GetName();
    std::string xformName = pointsName + "Xfo";

    Alembic::AbcGeom::OXform xform(GetOParent(), xformName.c_str(), GetCurrentJob()->GetAnimatedTs());
    Alembic::AbcGeom::OPoints points(xform, pointsName.c_str(), GetCurrentJob()->GetAnimatedTs());

    // create the generic properties
    mOVisibility = CreateVisibilityProperty(points, GetCurrentJob()->GetAnimatedTs());

    mXformSchema = xform.getSchema();
    mPointsSchema = points.getSchema();

    // create all properties
    mInstanceNamesProperty = OStringArrayProperty(mPointsSchema, ".instancenames", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );

    // particle attributes
    mScaleProperty = OV3fArrayProperty(mPointsSchema, ".scale", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mOrientationProperty = OQuatfArrayProperty(mPointsSchema, ".orientation", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mAngularVelocityProperty = OQuatfArrayProperty(mPointsSchema, ".angularvelocity", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mAgeProperty = OFloatArrayProperty(mPointsSchema, ".age", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mMassProperty = OFloatArrayProperty(mPointsSchema, ".mass", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeTypeProperty = OUInt16ArrayProperty(mPointsSchema, ".shapetype", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeTimeProperty = OFloatArrayProperty(mPointsSchema, ".shapetime", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeInstanceIDProperty = OUInt16ArrayProperty(mPointsSchema, ".shapeinstanceid", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mColorProperty = OC4fArrayProperty(mPointsSchema, ".color", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
}

AlembicPoints::~AlembicPoints()
{
    // we have to clear this prior to destruction this is a workaround for issue-171
    mOVisibility.reset();
}

Alembic::Abc::OCompoundProperty AlembicPoints::GetCompound()
{
    return mPointsSchema;
}



bool AlembicPoints::Save(double time)
{
    // Note: Particles are always considered to be animated even though
    //       the node does not have the IsAnimated() flag.

    // Extract our particle emitter at the given time
    TimeValue ticks = GetTimeValueFromFrame(time);
    Object *obj = GetRef().node->EvalWorldState(ticks).obj;

	bool bFlatten = GetCurrentJob()->GetOption("flattenHierarchy");

    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time, bFlatten);

    IPFSystem* particleSystem = PFSystemInterface(obj);
	IParticleObjectExt* particlesExt = GetParticleObjectExtInterface(obj);
	SimpleParticle* pSimpleParticle = (SimpleParticle*) obj->GetInterface(I_SIMPLEPARTICLEOBJ);
	if(!pSimpleParticle){

		particleSystem = PFSystemInterface(obj);
		if(!particleSystem){
			return false;
		}
		particlesExt = GetParticleObjectExtInterface(obj);
		if(!particlesExt){
			return false;
		}
	}
	//If we get here, either particlesExt (for particle flow system) is nonnull 
	//OR pSimpleParticle (for older particle systems) is nonnull 

	int numParticles = 0;
	if(particlesExt){
		particlesExt->UpdateParticles(GetRef().node, ticks);
		numParticles = particlesExt->NumParticles();
	}
	else if(pSimpleParticle){
		pSimpleParticle->Update(ticks, GetRef().node);
		numParticles = pSimpleParticle->parts.points.Count();
	}
	else{
		return false;
	}

    // Set the visibility
    float flVisibility = GetRef().node->GetLocalVisibility(ticks);
    mOVisibility.set(flVisibility > 0 ? Alembic::AbcGeom::kVisibilityVisible : Alembic::AbcGeom::kVisibilityHidden);

    // Store positions, velocity, width/size, scale, id, bounding box
    std::vector<Alembic::Abc::V3f> positionVec(numParticles);
    std::vector<Alembic::Abc::V3f> velocityVec(numParticles);
    std::vector<Alembic::Abc::V3f> scaleVec(numParticles);
    std::vector<float> widthVec(numParticles);
    std::vector<float> ageVec(numParticles);
    std::vector<float> massVec;
    std::vector<float> shapeTimeVec(numParticles);
    std::vector<uint64_t> idVec(numParticles);
    std::vector<uint16_t> shapeTypeVec(numParticles);
    std::vector<uint16_t> shapeInstanceIDVec(numParticles);
    std::vector<Alembic::Abc::Quatf> orientationVec(numParticles);
    std::vector<Alembic::Abc::Quatf> angularVelocityVec(numParticles);
    std::vector<Alembic::Abc::C4f> colorVec;
    std::vector<std::string> instanceNamesVec;
    Alembic::Abc::Box3d bbox;
    bool constantPos = true;
    bool constantVel = true;
    bool constantScale = true;
    bool constantWidth = true;
    bool constantAge = true;
    bool constantOrientation = true;
    bool constantAngularVel = true;

	//The MAX interfaces return everything in world coordinates,
	//so we need to multiply the inverse the node world transform matrix
    Matrix3 nodeWorldTM = GetRef().node->GetObjTMAfterWSM(ticks);
    // Convert the max transform to alembic
    Matrix3 alembicMatrix;
    ConvertMaxMatrixToAlembicMatrix(nodeWorldTM, alembicMatrix);
    Alembic::Abc::M44d nodeWorldTrans(	alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
										alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
										alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
										alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);
	Alembic::Abc::M44d nodeWorldTransInv = nodeWorldTrans.inverse();


	for (int i = 0; i < numParticles; ++i)
	{
		Imath::V3f pos(0.0);
		Imath::V3f vel(0.0);
		Imath::V3f scale(1.0);
		TimeValue age = 0;
		uint64_t id = 0;
	    Alembic::Abc::Quatd orientation(0.0, 0.0, 1.0, 0.0);
		Alembic::Abc::Quatd spin(0.0, 0.0, 1.0, 0.0);
		// Particle size is a uniform scale multiplier in XSI.  In Max, I need to learn where to get this 
		// For now, we'll just default to 1
		float width = 1.0f;

		ShapeType shapetype = ShapeType_Point;
		float shapeInstanceTime = time;
		uint16_t shapeInstanceId = 0;

		if(particlesExt){
			pos = ConvertMaxPointToAlembicPoint(*particlesExt->GetParticlePositionByIndex(i));
			vel = ConvertMaxVectorToAlembicVector(*particlesExt->GetParticleSpeedByIndex(i));
			scale = ConvertMaxScaleToAlembicScale(*particlesExt->GetParticleScaleXYZByIndex(i));
			ConvertMaxEulerXYZToAlembicQuat(*particlesExt->GetParticleOrientationByIndex(i), orientation);
			ConvertMaxAngAxisToAlembicQuat(*particlesExt->GetParticleSpinByIndex(i), spin);
			age = particlesExt->GetParticleAgeByIndex(i);
			id = particlesExt->GetParticleBornIndex(i);
			AlembicPoints::GetShapeType(particlesExt, i, ticks, shapetype, shapeInstanceId, shapeInstanceTime, instanceNamesVec);
		}
		else if(pSimpleParticle){
			pos = ConvertMaxPointToAlembicPoint(pSimpleParticle->ParticlePosition(ticks, i));
			vel = ConvertMaxVectorToAlembicVector(pSimpleParticle->ParticleVelocity(ticks, i));
			//simple particles have no scale?
			//simple particles have no orientation?
			age = pSimpleParticle->ParticleAge(ticks, i);
			//simple particles have born index
			width = pSimpleParticle->ParticleSize(ticks, i);
		}

		//move everything from world space to local space
		pos = pos * nodeWorldTransInv;
		vel = vel * nodeWorldTransInv;
		//scale = scale * nodeWorldTransInv;
		orientation = Imath::extractQuat(orientation.toMatrix44() * nodeWorldTransInv);
		spin = extractQuat(spin.toMatrix44() * nodeWorldTransInv);
		

		positionVec[i].setValue(pos.x, pos.y, pos.z);
		velocityVec[i].setValue(vel.x, vel.y, vel.z);
		scaleVec[i].setValue(scale.x, scale.y, scale.z);
		widthVec[i] = width;
		ageVec[i] = static_cast<float>(GetSecondsFromTimeValue(age));
		idVec[i] = id;
		orientationVec[i] = orientation;
		angularVelocityVec[i] = spin;
		bbox.extendBy(positionVec[i]);
        shapeTypeVec[i] = shapetype;
        shapeInstanceIDVec[i] = shapeInstanceId;
        shapeTimeVec[i] = shapeInstanceTime;

		constantPos &= (positionVec[i] == positionVec[0]);
		constantVel &= (velocityVec[i] == velocityVec[0]);
		constantScale &= (scaleVec[i] == scaleVec[0]);
		constantWidth &= (widthVec[i] == widthVec[0]);
		constantAge &= (ageVec[i] == ageVec[0]);
		constantOrientation &= (orientationVec[i] == orientationVec[0]);
		constantAngularVel &= (angularVelocityVec[i] == angularVelocityVec[0]);

		// Set the archive bounding box
		// Positions for particles are already cnsider to be in world space
		if (mJob)
		{
			mJob->GetArchiveBBox().extendBy(pos);
		}
	}
	
    // Set constant properties that are not currently supported by Max
    massVec.push_back(1.0f);
    shapeTimeVec.push_back(1.0f);
    shapeTypeVec.push_back(ShapeType_Point);
    shapeInstanceIDVec.push_back(0);
    colorVec.push_back(Alembic::Abc::C4f(0.0f, 0.0f, 0.0f, 1.0f));
    instanceNamesVec.push_back("");

    if (numParticles == 0)
    {
        positionVec.push_back(Alembic::Abc::V3f(FLT_MAX, FLT_MAX, FLT_MAX));
        velocityVec.push_back(Alembic::Abc::V3f(0.0f, 0.0f, 0.0f));
        scaleVec.push_back(Alembic::Abc::V3f(1.0f, 1.0f, 1.0f));
        widthVec.push_back(0.0f);
        ageVec.push_back(0.0f);
        idVec.push_back(static_cast<uint64_t>(-1));
        orientationVec.push_back(Alembic::Abc::Quatf(0.0f, 1.0f, 0.0f, 0.0f));
        angularVelocityVec.push_back(Alembic::Abc::Quatf(0.0f, 1.0f, 0.0f, 0.0f));
    }
    else
    {
        if (constantPos)        { positionVec.resize(1); }
        if (constantVel)        { velocityVec.resize(1); }
        if (constantScale)      { scaleVec.resize(1); }
        if (constantWidth)      { widthVec.resize(1); }
        if (constantAge)        { ageVec.resize(1); }
        if (constantOrientation){ orientationVec.resize(1); }
        if (constantAngularVel) { angularVelocityVec.resize(1); }
    }

    // Store the information into our properties and points schema
    Alembic::Abc::P3fArraySample positionSample(&positionVec.front(), positionVec.size());
    Alembic::Abc::P3fArraySample velocitySample(&velocityVec.front(), velocityVec.size());
    Alembic::Abc::P3fArraySample scaleSample(&scaleVec.front(), scaleVec.size());
    Alembic::Abc::FloatArraySample widthSample(&widthVec.front(), widthVec.size());
    Alembic::Abc::FloatArraySample ageSample(&ageVec.front(), ageVec.size());
    Alembic::Abc::FloatArraySample massSample(&massVec.front(), massVec.size());
    Alembic::Abc::FloatArraySample shapeTimeSample(&shapeTimeVec.front(), shapeTimeVec.size());
    Alembic::Abc::UInt64ArraySample idSample(&idVec.front(), idVec.size());
    Alembic::Abc::UInt16ArraySample shapeTypeSample(&shapeTypeVec.front(), shapeTypeVec.size());
    Alembic::Abc::UInt16ArraySample shapeInstanceIDSample(&shapeInstanceIDVec.front(), shapeInstanceIDVec.size());
    Alembic::Abc::QuatfArraySample orientationSample(&orientationVec.front(), orientationVec.size());
    Alembic::Abc::QuatfArraySample angularVelocitySample(&angularVelocityVec.front(), angularVelocityVec.size());
    Alembic::Abc::C4fArraySample colorSample(&colorVec.front(), colorVec.size());
    Alembic::Abc::StringArraySample instanceNamesSample(&instanceNamesVec.front(), instanceNamesVec.size());

    mScaleProperty.set(scaleSample);
    mAgeProperty.set(ageSample);
    mMassProperty.set(massSample);
    mShapeTimeProperty.set(shapeTimeSample);
    mShapeTypeProperty.set(shapeTypeSample);
    mShapeInstanceIDProperty.set(shapeInstanceIDSample);
    mOrientationProperty.set(orientationSample);
    mAngularVelocityProperty.set(angularVelocitySample);
    mColorProperty.set(colorSample);
    mInstanceNamesProperty.set(instanceNamesSample);

    mPointsSample.setPositions(positionSample);
    mPointsSample.setVelocities(velocitySample);
    mPointsSample.setWidths(Alembic::AbcGeom::OFloatGeomParam::Sample(widthSample, Alembic::AbcGeom::kVertexScope));
    mPointsSample.setIds(idSample);
    mPointsSample.setSelfBounds(bbox);
	mPointsSample.setChildBounds(bbox);

    mPointsSchema.set(mPointsSample);

    mNumSamples++;

    return true;
}

// static
void AlembicPoints::ConvertMaxEulerXYZToAlembicQuat(const Point3 &degrees, Alembic::Abc::Quatd &quat)
{
    // Get the angles as a float vector of radians - strangeley they already are even though the documentation says degrees
    float angles[] = { degrees.x, degrees.y, degrees.z };

    // Convert the angles to a quaternion
    Quat maxQuat;
    EulerToQuat(angles, maxQuat, EULERTYPE_XYZ);

    // Convert the quaternion to an angle and axis
    maxQuat.Normalize();
    AngAxis maxAngAxis(maxQuat);

    ConvertMaxAngAxisToAlembicQuat(maxAngAxis, quat);
}

// static
void AlembicPoints::ConvertMaxAngAxisToAlembicQuat(const AngAxis &angAxis, Alembic::Abc::Quatd &quat)
{
    Imath::V3f alembicAxis = ConvertMaxNormalToAlembicNormal(angAxis.axis);
    quat.setAxisAngle(alembicAxis, angAxis.angle);
    quat.normalize();
}

void AlembicPoints::GetShapeType(IParticleObjectExt *pExt, int particleId, TimeValue ticks, ShapeType &type, uint16_t &instanceId, float &animationTime, std::vector<std::string> &nameList)
{
    // Set up initial values
    type = ShapeType_Point;
    instanceId = 0;
    animationTime = 0.0f;

    // Go into the particle's action list
    INode *particleGroupNode = pExt->GetParticleGroup(particleId);
    Object *particleGroupObj = (particleGroupNode != NULL) ? particleGroupNode->EvalWorldState(ticks).obj : NULL;

    if (!particleGroupObj)
        return;

    IParticleGroup *particleGroup = GetParticleGroupInterface(particleGroupObj);
    INode *particleActionListNode = particleGroup->GetActionList();
    Object *particleActionObj = (particleActionListNode != NULL ? particleActionListNode->EvalWorldState(ticks).obj : NULL);

    if (!particleActionObj)
        return;

    PFSimpleOperator *pSimpleOperator = NULL;

    IPFActionList *particleActionList = GetPFActionListInterface(particleActionObj);
    for (int p = particleActionList->NumActions()-1; p >= 0; p -= 1)
    {
        INode *pActionNode = particleActionList->GetAction(p);
        Object *pActionObj = (pActionNode != NULL ? pActionNode->EvalWorldState(ticks).obj : NULL);

        if (pActionObj == NULL)
            continue;

        if (pActionObj->ClassID() == PFOperatorSimpleShape_Class_ID)
        {
            pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
            break;
        }
		else if(pActionObj->ClassID() == PFOperatorShapeLib_Class_ID)
		{
            pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
            break;
		}
        else if (pActionObj->ClassID() == PFOperatorInstanceShape_Class_ID)
        {
            pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
            break;
        }
    }

    if (pSimpleOperator && pSimpleOperator->ClassID() == PFOperatorSimpleShape_Class_ID)
    {
        IParamBlock2 *pblock = pSimpleOperator->GetParamBlockByID(0);
        int nShapeId = pblock->GetInt(PFlow_kSimpleShape_shape, ticks);

        switch(nShapeId)
        {
        case PFlow_kSimpleShape_shape_pyramid:
            type = ShapeType_Cone;
            break;
        case PFlow_kSimpleShape_shape_cube:
            type = ShapeType_Box;
            break;
        case PFlow_kSimpleShape_shape_sphere:
            type = ShapeType_Sphere;
            break;
        case PFlow_kSimpleShape_shape_vertex:
            type = ShapeType_Point;
            break;
        }
    }
	else if (pSimpleOperator && pSimpleOperator->ClassID() == PFOperatorShapeLib_Class_ID)
	{
        IParamBlock2 *pblock = pSimpleOperator->GetParamBlockByID(0);
		int nDimension = pblock->GetInt(PFlow_kShapeLibary_dimensionType, ticks);
		if(nDimension == PFlow_kShapeLibrary_dimensionType_2D){
			int n2DShapeId = pblock->GetInt(PFlow_kShapeLibary_2DType, ticks);
			if( n2DShapeId == PFlow_kShapeLibrary_dimensionType_2D_square){
				type = ShapeType_Rectangle;
			}
			else{
				ESS_LOG_INFO("Unsupported shape type.");
			}
		}
		else if(nDimension == PFlow_kShapeLibrary_dimensionType_3D){
			int n3DShapeId = pblock->GetInt(PFlow_kShapeLibary_3DType, ticks);
			if(n3DShapeId == PFlow_kShapeLibary_3DType_cube){
				type = ShapeType_Box;
			}
			else if(n3DShapeId == PFlow_kShapeLibary_3DType_Sphere20sides ||
					n3DShapeId == PFlow_kShapeLibary_3DType_Sphere80sides){
				type = ShapeType_Sphere;
			}
			else{
				ESS_LOG_INFO("Unsupported shape type.");
			}
		
			//ShapeType_Cylinder unsupported
			//ShapeType_Cone unsupported
			//ShapeType_Disc unsupported
			//ShapeType_NbElements unsupported
		}
		else{
			ESS_LOG_INFO("Unknown dimension.");
		}
			
		//int nNumParams = pblock->NumParams();

		//for(int i=0; i<nNumParams; i++){
	
		//	ParamID id = pblock->IndextoID(i);
		//	MSTR paramStr = pblock->GetLocalName(id, 0);
		//	int n = 0;
		//
		//}

	}
    else if (pSimpleOperator && pSimpleOperator->ClassID() == PFOperatorInstanceShape_Class_ID)
    {
        // Assign animation time and shape here
        IParamBlock2 *pblock = pSimpleOperator->GetParamBlockByID(0);
        INode *pNode = pblock->GetINode(PFlow_kInstanceShape_objectMaxscript, ticks);
        if (pNode == NULL || pNode->GetName() == NULL)
        {
            return;
        }
        
        type = ShapeType_Instance;

        // Find if the name is alerady registered, otherwise add it to the list
        instanceId = USHRT_MAX;
        for ( int i = 0; i < nameList.size(); i += 1)
        {
            if (!strcmp(nameList[i].c_str(), pNode->GetName()))
            {
                instanceId = i;
                break;
            }
        }

        if (instanceId == USHRT_MAX)
        {
            nameList.push_back(pNode->GetName());
            instanceId = (uint16_t)nameList.size()-1;
        }

        // Determine if we have an animated shape
        BOOL animatedShape = pblock->GetInt(PFlow_kInstanceShape_animatedShape);
	    BOOL acquireShape = pblock->GetInt(PFlow_kInstanceShape_acquireShape);

        if (!animatedShape && !acquireShape)
        {
            return;
        }

        // Get the necesary particle channels to grab the current time values
        ::IObject *pCont = particleGroup->GetParticleContainer();
        if (!pCont)
        {
            return;
        }

        // Get synch values that we are interested in fromt the param block
        int syncType = pblock->GetInt(PFlow_kInstanceShape_syncType);
        BOOL syncRandom = pblock->GetInt(PFlow_kInstanceShape_syncRandom);

        IParticleChannelPTVR* chTime = GetParticleChannelTimeRInterface(pCont);
        if (chTime == NULL) 
        {
            return; // can't find particle times in the container
        }

        IChannelContainer* chCont = GetChannelContainerInterface(pCont);
        if (chCont == NULL) 
        {
            return;  // can't get access to ChannelContainer interface
        }

        IParticleChannelPTVR* chBirthTime = NULL;
        IParticleChannelPTVR* chEventStartR = NULL;
        bool initEventStart = false;

        if (syncType == PFlow_kInstanceShape_syncBy_particleAge) 
        {
            chBirthTime = GetParticleChannelBirthTimeRInterface(pCont);
            if (chBirthTime == NULL) 
            {
                return; // can't read particle age
            }
        }
        else if (syncType == PFlow_kInstanceShape_syncBy_eventStart) 
        {
            chEventStartR = GetParticleChannelEventStartRInterface(pCont);
             
            if (chEventStartR == NULL) 
            {
                return; // can't read event start time
            }
        }

        IParticleChannelIntR* chLocalOffR = NULL;
        bool initLocalOff = false;

        // acquire LocalOffset particle channel; if not present then create it.
        if (syncRandom) 
        {
            chLocalOffR =  (IParticleChannelIntR*)chCont->GetPrivateInterface(PARTICLECHANNELLOCALOFFSETR_INTERFACE, pSimpleOperator);
        }

        // get new shape from the source
        PreciseTimeValue time = chTime->GetValue(particleId);
        switch(syncType)
        {
        case PFlow_kInstanceShape_syncBy_absoluteTime:
            break;
        case PFlow_kInstanceShape_syncBy_particleAge:
            time -= chBirthTime->GetValue(particleId);
            break;
        case PFlow_kInstanceShape_syncBy_eventStart:
            time -= chEventStartR->GetValue(particleId);
            break;
        default:
            break;
        }

        if (syncRandom) 
        {
            if (chLocalOffR != NULL)
                time += chLocalOffR->GetValue(particleId);
        }

        TimeValue t = TimeValue(time);
        animationTime = (float)GetSecondsFromTimeValue(t);
    }
}
