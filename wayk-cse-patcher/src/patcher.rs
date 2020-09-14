use std::{
    env,
    fs::{self, File},
    io,
    path::Path,
};

use clap::{App as ArgParser, Arg, ArgMatches};
use log::{info, warn};

use tempfile::tempdir;
use anyhow::Context;

use crate::{
    branding::{extract_branding_icon, get_product_name},
    bundle::{Bitness, BundlePackageType, BundlePacker},
    resource_patcher::ResourcePatcher,
    signing::sign_executable,
};

pub struct WaykCsePatcher;

impl WaykCsePatcher {
    pub fn new() -> Self {
        WaykCsePatcher
    }

    pub fn run(self) -> anyhow::Result<()> {

        let args = Self::parse_arguments();

        let working_dir = tempdir().context("Failed to create temp workind directory");
        let output_path = args.value_of("OUTPUT").unwrap();

        let wayk_cse_binary = include_bytes!(env!("WAYK_CSE_PATH"));
    /*

        let wayk_cse_dummy_path = match args.value_of("WAYK_CSE_DUMMY_PATH") {
            Some(path) => Path::new(path).into(),
            None => env::current_exe()
                .map(|current_exe| current_exe.join("WaykCseDummy.exe"))
                .map_err(|e| WaykCseError::PatchingError(format!("Failed to get default wayk dummy path -> {0}", e)))?,
        };

        if !wayk_cse_dummy_path.exists() {
            return Err(WaykCseError::PatchingError(
                "Provided WaykCseDummy path is does not exist".into(),
            ));
        }

        let mut patcher = ResourcePatcher::new();

        info!("Copying dummy binary to patch...");

        fs::copy(&wayk_cse_dummy_path, output_path)
            .map_err(|e| WaykCseError::PatchingError(format!("Failed to copy WaykCseDummy -> {0}", e)))?;

        patcher.set_original_binary_path(Path::new(output_path));

        if let Some(value) = args.value_of("WAYK_EXTRACTION_PATH") {
            patcher.set_wayk_cse_option(WaykCseOption::WaykExtractionPath, value);
        }
        if let Some(value) = args.value_of("WAYK_DATA_PATH") {
            patcher.set_wayk_cse_option(WaykCseOption::WaykDataPath, value);
        }
        if let Some(value) = args.value_of("WAYK_SYSTEM_PATH") {
            patcher.set_wayk_cse_option(WaykCseOption::WaykSystemPath, value);
        }

        let mut bundle_packer = BundlePacker::new();

        if let Some(value) = args.value_of("INIT_SCRIPT_PATH") {
            info!("Downloading powershell module...");

            let wayk_ps_version = args.value_of("WAYK_PS_VERSION");
            let wayk_ps_path = download_module("WaykNow", wayk_ps_version, working_dir.path())?;

            bundle_packer.add_bundle_package(BundlePackageType::PowerShellModule, &wayk_ps_path);

            bundle_packer.add_bundle_package(BundlePackageType::CustomInitializationScript, Path::new(value));
        }

        let mut product_name = None;

        if let Some(value) = args.value_of("WITH_BRANDING_PATH") {
            info!("Processing branding file...");

            bundle_packer.add_bundle_package(BundlePackageType::BrandingZip, Path::new(value));

            let icon_data = extract_branding_icon(Path::new(value))
                .map_err(|e| {
                    warn!("Failed to extract branding icon: {}", e);
                })
                .ok();

            if let Some(icon_data) = icon_data {
                let icon_path = working_dir.path().join("icon.ico");

                File::create(&icon_path)
                    .map(|mut file| {
                        let mut reader = icon_data.as_slice();
                        io::copy(&mut reader, &mut file).unwrap();
                    })
                    .map_err(|e| WaykCseError::PatchingError(format!("Failed to write icon file: -> {0}", e)))?;

                patcher.set_icon_path(&icon_path);
            }

            match get_product_name(Path::new(value)) {
                Ok(value) => {
                    product_name = Some(value);
                }
                Err(e) => {
                    warn!("Failed to get product name, fallback to default: {}", e);
                }
            };
        }

        patcher.set_wayk_cse_option(
            WaykCseOption::WaykProductName,
            product_name.as_deref().unwrap_or("Wayk Now"),
        );

        let enable_auto_clean = args.is_present("ENABLE_AUTO_CLEAN");

        let auto_clean_option_value = if enable_auto_clean { "1" } else { "0" };
        patcher.set_wayk_cse_option(WaykCseOption::EnableWaykAutoClean, auto_clean_option_value);

        let enable_unattended = args.is_present("WITH_UNATTENDED");

        let unattended_option_value = if enable_unattended { "1" } else { "0" };
        patcher.set_wayk_cse_option(WaykCseOption::EnableUnattendedService, unattended_option_value);

        if let Some(value) = args.value_of("WAYK_BIN_X86_PATH") {
            bundle_packer.add_bundle_package(
                BundlePackageType::WaykBinariesArchive {
                    bitness: Bitness::X86,
                    enable_unattended,
                },
                Path::new(value),
            );
        }

        if let Some(value) = args.value_of("WAYK_BIN_X64_PATH") {
            bundle_packer.add_bundle_package(
                BundlePackageType::WaykBinariesArchive {
                    bitness: Bitness::X64,
                    enable_unattended,
                },
                Path::new(value),
            );
        }

        info!("Packing bundle archive...");

        let bundle_path = working_dir.path().join("bundle.7z");
        bundle_packer.pack(&bundle_path)?;

        info!("Patching executable...");

        patcher.set_wayk_bundle_path(&bundle_path);
        patcher.patch()?;

        if args.is_present("ENABLE_SIGNING") {
            info!("Signing executable...");
            let cert_name = args.value_of("SIGNING_CERT_NAME").unwrap();
            sign_executable(Path::new(output_path), cert_name)?;
        }

        info!("Wayk CSE build successful!");
*/
        Ok(())
    }

    pub fn create_dir(path: &Path) -> anyhow::Result<()> {
        fs::create_dir(path)
            .with_context(|| format!("Failed to create directory {}", path.display()))
    }

    fn parse_arguments() -> ArgMatches<'static> {
        ArgParser::new(env!("CARGO_PKG_NAME"))
            .about(env!("CARGO_PKG_DESCRIPTION"))
            .version(env!("CARGO_PKG_VERSION"))
            .author(env!("CARGO_PKG_AUTHORS"))
            .arg(
                Arg::with_name("OUTPUT")
                    .short("o")
                    .long("output")
                    .help("Set output file for the patcher")
                    .takes_value(true)
                    .required(true),
            )
            .arg(
                Arg::with_name("CONFIG")
                    .short("-c")
                    .long("config")
                    .help("Set CSE configuration JSON file")
                    .takes_value(true)
                    .required(true),
            )
            .get_matches()
    }
}

impl Default for WaykCsePatcher {
    fn default() -> Self {
        Self::new()
    }
}
