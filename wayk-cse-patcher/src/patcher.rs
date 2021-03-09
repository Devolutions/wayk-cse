use std::{env, fs::File, io, path::Path};

use clap::{App as ArgParser, Arg, ArgMatches};
use log::{info, warn};

use anyhow::Context;
use tempfile::tempdir;

use crate::{
    branding::{extract_branding_icon, get_product_name},
    bundle::{BundlePackageType, BundlePacker},
    cse_options::CseOptions,
    download::{download_latest_msi, download_latest_zip},
    resource_patcher::ResourcePatcher,
    signing::sign_executable,
};

pub struct WaykCsePatcher;

impl WaykCsePatcher {
    pub fn new() -> Self {
        WaykCsePatcher
    }

    fn extract_empty_cse(path: &Path) -> anyhow::Result<()> {
        let cse_binary = include_bytes!(env!("WAYK_CSE_PATH"));

        let mut cse_binary_reader: &[u8] = cse_binary.as_ref();

        let mut cse_target_file = File::create(path).context("Failed to create output file")?;
        io::copy(&mut cse_binary_reader, &mut cse_target_file)
            .context("Failed to extract original cse file")?;

        Ok(())
    }

    pub fn run(self) -> anyhow::Result<()> {
        let args = Self::parse_arguments();
        let working_dir = tempdir().context("Failed to create temp workind directory")?;
        info!(
            "Patcher working directory: {}",
            working_dir.path().display()
        );

        let output_path = Path::new(args.value_of("OUTPUT").unwrap());
        let config_path = Path::new(args.value_of("CONFIG").unwrap());

        let options = CseOptions::load(config_path)?;

        info!("Extracting empty CSE binary...");
        Self::extract_empty_cse(output_path)?;

        let mut patcher = ResourcePatcher::new();
        patcher.set_original_binary_path(output_path);

        let mut bundle = BundlePacker::new();

        info!("Generating CSE configuration...");
        let processed_options_path = working_dir.path().join("options.json");
        options
            .save_finalized_options(&processed_options_path)
            .context("Failed to process options")?;
        bundle.add_bundle_package(BundlePackageType::CseOptions, &processed_options_path);

        for bitness in &options.install_options().supported_architectures {
            if options.install_options().embed_msi.unwrap_or(true) {
                info!("Downloading msi installer for {} architecture...", bitness);
                let msi_path = working_dir
                    .path()
                    .join(format!("Installer_{}.msi", bitness));
                download_latest_msi(&msi_path, *bitness).with_context(|| {
                    format!("Failed to download MSI for {} architecture", bitness)
                })?;
                bundle.add_bundle_package(
                    BundlePackageType::InstallationMsi { bitness: *bitness },
                    &msi_path,
                );
            }
        }

        if let Some(script_path) = &options.post_install_script_options().path {
            info!("Embedding Power Shell script...");
            let script_path = Path::new(script_path);
            bundle.add_bundle_package(BundlePackageType::CustomInitializationScript, script_path);
        }

        let mut product_name = None;
        if let Some(branding_path) = &options.branding_options().path {
            info!("Processing branding file...");
            let branding_path = Path::new(branding_path);

            info!("Branding path: {:?}", branding_path);
            bundle.add_bundle_package(BundlePackageType::BrandingZip, branding_path);

            let icon_data = extract_branding_icon(branding_path)
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
                    .context("Failed to write branding icon file")?;

                patcher.set_icon_path(&icon_path);
            }

            match get_product_name(branding_path) {
                Ok(value) => {
                    product_name = Some(value);
                }
                Err(e) => {
                    warn!(
                        "Failed to get product name, fallback to default name: {}",
                        e
                    );
                }
            };
        }

        patcher.set_product_name(product_name.as_deref().unwrap_or("Wayk Agent"));

        info!("Packing bundle archive...");
        let bundle_path = working_dir.path().join("bundle.7z");
        bundle.pack(&bundle_path)?;

        info!("Patching executable...");
        patcher.set_wayk_bundle_path(&bundle_path);
        patcher.patch().context("Failed to patch CSE executable")?;

        if let Some(cert_name) = &options.signing_options().cert_name {
            info!("Signing executable...");
            sign_executable(Path::new(output_path), &cert_name)?;
        }

        info!("Wayk CSE build successful!");
        Ok(())
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
