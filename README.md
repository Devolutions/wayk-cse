# WaykNow custom standalone executable

This project allows us to build custom standalone executable (CSE) for WaykNow.
Currently, only the Windows platform is supported.

#### Features 
- Pack both x86 and x64 WaykNow executables and automatically select the correct binary during the launch
	- Full WaykNow CI packages are required
- Provide custom [branding](https://helpwayk.devolutions.net/advanced_whitelabelbranding.html) for the application
- Launch WaykNow in unattended mode, using one-time service registration
- Customize extraction path / app data / unattended service data path
	- Environment variables can be used in the path using syntax `${VARIABLE}\folder1\folder2`
- Execute custom WaykNow PowerShell initialization script before launch
	- WaykNow-ps functionality can be used, the module itself will be integrated inside CSE, no internet connection needed

#### CSE modules description

**WaykCseDummy.exe** - Executable with logic, required to launch the WaykNow in the standalone mode with advanced customization options (in contrast to the old standalone WaykNow executable). Initially, this executable only contains the code, without required resources (e.g. binaries, init script, branding file). This executable is always a 32-bit application. 

**wayk-cse-patcher** – Utility which implements WaykCseDummy.exe patching to inject the actual WaykNow binaries, branding, script, and WaykNow-ps module to the binary. It provides a lot of options to configure the resulting CSE. If branding is provided, the patcher will also substitute the CSE icon.

#### Requirements
To use wayk-cse-patcher.exe, **7-zip** and **signtool** are should be installed on the machine and added to the **PATH** environment variable

#### How to use

Example call arguments:
```
wayk-cse-patcher.exe
    --with-unattended
    --with-branding "C:\cse\branding.zip"
    --wayk-bin-x86 "C:\cse\WaykNow-x86-2020.2.0.0.zip"
    --wayk-bin-x64 "C:\cse\WaykNow-x64-2020.2.0.0.zip"
    --wayk-cse-dummy-path "C:\cse\WaykCseDymmy.exe"
    --wayk-extraction-path "${APPDATA}\Windjammer"
    --wayk-data-path "${APPDATA}\Windjammer\data"
    --wayk-system-path "${APPDATA}\Windjammer\system"
    --init-script "C:\cse\init.ps1"
    --enable-signing
    --signing-cert-name DeveloperCert
    --output C:\cse\Windjammer-1.0.exe
```

#### Arguments description

- **--with-unattended** flag enables Unattended service support. without it, only WaykNow.exe binaries will be copied, with it - WaykNow.exe, WaykHost.exe, NowSession.exe, NowService.exe
- **--with-branding** specifies the path to branding zip archive which can be used to brand WaykNow application. Also, an icon from it will be used for resulting CSE, and the product name string will be used in unattended service name ("${PRODUCT_NAME} CSE Service")
- **--wayk-bin-x86, --wayk-bin-x64** specifies the path to wayk now build artifacts binaries, those artifacts can be obtained on azure pipelines. (required)
- **--wayk-cse-dummy-path** specifies path to WaykCseDummy.exe (optional, the patcher will automatically search for WaykCseDummy.exe near wayk-cse-patcher binary, so WaykCseDummy.exe availability for the patcher is required)
- **--wayk-extraction-path** specifies runtime-expandable (for env. variables) path to extraction folder. Defaults to "${TEMP}/WaykNowCse"
- **--wayk-data-path** specifies runtime-expandable (for env. variables) path to data folder. Defaults to "${TEMP}/WaykNowCse/data"
- **--wayk-system-path** specifies runtime-expandable (for env. variables) path to system folder. Defaults to "${TEMP}/WaykNowCse/system"
- **--init-script** specifies the path to PowerShell init script, which will be run before WaykNow CSE launch
- **--enable-signing enables** output executable signing
- **--signing-cert-name** specifies the name of the certificate (in system-wide cert storage) to sign resulting output CSE. If not specified, the signing will be skipped
- **--output** specifies resulting output path for CSE binary. (required)
- **--wayk-ps-version** specifies WaykNow-ps module version from PSGallery to embed to the CSE binary (for PowerShell init script). If not specified, the default version will be used
