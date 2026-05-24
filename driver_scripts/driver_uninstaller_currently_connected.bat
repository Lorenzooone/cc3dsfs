@echo off
setlocal

set "VID_TARGET=%1"
set "PID_TARGET=%2"

echo Finding device + driver...
echo.

powershell -NoProfile -Command ^
"$dev = Get-PnpDevice ^| Where-Object { $_.InstanceId -like '*VID_%VID_TARGET%*PID_%PID_TARGET%*' }; ^
foreach ($d in $dev) { ^
    Write-Host 'Device:' $d.InstanceId; ^
    $driver = Get-PnpDeviceProperty -InstanceId $d.InstanceId -KeyName 'DEVPKEY_Device_DriverInfPath' -ErrorAction SilentlyContinue; ^
    if ($driver) { ^
        Write-Host 'Removing driver:' $driver.Data; ^
        pnputil /delete-driver $driver.Data /uninstall /force; ^
    } ^
    Write-Host 'Removing device...'; ^
    pnputil /remove-device $d.InstanceId; ^
}"

echo Done.