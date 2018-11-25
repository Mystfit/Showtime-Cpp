@echo off
SETLOCAL EnableDelayedExpansion

set BUILD_FOLDER=%cd%
set CTEST_BIN=%BUILD_FOLDER%\dependencies\cmake\bin\ctest
set CONFIGURATION=debug

REM mkdir %BUILD_FOLDER%\build\Testing

pushd %BUILD_FOLDER%\build
echo Tests in !BUILD_FOLDER!\.appveyor\tests.txt
for /F "Tokens=* Delims=" %%A in (!BUILD_FOLDER!\.appveyor\tests.txt) do (
	echo "hi"
    set TEST_LOG=%BUILD_FOLDER%\build\Testing\%%A.log
    %CTEST_BIN% -R %%A -C %CONFIGURATION% --output-on-fail -V -O %TEST_LOG% --timeout 320

    set TEST_OUTCOME=Passed
    if NOT %errorlevel% == 0 set TEST_OUTCOME=Failed
    call "%BUILD_FOLDER%\.appveyor\get_test_time.bat" %TEST_LOG% TEST_DURATION

    echo Test ran for %TEST_DURATION% duration
popd

ENDLOCAL
