set "a=%~1"
set "b=%~2"
set "c=%~3"
set "a=%a:/=\%"
set "b=%b:/=\%"
set "c=%c:/=\%"
unzip %a%\WU_FTD3XXLib.zip -d %a%
copy %a%\WU_FTD3XXLib\Lib\%c% %b%\
copy %a%\WU_FTD3XXLib\Lib\FTD3XX.h %b%\ftd3xx.h
