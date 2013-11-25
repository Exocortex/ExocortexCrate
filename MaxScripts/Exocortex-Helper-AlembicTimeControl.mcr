plugin Helper AlembicTimeControl
    name:"Alembic Time Control"
    classID:#(0x4eb65d6, 0x7c454597)
    category:"Alembic"
    extends:dummy
(
    local lastSize=0, meshObj = undefined

    parameters pblock rollout:params
    (
        current type:#float ui:current animatable:true default:0
        offset type:#float ui:offset animatable:false default:0
        factor type:#float ui:factor animatable:false default:1

        cacheStart type:#float default:0 ui:uiCacheStart animatable:off
        cacheEnd type:#float default:animationrange.end ui:uiCacheEnd animatable:off
        frameOffset type:#float default:0 ui:uiFrameOffset animatable:off
        loop type:#Boolean default:true ui:uiLoop animatable:off
        
        timeOut type:#float ui:uiTimeOut animatable:true default:1

        gScale type:#integer ui:gScaleSp animatable:false default:100
        
        
        on current set val do this.OffsetCache()        
        on offset set val do this.OffsetCache()
        on factor set val do this.OffsetCache()
        on frameOffset set val do this.OffsetCache()        
        on cacheStart set val do this.OffsetCache()
        on cacheEnd set val do this.OffsetCache()
        on loop set val do this.OffsetCache()
    )

    rollout params "Alembic"
    (
        spinner current "Current Time" range:[-10000,10000,0]
        spinner offset "Time Offset" range:[-10000,10000,0]
        spinner factor "Time Scale" range:[-10000,10000,1]      

        spinner uiCacheStart "Cache Start: " fieldwidth:50 range:[-1e9,1e9,0] type:#float align:#right offset:[-18,8] 
        spinner uiCacheEnd "Cache End: " fieldwidth:50 range:[-1e9,1e9,0] type:#float align:#right offset:[-18,0] 
        spinner uiFrameOffset "Time Offset: " fieldwidth:50 range:[-1e9,1e9,0] type:#float align:#right offset:[-18,8] tooltip:"The frame in-which the cache will begin playing."
        checkbox uiLoop "Loop Cache" align:#right offset:[2,6]
     
        spinner uiTimeOut "Time Out" range:[-10000,10000,0]      
     
        spinner gScaleSp "Gizmo Scale:" range:[10, 1e9, 100] offset:[0,10] type:#integer

        on gScaleSp changed val do
        (
            classInstances = (getClassInstances AlembicTimeControl)
            for i in classInstances do
                for j in (refs.dependents i) do
                    if classOf j ==  AlembicTimeControl do
                        j.scale = [val*.01,val*.01,val*.01]
        )
    )


    fn OffsetCache =
    (
        current.controller = float_expression()
	    current.controller.setExpression "S"

        timeOut.controller = float_expression()
        

        local frameRange = (cacheEnd - cacheStart) / framerate
        local tScale = factor as string
        local timeOffset = ((frameOffset/framerate as float) + offset) as string
        local cStart = (cacheStart / framerate as float) as string
        if Loop then
        (
            fRange = frameRange as string
            fStart = (frameRange as string) as string

            str = "if(S*" + tScale + "-" + timeOffset + ">=0," + \
                    "mod (S*" + tScale + "-" + timeOffset + "," + fRange + ") + " + cStart + \
                    ",mod (S*" + tScale + "-" + timeOffset + "," + fRange + ") + " + cStart + "+ " + fStart + ")"

        )else(
            str = "(S*" + tScale + "-" + timeOffset + cStart + ")"
        )



        timeOut.controller.SetExpression str
        timeOut.controller.Update()
    )

    on getDisplayMesh do 
    (
        if (meshObj == undefined) do 
        (
            meshObj =  createInstance AlembicNull
            lastSize = gScale
        )

        meshObj.mesh
    )

    on useWireColor do 
    (
        false
    )

    tool create
    (
        on mousePoint click do
        (
            #stop
        )

    )
)