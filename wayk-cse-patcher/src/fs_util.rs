use std::{
    path::Path,
    ptr,
    io
};

use log::warn;
use widestring::WideCString;
use winapi::um::winbase::{MoveFileExW, MOVEFILE_DELAY_UNTIL_REBOOT};


pub fn remove_file_after_reboot(path: &Path) -> io::Result<()> {
    let wide_path = WideCString::from_os_str(path.as_os_str())
        .map_err(|_| {
            io::Error::new(
                io::ErrorKind::InvalidInput,
                "Path can't be converted to wide string".to_string()
            )
        })?;

    if unsafe { MoveFileExW(wide_path.as_ptr(), ptr::null(), MOVEFILE_DELAY_UNTIL_REBOOT) } == 0 {
        warn!("Can't mark file for removal after reboot");
    }

    Ok(())
}
