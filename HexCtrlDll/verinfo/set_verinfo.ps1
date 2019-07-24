#verpatch.exe can be found at: https://github.com/pavel-a/ddverpatch/releases
#Path to verpatch.exe
$verpatch = "$PSScriptRoot\verpatch.exe"
$versionfile = "..\HexCtrl\verinfo\version.h"

$PRODUCTNAME=""
$PRODUCTDESC=""
$COPYRIGHT=""
$MAJOR=""
$MINOR=""
$MAINTENANCE=""
$REVISION=""

$lines = Get-Content $versionfile
foreach ($line in $lines) {
#Split each line in 3 words separated by spaces
$fields=$line -Split ('\s+', 3)

if ($fields[1] -eq "PRODUCT_NAME"){
$PRODUCTNAME=$fields[2]
}
if ($fields[1] -eq "PRODUCT_DESC"){
$PRODUCTDESC=$fields[2]
}
if ($fields[1] -eq "COPYRIGHT_NAME"){
$COPYRIGHT=$fields[2]
}
if ($fields[1] -eq "MAJOR_VERSION"){
$MAJOR=$fields[2]
}
if ($fields[1] -eq "MINOR_VERSION"){
$MINOR=$fields[2]
}
if ($fields[1] -eq "MAINTENANCE_VERSION"){
[int]$MAINTENANCE=$fields[2]
}
if ($fields[1] -eq "REVISION_VERSION"){
$REVISION=$fields[2]
}

}

$VERSION="$MAJOR.$MINOR.$MAINTENANCE.$REVISION"

#Executing verpatch.exe with all args.
&$verpatch /va "$args" $VERSION /pv $VERSION /s copyright $COPYRIGHT /s ProductName $PRODUCTNAME /s desc $PRODUCTDESC

#Increasing current version by 1 and writing it back to $versionfile.
#So $versionfile always contains version number that's gonna be used on next compilation.
##$MAINTENANCE=$MAINTENANCE + 1
##$regexA=[regex]"#define MAINTENANCE_VERSION(.*)"
##$content = [System.IO.File]::ReadAllText($versionfile) -Replace($regexA,"#define MAINTENANCE_VERSION		$MAINTENANCE`r")
##[System.IO.File]::WriteAllText($versionfile, $content)