use std::{
    path::Path,
    fs::{read_to_string, File},
    io
};

use json::JsonValue;
use bitness::Bitness;
use thiserror::Error;

use crate::error::{WaykCseResult, WaykCseError};
use std::io::Write;


#[derive(Error, Debug)]
pub enum CseOptionsError {
    #[error("Failed to parse JSON ({0})")]
    JsonParsingFailure(json::Error),
    #[error("Encountered IO error ({0})")]
    IoError(io::Error),
}

impl From<json::Error> for CseOptionsError {
    fn from(e: json::Error) -> Self {
        Self::JsonParsingFailure(e)
    }
}

impl From<io::Error> for CseOptionsError {
    fn from(e: io::Error) -> Self {
        Self::IoError(e)
    }
}

type CseOptionsResult<T> = Result<T, CseOptionsError>;


#[derive(Default)]
pub struct BrandingOptions {
    pub path: Option<String>,
}

#[derive(Default)]
pub struct SigningOptions {
    pub cert_name: Option<String>,
}

#[derive(Default)]
pub struct PostInstallScriptOptions {
    pub path: Option<String>,
    pub import_wayk_now_module: Option<bool>,
}

#[derive(Default)]
pub struct InstallOptions {
    pub embed_msi: Option<bool>,
    pub supported_architectures: Vec<Bitness>,
}


pub struct CseOptions {
    json_data: JsonValue,
    branding_options: BrandingOptions,
    signing_options: SigningOptions,
    post_install_script_options: PostInstallScriptOptions,
    install_options: InstallOptions,
}

impl CseOptions {
    pub fn load(path: &Path) -> CseOptionsResult<Self> {
        let json_str = read_to_string(path)?;
        let json_data = json::parse(&json_str)?;

        let mut branding_options = BrandingOptions::default();

        if let Some(branding_path) = json_data["branding"]["path"].as_str() {
            branding_options.path.replace(branding_path.into());
        }
        let mut signing_options = SigningOptions::default();
        if let Some(cert_name) = json_data["signing"]["certName"].as_str() {
            signing_options.cert_name.replace(cert_name.into());
        }

        let mut post_install_script_options = PostInstallScriptOptions::default();
        if let Some(script_path) = json_data["postInstallScript"]["path"].as_str() {
            post_install_script_options.path.replace(script_path.into());
        }
        if let JsonValue::Boolean(ref import_wayk_now_module) = json_data["postInstallScript"]["importWaykNowModule"] {
            post_install_script_options.import_wayk_now_module.replace(*import_wayk_now_module);
        }

        let mut install_options = InstallOptions::default();
        if let JsonValue::Boolean(embed_msi) = json_data["install"]["embedMsi"] {
            install_options.embed_msi.replace(embed_msi);
        }
        if let Some(architectures) = json_data["install"]["architecture"].as_str() {
            match architectures {
                "x86" => install_options.supported_architectures.push(Bitness::X86_32),
                "x64" => install_options.supported_architectures.push(Bitness::X86_64),
                _ => {},
            };
        }

        if install_options.supported_architectures.is_empty() {
            install_options.supported_architectures.push(Bitness::X86_32);
            install_options.supported_architectures.push(Bitness::X86_64);
        }

        Ok(Self {
            json_data,
            branding_options,
            signing_options,
            post_install_script_options,
            install_options
        })
    }

    pub fn save_finalized_options(&self, path: &Path) -> CseOptionsResult<()> {
        let mut opts = self.json_data.clone();

        // Remove development environment info (e.g local paths, signing data)
        opts.remove("signing");
        if opts["branding"]["path"].is_string() {
            opts["branding"]["embedded"] = JsonValue::Boolean(true);
        }
        opts["branding"].remove("path");
        if opts["postInstallScript"]["path"].is_string() {
            opts["postInstallScript"]["embedded"] = JsonValue::Boolean(true);
        }
        opts["postInstallScript"].remove("path");
        opts["postInstallScript"].remove("waykNowPsVersion");

        opts["install"].remove("architecture");

        let mut file = File::create(path)?;

        // Save without pretty formatting, it will be read only by CSE binary
        Ok(file.write_all(json::stringify(opts).as_bytes())?)
    }

    pub fn install_options(&self) -> &InstallOptions {
        &self.install_options
    }

    pub fn post_install_script_options(&self) -> &PostInstallScriptOptions {
        &self.post_install_script_options
    }

    pub fn signing_options(&self) -> &SigningOptions {
        &self.signing_options
    }

    pub fn branding_options(&self) -> &BrandingOptions {
        &self.branding_options
    }
}


#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;
    
    #[test]
    fn options_processing() {
        let options = CseOptions::load(Path::new("tests/data/options.json"))
            .unwrap();

        let actual_file = tempfile::NamedTempFile::new().unwrap();

        options.save_finalized_options(actual_file.path()).unwrap();

        let expected = fs::read_to_string("tests/data/processed.json").unwrap();
        let acual = fs::read_to_string(actual_file.path()).unwrap();

        assert_eq!(acual, expected);
    }

    #[test]
    fn options_processing_all() {
        let options = CseOptions::load(Path::new("tests/data/options.json"))
            .unwrap();

        let expected_branding_path = Some("branding.zip".to_string());
        assert_eq!(options.branding_options().path.as_ref(), expected_branding_path.as_ref());

        let expected_signing_cert = Some("myCert".to_string());
        assert_eq!(options.signing_options().cert_name.as_ref(), expected_signing_cert.as_ref());

        let expected_script_path = Some("script.ps1".to_string());
        assert_eq!(options.post_install_script_options().path.as_ref(), expected_script_path.as_ref());

        let expected_import_wayk_now_module = Some(true);
        assert_eq!(
            options.post_install_script_options().import_wayk_now_module.as_ref(),
            expected_import_wayk_now_module.as_ref()
        );

        let expected_embed_msi = Some(true);
        assert_eq!(options.install_options().embed_msi.as_ref(), expected_embed_msi.as_ref());

        let expected_supported_architectures = vec![Bitness::X86_32, Bitness::X86_64];
        assert_eq!(options.install_options().supported_architectures, expected_supported_architectures);
    }

    #[test]
    fn options_processing_empty() {
        let options = CseOptions::load(Path::new("tests/data/options_empty.json"))
            .unwrap();

        assert!(options.branding_options().path.is_none());
        assert!(options.signing_options().cert_name.is_none());
        assert!(options.post_install_script_options().path.is_none());
        assert!(options.post_install_script_options().import_wayk_now_module.is_none());
        assert!(options.install_options().embed_msi.is_none());
        let expected_supported_architectures = vec![Bitness::X86_32, Bitness::X86_64];
        assert_eq!(
            options.install_options().supported_architectures,
            expected_supported_architectures
        );
    }
}