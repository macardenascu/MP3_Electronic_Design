# Activate the external ESP-IDF 6.1 installation for this PoC.
$idfRoot = 'C:\Espressif\v6.1\esp-idf'
$idfToolsRoot = 'C:\Espressif\tools'
$idfPython = Join-Path $idfToolsRoot 'python_env\idf6.1_py3.14_env\Scripts'
$gitPath = 'C:\Program Files\Git\cmd'

if (-not (Test-Path (Join-Path $idfRoot 'export.ps1'))) {
    throw "ESP-IDF 6.1 no encontrado en $idfRoot"
}
if (-not (Test-Path (Join-Path $idfPython 'python.exe'))) {
    throw "Entorno Python de ESP-IDF no encontrado en $idfPython"
}

$env:IDF_TOOLS_PATH = $idfToolsRoot
$env:PYTHONUTF8 = '1'
$env:PATH = "$idfPython;$gitPath;$env:PATH"
. (Join-Path $idfRoot 'export.ps1')
