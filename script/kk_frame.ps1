# @Author: Ricken
# @Email: me@ricken.cn
# @Date: 2025-05-29 09:18:36
# @FilePath: /kk_frame/script/kk_frame.ps1
# @Description: 
# @BugList: 
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
# A:net use \\10.0.0.88\ricken /user:username password
#
# Q:运行无任何提示，且未生成任何软链
# A:莫名其妙问题
# A:可以直接打开管理员终端运行此脚本，不使用UAC自动提权(推荐)
# A:使用VSCode等编辑器打开脚本，检查右下角文件编码是否为<UTF-8 with BOM>
# A:若不是，则可能导致脚本无法正确执行，需要重新保存为<UTF-8 with BOM>格式

# 固定配置区域 ################################################
# 目标网络配置
$targetIP = "10.0.0.88"                 # 服务器IP地址
$targetShare = "ricken"                 # 共享名称
$targetSubPath = "cdroid\apps\kk_frame" # 共享内子路径
$targetUser = "user"                    # 网络凭据用户名
$targetPass = "pass"                    # 网络凭据密码

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

# 设置窗口标题
$host.UI.RawUI.WindowTitle = "KK_Frame 符号链接工具"

# 检测是否通过UAC提权启动
$isElevatedByScript = $false
# $parentProcess = (Get-Process -Id (Get-CimInstance -ClassName Win32_Process -Filter "ProcessId = $PID").ParentProcessId).ProcessName

# 管理员权限检查
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    try {
        $currentPath = (Get-Location).Path
        $arguments = "-NoProfile -ExecutionPolicy Bypass -Command `"cd '$currentPath'; & '$PSCommandPath'`""
        
        # 标记为通过脚本提权
        $isElevatedByScript = $true

        Write-Host "[信息] 正在通过UAC请求管理员权限..." -ForegroundColor Cyan
        
        $arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -ElevatedByScript"
        Start-Process powershell.exe -Verb RunAs -ArgumentList $arguments -ErrorAction Stop
        # Start-Process pwsh.exe -Verb RunAs -ArgumentList $arguments -ErrorAction Stop

        Write-Host "请稍候，脚本将自动重新启动..." -ForegroundColor Cyan
        Write-Host "若启动异常，可以将脚本的Start-Process powershell.exe 替换为 Start-Process pwsh.exe" -ForegroundColor Yellow
        exit
    } catch {
        Write-Host "[错误] 请求管理员权限失败: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "按任意键退出..." -ForegroundColor Cyan
        $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
        exit 1
    }
}

# 开启日志记录
$logPath = Join-Path -Path $PSScriptRoot -ChildPath "kk_frame.log"
Start-Transcript -Path $logPath -Append | Out-Null
Write-Host "`n===== 脚本开始执行 [$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')] =====" -ForegroundColor Cyan

# 构建完整目标路径
$targetPath = "\\$targetIP\$targetShare\$targetSubPath"

# 函数：建立网络连接
function Connect-NetworkPath {
    param (
        [string]$IP,
        [string]$Share,
        [string]$User,
        [string]$Password,
        [string]$SubPath
    )
    
    $networkPath = "\\$IP\$Share"
    $fullPath = "\\$IP\$Share\$SubPath"
    
    try {
        # 检查是否已连接
        $existingConnections = net use | Where-Object { $_ -match [regex]::Escape($networkPath) }
        if ($existingConnections) {
            Write-Host "[网络] 已有连接: $networkPath" -ForegroundColor Cyan
            return $true
        }
        
        # 建立新连接
        Write-Host "[网络] 正在连接: $networkPath ..." -ForegroundColor Cyan
        $netUseCommand = "net use `"$networkPath`" /user:`"$User`" `"$Password`""
        $result = Invoke-Expression $netUseCommand 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[成功] 网络连接已建立" -ForegroundColor Green
            return $true
        } else {
            Write-Host "[失败] 网络连接错误: $result" -ForegroundColor Red
            return $false
        }
    } catch {
        Write-Host "[异常] 网络连接失败: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

# 主执行逻辑
$globalErrorOccurred = $false
try {
    # 打印调试信息
    Write-Host "`n[系统信息]" -ForegroundColor Cyan
    Write-Host "服务器地址     : $targetIP"
    Write-Host "共享名称       : $targetShare"
    Write-Host "子路径         : $targetSubPath"
    Write-Host "完整源路径     : $targetPath"
    Write-Host "当前工作目录   : $(Get-Location)" -ForegroundColor Cyan
    Write-Host "启动方式       : $(if ($isElevatedByScript) {'UAC提权'} else {'直接管理员运行'})`n" -ForegroundColor Cyan

    # 建立网络连接
    if (-not (Connect-NetworkPath -IP $targetIP -Share $targetShare -User $targetUser -Password $targetPass -SubPath $targetSubPath)) {
        throw "无法建立网络连接，请检查凭据和网络设置"
    }

    # 验证目标路径是否存在
    if (-not (Test-Path $targetPath -PathType Container)) {
        throw "目标路径不存在或无法访问: $targetPath"
    }

    # 处理文件夹映射
    foreach ($entry in $folderMap.GetEnumerator()) {
        try {
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

            # 处理目标冲突
            $existingItem = Get-Item $destination -ErrorAction SilentlyContinue
            if ($existingItem) {
                Write-Host "[冲突] 存在类型: $($existingItem.GetType().Name)" -ForegroundColor Yellow
                if ($enableOverwrite) {
                    try {
                        Remove-Item $destination -Force -Recurse -ErrorAction Stop
                        Write-Host "[清理] 已删除旧项" -ForegroundColor Cyan
                    } catch {
                        $globalErrorOccurred = $true
                        Write-Host "[失败] 删除失败: $($_.Exception.Message)" -ForegroundColor Red
                        continue
                    }
                } else {
                    Write-Host "[跳过] 已存在且未启用覆盖" -ForegroundColor Yellow
                    continue
                }
            }

            # 检查源是否存在
            if (-not (Test-Path $source -PathType Container)) {
                Write-Host "[错误] 源文件夹不存在: $source" -ForegroundColor Red
                $globalErrorOccurred = $true
                continue
            }

            # 创建符号链接
            try {
                $null = New-Item -Path $destination -ItemType SymbolicLink -Value $source -ErrorAction Stop
                Write-Host "[成功] 符号链接已创建 → $( (Get-Item $destination).Target )" -ForegroundColor Green
            } catch {
                $globalErrorOccurred = $true
                Write-Host "[失败] $($_.Exception.Message)" -ForegroundColor Red
            }
        } catch {
            $globalErrorOccurred = $true
            Write-Host "[处理条目时出错] $($_.Exception.Message)" -ForegroundColor Red
        }
    }

    # 最终验证部分
    Write-Host "`n[验证] 当前目录内容:" -ForegroundColor Cyan
    Get-ChildItem -Force | Format-Table Name, @{Label="Type";Expression={$_.GetType().Name}}, @{Label="IsLink";Expression={if ($_.Attributes -match "ReparsePoint") { "Yes" } else { "No" }}}, Length, LastWriteTime

} catch {
    $globalErrorOccurred = $true
    Write-Host "`n[严重错误] $($_.Exception.Message)" -ForegroundColor Red
} finally {
    # 显示最终状态
    if ($globalErrorOccurred) {
        Write-Host "`n[执行结果] 脚本执行过程中发生错误，请查看日志" -ForegroundColor Red
    } else {
        Write-Host "`n[执行结果] 所有操作已成功完成" -ForegroundColor Green
    }
    
    # 结束日志记录
    Write-Host "`n===== 脚本执行结束 [$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')] =====" -ForegroundColor Cyan
    try {
        Stop-Transcript | Out-Null
    } catch {
        Write-Host "[警告] 停止日志记录失败: $($_.Exception.Message)" -ForegroundColor Yellow
    }
    
    # 根据启动方式决定等待时间
    $waitSeconds = if ($globalErrorOccurred) { 15 } else { if ($isElevatedByScript) { 15 } else { 0 } }
    
    if ($waitSeconds -gt 0) {
        Write-Host "`n操作完成，窗口将在${waitSeconds}秒后自动关闭..." -ForegroundColor Cyan
        Write-Host "详细日志已保存至: $logPath" -ForegroundColor Cyan
        
        # 确保等待时间执行
        try {
            Start-Sleep -Seconds $waitSeconds
        } catch {
            # 即使Sleep失败也继续
        }
    } else {
        Write-Host "`n操作完成，窗口将保持打开..." -ForegroundColor Cyan
        Write-Host "详细日志已保存至: $logPath" -ForegroundColor Cyan
    }
}
