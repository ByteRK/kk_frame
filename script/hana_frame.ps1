###
 # @Author: hanakami
 # @Date: 2025-05-08 17:08:00
 # @email: hanakami@163.com
 # @LastEditTime: 2025-05-08 18:48:51
 # @FilePath: /hana_frame/script/hana_frame.ps1
 # @Description: 
 # Copyright (c) 2025 by hanakami, All Rights Reserved. 
### 
#
# 脚本描述：
# 用于快速创建Cdroid资源映射到Android Studio项目中的脚本，减少来回拷贝的烦恼
# 1、在Windows下正常创建Android Studio项目
# 2、将服务器上的文件通过SMB映射到Windows本地
# 2、在Android Studio项目下的SRC目录运行本脚本
# 3、在Android Studio中刷新资源文件
#
# 常见问题以及处理方式
# Q:显示在此系统上禁止运行脚本：
# A:以管理员身份运行PowerShell，并执行：Set-ExecutionPolicy RemoteSigned
#
# Q:显示路径不存在，但是资源管理器中实际能访问到
# A:管理员身份运行PowerShell时，未使用到正确的网络凭证
# A:net use \\10.0.0.88\wzt /user:username password

# 固定配置区域 ################################################
# 目标路径
$targetPath = "\\10.0.0.88\wzt\cdroid\apps\hana_frame"
# 定义映射关系：原文件夹名 → 自定义链接名
$folderMap = @{  # $null为保留原文件名，若源文件夹为嵌套文件夹，必须指定自定义链接名
    "assets\color"     = "color"
    "assets\drawable"  = "drawable"
    "assets\layout"    = "layout"
    "assets\mipmap"    = "mipmap"
    "assets\values"    = "values"
    "fonts"            = "font"
}
# 是否自动覆盖已存在项
$enableOverwrite = $true
##############################################################

# 管理员权限检查与保留工作目录
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $currentPath = (Get-Location).Path
    $arguments = "-NoProfile -ExecutionPolicy Bypass -Command `"cd '$currentPath'; & '$PSCommandPath'`""
    Start-Process powershell.exe -Verb RunAs -ArgumentList $arguments
    exit
}

# 打印调试信息
Write-Host "`n[系统信息]" -ForegroundColor Cyan
Write-Host "基础源路径     : $targetPath"
Write-Host "当前工作目录   : $(Get-Location)`n" -ForegroundColor Cyan

# 主逻辑
foreach ($entry in $folderMap.GetEnumerator()) {
    $subFolder = $entry.Key
    $linkName = if ($entry.Value) { $entry.Value } else { $subFolder }

    # 构建源路径
    $source = Join-Path -Path $targetPath -ChildPath $subFolder
    $destination = Join-Path -Path (Get-Location).Path -ChildPath $linkName

    Write-Host "`n[处理条目]" -ForegroundColor Cyan
    Write-Host "子文件夹名     : $subFolder"
    Write-Host "自定义链接名    : $linkName"
    Write-Host "完整源路径     : $source"
    Write-Host "完整目标链接    : $destination"

    # 检查源是否存在
    if (-not (Test-Path $source -PathType Container)) {
        Write-Host "[错误] 源文件夹不存在: $source" -ForegroundColor Red
        continue
    }

    # 处理目标冲突
    $existingItem = Get-Item $destination -ErrorAction SilentlyContinue
    if ($existingItem) {
        Write-Host "[冲突] 存在类型: $($existingItem.GetType().Name)" -ForegroundColor Yellow
        if ($enableOverwrite) {
            try {
                Remove-Item $destination -Force -Recurse -ErrorAction Stop
                Write-Host "[清理] 已删除旧项" -ForegroundColor Cyan
            } catch {
                Write-Host "[失败] 删除失败: $($_.Exception.Message)" -ForegroundColor Red
                continue
            }
        } else {
            continue
        }
    }

    # 创建符号链接
    try {
        $null = New-Item -Path $destination -ItemType SymbolicLink -Value $source -ErrorAction Stop
        Write-Host "[成功] 符号链接已创建 → $( (Get-Item $destination).Target )" -ForegroundColor Green
    } catch {
        Write-Host "[失败] $($_.Exception.Message)" -ForegroundColor Red
    }
}

# 最终验证部分
Write-Host "`n[验证] 当前目录内容:" -ForegroundColor Cyan
Get-ChildItem -Force | Format-Table Name, @{Label="Type";Expression={$_.GetType().Name}}, @{Label="IsLink";Expression={if ($_.Attributes -match "ReparsePoint") { "Yes" } else { "No" }}}, Length, LastWriteTime

# 退出
Write-Host "`n操作完成，按任意键退出..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")  # 等待按键