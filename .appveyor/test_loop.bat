SETLOCAL EnableDelayedExpansion

set tests[0]="TestStartup"
set tests[1]="TestEntities"
set tests[2]="TestGraph"
set tests[3]="TestExternalClients"
set tests[4]="TestPlugins"
set tests[5]="TestEntityFactories"
set tests[6]="TestBenchmark"

for /L %%i in (0,1,6) do (
    set test=!tests[%%i]!
    echo !test!
)

ENDLOCAL