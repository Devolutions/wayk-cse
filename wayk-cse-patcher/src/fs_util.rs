use std::{io, path::PathBuf};

use log::warn;

#[cfg(not(unix))]
pub fn remove_file_after_reboot(path: PathBuf) -> io::Result<PathBuf> {
    use std::ptr;
    use widestring::WideCString;
    use winapi::um::winbase::{MoveFileExW, MOVEFILE_DELAY_UNTIL_REBOOT};

    let wide_path = WideCString::from_os_str(path.as_path().as_os_str()).map_err(|_| {
        io::Error::new(
            io::ErrorKind::InvalidInput,
            "Path can't be converted to wide string".to_string(),
        )
    })?;

    if unsafe { MoveFileExW(wide_path.as_ptr(), ptr::null(), MOVEFILE_DELAY_UNTIL_REBOOT) } == 0 {
        warn!("Can't mark file for removal after reboot");
    }

    Ok(path)
}

#[cfg(unix)]
pub fn remove_file_after_reboot(path: PathBuf) -> io::Result<PathBuf> {
    use fs_extra::file::{move_file, CopyOptions};
    use std::{path::PathBuf, process::Command};

    let temp_file = Command::new("mktemp")
        .output()
        .map_err(|err| {
            io::Error::new(
                io::ErrorKind::Other,
                format!(
                    "Failed to execute `mktemp` command to create e tamp file:{}",
                    err
                ),
            )
        })?
        .stdout;

    let temp_file = String::from_utf8(temp_file).map_err(|_| {
        io::Error::new(
            io::ErrorKind::Other,
            format!("Failed to create UTF-8 string from `mktemp` output"),
        )
    })?;

    let mut temp_file = Path::new(&temp_file);

    match move_file(path.as_path(), temp_file, &CopyOptions::new()) {
        Ok(_) => {}
        Err(e) => warn!("Can't move file to /tmp dir for removal after reboot:{}", e),
    }

    Ok(temp_file.to_path_buf())
}
