use std::{
    path::{Path, PathBuf},
    process::Command,
};

use log::warn;
use thiserror::Error;


#[derive(Error, Debug)]
pub enum Error {
    #[error("Signtool executaion failed: ({0})")]
    SigntoolFailed(String),
    #[error("Signtool was not found")]
    SigntoolNotFound,
}

pub type SigningResult<T> = Result<T, Error>;


pub fn sign_executable(executable_path: &Path, cert_name: &str) -> SigningResult<()> {
    let signtool_path = get_signtool_path()?;

    let signtool_output = Command::new(&signtool_path)
        .args(make_signtool_args(executable_path, cert_name))
        .output()
        .map_err(|_| Error::SigntoolFailed("Can't execute 'signtool' command".into()))?;

    match signtool_output.status.code().unwrap() {
        0 => {}
        2 => {
            let stderr = std::str::from_utf8(&signtool_output.stdout).unwrap_or("<INVALID STDOUT>");
            warn!("Signtool returned warning -> {}", stderr);
        }
        _ => {
            let stderr = std::str::from_utf8(&signtool_output.stderr).unwrap_or("<INVALID STDERR>");

            return Err(Error::SigntoolFailed(format!(
                "Failed to sign the binary -> {}",
                stderr
            )));
        }
    }

    Ok(())
}

pub fn make_signtool_args(file_path: &Path, cert_name: &str) -> Vec<String> {
    let mut args: Vec<String> = Vec::new();

    args.push("sign".into());
    args.push("/n".into());
    args.push(cert_name.into());
    args.push("/fd".into());
    args.push("sha256".into());
    args.push("/tr".into());
    args.push("http://timestamp.comodoca.com/?td=sha256".into());
    args.push("/td".into());
    args.push("sha256".into());
    args.push("/v".into());
    args.push(file_path.to_str().unwrap().into());
    args
}

pub fn get_signtool_path() -> SigningResult<PathBuf> {
    which::which("signtool").map_err(|_| Error::SigntoolNotFound)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::powershell::get_powershell_path;
    use tempfile::Builder as TempFileBuilder;

    // UT requires real cert in certificate store, so it is normally ignored
    #[test]
    #[ignore]
    fn signing_succeeds() {
        let mock_cert_name = "WaykTestCert";

        let executable = TempFileBuilder::new().suffix(".exe").tempfile().unwrap();
        std::fs::copy(get_signtool_path().unwrap(), &executable).unwrap();

        let executable_path = executable.into_temp_path();

        sign_executable(&executable_path, mock_cert_name).unwrap();

        let get_cert_command = format!(
            "Get-AuthenticodeSignature \"{}\" | Format-List",
            executable_path.display()
        );

        let powershell_path = get_powershell_path().unwrap();
        let cert_info_output = Command::new(powershell_path)
            .arg("-Command")
            .arg(get_cert_command)
            .output()
            .unwrap();

        let stdout = std::str::from_utf8(&cert_info_output.stdout).unwrap();
        assert!(stdout.contains(mock_cert_name))
    }
}
