CALL emsdk.bat
CALL vcvars.bat
pushd src
mkdir buildweb
pushd buildweb

CALL emcmake cmake -DPLATFORM=Web -GNinja ..
ninja

popd
popd