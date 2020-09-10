use std::{
    path::Path,
    fs::{read_to_string, File},
};

use json::JsonValue;

use crate::error::{WaykCseResult, WaykCseError};
use std::io::Write;

pub struct CseOptions {
    json_data: JsonValue,
}

impl CseOptions {
    pub fn load(path: &Path) -> WaykCseResult<Self> {
        let json_str = read_to_string(path).map_err(|e| {
            WaykCseError::CseOptionsParsingFailed("Can't read cse options file".to_string())
        })?;

        let json_data = json::parse(&json_str).map_err(|e| {
            WaykCseError::CseOptionsParsingFailed(format!("Failed to parse json: {}", e))
        })?;

        Ok(Self { json_data })
    }

    pub fn save_finalized_options(&self, path: &Path) -> WaykCseResult<()> {
        let mut opts = self.json_data.clone();

        // Remove development environment info (e.g local paths, signing data)
        opts["branding"]["embedded"] = JsonValue::Boolean(true);
        opts["branding"].remove("path");
        opts.remove("signing");
        opts["postInstallScript"].remove("path");
        opts["postInstallScript"].remove("waykNowPsVersion");

        let mut file = File::create(path).map_err(|e| {
            WaykCseError::CseOptionsParsingFailed(format!("Failed to create temp cse options file: {}", e))
        })?;

        // Save without pretty formatting, it will be read only by CSE binary
        file.write_all(json::stringify(opts).as_bytes()).map_err(|e| {
            WaykCseError::CseOptionsParsingFailed(format!("Failed to write cse options to file: {}", e))
        })
    }
}