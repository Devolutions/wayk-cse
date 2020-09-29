use std::{
    fmt::{Display, Formatter, Result as FmtResult},
    fs::{self, File},
    io,
    path::{Path, PathBuf},
    process::Command,
};

use tempfile::Builder as TempFileBuilder;
use thiserror::Error;

use crate::artifacts_bundle::{self, ArtifactsBundle};

#[derive(Clone, Copy, Eq, PartialEq, Debug)]
pub enum Bitness {
    X86,
    X64,
}

impl Display for Bitness {
    fn fmt(&self, f: &mut Formatter<'_>) -> FmtResult {
        let string_representation = match self {
            Bitness::X86 => "x86",
            Bitness::X64 => "x64",
        };

        write!(f, "{}", string_representation)
    }
}

#[derive(Error, Debug)]
pub enum Error {
    #[error("Bunle packing failed during io operation ({0})")]
    IoError(#[from] io::Error),
    #[error("Artifacts bundle processing failed ({0})")]
    ArtifactsBundleFailed(#[from] artifacts_bundle::Error),
    #[error("Failed to compress bunlde via 7z ({0})")]
    ArchiverFailure(String),
}

type BundlePackerResult<T> = Result<T, Error>;

pub enum BundlePackageType {
    WaykBinaries { bitness: Bitness },
    InstallationMsi { bitness: Bitness },
    BrandingZip,
    CustomInitializationScript,
    CseOptions,
}

pub struct BundlePacker {
    packages: Vec<(BundlePackageType, PathBuf)>,
}

impl BundlePacker {
    pub fn new() -> Self {
        Self {
            packages: Vec::new(),
        }
    }

    pub fn add_bundle_package(&mut self, package_type: BundlePackageType, path: &Path) {
        self.packages.push((package_type, path.into()));
    }

    pub fn pack(&self, output_path: &Path) -> BundlePackerResult<()> {
        let bundle_directory = TempFileBuilder::new().prefix("bundle_").tempdir()?;

        for (package_type, package_path) in &self.packages {
            match package_type {
                BundlePackageType::WaykBinaries { bitness } => {
                    let mut artifacts = ArtifactsBundle::open(&package_path)?;

                    let executable_path = bundle_directory
                        .path()
                        .join(format!("WaykNow_{}.exe", bitness));

                    let mut file = File::create(executable_path)?;
                    artifacts.extract_wayk_now_binary(&mut file)?;
                }
                BundlePackageType::InstallationMsi { bitness } => {
                    fs::copy(
                        package_path,
                        bundle_directory
                            .path()
                            .join(format!("Installer_{}.msi", bitness)),
                    )?;
                }
                BundlePackageType::BrandingZip => {
                    fs::copy(package_path, bundle_directory.path().join("branding.zip"))?;
                }
                BundlePackageType::CustomInitializationScript => {
                    fs::copy(package_path, bundle_directory.path().join("init.ps1"))?;
                }
                BundlePackageType::CseOptions => {
                    fs::copy(package_path, bundle_directory.path().join("options.json"))?;
                }
            }
        }

        compress_bundle(bundle_directory.path(), output_path)?;

        Ok(())
    }
}

impl Default for BundlePacker {
    fn default() -> Self {
        Self::new()
    }
}

fn get_archiver_path() -> BundlePackerResult<PathBuf> {
    which::which("7z")
        .map_err(|e| Error::ArchiverFailure(format!("Failed to find 7z path -> {}", e)))
}

fn compress_bundle(unpacked_bundle_path: &Path, output_path: &Path) -> BundlePackerResult<()> {
    let archiver_output = Command::new(get_archiver_path()?)
        .arg("a")
        .arg(format!("{}", output_path.display()))
        .arg(format!("{}", unpacked_bundle_path.join("*").display()))
        .output()
        .map_err(|e| Error::ArchiverFailure(format!("7z invocation failed -> {}", e)))?;

    if !archiver_output.status.success() {
        let stderr = std::str::from_utf8(&archiver_output.stderr).unwrap_or("<INVALID STDERR>");
        return Err(Error::ArchiverFailure(format!(
            "7z packing failed: -> {}",
            stderr
        )));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_bundle_packing() {
        let temp_path = {
            let tmp = TempFileBuilder::new()
                .prefix("a")
                .suffix(".7z")
                .tempfile()
                .unwrap()
                .into_temp_path();
            tmp.to_path_buf()
            // Remove actual temp file; just use temp path
        };

        let mut packer = BundlePacker::new();
        packer.add_bundle_package(
            BundlePackageType::BrandingZip,
            Path::new("tests/data/branding.zip"),
        );
        packer.add_bundle_package(
            BundlePackageType::CustomInitializationScript,
            Path::new("tests/data/fake_init_script.ps1"),
        );
        packer.add_bundle_package(
            BundlePackageType::WaykBinaries {
                bitness: Bitness::X64,
            },
            Path::new("tests/data/wayk_now_mock_binaries.zip"),
        );
        packer.add_bundle_package(
            BundlePackageType::WaykBinaries {
                bitness: Bitness::X86,
            },
            Path::new("tests/data/wayk_now_mock_binaries.zip"),
        );
        packer.add_bundle_package(
            BundlePackageType::CseOptions,
            Path::new("tests/data/options.json"),
        );
        packer.add_bundle_package(
            BundlePackageType::InstallationMsi {
                bitness: Bitness::X86,
            },
            Path::new("tests/data/fake_installer.msi"),
        );
        packer.add_bundle_package(
            BundlePackageType::InstallationMsi {
                bitness: Bitness::X64,
            },
            Path::new("tests/data/fake_installer.msi"),
        );

        packer.pack(&temp_path).unwrap();

        let output = Command::new(get_archiver_path().unwrap())
            .arg("l")
            .arg(format!("{}", temp_path.display()))
            .output()
            .unwrap();

        let stdout = std::str::from_utf8(&output.stdout).unwrap().to_string();
        assert!(stdout.contains("WaykNow_x64.exe"));
        assert!(stdout.contains("WaykNow_x86.exe"));
        assert!(stdout.contains("Installer_x86.msi"));
        assert!(stdout.contains("Installer_x64.msi"));
        assert!(stdout.contains("branding.zip"));
        assert!(stdout.contains("init.ps1"));
        assert!(stdout.contains("options.json"));
    }
}
