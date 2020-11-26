# System requirements for this script:
#   - git >= 2.7.0
#   - cmake >= 3.13.0
#   - Latest C++ compiler

$chess_root_dir = $PSScriptRoot
$chess_conf_current_dir = $(Get-Location)

try {
    Write-Host "Bootstrapping..."
    & "$chess_root_dir\scripts\bootstrap.ps1"
}
finally {
    Set-Location $chess_conf_current_dir
}
