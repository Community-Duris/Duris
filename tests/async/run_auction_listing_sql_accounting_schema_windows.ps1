param(
    [ValidateSet('mariadb:10.11', 'mysql:8.4')]
    [string]$Image = 'mariadb:10.11',
    [string]$Distro = 'Ubuntu-22.04'
)

# Disposable auction listing EAP1 journey for a Windows Docker CLI and WSL
# compiler. Never reads .env or an existing game database.
$ErrorActionPreference = 'Stop'
$repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$drive = $repository.Substring(0, 1).ToLowerInvariant()
$wslRepository = '/mnt/' + $drive + $repository.Substring(2).Replace('\', '/')
$name = 'duris-auction-listing-' + [guid]::NewGuid().ToString('N').Substring(0, 8)
$password = [guid]::NewGuid().ToString('N')
$passwordKey = if ($Image.StartsWith('mariadb:')) { 'MARIADB_ROOT_PASSWORD' } else { 'MYSQL_ROOT_PASSWORD' }

try {
    docker run -d --name $name -p 127.0.0.1::3306 -e "${passwordKey}=$password" $Image | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "docker run failed: $Image" }
    $mapping = docker port $name 3306/tcp
    if ($LASTEXITCODE -ne 0) { throw "docker port failed: $Image" }
    $port = ($mapping | Select-Object -First 1).Split(':')[-1]

    $ready = $false
    for ($attempt = 0; $attempt -lt 90; $attempt++) {
        wsl -d $Distro -- bash -lc "MYSQL_PWD=$password mysql --protocol=tcp --ssl-mode=PREFERRED -h 127.0.0.1 -P $port -u root -e 'SELECT 1' >/dev/null 2>&1"
        if ($LASTEXITCODE -eq 0) { $ready = $true; break }
        Start-Sleep -Seconds 1
    }
    if (-not $ready) { throw "Disposable database did not start: $Image" }

    wsl -d $Distro -- bash -lc "MYSQL_PWD=$password mysql --protocol=tcp --ssl-mode=PREFERRED -h 127.0.0.1 -P $port -u root -e 'CREATE DATABASE economic_schema_test_auction CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci'"
    if ($LASTEXITCODE -ne 0) { throw "Schema creation failed: $Image" }
    wsl -d $Distro -- bash -lc "cd '$wslRepository' && MYSQL_PWD=$password mysql --protocol=tcp --ssl-mode=PREFERRED -h 127.0.0.1 -P $port -u root economic_schema_test_auction < migrations/bootstrap_multithread_safe.sql"
    if ($LASTEXITCODE -ne 0) { throw "Schema bootstrap failed: $Image" }
    wsl -d $Distro -- bash -lc "cd '$wslRepository' && ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1 DB_HOST=127.0.0.1 DB_PORT=$port DB_USER=root DB_PASSWD=$password DB_NAME=economic_schema_test_auction python3 tests/async/run_auction_listing_sql_accounting_mysql.py"
    if ($LASTEXITCODE -ne 0) { throw "Listing journey failed: $Image" }
    Write-Output "Auction listing EAP1 journey passed: $Image"
} finally {
    docker rm -f $name | Out-Null
}
