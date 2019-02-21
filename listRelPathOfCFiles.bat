@ECHO OFF
SETLOCAL DisableDelayedExpansion
SET "r=%__CD__%"
FOR /R . %%F IN (*.c) DO (
  SET "p=%%F"
  SETLOCAL EnableDelayedExpansion
  ECHO(!p:%r%=!
  ENDLOCAL
)