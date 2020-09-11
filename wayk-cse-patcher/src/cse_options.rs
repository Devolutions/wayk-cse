use std::{
    path::Path,
    fs::{read_to_string, File},
};

use json::JsonValue;
use bitness::Bitness;

use crate::error::{WaykCseResult, WaykCseError};
use std::io::Write;

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
    pub fn load(path: &Path) -> WaykCseResult<Self> {
        let json_str = read_to_string(path).map_err(|e| {
            WaykCseError::CseOptionsParsingFailed("Can't read cse options file".to_string())
        })?;

        let json_data = json::parse(&json_str).map_err(|e| {
            WaykCseError::CseOptionsParsingFailed(format!("Failed to parse json: {}", e))
        })?;

        let mut branding_options = BrandingOptions::default();
        if let JsonValue::String(ref branding_path) = json_data["branding"]["path"] {
            branding_options.path.replace(branding_path.clone());
        }
        let mut signing_options = SigningOptions::default();
        if let JsonValue::String(ref cert_name) = json_data["signing"]["certName"] {
            signing_options.cert_name.replace(cert_name.clone());
        }

        let mut post_install_script_options = PostInstallScriptOptions::default();
        if let JsonValue::String(ref script_path) = json_data["postInstallScript"]["path"] {
            post_install_script_options.path.replace(script_path.clone());
        }
        if let JsonValue::Boolean(ref import_wayk_now_module) = json_data["postInstallScript"]["importWaykNowModule"] {
            post_install_script_options.import_wayk_now_module.replace(import_wayk_now_module.clone());
        }

        let mut install_options = InstallOptions::default();
        if let JsonValue::Boolean(embed_msi) = json_data["install"]["embedMsi"] {
            install_options.embed_msi.replace(embed_msi);
        }
        if let JsonValue::String(ref architectures) = json_data["install"]["embedMsi"] {
            match architectures.as_str() {
                "x86" => install_options.supported_architectures.push(Bitness::X86_32),
                "x64" => install_options.supported_architectures.push(Bitness::X86_64),
                _ => {
                    install_options.supported_architectures.push(Bitness::X86_32);
                    install_options.supported_architectures.push(Bitness::X86_64);
                },
            };
        }

        Ok(Self {
            json_data,
            branding_options,
            signing_options,
            post_install_script_options,
            install_options
        })
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