SETLOCAL EnableDelayedExpansion
appveyor AddMessage "Running tests from %BUILD_FOLDER%/.appveyor/tests.txt"

for /F "Tokens=* Delims=" %%A in (%BUILD_FOLDER%\.appveyor\tests.txt) do (
    appveyor AddTest %%A -Framework ctest -Filename %%A.exe -Outcome Running
    set TEST_LOG=%BUILD_FOLDER%/build/Testing/%%A.log
    %CTEST_BIN% -R %%A -C %CONFIGURATION% --output-on-fail -V -O "%TEST_LOG%" --timeout 320
    set TEST_OUTCOME=Passed
    if NOT %errorlevel% == 0 set TEST_OUTCOME=Failed
    appveyor UpdateTest -Name %%A -Framework ctest -Filename %%A.exe -Outcome %TEST_OUTCOME%
)

ENDLOCAL
