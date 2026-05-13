param(
    [switch]$Forzar,
    [switch]$SinVerificar
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$envPath = Join-Path $root ".env"

function Clean-Key([string]$value) {
    if ([string]::IsNullOrWhiteSpace($value)) {
        return ""
    }
    $trimmed = $value.Trim().Trim('"').Trim("'")
    return -join ($trimmed.ToCharArray() | Where-Object {
        $code = [int][char]$_
        $code -gt 32 -and $code -ne 127 -and $code -ne 65279
    })
}

function Looks-Like-GeminiKey([string]$key) {
    return $key -match '^AIza[0-9A-Za-z_-]{20,}$'
}

function Get-SavedKey {
    if (-not (Test-Path -LiteralPath $envPath)) {
        return ""
    }
    $line = Get-Content -LiteralPath $envPath | Where-Object {
        $_ -match '^\s*(GEMINI_API_KEY|GOOGLE_API_KEY)\s*='
    } | Select-Object -First 1
    if (-not $line) {
        return ""
    }
    return Clean-Key (($line -split '=', 2)[1])
}

function Save-Key([string]$key) {
    [Environment]::SetEnvironmentVariable("GEMINI_API_KEY", $key, "User")
    $env:GEMINI_API_KEY = $key

    $content = @(
        "# Archivo local. No subir al repositorio."
        "GEMINI_API_KEY=$key"
    )
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllLines($envPath, $content, $utf8NoBom)
}

function Test-GeminiKey([string]$key) {
    $headers = @{
        "x-goog-api-key" = $key
        "Content-Type" = "application/json"
    }
    $body = @{
        contents = @(
            @{
                parts = @(
                    @{ text = "Responde solo: OK" }
                )
            }
        )
    } | ConvertTo-Json -Depth 8 -Compress

    $models = @("gemini-2.5-flash", "gemini-2.5-flash-lite", "gemini-2.0-flash", "gemini-1.5-flash")
    foreach ($model in $models) {
        $uri = "https://generativelanguage.googleapis.com/v1beta/models/$model`:generateContent"
        try {
            Invoke-RestMethod -Method Post -Uri $uri -Headers $headers -Body $body -TimeoutSec 30 | Out-Null
            Write-Host "Verificacion OK con $model." -ForegroundColor Green
            return $true
        }
        catch {
            $status = ""
            $detail = $_.Exception.Message
            if ($_.Exception.Response) {
                $status = [int]$_.Exception.Response.StatusCode
                try {
                    $reader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream())
                    $raw = $reader.ReadToEnd()
                    if ($raw) {
                        $detail = $raw
                    }
                }
                catch {}
            }
            Write-Host "No paso con $model. HTTP $status" -ForegroundColor Yellow
        }
    }
    Write-Host "La clave se guardo, pero Gemini no respondio. Revisa permisos/restricciones si la app muestra 403." -ForegroundColor Yellow
    return $false
}

Write-Host "Configuracion de Gemini para Conca Gym C++/Qt" -ForegroundColor Cyan
Write-Host "La clave queda guardada localmente. No vas a tener que cargarla cada vez."
Write-Host ""

$savedKey = Get-SavedKey
if ((Looks-Like-GeminiKey $savedKey) -and -not $Forzar) {
    Save-Key $savedKey
    Write-Host "Gemini ya estaba configurado. No hace falta pegar la clave de nuevo." -ForegroundColor Green
    if (-not $SinVerificar) {
        Test-GeminiKey $savedKey | Out-Null
    }
    Write-Host "Ahora ejecuta: .\run_cpp_qt.bat"
    exit 0
}

$key = ""
try {
    $clipboard = Clean-Key (Get-Clipboard -Raw -ErrorAction SilentlyContinue)
    if (Looks-Like-GeminiKey $clipboard) {
        $useClipboard = Read-Host "Detecte una API key de Gemini en el portapapeles. Usarla? (S/N)"
        if ($useClipboard.Trim().ToUpperInvariant().StartsWith("S")) {
            $key = $clipboard
        }
    }
}
catch {}

if (-not (Looks-Like-GeminiKey $key)) {
    $secureKey = Read-Host "Pega tu GEMINI_API_KEY" -AsSecureString
    $ptr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secureKey)
    try {
        $key = Clean-Key ([Runtime.InteropServices.Marshal]::PtrToStringBSTR($ptr))
    }
    finally {
        if ($ptr -ne [IntPtr]::Zero) {
            [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($ptr)
        }
    }
}

if (-not (Looks-Like-GeminiKey $key)) {
    throw "La clave parece incompleta o no tiene formato de API key de Gemini. Copiala de nuevo desde Google AI Studio."
}

Save-Key $key
Write-Host ""
Write-Host "Listo. Gemini quedo guardado para Conca Gym C++/Qt." -ForegroundColor Green
if (-not $SinVerificar) {
    Test-GeminiKey $key | Out-Null
}
Write-Host "Ahora ejecuta: .\run_cpp_qt.bat"
