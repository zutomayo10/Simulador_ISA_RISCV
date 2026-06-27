# Launcher for the RISC-V simulator via WSL.
# Usage: .\run.ps1 [archivo.bin]
param([string]$bin = "")

$wsl_dir = "/mnt/c/Users/Isaac/Desktop/tarea6_arqui"

# Build if binary is missing
$exists = wsl bash -c "test -f $wsl_dir/build/riscv-sim && echo yes"
if ($exists -ne "yes") {
    Write-Host "Compilando simulador..."
    wsl bash -c "cd $wsl_dir && bash build.sh"
}

if ($bin -eq "") {
    wsl bash -c "cd $wsl_dir && ./build/riscv-sim"
} else {
    # Convert Windows path to WSL path if needed
    $wsl_bin = $bin -replace "\\", "/" -replace "^([A-Za-z]):", { "/mnt/$($_.Groups[1].Value.ToLower())" }
    wsl bash -c "cd $wsl_dir && ./build/riscv-sim '$wsl_bin'"
}
