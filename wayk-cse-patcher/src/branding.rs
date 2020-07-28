use std::{fs::File, io::Read, path::Path};

use zip::ZipArchive;

use crate::error::{WaykCseError, WaykCseResult};
use json::JsonValue;

/// Extracts app icon from the branding zip archive
pub fn extract_branding_icon(branding_path: &Path) -> WaykCseResult<Vec<u8>> {
    let file = File::open(branding_path)
        .map_err(|e| WaykCseError::BrandingIconExtractionFailed(format!("Can't open branding archive: {}", &e)))?;

    let mut archive = ZipArchive::new(file)
        .map_err(|e| WaykCseError::BrandingIconExtractionFailed(format!("Can't extract the archive: {}", &e)))?;

    let icon_name = get_app_icon_name(&mut archive)?;

    let mut compressed_icon = archive.by_name(&icon_name).map_err(|_| {
        WaykCseError::BrandingIconExtractionFailed("icons.app_icon_ico is missing from the branding archive".into())
    })?;

    let mut icon_bytes = Vec::new();
    compressed_icon
        .read_to_end(&mut icon_bytes)
        .map_err(|_| WaykCseError::BrandingIconExtractionFailed("failed to decompress icon file".into()))?;

    Ok(icon_bytes)
}

pub fn get_app_icon_name(archive: &mut ZipArchive<File>) -> WaykCseResult<String> {
    let manifest = get_manifest(archive)?;

    match &manifest["icons"]["app_icon_ico"] {
        json::JsonValue::String(str) => Ok(str.clone()),
        json::JsonValue::Short(str) => Ok(str.to_string()),
        _ => Err(WaykCseError::BrandingIconExtractionFailed(
            "icons.app_icon_ico is missing inside manifest.json".into(),
        )),
    }
}

pub fn get_product_name(branding_path: &Path) -> WaykCseResult<String> {
    let file = File::open(branding_path)
        .map_err(|e| WaykCseError::BrandingIconExtractionFailed(format!("Can't open branding archive: {}", &e)))?;

    let mut archive = ZipArchive::new(file)
        .map_err(|e| WaykCseError::BrandingIconExtractionFailed(format!("Can't extract the archive: {}", &e)))?;

    let manifest = get_manifest(&mut archive)?;

    match &manifest["strings"]["wayk"]["productName"] {
        json::JsonValue::String(str) => Ok(str.clone()),
        json::JsonValue::Short(str) => Ok(str.to_string()),
        _ => Err(WaykCseError::BrandingIconExtractionFailed(
            "string.wayk.productName is missing inside manifest.json".into(),
        )),
    }
}

fn get_manifest(archive: &mut ZipArchive<File>) -> WaykCseResult<JsonValue> {
    let mut compressed_manifest = archive
        .by_name("manifest.json")
        .map_err(|_| WaykCseError::BrandingIconExtractionFailed("manifest.json is missing".into()))?;

    let mut manifest_bytes = Vec::new();
    compressed_manifest
        .read_to_end(&mut manifest_bytes)
        .map_err(|_| WaykCseError::BrandingIconExtractionFailed("failed to decompress manifest.json".into()))?;

    let manifest_str = std::str::from_utf8(&manifest_bytes)
        .map_err(|_| WaykCseError::BrandingIconExtractionFailed("manifest.json is not an valid test file".into()))?;

    json::parse(manifest_str).map_err(|_| {
        WaykCseError::BrandingIconExtractionFailed(
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
            Err(WaykCseError::BrandingIconExtractionFailed(_))
        ));
    }

    #[test]
    fn no_manifest() {
        assert!(matches!(
            extract_branding_icon(Path::new("tests/data/branding_no_manifest.zip")),
            Err(WaykCseError::BrandingIconExtractionFailed(_))
        ));
    }

    #[test]
    fn invalid_manifest() {
        assert!(matches!(
            extract_branding_icon(Path::new("tests/data/branding_invalid_manifest.zip")),
            Err(WaykCseError::BrandingIconExtractionFailed(_))
        ));
    }

    #[test]
    fn no_icon() {
        assert!(matches!(
            extract_branding_icon(Path::new("tests/data/branding_no_icon.zip")),
            Err(WaykCseError::BrandingIconExtractionFailed(_))
        ));
    }
}
