# escape=`

# Use the latest Windows Server Core 2022 image.
FROM --platform=windows/amd64 mcr.microsoft.com/windows/servercore:ltsc2022

# Restore the default Windows shell for correct batch processing.
SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop'; $ProgressPreference = 'SilentlyContinue';"]

# Install Chocolatey
RUN Set-ExecutionPolicy Bypass -Scope Process -Force; `
  [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; `
  iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'));

SHELL ["cmd", "/S", "/C"]

RUN (choco install git.install -y --verbose) ` 
    & (choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System') & (refreshenv)

# Install 7zip to extract files faster.
	RUN curl -SL --output 7zip.msi https://www.7-zip.org/a/7z2201-x64.msi && `
	MsiExec.exe /i 7zip.msi /qn && setx /M PATH "%PATH%;C:\\Program Files\\7-Zip" && `
    # Download the Build Tools bootstrapper.
    curl -SL --output vs_buildtools.exe https://aka.ms/vs/17/release/vs_buildtools.exe `
    `
    # Install Build Tools, excluding workloads and components with known issues.
    && (start /w vs_buildtools.exe --quiet --wait --norestart --nocache `
        --installPath "C:\BuildTools" `
        --add  Microsoft.VisualStudio.Workload.VCTools --includeRecommended`
        || IF "%ERRORLEVEL%"=="3010" EXIT 0) `
    `
    # Cleanup
    && del /q vs_buildtools.exe
	
# Define the entry point for the docker container.
# This entry point starts the developer command prompt and launches the PowerShell shell.
# ENTRYPOINT CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
ENTRYPOINT ["C:\\BuildTools\\Common7\\Tools\\VsDevCmd.bat", "&&", "powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]