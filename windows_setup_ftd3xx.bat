set "a=%~1"
set "b=%~2"
set "c=%~3"
set "a=%a:/=\%"
set "b=%b:/=\%"
set "c=%c:/=\%"
copy %a%\%c% %b%\
copy %a%\FTD3XX.h %b%\
