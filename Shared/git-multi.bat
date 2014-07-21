call git %1 %2 %3 %4 %5 %6
cd ..

IF NOT EXIST ExocortexAlembicArnold GOTO NoExocortexAlembicArnold

cd ExocortexAlembicArnold
call git %1 %2 %3 %4 %5 %6
cd ..

:NoExocortexAlembicArnold

IF NOT EXIST ExocortexAlembicPython GOTO NoExocortexAlembicPython

cd ExocortexAlembicPython
call git %1 %2 %3 %4 %5 %6
cd ..

:NoExocortexAlembicPython

IF NOT EXIST ExocortexAlembicMaya GOTO NoExocortexAlembicMaya

cd ExocortexAlembicMaya
call git %1 %2 %3 %4 %5 %6
cd ..

:NoExocortexAlembicMaya

IF NOT EXIST ExocortexAlembicSoftimage GOTO NoExocortexAlembicSoftimage

cd ExocortexAlembicSoftimage
call git %1 %2 %3 %4 %5 %6
cd ..

:NoExocortexAlembicSoftimage

IF NOT EXIST ExocortexAlembic3DSMax GOTO NoExocortexAlembic3DSMax

cd ExocortexAlembic3DSMax
call git %1 %2 %3 %4 %5 %6
cd ..

:NoExocortexAlembicSoftimage

cd ExocortexAlembicShared
