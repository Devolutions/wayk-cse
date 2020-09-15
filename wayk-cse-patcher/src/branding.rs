use std::{
    fs::File,
    io::{self, Read},
    path::Path,
};

use json::JsonValue;
use thiserror::Error;
use zip::ZipArchive;

#[derive(Error, Debug)]
pub enum Error {
    #[error("IO error has been encountered during branding zip parsing ({0})")]
    IoError(#[from] io::Error),
    #[error("Failed ro perform zip extraction ({0})")]
    ExtractionError(#[from] zip::result::ZipError),
    #[error("manifest.json is missing")]
    MissingManifest,
    #[error("Invalid manifest ({0})")]
    InvalidManifest(String),
}

pub type BrandingResult<T> = Result<T, Error>;

/// Extracts app icon from the branding zip archive
pub fn extract_branding_icon(branding_path: &Path) -> BrandingResult<Vec<u8>> {
    let file = File::open(branding_path)?;

    let mut archive = ZipArchive::new(file)?;
    let icon_name = get_app_icon_name(&mut archive)?;
    let mut compressed_icon = archive.by_name(&icon_name)?;

    let mut icon_bytes = Vec::new();
    compressed_icon.read_to_end(&mut icon_bytes)?;

    Ok(icon_bytes)
}

pub fn get_app_icon_name(archive: &mut ZipArchive<File>) -> BrandingResult<String> {
    let manifest = get_manifest(archive)?;

    match &manifest["icons"]["app_icon_ico"] {
        json::JsonValue::String(str) => Ok(str.clone()),
        json::JsonValue::Short(str) => Ok(str.to_string()),
        _ => Err(Error::InvalidManifest(
            "icons.app_icon_ico is missing inside manifest.json".into(),
        )),
    }
}

pub fn get_product_name(branding_path: &Path) -> BrandingResult<String> {
    let file = File::open(branding_path)?;

    let mut archive = ZipArchive::new(file)?;
    let manifest = get_manifest(&mut archive)?;

    match &manifest["strings"]["wayk"]["productName"] {
        json::JsonValue::String(str) => Ok(str.clone()),
        json::JsonValue::Short(str) => Ok(str.to_string()),
        _ => Err(Error::InvalidManifest(
            "string.wayk.productName is missing inside manifest.json".into(),
        )),
    }
}

fn get_manifest(archive: &mut ZipArchive<File>) -> BrandingResult<JsonValue> {
    let mut compressed_manifest = archive
        .by_name("manifest.json")
        .map_err(|_| Error::MissingManifest)?;

    let mut manifest_bytes = Vec::new();
    compressed_manifest.read_to_end(&mut manifest_bytes)?;

    let manifest_str = std::str::from_utf8(&manifest_bytes)
        .map_err(|_| Error::InvalidManifest("manifest.json is not an valid text file".into()))?;

    json::parse(manifest_str).map_err(|_| {
        Error::InvalidManifest(
            "manifest.json inside branding archive is not an valid json file".into(),
        )
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn successful_icon_extraction() {
        let icon = extract_branding_icon(Path::new("tests/data/branding.zip")).unwrap();
        assert_eq!(icon.len(), 163673);
    }

    #[test]
    fn file_is_absent() {
        assert!(matches!(
            extract_branding_icon(Path::new("tests/data/non_existing.zip")),
            Err(Error::IoError(_))
        ));
    }

    #[test]
    fn no_manifest() {
        assert!(matches!(
            extract_branding_icon(Path::new("tests/data/branding_no_manifest.zip")),
            Err(Error::MissingManifest)
        ));
    }

    #[test]
    fn invalid_manifest() {
        assert!(matches!(
            extract_branding_icon(Path::new("tests/data/branding_invalid_manifest.zip")),
            Err(Error::InvalidManifest(_))
        ));
    }

    #[test]
    fn no_icon() {
        assert!(matches!(
            extract_branding_icon(Path::new("tests/data/branding_no_icon.zip")),
            Err(Error::ExtractionError(_))
        ));
    }
}
