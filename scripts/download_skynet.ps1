#!/usr/bin/env pwsh
# Download skynet from GitHub

$SkynetUrl = "https://github.com/cloudwu/skynet/archive/refs/heads/master.zip"
$OutputPath = "e:\code\trae\mmo\third_party\skynet.zip"
$ExtractPath = "e:\code\trae\mmo\third_party"

Write-Host "Downloading skynet from $SkynetUrl..."

try {
    Invoke-WebRequest -Uri $SkynetUrl -OutFile $OutputPath -ErrorAction Stop
    Write-Host "Download completed successfully"
    
    Write-Host "Extracting skynet..."
    Expand-Archive -Path $OutputPath -DestinationPath $ExtractPath -Force
    
    # Rename the extracted folder
    $ExtractedFolder = Join-Path $ExtractPath "skynet-master"
    $TargetFolder = Join-Path $ExtractPath "skynet"
    
    if (Test-Path $TargetFolder) {
        Remove-Item -Path $TargetFolder -Recurse -Force
    }
    
    Rename-Item -Path $ExtractedFolder -NewName "skynet"
    
    # Remove the zip file
    Remove-Item -Path $OutputPath -Force
    
    Write-Host "Skynet has been successfully downloaded and extracted to $TargetFolder"
} catch {
    Write-Error "Failed to download or extract skynet: $_"
    exit 1
}
