param(
    [ValidateSet('mariadb:10.11', 'mysql:8.4')]
    [string]$Image = 'mariadb:10.11',
    [string]$Distro = 'Ubuntu-22.04',
    [switch]$OnlyMoneyClaim,
    [switch]$OnlyBid
)

# Disposable bid/outbid, timed-settlement, and pending-money collection journeys.
# Never reads .env or an existing game database.
$ErrorActionPreference = 'Stop'
if ($OnlyMoneyClaim -and $OnlyBid) { throw 'Choose one focused journey' }
$repository = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$drive = $repository.Substring(0, 1).ToLowerInvariant()
$wslRepository = '/mnt/' + $drive + $repository.Substring(2).Replace('\', '/')
$name = 'duris-claim-source-' + [guid]::NewGuid().ToString('N').Substring(0, 8)
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

    $journeys = if ($OnlyMoneyClaim) { @('money_claim') } elseif ($OnlyBid) { @('bid') } else { @('bid', 'settlement', 'money_claim') }
    foreach ($journey in $journeys) {
        $database = 'economic_schema_test_auction_' + $journey
        wsl -d $Distro -- bash -lc "MYSQL_PWD=$password mysql --protocol=tcp --ssl-mode=PREFERRED -h 127.0.0.1 -P $port -u root -e 'CREATE DATABASE $database CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci'"
        if ($LASTEXITCODE -ne 0) { throw "Schema creation failed: $Image / $journey" }
        wsl -d $Distro -- bash -lc "cd '$wslRepository' && MYSQL_PWD=$password mysql --protocol=tcp --ssl-mode=PREFERRED -h 127.0.0.1 -P $port -u root $database < migrations/bootstrap_multithread_safe.sql"
        if ($LASTEXITCODE -ne 0) { throw "Schema bootstrap failed: $Image / $journey" }
        wsl -d $Distro -- bash -lc "cd '$wslRepository' && MYSQL_PWD=$password mysql --protocol=tcp --ssl-mode=PREFERRED -h 127.0.0.1 -P $port -u root $database < migrations/economic_pending_claim_source.sql"
        if ($LASTEXITCODE -ne 0) { throw "Source migration replay failed: $Image / $journey" }
        $runner = switch ($journey) {
            'bid' { 'run_auction_bid_sql_accounting_mysql.py' }
            'settlement' { 'run_auction_settlement_sql_accounting_mysql.py' }
            'money_claim' { 'run_auction_money_claim_sql_accounting_mysql.py' }
        }
        wsl -d $Distro -- bash -lc "cd '$wslRepository' && ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1 DB_HOST=127.0.0.1 DB_PORT=$port DB_USER=root DB_PASSWD=$password DB_NAME=$database python3 tests/async/$runner"
        if ($LASTEXITCODE -ne 0) { throw "Auction $journey source journey failed: $Image" }
    }
    Write-Output "Auction pending-claim source journeys passed: $Image"
} finally {
    docker rm -f $name | Out-Null
}
