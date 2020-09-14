use std::{
    io,
    path::PathBuf,
};

pub fn get_powershell_path() -> io::Result<PathBuf> {
    if let Ok(powershell) = which::which("pwsh") {
        return Ok(powershell);
    }

    which::which("powershell")
        .map_err(|e| {
            io::Error::new(
                io::ErrorKind::Other,
                format!("Failed to get Power Shell path -> {}", e)
            )
        })
}