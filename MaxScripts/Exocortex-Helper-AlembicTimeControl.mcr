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
        offset type:#float ui:offset animatable:true default:0
        factor type:#float ui:factor animatable:true default:1

        gScale type:#integer ui:gScaleSp animatable:false default:100
    )

    rollout params "Alembic"
    (
        spinner current "Current Time" range:[-10000,10000,0]
        spinner offset "Time Offset" range:[-10000,10000,0]
        spinner factor "Time Scale" range:[-10000,10000,0]      

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