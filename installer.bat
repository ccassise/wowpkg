@ECHO OFF

SETLOCAL enabledelayedexpansion

SET app_name=wowpkg
SET wow_dir=World of Warcraft
SET install_path=%APPDATA%\%app_name%
SET bin_path=%install_path%\bin
SET addon_path=_retail_\Interface\AddOns
SET config_json=config.json
SET state_json=state.json
SET catalog_dir=catalog

TITLE %app_name% installer

if EXIST "%install_path%\" (
	ECHO %app_name% install directory already found
	SET /P input=Reinstall [y/N] 
	if /I not !input!==y (
		exit /B 1
	)
)

:: Check that all files/directories needed for installation are present.
SET required_files=%app_name%.exe catalog
for %%f in (%required_files%) do (
	if not EXIST %%f (
		ECHO %%f not found in current directory
		exit /B 1
	)
)


:: Search these paths for the WoW directory.
SET search_paths[0]=%PROGRAMFILES%\%wow_dir%
SET search_paths[1]=%PROGRAMFILES(x86)%\%wow_dir%
SET search_paths[2]=%PUBLIC%\Games\%wow_dir%
SET search_paths[3]=%PROGRAMFILES%\Battle.net\%wow_dir%
SET search_paths[4]=%PROGRAMFILES(x86)%\Battle.net\%wow_dir%

for /L %%i in (0, 1, 4) do (
	if EXIST !search_paths[%%i]! (
		SET wow_path=!search_paths[%%i]!
	)
)

:: TODO: If wow_path is defined ask the user if that is correct and if not they
:: can enter their own path.
if not DEFINED wow_path (
	ECHO %wow_dir% directory not found
	SET /P wow_path=Enter path to "%wow_dir%" directory: 
)

if not EXIST "%wow_path%\" (
	ECHO !wow_path! does not exist
	exit /B 1
)

if not EXIST "%wow_path%\%addon_path%\" (
	ECHO !wow_path!\%addon_path% does not exist
	exit /B 1
)

ECHO %app_name% will be installed to %install_path%
SET /P input=Continue [y/N] 
if /I not !input!==y (
	exit /B 1
)

ECHO Installing to %install_path%
if not EXIST "%install_path%\" (
	ECHO Creating %install_path%
	MKDIR %install_path%
)

if not EXIST "%bin_path%\" (
	ECHO Creating %bin_path%
	MKDIR %bin_path%
)

if not EXIST "%install_path%\%config_json%" (
	ECHO Creating %config_json% file in %install_path%
	ECHO { "addon_path": "%wow_path:\=\\%\\%addon_path:\=\\%" } > %install_path%\%config_json%
)

if not EXIST "%install_path%\%state_json%" (
	ECHO Creating %state_json% in %install_path%
	ECHO {} > %install_path%\%state_json%
)

ECHO Copying %app_name%.exe to %bin_path%
COPY %app_name%.exe %bin_path%

ECHO Copying %catalog_dir% to %install_path%\%catalog_dir
MKDIR %install_path%\%catalog_dir%
COPY %catalog_dir% %install_path%\%catalog_dir%

if not "!PATH:%bin_path%=!" == "%PATH%" (
	ECHO Already in path
) else (
	for /F "usebackq tokens=2,*" %%A in (`reg query HKCU\Environment /V PATH`) do (
		set user_path=%%B
	)

	if DEFINED user_path (
		ECHO Adding %bin_path% to user PATH
		ECHO !user_path!
		SETX PATH "!user_path!;%bin_path%"
	) else (
		ECHO Error getting user path
		ECHO %bin_path% was not added to user PATH
	)
)

ECHO Done

PAUSE
