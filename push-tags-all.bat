cd ..

cd ExocortexAlembicShared
call git push --tags %1 %2
cd ..

cd ExocortexAlembicArnold
call git push --tags %1 %2
cd ..

cd ExocortexAlembicPython
call git push --tags %1 %2
cd ..

cd ExocortexAlembicMaya
call git push --tags %1 %2
cd ..

cd ExocortexAlembicSoftimage
call git push --tags %1 %2
cd ..

cd ExocortexAlembic3DSMax
call git push --tags %1 %2
cd ..

cd ExocortexAlembicShared