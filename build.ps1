$ErrorActionPreference = "Stop"

$avrGccCandidates = @(
    "C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-gcc.exe",
    "C:\Program Files\Microchip\xc8\v2.36\avr\bin\avr-gcc.exe"
)

$avrObjcopyCandidates = @(
    "C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-objcopy.exe",
    "C:\Program Files\Microchip\xc8\v2.36\avr\bin\avr-objcopy.exe"
)

$avrGcc = $avrGccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
$avrObjcopy = $avrObjcopyCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $avrGcc) {
    throw "avr-gcc nao encontrado. Instale Atmel Studio/Microchip Studio ou adicione avr-gcc ao PATH."
}

if (-not $avrObjcopy) {
    throw "avr-objcopy nao encontrado. Instale Atmel Studio/Microchip Studio ou adicione avr-objcopy ao PATH."
}

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root "build"
$proteus = Join-Path $root "simulation\proteus"

New-Item -ItemType Directory -Force -Path $build, $proteus | Out-Null

$objects = @()
Get-ChildItem -Path "$root\firmware" -Filter "*.c" | ForEach-Object {
    $object = Join-Path $build ($_.BaseName + ".o")
    & $avrGcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra -I"$root\firmware" -c $_.FullName -o $object
    $objects += $object
}

& $avrGcc -mmcu=atmega328p $objects -o "$build\forno-fundicao-pessoa1.elf"
& $avrObjcopy -O ihex -R .eeprom "$build\forno-fundicao-pessoa1.elf" "$build\forno-fundicao-pessoa1.hex"

Copy-Item -Force "$build\forno-fundicao-pessoa1.hex" "$proteus\forno-fundicao.hex"

Write-Host "Build OK"
Write-Host "HEX Proteus: $proteus\forno-fundicao.hex"
