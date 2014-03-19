#include "stdafx.h"
#include "Alembic.h"
#include "AlembicPoints.h"
#include "AlembicXform.h"
#include "SceneEnumProc.h"
#include "Utility.h"

#include "AlembicMetadataUtils.h"
#include "MaxSceneTimeManager.h"
#include "AlembicPointsUtils.h"
#include "AlembicParticles.h"
#include "CommonMeshUtilities.h"

#define PARTICLECHANNELLOCALOFFSETR_INTERFACE Interface_ID(0x12ec5d1d, 0x1eb34500) 
#define GetParticleChannelLocalOffsetRInterface(obj) ((IParticleChannelIntR*)obj->GetInterface(PARTICLECHANNELLOCALOFFSETR_INTERFACE)) 

AlembicPoints::AlembicPoints(SceneNodePtr eNode, AlembicWriteJob * in_Job, Abc::OObject oParent)
    : AlembicObject(eNode, in_Job, oParent)
{
	mNumShapeMeshes = 0;
	mTotalShapeMeshes = 0;
	//mTimeSamplesCount = 0;

    std::string xformName = EC_MCHAR_to_UTF8( mMaxNode->GetName() );
	std::string pointsName = xformName + "Shape";

    AbcG::OPoints points(GetOParent(), pointsName.c_str(), GetCurrentJob()->GetAnimatedTs());
    mPointsSchema = points.getSchema();

	Abc::OCompoundProperty argGeomParams = mPointsSchema.getArbGeomParams();

    // create all properties
    mInstanceNamesProperty = Abc::OStringArrayProperty(argGeomParams, ".instancenames", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );

    // particle attributes
    mScaleProperty = Abc::OV3fArrayProperty(argGeomParams, ".scale", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mOrientationProperty = Abc::OQuatfArrayProperty(argGeomParams, ".orientation", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mAngularVelocityProperty = Abc::OQuatfArrayProperty(argGeomParams, ".angularvelocity", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mAgeProperty = Abc::OFloatArrayProperty(argGeomParams, ".age", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mMassProperty = Abc::OFloatArrayProperty(argGeomParams, ".mass", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeTypeProperty = Abc::OUInt16ArrayProperty(argGeomParams, ".shapetype", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeTimeProperty = Abc::OFloatArrayProperty(argGeomParams, ".shapetime", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mShapeInstanceIDProperty = Abc::OUInt16ArrayProperty(argGeomParams, ".shapeinstanceid", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
    mColorProperty = Abc::OC4fArrayProperty(argGeomParams, ".color", mPointsSchema.getMetaData(), GetCurrentJob()->GetAnimatedTs() );
}

AlembicPoints::~AlembicPoints()
{
}

Abc::OCompoundProperty AlembicPoints::GetCompound()
{
    return mPointsSchema;
}



bool AlembicPoints::Save(double time, bool bLastFrame)
{
   ESS_PROFILE_FUNC();

    // Note: Particles are always considered to be animated even though
    //       the node does not have the IsAnimated() flag.

    // Extract our particle emitter at the given time
    TimeValue ticks = GetTimeValueFromFrame(time);
    Object *obj = mMaxNode->EvalWorldState(ticks).obj;

	SaveMetaData(mMaxNode, this);

	SimpleParticle* pSimpleParticle = (SimpleParticle*)obj->GetInterface(I_SIMPLEPARTICLEOBJ);
	IPFSystem* ipfSystem = GetPFSystemInterface(obj);
	IParticleObjectExt* particlesExt = GetParticleObjectExtInterface(obj);
	
#ifdef THINKING_PARTICLES
	ParticleMat* pThinkingParticleMat = NULL;
	TP_MasterSystemInterface* pTPMasterSystemInt = NULL;
	if(obj->CanConvertToType(MATTERWAVES_CLASS_ID))
	{
		pThinkingParticleMat = reinterpret_cast<ParticleMat*>(obj->ConvertToType(ticks, MATTERWAVES_CLASS_ID));
		pTPMasterSystemInt = reinterpret_cast<TP_MasterSystemInterface*>(obj->GetInterface(IID_TP_MASTERSYSTEM));     

	}
#endif

	const bool bAutomaticInstancing = GetCurrentJob()->GetOption("automaticInstancing");

	if( 
#ifdef THINKING_PARTICLES
		!pThinkingParticleMat && 
#endif
		!particlesExt && !pSimpleParticle){
		return false;
	}


	//We have to put the particle system into the renders state so that PFOperatorMaterialFrequency::Proceed will set the materialID channel
	//Note: settting the render state to true breaks the shape node instancing export
	bool bRenderStateForced = false;
	if(bAutomaticInstancing && ipfSystem && !ipfSystem->IsRenderState()){
		ipfSystem->SetRenderState(true);
		bRenderStateForced = true;
	}

	int numParticles = 0;
#ifdef THINKING_PARTICLES
	if(pThinkingParticleMat){
		numParticles = pThinkingParticleMat->NumParticles();	
	}
	else 
#endif
	if(particlesExt){
		particlesExt->UpdateParticles(mMaxNode, ticks);
		numParticles = particlesExt->NumParticles();
	}
	else if(pSimpleParticle){
		pSimpleParticle->Update(ticks, mMaxNode);
		numParticles = pSimpleParticle->parts.points.Count();
	}


    // Store positions, velocity, width/size, scale, id, bounding box
    std::vector<Abc::V3f> positionVec;
    std::vector<Abc::V3f> velocityVec;
    std::vector<Abc::V3f> scaleVec;
    std::vector<float> widthVec;
    std::vector<float> ageVec;
    std::vector<float> massVec;
    std::vector<float> shapeTimeVec;
    std::vector<Abc::uint64_t> idVec;
    std::vector<Abc::uint16_t> shapeTypeVec;
    std::vector<Abc::uint16_t> shapeInstanceIDVec;
    std::vector<Abc::Quatf> orientationVec;
    std::vector<Abc::Quatf> angularVelocityVec;
    std::vector<Abc::C4f> colorVec;

    positionVec.reserve(numParticles);
    velocityVec.reserve(numParticles);
    scaleVec.reserve(numParticles);
    widthVec.reserve(numParticles);
    ageVec.reserve(numParticles);
    massVec.reserve(numParticles);
    shapeTimeVec.reserve(numParticles);
    idVec.reserve(numParticles);
    shapeTypeVec.reserve(numParticles);
    shapeInstanceIDVec.reserve(numParticles);
    orientationVec.reserve(numParticles);
    angularVelocityVec.reserve(numParticles);
    colorVec.reserve(numParticles);

    //std::vector<std::string> instanceNamesVec;
    Abc::Box3d bbox;
    bool constantPos = true;
    bool constantVel = true;
    bool constantScale = true;
    bool constantWidth = true;
    bool constantAge = true;
    bool constantOrientation = true;
    bool constantAngularVel = true;
	bool constantColor = true;


	if(bAutomaticInstancing){
		SetMaxSceneTime(ticks);
	}

	//The MAX interfaces return everything in world coordinates,
	//so we need to multiply the inverse the node world transform matrix
    Matrix3 nodeWorldTM = mMaxNode->GetObjTMAfterWSM(ticks);
    // Convert the max transform to alembic
    Matrix3 alembicMatrix;
    ConvertMaxMatrixToAlembicMatrix(nodeWorldTM, alembicMatrix);
    Abc::M44d nodeWorldTrans(	alembicMatrix.GetRow(0).x,  alembicMatrix.GetRow(0).y,  alembicMatrix.GetRow(0).z,  0,
										alembicMatrix.GetRow(1).x,  alembicMatrix.GetRow(1).y,  alembicMatrix.GetRow(1).z,  0,
										alembicMatrix.GetRow(2).x,  alembicMatrix.GetRow(2).y,  alembicMatrix.GetRow(2).z,  0,
										alembicMatrix.GetRow(3).x,  alembicMatrix.GetRow(3).y,  alembicMatrix.GetRow(3).z,  1);
	Abc::M44d nodeWorldTransInv = nodeWorldTrans.inverse();


	//ESS_LOG_WARNING("tick: "<<ticks<<"   numParticles: "<<numParticles<<"\n");

	ExoNullView nullView;
	particleGroupInterface groupInterface(particlesExt, obj, mMaxNode, &nullView);

    {
    ESS_PROFILE_SCOPE("AlembicPoints::SAVE - numParticlesLoop");

	for (int i = 0; i < numParticles; ++i)
	{
		Abc::V3f pos(0.0);
		Abc::V3f vel(0.0);
		Abc::V3f scale(1.0);
		Abc::C4f color(0.5, 0.5, 0.5, 1.0);
		float age = 0;
		Abc::uint64_t id = 0;
	    Abc::Quatd orientation(0.0, 0.0, 1.0, 0.0);
		Abc::Quatd spin(0.0, 0.0, 1.0, 0.0);
		// Particle size is a uniform scale multiplier in XSI.  In Max, I need to learn where to get this 
		// For now, we'll just default to 1
		float width = 1.0f;

		ShapeType shapetype = ShapeType_Point;
		float shapeInstanceTime = (float)time;
		Abc::uint16_t shapeInstanceId = 0;

#ifdef THINKING_PARTICLES
		if(pThinkingParticleMat){
            

			if(pTPMasterSystemInt->IsAlive(i) == FALSE){
				continue;
			}

			//TimeValue ageValue = particlesExt->GetParticleAgeByIndex(i);
			TimeValue ageValue = pTPMasterSystemInt->Age(i);
			if(ageValue == -1){
				continue;
			}

            ESS_PROFILE_SCOPE("AlembicPoints::SAVE - numParticlesLoop - ThinkingParticles");
			age = (float)GetSecondsFromTimeValue(ageValue);

			//pos = ConvertMaxPointToAlembicPoint(*particlesExt->GetParticlePositionByIndex(i));
			pos = ConvertMaxPointToAlembicPoint(pTPMasterSystemInt->Position(i));
			//vel = ConvertMaxVectorToAlembicVector(*particlesExt->GetParticleSpeedByIndex(i) * TIME_TICKSPERSEC);
			vel = ConvertMaxVectorToAlembicVector(pTPMasterSystemInt->Velocity(i) * TIME_TICKSPERSEC);
			scale = ConvertMaxScaleToAlembicScale(pTPMasterSystemInt->Scale(i));
			scale *= pTPMasterSystemInt->Size(i);
			
			//ConvertMaxEulerXYZToAlembicQuat(*particlesExt->GetParticleOrientationByIndex(i), orientation);

			Matrix3 alignmentMatMax = pTPMasterSystemInt->Alignment(i);
			Abc::M44d alignmentMat;
			ConvertMaxMatrixToAlembicMatrix(alignmentMatMax, alignmentMat);
			/*alignmentMat = Abc::M44d( alignmentMatMax.GetRow(0).x,  alignmentMatMax.GetRow(0).y,  alignmentMatMax.GetRow(0).z,  0,
                                 alignmentMatMax.GetRow(1).x,  alignmentMatMax.GetRow(1).y,  alignmentMatMax.GetRow(1).z,  0,
                                 alignmentMatMax.GetRow(2).x,  alignmentMatMax.GetRow(2).y,  alignmentMatMax.GetRow(2).z,  0,
                                 alignmentMatMax.GetRow(3).x,  alignmentMatMax.GetRow(3).y,  alignmentMatMax.GetRow(3).z,  1);*/
			//orientation = ConvertMaxQuatToAlembicQuat(extracctuat(alignmentMat), true);

			alignmentMat = alignmentMat * nodeWorldTransInv;
			orientation = extractQuat(alignmentMat);

			//ConvertMaxAngAxisToAlembicQuat(*particlesExt->GetParticleSpinByIndex(i), spin);
			ConvertMaxAngAxisToAlembicQuat(pTPMasterSystemInt->Spin(i), spin);


			id = particlesExt->GetParticleBornIndex(i);

			//seems to always return 0
			//int nPid = pThinkingParticleMat->ParticleID(i);
					
			int nMatId = -1;
			Matrix3 meshTM;
			meshTM.IdentityMatrix();
			BOOL bNeedDelete = FALSE;
			BOOL bChanged = FALSE;
            Mesh* pMesh = NULL;
            {
            ESS_PROFILE_SCOPE("AlembicPoints::SAVE - numParticlesLoop - ThinkingParticles - GetParticleRenderMesh");
			pMesh = pThinkingParticleMat->GetParticleRenderMesh(ticks, mMaxNode, nullView, bNeedDelete, i, meshTM, bChanged);
            }

			if(pMesh){
                ESS_PROFILE_SCOPE("AlembicPoints::SAVE - numParticlesLoop - ThinkingParticles - CacheShapeMesh");
            meshInfo mi = CacheShapeMesh(pMesh, bNeedDelete, meshTM, nMatId, i, ticks, shapetype, shapeInstanceId, shapeInstanceTime);
            Abc::V3d min = pos + mi.bbox.min;
            Abc::V3d max = pos + mi.bbox.max;
            bbox.extendBy(min);
            bbox.extendBy(max);
			}
			else{
				shapetype = ShapeType_Point;
			}
		}
		else 
#endif
		if(particlesExt && ipfSystem){

			TimeValue ageValue = particlesExt->GetParticleAgeByIndex(i);
			if(ageValue == -1){
				continue;
			}
			age = (float)GetSecondsFromTimeValue(ageValue);

			pos = ConvertMaxPointToAlembicPoint(*particlesExt->GetParticlePositionByIndex(i));
			vel = ConvertMaxVectorToAlembicVector(*particlesExt->GetParticleSpeedByIndex(i) * TIME_TICKSPERSEC);
			scale = ConvertMaxScaleToAlembicScale(*particlesExt->GetParticleScaleXYZByIndex(i));
			ConvertMaxEulerXYZToAlembicQuat(*particlesExt->GetParticleOrientationByIndex(i), orientation);
			ConvertMaxAngAxisToAlembicQuat(*particlesExt->GetParticleSpinByIndex(i), spin);
			//age = (float)GetSecondsFromTimeValue(particlesExt->GetParticleAgeByIndex(i));
			id = particlesExt->GetParticleBornIndex(i);
			if(bAutomaticInstancing){

				int nMatId = -1;
				if(ipfSystem){
					if( groupInterface.setCurrentParticle(ticks, i) ){
						nMatId = groupInterface.getCurrentMtlId();
					}
					else{
						ESS_LOG_WARNING("Error: cound retrieve material ID for particle mesh "<<i);
					}
				}

				Matrix3 meshTM;
				meshTM.IdentityMatrix();
				BOOL bNeedDelete = FALSE;
				BOOL bChanged = FALSE;
				Mesh* pMesh = pMesh = particlesExt->GetParticleShapeByIndex(i);

				if(pMesh){
					meshInfo mi = CacheShapeMesh(pMesh, bNeedDelete, meshTM, nMatId, i, ticks, shapetype, shapeInstanceId, shapeInstanceTime);
               Abc::V3d min = pos + mi.bbox.min;
               Abc::V3d max = pos + mi.bbox.max;
               bbox.extendBy(min);
               bbox.extendBy(max);
				}
				else{
					shapetype = ShapeType_Point;
				}
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

         Abc::V3d min(pos.x - width/2, pos.y - width/2, pos.z - width/2);
         Abc::V3d max(pos.x + width/2, pos.y + width/2, pos.z + width/2);
         bbox.extendBy(min);
         bbox.extendBy(max);
		}

        {
        ESS_PROFILE_SCOPE("AlembicPoints::SAVE - numParticlesLoop - end loop save");

		//move everything from world space to local space
		pos = pos * nodeWorldTransInv;

		Abc::V4f vel4(vel.x, vel.y, vel.z, 0.0);
		vel4 = vel4 * nodeWorldTransInv;
		vel.setValue(vel4.x, vel4.y, vel4.z);

		//scale = scale * nodeWorldTransInv;
		//orientation = Abc::extractQuat(orientation.toMatrix44() * nodeWorldTransInv);
		//spin = Abc::extractQuat(spin.toMatrix44() * nodeWorldTransInv);

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
	}

    }

    if (numParticles > 1)
    {
       ESS_PROFILE_SCOPE("AlembicPoints::Save - vectorResize");
        if (constantPos)        { positionVec.resize(1); }
        if (constantVel)        { velocityVec.resize(1); }
        if (constantScale)      { scaleVec.resize(1); }
        if (constantWidth)      { widthVec.resize(1); }
        if (constantAge)        { ageVec.resize(1); }
        if (constantOrientation){ orientationVec.resize(1); }
        if (constantAngularVel) { angularVelocityVec.resize(1); }
		if (constantColor)		{ colorVec.resize(1); }
    }

    {
    ESS_PROFILE_SCOPE("AlembicPoints::Save - sample writing");
    // Store the information into our properties and points schema
    Abc::P3fArraySample positionSample( positionVec);
    Abc::P3fArraySample velocitySample(velocityVec);
    Abc::P3fArraySample scaleSample(scaleVec);
    Abc::FloatArraySample widthSample(widthVec);
    Abc::FloatArraySample ageSample(ageVec);
    Abc::FloatArraySample massSample(massVec);
    Abc::FloatArraySample shapeTimeSample(shapeTimeVec);
    Abc::UInt64ArraySample idSample(idVec);
    Abc::UInt16ArraySample shapeTypeSample(shapeTypeVec);
    Abc::UInt16ArraySample shapeInstanceIDSample(shapeInstanceIDVec);
    Abc::QuatfArraySample orientationSample(orientationVec);
    Abc::QuatfArraySample angularVelocitySample(angularVelocityVec);
    Abc::C4fArraySample colorSample(colorVec);  

    mScaleProperty.set(scaleSample);
    mAgeProperty.set(ageSample);
    mMassProperty.set(massSample);
    mShapeTimeProperty.set(shapeTimeSample);
    mShapeTypeProperty.set(shapeTypeSample);
    mShapeInstanceIDProperty.set(shapeInstanceIDSample);
    mOrientationProperty.set(orientationSample);
    mAngularVelocityProperty.set(angularVelocitySample);
    mColorProperty.set(colorSample);

    mPointsSample.setPositions(positionSample);
    mPointsSample.setVelocities(velocitySample);
    mPointsSample.setWidths(AbcG::OFloatGeomParam::Sample(widthSample, AbcG::kVertexScope));
    mPointsSample.setIds(idSample);
    mPointsSample.setSelfBounds(bbox);
	mPointsSchema.getChildBoundsProperty().set( bbox);

    mPointsSchema.set(mPointsSample);
    }

    mNumSamples++;

	//mInstanceNames.pop_back();

	if(bAutomaticInstancing){
		saveCurrentFrameMeshes();
	}

	if(bRenderStateForced){
		ipfSystem->SetRenderState(false);
	}

    if(bLastFrame){
       ESS_PROFILE_SCOPE("AlembicParticles::Save - save instance names property");

       std::vector<std::string> instanceNames(mNumShapeMeshes);

       for(faceVertexHashToShapeMap::iterator it = mShapeMeshCache.begin(); it != mShapeMeshCache.end(); it++){
          std::stringstream pathStream;
          pathStream << "/" << it->second.name << "Xfo/" << it->second.name;
          instanceNames[it->second.nMeshInstanceId] = pathStream.str();
       }

	   //for some reason the .dims property is not written when there is exactly one entry if we don't push an empty string
	   //having an extra unreferenced entry seems to be harmless
	   instanceNames.push_back("");

       mInstanceNamesProperty.set(Abc::StringArraySample(instanceNames));
    }

    return true;
}

// static
void AlembicPoints::ConvertMaxEulerXYZToAlembicQuat(const Point3 &degrees, Abc::Quatd &quat)
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
void AlembicPoints::ConvertMaxAngAxisToAlembicQuat(const AngAxis &angAxis, Abc::Quatd &quat)
{
    Abc::V3f alembicAxis = ConvertMaxNormalToAlembicNormal(angAxis.axis);
    quat.setAxisAngle(alembicAxis, angAxis.angle);
    quat.normalize();
}


Abc::C4f AlembicPoints::GetColor(IParticleObjectExt *pExt, int particleId, TimeValue ticks)
{
	Abc::C4f color(0.5, 0.5, 0.5, 1.0);

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

void AlembicPoints::ReadShapeFromOperator( IParticleGroup *particleGroup, PFSimpleOperator *pSimpleOperator, int particleId, TimeValue ticks, ShapeType &type, Abc::uint16_t &instanceId, float &animationTime)
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

		bool bFlatten = GetCurrentJob()->GetOption("flattenHierarchy");
		std::string nodePath = getNodeAlembicPath( EC_MCHAR_to_UTF8( pNode->GetName() ), bFlatten);

        // Find if the name is alerady registered, otherwise add it to the list
   //     instanceId = FindInstanceName(nodePath);
   //     if (instanceId == USHRT_MAX)
   //     {
			//mInstanceNames.push_back(nodePath);
   //         instanceId = (Abc::uint16_t)mInstanceNames.size()-1;
   //     }

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

void AlembicPoints::GetShapeType(IParticleObjectExt *pExt, int particleId, TimeValue ticks, ShapeType &type, Abc::uint16_t &instanceId, float &animationTime)
{
    // Set up initial values
    type = ShapeType_Point;
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
		
		//if(sInfo.type == ShapeType_NbElements){
		//	sInfo.type = type;
		//	sInfo.animationTime = animationTime;
		//	if(sInfo.type == ShapeType_Instance){
		//		sInfo.instanceName = mInstanceNames[instanceId];
		//	}
		//}
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
			//instanceId = FindInstanceName(sInfo.instanceName);
			//if (instanceId == USHRT_MAX)
			//{
			//	mInstanceNames.push_back(sInfo.instanceName);
			//	instanceId = (Abc::uint16_t)mInstanceNames.size()-1;
			//}
			//type = sInfo.type;
		}
		else{
			int nBornIndex = pExt->GetParticleBornIndex(particleId);
			ESS_LOG_INFO("Could not determine shape type for particle with born index: "<<nBornIndex<<". Defaulting to point.");
 			type = ShapeType_Point;
		}
	}
}

//Abc::uint16_t AlembicPoints::FindInstanceName(const std::string& name)
//{
//    ESS_PROFILE_FUNC();
//
//	for ( int i = 0; i < mInstanceNames.size(); i += 1){
//		if (strcmp(mInstanceNames[i].c_str(), name.c_str()) == 0){
//			return i;
//		}
//	}
//	return USHRT_MAX;
//}


Abc::Box3d computeBoundingBox(Mesh* pShapeMesh)
{
   Abc::Box3d box;
   box.min = Abc::V3d(DBL_MAX, DBL_MAX, DBL_MAX);
   box.max = Abc::V3d(DBL_MIN, DBL_MIN, DBL_MIN);

   for(int i=0; i<pShapeMesh->numVerts; i++)
   {
      Point3& p = pShapeMesh->verts[i];

      if(p.x < box.min.x){
         box.min.x = p.x;
      }
      if(p.y < box.min.y){
         box.min.y = p.y;
      }
      if(p.z < box.min.z){
         box.min.z = p.z;
      }

      if(p.x > box.max.x){
         box.max.x = p.x;
      }
      if(p.y > box.max.y){
         box.max.y = p.y;
      }
      if(p.z > box.max.z){
         box.max.z = p.z;
      }
   }

   return box;
}

AlembicPoints::meshInfo AlembicPoints::CacheShapeMesh(Mesh* pShapeMesh, BOOL bNeedDelete, Matrix3 meshTM, int nMatId, int particleId, TimeValue ticks, ShapeType &type, Abc::uint16_t &instanceId, float &animationTime)
{
    ESS_PROFILE_FUNC();

	type = ShapeType_Instance;
	//animationTime = 0;
	
	//Mesh* pShapeMesh = pExt->GetParticleShapeByIndex(particleId);

	if(pShapeMesh->getNumFaces() == 0 || pShapeMesh->getNumVerts() == 0){
		meshInfo mi;
      return mi;
	}

    meshDigests digests;
    {
    ESS_PROFILE_SCOPE("AlembicPoints::CacheShapeMesh - compute hash");
	AbcU::MurmurHash3_x64_128( pShapeMesh->verts, pShapeMesh->numVerts * sizeof(Point3), sizeof(Point3), digests.Vertices.words );
	AbcU::MurmurHash3_x64_128( pShapeMesh->faces, pShapeMesh->numFaces * sizeof(Face), sizeof(Face), digests.Faces.words );
	if(mJob->GetOption("exportMaterialIds")){

		std::vector<MtlID> matIds;

		if(nMatId < 0){
			matIds.reserve(pShapeMesh->getNumFaces());
			for(int i=0; i<pShapeMesh->getNumFaces(); i++){
				matIds.push_back(pShapeMesh->getFaceMtlIndex(i));
			}
		}
		else{
			matIds.push_back(nMatId);
		}

		AbcU::MurmurHash3_x64_128( &matIds[0], matIds.size() * sizeof(MtlID), sizeof(MtlID), digests.MatIds.words );
	}
	if(mJob->GetOption("exportUVs")){
		//TODO...
	}
    }
	
	mTotalShapeMeshes++;

	meshInfo currShapeInfo;
	faceVertexHashToShapeMap::iterator it = mShapeMeshCache.find(digests);
	if( it != mShapeMeshCache.end() ){
		currShapeInfo = it->second;
	}
	else{
		meshInfo& mi = mShapeMeshCache[digests];
		mi.pMesh = pShapeMesh;

      mi.bbox = computeBoundingBox(pShapeMesh);

		mi.nMatId = nMatId;
		
		std::stringstream nameStream;
		nameStream<< EC_MCHAR_to_UTF8( mMaxNode->GetName() ) <<"_";
		nameStream<<"InstanceMesh"<<mNumShapeMeshes;
		mi.name=nameStream.str();

		mi.bNeedDelete = bNeedDelete;
		mi.meshTM = meshTM;
        mi.nMeshInstanceId = mNumShapeMeshes;

		currShapeInfo = mi;
		mNumShapeMeshes++;

		mMeshesToSaveForCurrentFrame.push_back(&mi);

		//ESS_LOG_WARNING("ticks: "<<ticks<<" particleId: "<<particleId);
		//ESS_LOG_WARNING("Adding shape with hash("<<vertexDigest.str()<<", "<<faceDigest.str()<<") \n auto assigned name "<<mi.name <<" efficiency:"<<(double)mNumShapeMeshes/mTotalShapeMeshes);
	}

    instanceId = currShapeInfo.nMeshInstanceId;

 //   {
 //      ESS_PROFILE_SCOPE("CacheShapeMesh push_back instance name");

	//std::string pathName("/");
	//pathName += currShapeInfo.name;

	//instanceId = FindInstanceName(pathName);
	//if (instanceId == USHRT_MAX){
	//	mInstanceNames.push_back(pathName);
	//	instanceId = (Abc::uint16_t)mInstanceNames.size()-1;
	//}

 //   }

    return currShapeInfo;
}

//we have to save out all mesh encountered on the current frame of this object immediately, because 
//the pointers will no longer be valid when the object is evaluated at a new time
void AlembicPoints::saveCurrentFrameMeshes()
{
    ESS_PROFILE_FUNC();

	for(int i=0; i<mMeshesToSaveForCurrentFrame.size(); i++){

        //TODO: save immediately instead accumulating in a vector
		meshInfo* mi = mMeshesToSaveForCurrentFrame[i];

		if(mi->pMesh){
			Mesh* pMesh = mi->pMesh;
			//mi->pMesh = NULL;//each mesh only needs to be saved once

			//Matrix3 worldTrans;
			//worldTrans.IdentityMatrix();

			CommonOptions options;
         options.SetOption("exportNormals", mJob->GetOption("exportNormals"));
			options.SetOption("exportMaterialIds", mJob->GetOption("exportMaterialIds"));

			//gather the mesh data
			mi->meshTM.IdentityMatrix();
			IntermediatePolyMesh3DSMax finalPolyMesh;
            {
            ESS_PROFILE_SCOPE("AlembicPoints::saveCurrentFrameMeshes - finalPolyMesh.Save");

            Imath::M44f transform44f;
            ConvertMaxMatrixToAlembicMatrix( mi->meshTM, transform44f );
            SceneNodeMaxParticlesPtr inputSceneNode(new SceneNodeMaxParticles(pMesh, NULL, mi->nMatId));
            finalPolyMesh.Save(inputSceneNode, transform44f, options, 0.0);

            }

            AbcG::OPolyMeshSchema::Sample meshSample;
            AbcG::OPolyMeshSchema meshSchema;
            {
            ESS_PROFILE_SCOPE("AlembicPoints::saveCurrentFrameMeshes - save xforms, bbox, geo, topo");
			//save out the mesh xForm
			//std::string xformName = mi->name + "Xfo";
			AbcG::OXform xform(mJob->GetArchive().getTop(), mi->name, GetCurrentJob()->GetAnimatedTs());
			AbcG::OXformSchema& xformSchema = xform.getSchema();//mi->xformSchema;

			std::string meshName = mi->name + "Shape";
			AbcG::OPolyMesh mesh(xform, meshName, GetCurrentJob()->GetAnimatedTs());
			meshSchema = mesh.getSchema();

			AbcG::XformSample xformSample;

			Matrix3 alembicMatrix;
			alembicMatrix.IdentityMatrix();
			alembicMatrix.SetTrans(Point3(-10000.0f, -10000.0f, -10000.0f));
			Abc::M44d iMatrix;
			ConvertMaxMatrixToAlembicMatrix(alembicMatrix, iMatrix);

			xformSample.setMatrix(iMatrix);
			xformSchema.set(xformSample);

			//update the archive bounding box
			Abc::Box3d bbox;
			bbox.min = finalPolyMesh.bbox.min * iMatrix;
			bbox.max = finalPolyMesh.bbox.max * iMatrix;

			mJob->GetArchiveBBox().extendBy(bbox);

			//save out the mesh data			
			meshSample.setPositions(Abc::P3fArraySample(finalPolyMesh.posVec));
			meshSample.setSelfBounds(finalPolyMesh.bbox);
			meshSchema.getChildBoundsProperty().set(finalPolyMesh.bbox);

			meshSample.setFaceCounts(Abc::Int32ArraySample(finalPolyMesh.mFaceCountVec));
			meshSample.setFaceIndices(Abc::Int32ArraySample(finalPolyMesh.mFaceIndicesVec));
            }

			if(mJob->GetOption("validateMeshTopology")){
                ESS_PROFILE_SCOPE("AlembicPoints::saveCurrentFrameMeshes - validateMeshTopology");
				mJob->mMeshErrors += validateAlembicMeshTopo(finalPolyMesh.mFaceCountVec, finalPolyMesh.mFaceIndicesVec, mi->name);
			}

			if(mJob->GetOption("exportNormals")){
                ESS_PROFILE_SCOPE("AlembicPoints::saveCurrentFrameMeshes - exportNormals");
				AbcG::ON3fGeomParam::Sample normalSample;
				normalSample.setScope(AbcG::kFacevaryingScope);
				normalSample.setVals(Abc::N3fArraySample(finalPolyMesh.mIndexedNormals.values));
				normalSample.setIndices(Abc::UInt32ArraySample(finalPolyMesh.mIndexedNormals.indices));
				meshSample.setNormals(normalSample);
			}

			if(mJob->GetOption("exportMaterialIds")){
                ESS_PROFILE_SCOPE("AlembicPoints::saveCurrentFrameMeshes - exportMaterialIds");
				Abc::OUInt32ArrayProperty mMatIdProperty = 
					Abc::OUInt32ArrayProperty(meshSchema, ".materialids", meshSchema.getMetaData(), mJob->GetAnimatedTs());
				mMatIdProperty.set(Abc::UInt32ArraySample(finalPolyMesh.mMatIdIndexVec));

				for ( facesetmap_it it=finalPolyMesh.mFaceSets.begin(); it != finalPolyMesh.mFaceSets.end(); it++)
				{
					std::stringstream nameStream;
					int nMaterialId = it->first+1;
					nameStream<<it->second.name<<"_"<<nMaterialId;

					std::vector<Abc::int32_t>& faceSetVec = it->second.faceIds;

					AbcG::OFaceSet faceSet = meshSchema.createFaceSet(nameStream.str());
					AbcG::OFaceSetSchema::Sample faceSetSample(Abc::Int32ArraySample(&faceSetVec.front(), faceSetVec.size()));
					faceSet.getSchema().set(faceSetSample);
				}
			}

			if(mJob->GetOption("exportUVs")){
				//TODO...
			}

            {
            ESS_PROFILE_SCOPE("AlembicPoints::saveCurrentFrameMeshes meshSchema.set");
			meshSchema.set(meshSample);
            }

			if(mi->bNeedDelete){
				delete mi->pMesh;
				mi->bNeedDelete = FALSE;
			}
			mi->pMesh = NULL;
		}
	}
	
	mMeshesToSaveForCurrentFrame.clear();
}