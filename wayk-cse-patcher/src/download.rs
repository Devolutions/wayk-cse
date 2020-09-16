use std::{
    fs, io,
    path::{Path},
};

use regex::Regex;
use thiserror::Error;
use url::Url;

use crate::{bundle::Bitness, fs_util::remove_file_after_reboot, version::NowVersion};

const VERSION_URL: &str = "https://devolutions.net/products.htm";

#[derive(Error, Debug)]
pub enum Error {
    #[error("Download failed due to IO error ({0})")]
    IoError(#[from] io::Error),
    #[error("Download url is invalid ({0})")]
    InvalidUrl(#[from] url::ParseError),
    #[error("Invalid bitness")]
    InvalidBitness,
    #[error("Reqwest failure ({0})")]
    ReqwestFailure(#[from] reqwest::Error),
    #[error("Failed to detect remote version")]
    RemoteVersionNotDetected(String),
}

impl Error {
    fn from_quad_parse_error(_: std::num::ParseIntError) -> Self {
        Self::RemoteVersionNotDetected(String::from("ivalid version quad format"))
    }
}

pub type DownloadResult<T> = Result<T, Error>;

fn construct_package_url(
    bitness: Bitness,
    version: NowVersion,
    extension: &str,
) -> DownloadResult<Url> {
    let url = Url::parse(&format!(
        "https://cdn.devolutions.net/download/Wayk/{0}/WaykNow-{1}-{0}.{2}",
        version.as_quad(),
        bitness,
        extension
    ))?;

    Ok(url)
}

fn construct_msi_url(bitness: Bitness, version: NowVersion) -> DownloadResult<Url> {
    construct_package_url(bitness, version, "msi")
}

fn construct_zip_url(bitness: Bitness, version: NowVersion) -> DownloadResult<Url> {
    construct_package_url(bitness, version, "zip")
}

fn download_msi(destination: &Path, url: &Url) -> DownloadResult<()> {
    let client = reqwest::blocking::Client::builder()
        .user_agent(get_agent())
        .build()?;
    let mut response = client.get(url.as_str()).send()?;
    let mut out = fs::File::create(destination)?;
    io::copy(&mut response, &mut out)?;
    remove_file_after_reboot(destination)?;
    Ok(())
}

pub fn download_latest_zip(destination: &Path, bitness: Bitness) -> DownloadResult<()> {
    let version = get_remote_version()?;
    let url = construct_zip_url(bitness, version)?;
    download_msi(destination, &url)
}

pub fn download_latest_msi(destination: &Path, bitness: Bitness) -> DownloadResult<()> {
    let version = get_remote_version()?;
    let url = construct_msi_url(bitness, version)?;
    download_msi(destination, &url)
}

pub fn get_remote_version() -> DownloadResult<NowVersion> {
    let client = reqwest::blocking::Client::builder()
        .user_agent(get_agent())
        .build()?;
    let body = client.get(VERSION_URL).send()?.text()?;

    let version_regex = Regex::new(r#"Wayk\.Version=(\d+).(\d+).(\d+).(\d+)"#).unwrap();
    let captures = version_regex.captures(&body).ok_or_else(|| {
        Error::RemoteVersionNotDetected(String::from("failed to query latest WaykNow version"))
    })?;

    let expected_captures_len = 5;
    if expected_captures_len != captures.len() {
        return Err(Error::RemoteVersionNotDetected(String::from(
            "version has invalid fomat",
        )));
    }

    Ok(NowVersion::from_quad([
        captures[1]
            .parse::<u32>()
            .map_err(Error::from_quad_parse_error)?,
        captures[2]
            .parse::<u32>()
            .map_err(Error::from_quad_parse_error)?,
        captures[3]
            .parse::<u32>()
            .map_err(Error::from_quad_parse_error)?,
        captures[4]
            .parse::<u32>()
            .map_err(Error::from_quad_parse_error)?,
    ]))
}

pub fn get_agent() -> String {
    format!("WaykCse/{} (Windows)", env!("CARGO_PKG_VERSION"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[ignore]
    fn test_download() {
        download_latest_msi(Path::new("D:\\wn.msi"), Bitness::X64).unwrap();
    }

    #[test]
    #[ignore]
    fn test_download_zip() {
        download_latest_zip(Path::new("D:\\wn.zip"), Bitness::X86).unwrap();
    }
}
