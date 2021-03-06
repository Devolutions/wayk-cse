use std::{io, path::Path};

use log::warn;

#[cfg(not(unix))]
pub fn remove_file_after_reboot(path: &Path) -> io::Result<()> {
    use std::ptr;
    use widestring::WideCString;
    use winapi::um::winbase::{MoveFileExW, MOVEFILE_DELAY_UNTIL_REBOOT};

    let wide_path = WideCString::from_os_str(path.as_os_str()).map_err(|_| {
        io::Error::new(
            io::ErrorKind::InvalidInput,
            "Path can't be converted to wide string".to_string(),
        )
    })?;

    if unsafe { MoveFileExW(wide_path.as_ptr(), ptr::null(), MOVEFILE_DELAY_UNTIL_REBOOT) } == 0 {
        warn!("Can't mark file for removal after reboot");
    }

    Ok(())
}

#[cfg(unix)]
pub fn remove_file_after_reboot(path: &Path) -> io::Result<()> {
    use fs_extra::file::{move_file, CopyOptions};
    use std::env;

    let temp_dir = env::var("TMPDIR").map_err(|_| {
        io::Error::new(
            io::ErrorKind::Other,
            "Failed to get path of /tmp dir by $TMPDIR env variable",
        )
    })?;

    let temp_dir = Path::new(&temp_dir);

    let target_temp_file = temp_dir.join(path.file_name().unwrap_or("temp".as_ref()));
    match move_file(path, target_temp_file, &CopyOptions::new()) {
        Ok(_) => {}
        Err(e) => warn!("Can't move file to /tmp dir for removal after reboot:{}", e),
    }

    Ok(())
}
