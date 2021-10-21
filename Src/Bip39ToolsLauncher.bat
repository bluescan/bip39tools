@echo off
:start
echo.
echo 1) dice2bip39
echo 2) finalwordsbip39
echo 3) validatebip39
echo 4) makecompliantbip39
echo 5) unittestsbip39
echo 6) quit
echo.
set /p x=Choose tool:
IF '%x%' == '%x%' GOTO Item_%x%

:Item_1
dice2bip39.exe
GOTO Start

:Item_2
finalwordsbip39.exe
GOTO Start

:Item_3
validatebip39.exe
GOTO Start

:Item_4
makecompliantbip39.exe
GOTO Start

:Item_5
unittestsbip39.exe
GOTO Start

:Item_6
