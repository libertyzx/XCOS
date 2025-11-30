@echo off
echo.
echo 清理编译中生成的中间文件[V0.1]
::pause
echo.

echo Keil下List文件
del /f /s /q  *.lst
del /f /s /q  *.map
del /f /s /q  *.i
echo.
echo Keil下Obj文件
del /f /s /q  *.__i
del /f /s /q  *.obj
del /f /s /q  *._ia
del /f /s /q  *.htm
del /f /s /q  *.lnp
del /f /s /q  *.ORC
del /f /s /q  *.SBR
del /f /s /q  *.d
del /f /s /q  *.crf
del /f /s /q  *.dep
del /f /s /q  *.axf
::del /f /s /q  *.sct   ::flash,ram分布
echo.
echo IAR下Obj文件
del /f /s /q  *.pbi
del /f /s /q  *.pbd
del /f /s /q  *.o
del /f /s /q  *.cout
del /f /s /q  *.browse
del /f /s /q  *.r51
echo.

echo Source Insight产生的缓存
::del /f /s /q  *.sisc
echo.

echo 清理完成！
pause