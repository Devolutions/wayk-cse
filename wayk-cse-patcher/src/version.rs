use std::fmt::Write;

use log::info;
use regex::Regex;
use version_compare::{CompOp, VersionCompare};

use crate::error::{WaykCseResult, WaykCseError};

pub const VERSION_URL: &str = "https://devolutions.net/products.htm";

#[derive(Debug, Clone, Copy)]
pub struct NowVersion {
    quad: [u32; 4],
}

pub fn get_user_agent(version: String) -> String {
    format!("NowUpdater/{} (Windows)", version)
}

impl NowVersion {
    pub fn as_triple(&self) -> String {
        format!("{}.{}.{}", self.quad[0], self.quad[1], self.quad[2])
    }

    pub fn as_quad(&self) -> String {
        let mut version = self.as_triple();
        write!(&mut version, ".{}", self.quad[3]).unwrap();
        version
    }

    pub fn get_remote_version() -> WaykCseResult<Self> {
        let client = reqwest::blocking::Client::builder()
            .user_agent(format!("WaykCse/{} (Windows)", env!("CARGO_PKG_VERSION")))
            .build().map_err(|_| {
                WaykCseError::DownloadFailed("failed to create request client".to_string())
            })?;
        let body = client
            .get(VERSION_URL)
            .send().map_err(|_| {
                WaykCseError::DownloadFailed("failed send request".to_string())
            })?
            .text().map_err(|_| {
                WaykCseError::DownloadFailed("failed covert response to text".to_string())
            })?;

        let version_regex = Regex::new(r#"Wayk\.Version=(\d+).(\d+).(\d+).(\d+)"#).unwrap();
        let captures = version_regex.captures(&body).ok_or_else(|| {
            WaykCseError::DownloadFailed("failed to query latest WaykNow version".to_string())
        })?;

        let expected_captures_len = 5;
        if expected_captures_len != captures.len() {
            return Err(WaykCseError::DownloadFailed("version has invalid fomat".to_string()));
        }

        Ok(Self {
            quad: [
                captures[1].parse::<u32>().map_err(
                    |e| WaykCseError::Other("version parsing error".to_string())
                )?,
                captures[2].parse::<u32>().map_err(
                    |e| WaykCseError::Other("version parsing error".to_string())
                )?,
                captures[3].parse::<u32>().map_err(
                    |e| WaykCseError::Other("version parsing error".to_string())
                )?,
                captures[4].parse::<u32>().map_err(
                    |e| WaykCseError::Other("version parsing error".to_string())
                )?,
            ],
        })
    }
}
