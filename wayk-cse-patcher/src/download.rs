use std::{fs, io, path::{Path, PathBuf}};

use bitness::Bitness;
use url::Url;

use crate::{
    fs_util::remove_file_after_reboot,
    error::{WaykCseError, WaykCseResult},
    version::NowVersion,
};

fn bitness_to_str(bitness: &Bitness) -> WaykCseResult<&'static str> {
    match bitness {
        Bitness::X86_64 => Ok("x64"),
        Bitness::X86_32 => Ok("x86"),
        Bitness::Unknown => Err(WaykCseError::Other("Invalid bitness".to_string())),
    }
}

fn construct_package_url(bitness: &Bitness, version: NowVersion, extension: &str) -> WaykCseResult<Url> {
    let url = Url::parse(&format!(
        "https://cdn.devolutions.net/download/Wayk/{0}/WaykNow-{1}-{0}.{2}",
        version.as_quad(),
        bitness_to_str(bitness)?,
        extension
    )).map_err(|e| {
        WaykCseError::DownloadFailed("Failed to parse download URL".to_string())
    })?;
    Ok(url)
}

fn construct_msi_url(bitness: &Bitness, version: NowVersion) -> WaykCseResult<Url> {
    construct_package_url(bitness, version, "msi")
}

fn construct_zip_url(bitness: &Bitness, version: NowVersion) -> WaykCseResult<Url> {
    construct_package_url(bitness, version, "zip")
}


fn download_msi(destination: &Path, url: &Url) -> WaykCseResult<()> {
    let client = reqwest::blocking::Client::builder()
        .user_agent(format!("WaykCse/{} (Windows)", env!("CARGO_PKG_VERSION")))
        .build()
        .map_err(|e| {
            WaykCseError::DownloadFailed("Failed to create request client".to_string())
        })?;
    let mut response = client
        .get(url.as_str())
        .send().map_err(|e| {
            WaykCseError::DownloadFailed("Failed to send request".to_string())
        })?;

    let mut out = fs::File::create(destination).map_err(|e| {
        WaykCseError::DownloadFailed("Can't create msi file".to_string())
    })?;
    io::copy(&mut response, &mut out).map_err(|e| {
        WaykCseError::DownloadFailed("Failed to save msi".to_string())
    })?;
    remove_file_after_reboot(destination)?;

    Ok(())
}

pub fn download_latest_zip(destination: &Path, bitness: &Bitness) -> WaykCseResult<()> {
    let version = NowVersion::get_remote_version()?;
    let url = construct_zip_url(bitness, version)?;
    download_msi(destination, &url)
}

pub fn download_latest_msi(destination: &Path, bitness: &Bitness) -> WaykCseResult<()> {
    let version = NowVersion::get_remote_version()?;
    let url = construct_msi_url(bitness, version)?;
    download_msi(destination, &url)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_download() {
        download_latest_msi(Path::new("D:\\wn.msi"), &Bitness::X86_64).unwrap();
    }

    #[test]
    fn test_download_zip() {
        download_latest_zip(Path::new("D:\\wn.zip"), &Bitness::X86_64).unwrap();
    }
}