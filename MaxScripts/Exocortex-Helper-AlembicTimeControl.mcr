plugin Helper AlembicTimeControl
	name:"Alembic Time Control"
	classID:#(0x4eb65d6, 0x7c454597)
	category:"Alembic"
	extends:dummy
(
	parameters pblock rollout:params
	(
		current type:#float ui:current animatable:true default:0
		offset type:#float ui:offset animatable:true default:0
		factor type:#float ui:factor animatable:true default:1
	)
 
	rollout params "Alembic"
	(
		spinner current "Current Time" range:[-10000,10000,0]
		spinner offset "Time Offset" range:[-10000,10000,0]
		spinner factor "Time Scale" range:[-10000,10000,0]		
	)
 
	tool create
	(
		on mousePoint click do
		(
			#stop
		)
	)
)