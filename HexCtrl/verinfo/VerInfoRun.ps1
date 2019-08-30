#Checks for SET_VERINFO environment variable, and only if exist runs it.
#Passes -versionFile and -patchFile command line parameters to it.
param(
    [string]$versionFile,
    [string]$patchFile
)

$set_verinfoPath=[Environment]::GetEnvironmentVariable('SET_VERINFO','User')
if($set_verinfoPath)
{
	&$set_verinfoPath -versionFile "$versionFile" -patchFile "$patchFile"
}