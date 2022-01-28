WHERE cl
if %ERRORLEVEL% NEQ 0 CALL vcvars.bat
pushd src
if exist builddesktop\ (
    pushd builddesktop
) else (
    mkdir builddesktop
    pushd builddesktop
    cmake -DPLATFORM=Desktop -GNinja .. || goto :error
)
ninja || goto :error
popd
builddesktop\projectname.exe || goto :error
popd


:error
echo Failed with error #%errorlevel%.
popd
popd
popd
popd
exit /b %errorlevel%