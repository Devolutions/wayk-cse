# WaykAgent custom standalone executable

This project allows us to build custom standalone executable (CSE) for WaykAgent.
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

#### Requirements

Download the latest **Ninja** binaries https://github.com/ninja-build/ninja/releases and add Ninja to the the **PATH** environment variable

#### build wayk-cse-patcher

```
    cd /wayk-cse-patcher
    cargo build
```
the executable is available in the /target repo

#### CSE modules description and options

**WaykCseDummy.exe** - Executable with logic, required to launch the WaykNow in the standalone mode with advanced customization options (in contrast to the old standalone WaykNow executable). Initially, this executable only contains the code, without required resources (e.g. binaries, init script, branding file). This executable is always a 32-bit application.

**wayk-cse-patcher** – Utility which implements WaykCseDummy.exe patching to inject the actual WaykNow binaries, branding, script, and WaykNow-ps module to the binary. It provides a lot of options to configure the resulting CSE. If branding is provided, the patcher will also substitute the CSE icon.

**options json file** - json file that will be used by the patcher to set some options in order to get a costumized WaykAgent installation

```
example of a option file

{
    "enrollment": {
        "url": "https://test.url.com",
        "token": "123456789"
    },
    "branding": {
        "embedded": true,
        "path": "branding.zip"
    },
    "config": {
        "autoUpdateEnabled": true,
        "autoLaunchOnUserLogon": true
    },
    "install": {
        "embedMsi": true,
        "architecture": "x64",
        "startAfterInstall": true,
        "quiet": true
    }

```

#### How to use

Download the latest **7-zip** add 7zip to the the **PATH** environment variable

Maker sur **signtool** is installed on the machine and added to the **PATH** environment variable

Example call arguments:
```
USAGE:
    wayk-cse-patcher.exe --config <CONFIG> --output <OUTPUT>

#### Arguments description

- **--config** specifies the json file path that will set all the CSE installation options. (required)
- **--output** specifies resulting output path for CSE binary. (required)