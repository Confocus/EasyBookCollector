@echo off
echo 检查Firefox原生消息主机配置...
echo.

echo 1. 检查用户级目录：
if exist "%APPDATA%\Mozilla\NativeMessagingHosts\com.demo.native_host.json" (
    echo [√] 找到: %APPDATA%\Mozilla\NativeMessagingHosts\com.demo.native_host.json
    type "%APPDATA%\Mozilla\NativeMessagingHosts\com.demo.native_host.json"
) else (
    echo [×] 未找到: %APPDATA%\Mozilla\NativeMessagingHosts\com.demo.native_host.json
)

echo.
echo 2. 检查系统级目录：
if exist "C:\ProgramData\Mozilla\NativeMessagingHosts\com.demo.native_host.json" (
    echo [√] 找到: C:\ProgramData\Mozilla\NativeMessagingHosts\com.demo.native_host.json
) else (
    echo [×] 未找到: C:\ProgramData\Mozilla\NativeMessagingHosts\com.demo.native_host.json
)

echo.
echo 3. 检查路径是否正确：
echo 当前路径: D:\AllProjects\GlobalBookmark\NativeMessageDemo\NativeMessageDemo\x64\Debug\NativeMessageDemo.exe
if exist "D:\AllProjects\GlobalBookmark\NativeMessageDemo\NativeMessageDemo\x64\Debug\NativeMessageDemo.exe" (
    echo [√] EXE文件存在
) else (
    echo [×] EXE文件不存在！
)

pause