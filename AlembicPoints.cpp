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
#include <ParticleFlow/IPFTest.h>
#include <ifnpub.h>
#include <ImathMatrixAlgo.h>
#include "AlembicMetadataUtils.h"

#include <Alembic/Util/Murmur3.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
namespace AbcB = ::Alembic::Abc::ALEMBIC_VERSION_NS;
using namespace AbcA;
using namespace AbcB;

#define PARTICLECHANNELLOCALOFFSETR_INTERFACE Interface_ID(0x12ec5d1d, 0x1eb34500) 
#define GetParticleChannelLocalOffsetRInterface(obj) ((IParticleChannelIntR*)obj->GetInterface(PARTICLECHANNELLOCALOFFSETR_INTERFACE)) 

AlembicPoints::AlembicPoints(const SceneEntry &in_Ref, AlembicWriteJob *in_Job)
    : AlembicObject(in_Ref, in_Job)
{
	mNumShapeMeshes = 0;
	mTotalShapeMeshes = 0;
	//mTimeSamplesCount = 0;

    std::string pointsName = in_Ref.node->GetName();
    std::string xformName = pointsName + "Xfo";

    Alembic::AbcGeom::OXform xform(GetOParent(), xformName.c_str(), GetCurrentJob()->GetAnimatedTs());
    Alembic::AbcGeom::OPoints points(xform, pointsName.c_str(), GetCurrentJob()->GetAnimatedTs());

    // create the generic properties
    mOVisibility = CreateVisibilityProperty(points, GetCurrentJob()->GetAnimatedTs());

    mXformSchema = xform.getSchema();
    mPointsSchema = points.getSchema();

	OCompoundProperty argGeomParams = mPointsSchema.getArbGeomParams();

    // create all properties
    mInstanceNamesProperty = OStringArrayProperty(argGeomParams, ".instancenames", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );

    // particle attributes
    mScaleProperty = OV3fArrayProperty(argGeomParams, ".scale", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mOrientationProperty = OQuatfArrayProperty(argGeomParams, ".orientation", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mAngularVelocityProperty = OQuatfArrayProperty(argGeomParams, ".angularvelocity", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mAgeProperty = OFloatArrayProperty(argGeomParams, ".age", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mMassProperty = OFloatArrayProperty(argGeomParams, ".mass", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeTypeProperty = OUInt16ArrayProperty(argGeomParams, ".shapetype", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeTimeProperty = OFloatArrayProperty(argGeomParams, ".shapetime", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeInstanceIDProperty = OUInt16ArrayProperty(argGeomParams, ".shapeinstanceid", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mColorProperty = OC4fArrayProperty(argGeomParams, ".color", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
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



bool AlembicPoints::Save(double time, bool bLastFrame)
{
    // Note: Particles are always considered to be animated even though
    //       the node does not have the IsAnimated() flag.

    // Extract our particle emitter at the given time
    TimeValue ticks = GetTimeValueFromFrame(time);
    Object *obj = GetRef().node->EvalWorldState(ticks).obj;

	bool bFlatten = GetCurrentJob()->GetOption("flattenHierarchy");

    // Store the transformation
    SaveXformSample(GetRef(), mXformSchema, mXformSample, time, bFlatten);

	SaveMetaData(GetRef().node, this);

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
    std::vector<Alembic::Abc::V3f> positionVec;
    std::vector<Alembic::Abc::V3f> velocityVec;
    std::vector<Alembic::Abc::V3f> scaleVec;
    std::vector<float> widthVec;
    std::vector<float> ageVec;
    std::vector<float> massVec;
    std::vector<float> shapeTimeVec;
    std::vector<uint64_t> idVec;
    std::vector<uint16_t> shapeTypeVec;
    std::vector<uint16_t> shapeInstanceIDVec;
    std::vector<Alembic::Abc::Quatf> orientationVec;
    std::vector<Alembic::Abc::Quatf> angularVelocityVec;
    std::vector<Alembic::Abc::C4f> colorVec;
    //std::vector<std::string> instanceNamesVec;
    Alembic::Abc::Box3d bbox;
    bool constantPos = true;
    bool constantVel = true;
    bool constantScale = true;
    bool constantWidth = true;
    bool constantAge = true;
    bool constantOrientation = true;
    bool constantAngularVel = true;
	bool constantColor = true;

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

	const bool bAutomaticInstancing = GetCurrentJob()->GetOption("automaticInstancing");

	for (int i = 0; i < numParticles; ++i)
	{
		Imath::V3f pos(0.0);
		Imath::V3f vel(0.0);
		Imath::V3f scale(1.0);
		Imath::C4f color(0.5, 0.5, 0.5, 1.0);
		float age = 0;
		uint64_t id = 0;
	    Alembic::Abc::Quatd orientation(0.0, 0.0, 1.0, 0.0);
		Alembic::Abc::Quatd spin(0.0, 0.0, 1.0, 0.0);
		// Particle size is a uniform scale multiplier in XSI.  In Max, I need to learn where to get this 
		// For now, we'll just default to 1
		float width = 1.0f;

		ShapeType shapetype = ShapeType_Point;
		float shapeInstanceTime = (float)time;
		uint16_t shapeInstanceId = 0;

		if(particlesExt){
			pos = ConvertMaxPointToAlembicPoint(*particlesExt->GetParticlePositionByIndex(i));
			vel = ConvertMaxVectorToAlembicVector(*particlesExt->GetParticleSpeedByIndex(i) * TIME_TICKSPERSEC);
			scale = ConvertMaxScaleToAlembicScale(*particlesExt->GetParticleScaleXYZByIndex(i));
			ConvertMaxEulerXYZToAlembicQuat(*particlesExt->GetParticleOrientationByIndex(i), orientation);
			ConvertMaxAngAxisToAlembicQuat(*particlesExt->GetParticleSpinByIndex(i), spin);
			age = (float)GetSecondsFromTimeValue(particlesExt->GetParticleAgeByIndex(i));
			id = particlesExt->GetParticleBornIndex(i);
			if(bAutomaticInstancing){
				ReadShapeMesh(particlesExt, i, ticks, shapetype, shapeInstanceId, shapeInstanceTime);
			}
			else{
				GetShapeType(particlesExt, i, ticks, shapetype, shapeInstanceId, shapeInstanceTime);
			}
			color = GetColor(particlesExt, i, ticks);
		}
		else if(pSimpleParticle){
			if( ! pSimpleParticle->parts.Alive( i ) ) {
				continue;
			}

			pos = ConvertMaxPointToAlembicPoint(pSimpleParticle->ParticlePosition(ticks, i));
			vel = ConvertMaxVectorToAlembicVector(pSimpleParticle->ParticleVelocity(ticks, i));
			//simple particles have no scale?
			//simple particles have no orientation?
			age = (float)GetSecondsFromTimeValue( pSimpleParticle->ParticleAge(ticks, i) );
			//simple particles have born index
			width = pSimpleParticle->ParticleSize(ticks, i);
		}

		//move everything from world space to local space
		pos = pos * nodeWorldTransInv;

		Imath::V4f vel4(vel.x, vel.y, vel.z, 0.0);
		vel4 = vel4 * nodeWorldTransInv;
		vel.setValue(vel4.x, vel4.y, vel4.z);

		//scale = scale * nodeWorldTransInv;
		//orientation = Imath::extractQuat(orientation.toMatrix44() * nodeWorldTransInv);
		//spin = Imath::extractQuat(spin.toMatrix44() * nodeWorldTransInv);

		bbox.extendBy( pos );

		positionVec.push_back( pos );
		velocityVec.push_back( vel );
		scaleVec.push_back( scale );
		widthVec.push_back( width );
		ageVec.push_back( age );
		idVec.push_back( id );
		orientationVec.push_back( orientation );
		angularVelocityVec.push_back( spin );
        shapeTypeVec.push_back( shapetype );
        shapeInstanceIDVec.push_back( shapeInstanceId );
        shapeTimeVec.push_back( shapeInstanceTime );
		colorVec.push_back( color );

		constantPos &= (pos == positionVec[0]);
		constantVel &= (vel == velocityVec[0]);
		constantScale &= (scale == scaleVec[0]);
		constantWidth &= (width == widthVec[0]);
		constantAge &= (age == ageVec[0]);
		constantOrientation &= (orientation == orientationVec[0]);
		constantAngularVel &= (spin == angularVelocityVec[0]);
		constantColor &= (color == colorVec[0]);

		// Set the archive bounding box
		// Positions for particles are already cnsider to be in world space
		if (mJob)
		{
			mJob->GetArchiveBBox().extendBy(pos);
		}
	}



    if (numParticles > 1)
    {
        if (constantPos)        { positionVec.resize(1); }
        if (constantVel)        { velocityVec.resize(1); }
        if (constantScale)      { scaleVec.resize(1); }
        if (constantWidth)      { widthVec.resize(1); }
        if (constantAge)        { ageVec.resize(1); }
        if (constantOrientation){ orientationVec.resize(1); }
        if (constantAngularVel) { angularVelocityVec.resize(1); }
		if (constantColor)		{ colorVec.resize(1); }
    }

    // Store the information into our properties and points schema
    Alembic::Abc::P3fArraySample positionSample( positionVec);
    Alembic::Abc::P3fArraySample velocitySample(velocityVec);
    Alembic::Abc::P3fArraySample scaleSample(scaleVec);
    Alembic::Abc::FloatArraySample widthSample(widthVec);
    Alembic::Abc::FloatArraySample ageSample(ageVec);
    Alembic::Abc::FloatArraySample massSample(massVec);
    Alembic::Abc::FloatArraySample shapeTimeSample(shapeTimeVec);
    Alembic::Abc::UInt64ArraySample idSample(idVec);
    Alembic::Abc::UInt16ArraySample shapeTypeSample(shapeTypeVec);
    Alembic::Abc::UInt16ArraySample shapeInstanceIDSample(shapeInstanceIDVec);
    Alembic::Abc::QuatfArraySample orientationSample(orientationVec);
    Alembic::Abc::QuatfArraySample angularVelocitySample(angularVelocityVec);
    Alembic::Abc::C4fArraySample colorSample(colorVec);

	//if(instanceNamesVec.size() == 1){
	//for some reason the .dims property is not written when there is exactly one entry if we don't push an empty string
	//having an extra unreferenced entry seems to be harmless
	mInstanceNames.push_back("");
	//}

    Alembic::Abc::StringArraySample instanceNamesSample(mInstanceNames);

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

	mInstanceNames.pop_back();

	if(bAutomaticInstancing){
		saveCurrentFrameMeshes();
	}

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


Alembic::Abc::C4f AlembicPoints::GetColor(IParticleObjectExt *pExt, int particleId, TimeValue ticks)
{
	Alembic::Abc::C4f color(0.5, 0.5, 0.5, 1.0);

    // Go into the particle's action list
    INode *particleGroupNode = pExt->GetParticleGroup(particleId);
    Object *particleGroupObj = (particleGroupNode != NULL) ? particleGroupNode->EvalWorldState(ticks).obj : NULL;

	if (!particleGroupObj){
        return color;
	}

    IParticleGroup *particleGroup = GetParticleGroupInterface(particleGroupObj);
    INode *particleActionListNode = particleGroup->GetActionList();
    Object *particleActionObj = (particleActionListNode != NULL ? particleActionListNode->EvalWorldState(ticks).obj : NULL);

	if (!particleActionObj){
        return color;
	}

	PFSimpleOperator *pSimpleOperator = NULL;

	//In the case of multiple shape operators in an action list, the one furthest down the list seems to be the one that applies
    IPFActionList *particleActionList = GetPFActionListInterface(particleActionObj);
	
	for (int p = particleActionList->NumActions()-1; p >= 0; p -= 1)
	{
		INode *pActionNode = particleActionList->GetAction(p);
		Object *pActionObj = (pActionNode != NULL ? pActionNode->EvalWorldState(ticks).obj : NULL);

		if (pActionObj == NULL){
			continue;
		}

		if (pActionObj->ClassID() == PFOperatorDisplay_Class_ID){
			pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
			break;
		}
	}
	
	if(pSimpleOperator){
		IParamBlock2 *pblock = pSimpleOperator->GetParamBlockByID(0);
		Point3 c = pblock->GetPoint3(kDisplay_color);
		color.r = c.x;
		color.g = c.y;
		color.b = c.z;
		color.a = 1.0;
	}

	return color;
}

void AlembicPoints::ReadShapeFromOperator( IParticleGroup *particleGroup, PFSimpleOperator *pSimpleOperator, int particleId, TimeValue ticks, ShapeType &type, uint16_t &instanceId, float &animationTime)
{
	if(!pSimpleOperator){
		return;
	}

	if (pSimpleOperator->ClassID() == PFOperatorSimpleShape_Class_ID)
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
		default:
			type = ShapeType_Point;
        }
    }
	else if (pSimpleOperator->ClassID() == PFOperatorShapeLib_Class_ID)
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
				type = ShapeType_Point;
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
				type = ShapeType_Point;
			}
		
			//ShapeType_Cylinder unsupported
			//ShapeType_Cone unsupported
			//ShapeType_Disc unsupported
			//ShapeType_NbElements unsupported
		}
		else{
			ESS_LOG_INFO("Unknown dimension.");
			type = ShapeType_Point;
		}
			
		//int nNumParams = pblock->NumParams();

		//for(int i=0; i<nNumParams; i++){
	
		//	ParamID id = pblock->IndextoID(i);
		//	MSTR paramStr = pblock->GetLocalName(id, 0);
		//	int n = 0;
		//
		//}

	}
    else if (pSimpleOperator->ClassID() == PFOperatorInstanceShape_Class_ID)
    {
        // Assign animation time and shape here
        IParamBlock2 *pblock = pSimpleOperator->GetParamBlockByID(0);
        INode *pNode = pblock->GetINode(PFlow_kInstanceShape_objectMaxscript, ticks);
        if (pNode == NULL || pNode->GetName() == NULL)
        {
            return;
        }
        
        type = ShapeType_Instance;

		std::string nodePath = getNodePath(pNode->GetName());

        // Find if the name is alerady registered, otherwise add it to the list
        instanceId = FindInstanceName(nodePath);
        if (instanceId == USHRT_MAX)
        {
			mInstanceNames.push_back(nodePath);
            instanceId = (uint16_t)mInstanceNames.size()-1;
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
		
		//timeValueMap::iterator it = mTimeValueMap.find(time);
		//if( it != mTimeValueMap.end() ){
		//	ESS_LOG_WARNING("sampleTime already seen.");
		//}
		//else{
		//	mTimeValueMap[time] = true;
		//	mTimeSamplesCount++;
		//	ESS_LOG_WARNING("sampleTime: "<<(float)time<<" totalSamples: "<<mTimeSamplesCount);
		//}

        TimeValue t = TimeValue(time);
        animationTime = (float)GetSecondsFromTimeValue(t);
    }
	else if (pSimpleOperator->ClassID() == PFOperatorMarkShape_Class_ID)
	{
		ESS_LOG_INFO("Shape Mark operator not supported.");
	}
	else if (pSimpleOperator->ClassID() == PFOperatorFacingShape_Class_ID)
	{
		ESS_LOG_INFO("Shape Facing operator not supported.");
	}

}

void AlembicPoints::GetShapeType(IParticleObjectExt *pExt, int particleId, TimeValue ticks, ShapeType &type, uint16_t &instanceId, float &animationTime)
{
    // Set up initial values
    type = ShapeType_NbElements;//default to nothing
    instanceId = 0;
    animationTime = 0.0f;

    // Go into the particle's action list
    INode *particleGroupNode = pExt->GetParticleGroup(particleId);
    Object *particleGroupObj = (particleGroupNode != NULL) ? particleGroupNode->EvalWorldState(ticks).obj : NULL;

	if (!particleGroupObj){
        return;
	}

    IParticleGroup *particleGroup = GetParticleGroupInterface(particleGroupObj);
    INode *particleActionListNode = particleGroup->GetActionList();
    Object *particleActionObj = (particleActionListNode != NULL ? particleActionListNode->EvalWorldState(ticks).obj : NULL);

	if (!particleActionObj){
        return;
	}
	
    PFSimpleOperator *pSimpleOperator = NULL;

	//In the case of multiple shape operators in an action list, the one furthest down the list seems to be the one that applies
    IPFActionList *particleActionList = GetPFActionListInterface(particleActionObj);
	
	for (int p = particleActionList->NumActions()-1; p >= 0; p -= 1)
	{
		INode *pActionNode = particleActionList->GetAction(p);
		Object *pActionObj = (pActionNode != NULL ? pActionNode->EvalWorldState(ticks).obj : NULL);

		if (pActionObj == NULL){
			continue;
		}

		if (pActionObj->ClassID() == PFOperatorSimpleShape_Class_ID){
			pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
			break;
		}else if(pActionObj->ClassID() == PFOperatorShapeLib_Class_ID){
			pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
			break;
		}else if(pActionObj->ClassID() == PFOperatorInstanceShape_Class_ID){
			pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
			break;
		}else if(pActionObj->ClassID() == PFOperatorMarkShape_Class_ID){
			pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
			break;
		}else if(pActionObj->ClassID() == PFOperatorFacingShape_Class_ID){
			pSimpleOperator = static_cast<PFSimpleOperator*>(pActionObj);
			break;
		}
	}

	

	for (int p = particleActionList->NumActions()-1; p >= 0; p -= 1)
	{
		INode *pActionNode = particleActionList->GetAction(p);
		Object *pActionObj = (pActionNode != NULL ? pActionNode->EvalWorldState(ticks).obj : NULL);

		if (pActionObj == NULL){
			continue;
		}

		IPFTest* pTestAction = GetPFTestInterface(pActionObj);

		if (pTestAction){
			
			INode* childActionListNode = pTestAction->GetNextActionList(pActionNode, NULL);

			if(childActionListNode){
				AlembicPoints::perActionListShapeMap_it actionListIt = mPerActionListShapeMap.find(childActionListNode);

				//create a cache entry if necessary
				if(actionListIt == mPerActionListShapeMap.end()){
					mPerActionListShapeMap[childActionListNode] = AlembicPoints::shapeInfo();
				}
				AlembicPoints::shapeInfo& sInfo = mPerActionListShapeMap[childActionListNode];
				
				if(!sInfo.pParentActionList){
					sInfo.pParentActionList = particleActionListNode;
				}
			}
		}
	}

	ReadShapeFromOperator(particleGroup, pSimpleOperator, particleId, ticks, type, instanceId, animationTime);

	if(type != ShapeType_NbElements){//write the shape to the cache

		// create cache entry for the current action list node, and then fill in the shape info
		// we will fill in the parent later

		AlembicPoints::perActionListShapeMap_it actionListIt = mPerActionListShapeMap.find(particleActionListNode);

		//create a cache entry if necessary
		if(actionListIt == mPerActionListShapeMap.end()){
			mPerActionListShapeMap[particleActionListNode] = AlembicPoints::shapeInfo();
		}
		AlembicPoints::shapeInfo& sInfo = mPerActionListShapeMap[particleActionListNode];
		
		if(sInfo.type == ShapeType_NbElements){
			sInfo.type = type;
			sInfo.animationTime = animationTime;
			if(sInfo.type == ShapeType_Instance){
				sInfo.instanceName = mInstanceNames[instanceId];
			}
		}
	}
	else{ //read the shape from the cache

		AlembicPoints::shapeInfo sInfo;
		INode* currActionNode = particleActionListNode;
		
		//search for shape along path from this node to the root node
		const int MAX_DEPTH = 10; //just in case there is an infinite loop due a bug
		int i = 0;
		while(currActionNode && sInfo.type == ShapeType_NbElements && i<MAX_DEPTH){

			AlembicPoints::perActionListShapeMap_it actionListIt = mPerActionListShapeMap.find(currActionNode);
			if(actionListIt != mPerActionListShapeMap.end()){
				sInfo = actionListIt->second;
			}

			currActionNode = sInfo.pParentActionList;
			i++;
		}

		if(sInfo.type != ShapeType_NbElements){//We have found shape, so add it to the list if necessary

			// Find if the name is alerady registered, otherwise add it to the list
			instanceId = FindInstanceName(sInfo.instanceName);
			if (instanceId == USHRT_MAX)
			{
				mInstanceNames.push_back(sInfo.instanceName);
				instanceId = (uint16_t)mInstanceNames.size()-1;
			}
			type = sInfo.type;
		}
		else{
			int nBornIndex = pExt->GetParticleBornIndex(particleId);
			ESS_LOG_INFO("Could not determine shape type for particle with born index: "<<nBornIndex<<". Defaulting to point.");
 			type = ShapeType_Point;
		}
	}
}

uint16_t AlembicPoints::FindInstanceName(const std::string& name)
{
	for ( int i = 0; i < mInstanceNames.size(); i += 1){
		if (strcmp(mInstanceNames[i].c_str(), name.c_str()) == 0){
			return i;
		}
	}
	return USHRT_MAX;
}

void AlembicPoints::ReadShapeMesh(IParticleObjectExt *pExt, int particleId, TimeValue ticks, ShapeType &type, uint16_t &instanceId, float &animationTime)
{
	type = ShapeType_Instance;
	//animationTime = 0;
	
	Mesh* pShapeMesh = pExt->GetParticleShapeByIndex(particleId);

	Digest vertexDigest;
	MurmurHash3_x64_128( pShapeMesh->verts, pShapeMesh->numVerts * sizeof(Point3), sizeof(Point3), vertexDigest.words );

	Digest faceDigest;
	MurmurHash3_x64_128( pShapeMesh->faces, pShapeMesh->numFaces * sizeof(Face), sizeof(Face), faceDigest.words );
	
	mTotalShapeMeshes++;

	meshInfo currShapeInfo;
	faceVertexHashToShapeMap::iterator it = mShapeMeshCache.find(faceVertexHashPair(vertexDigest, faceDigest));
	if( it != mShapeMeshCache.end() ){
		meshInfo& mi = it->second;
		currShapeInfo = mi;
	}
	else{
		meshInfo& mi = mShapeMeshCache[faceVertexHashPair(vertexDigest, faceDigest)];
		mi.pMesh = pShapeMesh;
		std::stringstream nameStream;
		nameStream<<GetRef().node->GetName()<<" ";
		nameStream<<"InstanceMesh"<<mNumShapeMeshes;
		mi.name=nameStream.str();
		currShapeInfo = mi;
		mNumShapeMeshes++;

		mMeshesToSaveForCurrentFrame.push_back(&mi);

		ESS_LOG_WARNING("ticks: "<<ticks<<" particleId: "<<particleId);
		ESS_LOG_WARNING("Adding shape with hash("<<vertexDigest.str()<<", "<<faceDigest.str()<<") \n auto assigned name "<<mi.name <<" efficiency:"<<(double)mNumShapeMeshes/mTotalShapeMeshes);
	}

	std::string pathName("/");
	pathName += currShapeInfo.name;

	instanceId = FindInstanceName(pathName);
	if (instanceId == USHRT_MAX){
		mInstanceNames.push_back(pathName);
		instanceId = (uint16_t)mInstanceNames.size()-1;
	}
}

//we have to save out all mesh encountered on the current frame of this object immediately, because 
//the pointers will no longer be valid when the object is evaluated at a new time
void AlembicPoints::saveCurrentFrameMeshes()
{
	for(int i=0; i<mMeshesToSaveForCurrentFrame.size(); i++){

		meshInfo* mi = mMeshesToSaveForCurrentFrame[i];

		if(mi->pMesh){
			Mesh* pMesh = mi->pMesh;
			mi->pMesh = NULL;//each mesh only needs to be saved once

			Matrix3 worldTrans;
			worldTrans.IdentityMatrix();

			std::map<std::string, bool> options;
			options["exportNormals"] = mJob->GetOption("exportNormals");
			options["indexedNormals"] = mJob->GetOption("indexedNormals");

			//gather the mesh data
			IntermediatePolyMesh3DSMax finalPolyMesh;
			finalPolyMesh.Save(options, pMesh, NULL, worldTrans, NULL, false, NULL);

			//save out the mesh xForm
		
			std::string xformName = mi->name + "Xfo";
			Alembic::AbcGeom::OXform xform(mJob->GetArchive().getTop(), xformName.c_str(), GetCurrentJob()->GetAnimatedTs());
			Alembic::AbcGeom::OXformSchema& xformSchema = xform.getSchema();//mi->xformSchema;

			Alembic::AbcGeom::OPolyMesh mesh(xform, mi->name.c_str(), GetCurrentJob()->GetAnimatedTs());
			Alembic::AbcGeom::OPolyMeshSchema& meshSchema = mesh.getSchema();

			Alembic::AbcGeom::XformSample xformSample;

			Matrix3 alembicMatrix;
			alembicMatrix.IdentityMatrix();
			alembicMatrix.SetTrans(Point3(-10000.0f, -10000.0f, -10000.0f));
			Alembic::Abc::M44d iMatrix;
			ConvertMaxMatrixToAlembicMatrix(alembicMatrix, iMatrix);

			xformSample.setMatrix(iMatrix);
			xformSchema.set(xformSample);

			//update the archive bounding box
			Alembic::Abc::Box3d bbox;
			bbox.min = finalPolyMesh.bbox.min * iMatrix;
			bbox.max = finalPolyMesh.bbox.max * iMatrix;

			mJob->GetArchiveBBox().extendBy(bbox);

			//save out the mesh data			
			Alembic::AbcGeom::OPolyMeshSchema::Sample meshSample;

			meshSample.setPositions(Alembic::Abc::P3fArraySample(finalPolyMesh.posVec));
			meshSample.setSelfBounds(finalPolyMesh.bbox);
			meshSample.setChildBounds(finalPolyMesh.bbox);

			meshSample.setFaceCounts(Alembic::Abc::Int32ArraySample(finalPolyMesh.mFaceCountVec));
			meshSample.setFaceIndices(Alembic::Abc::Int32ArraySample(finalPolyMesh.mFaceIndicesVec));

			if(mJob->GetOption("exportNormals")){
				Alembic::AbcGeom::ON3fGeomParam::Sample normalSample;
				normalSample.setScope(Alembic::AbcGeom::kFacevaryingScope);
				normalSample.setVals(Alembic::Abc::N3fArraySample(finalPolyMesh.normalVec));
				if(mJob->GetOption("indexedNormals")){
					normalSample.setIndices(Alembic::Abc::UInt32ArraySample(finalPolyMesh.normalIndexVec));
				}
				meshSample.setNormals(normalSample);
			}

			meshSchema.set(meshSample);


		}
	}
	
	mMeshesToSaveForCurrentFrame.clear();
}