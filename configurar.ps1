param([string]$OpenCVDir = "")
$ErrorActionPreference = 'Stop'
$raizTrabalho = $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($OpenCVDir)) {
    $pastaDependencias = Join-Path $raizTrabalho '.deps'
    $OpenCVDir = Join-Path $pastaDependencias 'opencv/build'
    if (-not (Test-Path -LiteralPath (Join-Path $OpenCVDir 'OpenCVConfig.cmake'))) {
        New-Item -ItemType Directory -Force -Path $pastaDependencias | Out-Null
        $pacoteOpenCV = Join-Path $pastaDependencias 'opencv-4.12.0-windows.exe'
        if (-not (Test-Path -LiteralPath $pacoteOpenCV)) {
            Write-Host 'Baixando o pacote oficial OpenCV 4.12.0...'
            Invoke-WebRequest -Uri 'https://github.com/opencv/opencv/releases/download/4.12.0/opencv-4.12.0-windows.exe' -OutFile $pacoteOpenCV
        }
        $hashEsperado = 'B753B14D880B9BC8D89D6ACD3B665C040BAEC0211078435432FCAE117DB707AF'
        if ((Get-FileHash -LiteralPath $pacoteOpenCV -Algorithm SHA256).Hash -ne $hashEsperado) {
            throw 'O pacote OpenCV nao corresponde ao SHA256 esperado. Remova somente esse download e tente novamente.'
        }
        Write-Host 'Extraindo o OpenCV dentro de .deps...'
        $extracao = Start-Process -FilePath $pacoteOpenCV -ArgumentList ('-o"{0}" -y' -f $pastaDependencias) -WindowStyle Hidden -Wait -PassThru
        if ($extracao.ExitCode -ne 0) { throw 'Falha ao extrair o pacote OpenCV.' }
    }
}

& cmake -S $raizTrabalho -B (Join-Path $raizTrabalho 'build') -G 'Visual Studio 17 2022' -A x64 "-DOpenCV_DIR=$OpenCVDir"
if ($LASTEXITCODE -ne 0) { throw 'Falha na configuracao do CMake.' }
& cmake --build (Join-Path $raizTrabalho 'build') --config Release
if ($LASTEXITCODE -ne 0) { throw 'Falha na compilacao.' }
& ctest --test-dir (Join-Path $raizTrabalho 'build') -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Os testes falharam.' }
Write-Host 'Pronto. Execute .\build\Release\grau_b.exe'
