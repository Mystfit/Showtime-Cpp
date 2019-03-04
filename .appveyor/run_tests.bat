SETLOCAL EnableDelayedExpansion
appveyor AddMessage "Running tests from %BUILD_FOLDER%\.appveyor\tests.txt"

mkdir %BUILD_FOLDER%\build\Testing

pushd %BUILD_FOLDER%\build
for /F "Tokens=* Delims=" %%A in (%BUILD_FOLDER%\.appveyor\tests.txt) do (
    appveyor AddTest %%A -Framework ctest -Filename %%A.exe -Outcome Running
    set TEST_LOG=%BUILD_FOLDER%\build\Testing\%%A.log
    %CTEST_BIN% -R %%A -C %CONFIGURATION% --output-on-fail -V -O %TEST_LOG% --timeout 320

    set TEST_OUTCOME=Passed
    if NOT %errorlevel% == 0 set TEST_OUTCOME=Failed
    call "%BUILD_FOLDER%\.appveyor\get_test_time.bat" %TEST_LOG% TEST_DURATION

    appveyor UpdateTest -Name %%A -Framework ctest -Filename %%A.exe -Outcome !TEST_OUTCOME! -Duration !TEST_DURATION!
popd

ENDLOCAL
