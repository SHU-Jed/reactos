::
:: PROJECT:     RosBE - ReactOS Build Environment for Windows
:: LICENSE:     GNU General Public License v2. (see LICENSE.txt)
:: FILE:        Root/Clean.cmd
:: PURPOSE:     Clean the ReactOS source directory.
:: COPYRIGHT:   Copyright 2007 Daniel Reimer <reimer.daniel@freenet.de>
::                             Peter Ward <dralnix@gmail.com>
::
::
@echo off
if not defined _ROSBE_DEBUG set _ROSBE_DEBUG=0
if %_ROSBE_DEBUG% == 1 (
    @echo on
)

title Cleaning...

if "%1" == "" (
    call :DEL
    goto :EOC
)
if /i "%1" == "logs" (
    call :LOG
    goto :EOC
)
if /i "%1" == "all" (
    call :DEL
    call :LOG
    goto :EOC
)
if not "%1" == "" (
    echo Unknown parameter specified. Try 'help [COMMAND]'.
    goto :EOC
)

:LOG
::
:: Check if we have any logs to clean, if so, clean them.
::
if exist "%_ROSBE_LOGDIR%\*.txt" (
    echo Cleaning build logs...
    del /f "%_ROSBE_LOGDIR%\*.txt" 1> NUL 2> NUL
    echo Done cleaning build logs.
) else (
    echo ERROR: There are no logs to clean.
)
goto :EOF

:DEL
::
:: Check if we have something to clean, if so, clean it.
::
if exist "obj-i386\." (
    echo Cleaning ReactOS source directory...
    ::
    : Remove directories/makefile.auto created by the build.
    ::
    if exist "obj-i386\." (
        rd /s /q "obj-i386" 1> NUL 2> NUL
    )
    if exist "output-i386\." (
        rd /s /q "output-i386" 1> NUL 2> NUL
    )
    if exist "reactos\." (
        rd /s /q "reactos" 1> NUL 2> NUL
    )
    if exist "makefile*.auto" (
        del "makefile*.auto" 1> NUL 2> NUL
    )
    echo Done cleaning ReactOS source directory.
) else (
    echo ERROR: There is no compiler output to clean.
)
goto :EOF

:EOC
if defined _ROSBE_VERSION (
    title ReactOS Build Environment %_ROSBE_VERSION%
)
