@echo off
setlocal
set PROJ=C:\Users\apoor\Desktop\SOS Trade Smarter

:: ---- 1  Build & export features ----
cd "%PROJ%\C++"
if not exist export_features.exe (
    echo Building C++ exporter…
    mingw32-make.exe                         || goto :err
)
export_features.exe "%PROJ%\data\MSFT_1986-03-13_2025-04-06.csv" ^
                     "%PROJ%\data\features.csv"                    || goto :err

:: ---- 2  Activate venv (create if missing) ----
cd "%PROJ%\python"
if not exist .venv (
    py -3.11 -m venv .venv                     || goto :err
    call .venv\Scripts\activate.bat
    pip install --upgrade pip
    pip install -r requirements.txt            || goto :err
) else (
    call .venv\Scripts\activate.bat
)

:: ---- 3  Train / validate / test ----
python train_validate_test.py                  || goto :err
echo.
echo All done ✓
exit /b 0
:err
echo Error encountered. Batch aborted.
exit /b 1
