WHERE cl
if %ERRORLEVEL% NEQ 0 CALL vcvars.bat
pushd src
if exist builddesktop\ (
    pushd builddesktop
) else (
    mkdir builddesktop
    pushd builddesktop
    cmake -DPLATFORM=Desktop -GNinja ..
)
ninja
popd
builddesktop\projectname.exe
popd