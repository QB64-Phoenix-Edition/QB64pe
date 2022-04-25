
$Filename = "qb64_win-${Env:PLATFORM}.7z"

Set-Location ..
7z a "-xr@$QB64pe\.ci\common-exclusion.list" "-xr@$QB64pe\.ci\win-exclusion.list" $Filename "QB64pe"

Move-Item -Path $Filename -Destination ./QB64pe
