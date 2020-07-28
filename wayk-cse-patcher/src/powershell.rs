use std::{
    env,
    path::{Path, PathBuf},
    process::Command,
};

use crate::error::{WaykCseError, WaykCseResult};

pub fn download_module(
    module_name: &str,
    version: Option<&str>,
    parent_output_folder: &Path,
) -> WaykCseResult<PathBuf> {
    let powershell_path = get_powershell_path()?;

    let command_output = Command::new(&powershell_path)
        .args(&make_save_module_powershell_args(
            module_name,
            version,
            parent_output_folder,
        ))
        .output()
        .map_err(|output_err| {
            WaykCseError::PowerShellModuleDownloadFailed(format!("Failed to run powershell command -> {}", output_err))
        })?;

    if !command_output.status.success() {
        let stderr = std::str::from_utf8(&command_output.stderr).unwrap_or("<INVALID STDERR>");

        return Err(WaykCseError::PowerShellModuleDownloadFailed(format!(
            "Powershell command failed -> {}",
            stderr
        )));
    }

    let version_str = if let Some(version) = version {
        version.to_string()
    } else {
        let dir_content = fs_extra::dir::get_dir_content(parent_output_folder.join(module_name)).map_err(|e| {
            WaykCseError::PowerShellModuleDownloadFailed(format!("Failed to enumerate downloaded module dir -> {}", e))
        })?;

        if dir_content.directories.is_empty() {
            return Err(WaykCseError::PowerShellModuleDownloadFailed(
                "Downloaded PowerShell module is missing version folder".into(),
            ));
        }

        // Index 0 is directory itself, Index 1 - first child folder
        dir_content.directories[1].clone()
    };

    Ok(parent_output_folder.join(module_name).join(version_str))
}

pub fn get_powershell_path() -> WaykCseResult<PathBuf> {
    if let Ok(powershell) = which::which("pwsh") {
        return Ok(powershell);
    }

    which::which("powershell")
        .map_err(|e| WaykCseError::InvalidEnvironment(
            format!("Failed to get Power Shell path -> {}", e)
        ))
}

pub fn make_save_module_powershell_args(
    module_name: &str,
    version: Option<&str>,
    parent_output_folder: &Path,
) -> Vec<String> {
    let mut args: Vec<String> = Vec::new();

    args.push("Save-Module".into());
    args.push("-Name".into());
    args.push(module_name.into());
    args.push("-Force".into());
    if let Some(version) = version {
        args.push("-RequiredVersion".into());
        args.push(version.into());
    }
    args.push("-Repository".into());
    args.push("PSGallery".into());
    args.push("-Path".into());
    args.push(parent_output_folder.to_str().unwrap().into());
    args
}

#[cfg(test)]
mod tests {
    use super::*;

    use tempfile::tempdir;

    // UT requires internet connection, so it is normally ignored
    #[test]
    #[ignore]
    fn module_downloads() {
        let temp_dir = tempdir().unwrap();
        let path = download_module("WaykNow", Some("1.0.0"), temp_dir.path()).unwrap();
        assert!(path.join(Path::new("PSGetModuleInfo.xml")).exists());
    }

    #[test]
    #[ignore]
    fn module_downloads_without_version() {
        let temp_dir = tempdir().unwrap();
        let path = download_module("WaykNow", None, temp_dir.path()).unwrap();
        assert!(path.join(Path::new("PSGetModuleInfo.xml")).exists());
    }

    // UT requires internet connection, so it is normally ignored
    #[test]
    #[ignore]
    fn module_download_fails() {
        let temp_dir = tempdir().unwrap();
        let path = download_module("WaykNow", Some("0xDEADBEEF"), temp_dir.path());
        assert!(path.is_err());

        // validate that stderr captures correctly to show useful logs
        let err_msg = format!("{}", path.err().unwrap());
        assert!(err_msg.contains("ParameterBindingArgumentTransformationException"));
    }
}
