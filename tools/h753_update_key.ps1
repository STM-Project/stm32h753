# Read by powershell.exe -Command - from Debug or Release.
& {
    $ErrorActionPreference = 'Stop'
    try {
        $build = (Get-Location).Path
        $keyDir = [IO.Path]::GetFullPath([IO.Path]::Combine($build, '../Debug/bootloader-app'))
        [void][IO.Directory]::CreateDirectory($keyDir)
        $keyFile = [IO.Path]::Combine($keyDir, 'update-key')
        $encoding = [Text.Encoding]::ASCII
        if (-not [IO.File]::Exists($keyFile)) {
            $bytes = New-Object byte[] 16
            $rng = [Security.Cryptography.RandomNumberGenerator]::Create()
            try {
                $rng.GetBytes($bytes)
            } finally {
                $rng.Dispose()
            }
            $key = [BitConverter]::ToString($bytes).Replace('-', '').ToLowerInvariant()
            $stream = [IO.File]::Open($keyFile, [IO.FileMode]::CreateNew)
            try {
                $encoded = $encoding.GetBytes($key + [Environment]::NewLine)
                $stream.Write($encoded, 0, $encoded.Length)
            } finally {
                $stream.Dispose()
            }
        }

        $key = [IO.File]::ReadAllText($keyFile, $encoding).Trim()
        if ($key -cnotmatch '^[0-9a-f]{32}$') {
            throw 'update-key must contain 32 lowercase hexadecimal characters'
        }
        $header = [IO.Path]::Combine($build, 'h753_update_key.h')
        $contents = '#define H753_BOOT_UPDATE_KEY "' + $key + '"' + [Environment]::NewLine
        if (-not [IO.File]::Exists($header) -or [IO.File]::ReadAllText($header, $encoding) -cne $contents) {
            [IO.File]::WriteAllText($header, $contents, $encoding)
        }
        exit 0
    } catch {
        [Console]::Error.WriteLine($_.Exception.Message)
        exit 1
    }
}

# End of the PowerShell stdin command.
