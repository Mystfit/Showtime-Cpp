@echo off
SETLOCAL EnableDelayedExpansion

rem Tail command in pure Batch, version 2: Tail.bat filename numOfLines
rem Antonio Perez Ayala

set /A firstTail=1, lastTail=0
for /F "delims=" %%a in (%1) do (
   set /A lastTail+=1, lines=lastTail-firstTail+1
   set "lastLine[!lastTail!]=%%a"
   if !lines! gtr 2 (
      set "lastLine[!firstTail!]="
      set /A firstTail+=1
   )
)
for /L %%i in (%firstTail%,1,%lastTail%) do set lastline=!lastLine[%%i]!
for /f "tokens=6" %%i in ("%lastline%") do set TEST_TIME=%%i
for /f  "delims=" %%x in ('powershell %TEST_TIME% * 1000') do set result=%%x

ENDLOCAL & set /A %2=%result%
