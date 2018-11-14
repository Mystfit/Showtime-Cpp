SETLOCAL EnableDelayedExpansion
appveyor AddMessage "Running tests from %BUILD_FOLDER%\.appveyor\tests.txt"

pushd %BUILD_FOLDER%\build

for /F "Tokens=* Delims=" %%A in (%BUILD_FOLDER%\.appveyor\tests.txt) do (
	echo "Running appveyor AddTest %%A -Framework ctest -Filename %%A.exe -Outcome Running"
    appveyor AddTest %%A -Framework ctest -Filename %%A.exe -Outcome Running
    set TEST_LOG=%BUILD_FOLDER%\build\Testing\%%A.log

    echo "Running %CTEST_BIN% -R %%A -C %CONFIGURATION% --output-on-fail -V -O %TEST_LOG% --timeout 320"
    %CTEST_BIN% -R %%A -C %CONFIGURATION% --output-on-fail -V -O %TEST_LOG% --timeout 320

    set TEST_OUTCOME=Passed
    if NOT %errorlevel% == 0 set TEST_OUTCOME=Failed

    echo "Running appveyor UpdateTest -Name %%A -Framework ctest -Filename %%A.exe -Outcome %TEST_OUTCOME%"
    appveyor UpdateTest -Name %%A -Framework ctest -Filename %%A.exe -Outcome %TEST_OUTCOME%
)

popd

ENDLOCAL
