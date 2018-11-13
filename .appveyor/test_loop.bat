set BUILD_FOLDER=%cd%

for /F "Tokens=* Delims=" %%A in (%BUILD_FOLDER%\..\.appveyor\tests.txt) do (
    echo Adding %%A test
)
