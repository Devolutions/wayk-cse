use std::{
    fmt::{Display, Formatter, Result as FmtResult},
    fs::{self, create_dir_all, File},
    path::{Path, PathBuf},
    process::Command,
};

use tempfile::Builder as TempFileBuilder;

use fs_extra::dir::CopyOptions as DirCopyOptions;
use zip::ZipArchive;

use crate::error::{WaykCseError, WaykCseResult};

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

pub enum BundlePackageType {
    WaykBinariesArchive { bitness: Bitness, enable_unattended: bool },
    PowerShellModule,
    BrandingZip,
    CustomInitializationScript,
}

pub struct BundlePacker {
    packages: Vec<(BundlePackageType, PathBuf)>,
}

impl BundlePacker {
    pub fn new() -> Self {
        Self { packages: Vec::new() }
    }

    pub fn add_bundle_package(&mut self, package_type: BundlePackageType, path: &Path) {
        self.packages.push((package_type, path.into()));
    }

    pub fn pack(&self, output_path: &Path) -> WaykCseResult<()> {
        let bundle_directory = TempFileBuilder::new().prefix("bundle_").tempdir().map_err(|_| {
            WaykCseError::BundleGenerationFailed("Can't create temp directory for bundle packing".into())
        })?;

        for (package_type, package_path) in &self.packages {
            match package_type {
                BundlePackageType::WaykBinariesArchive {
                    bitness,
                    enable_unattended,
                } => {
                    let output_dir = bundle_directory.path().join(format!("Wayk_{}", bitness));

                    create_dir_all(&output_dir).map_err(|e| {
                        WaykCseError::BundleGenerationFailed(format!(
                            "Failed to create bundled WaykNow binaries directory -> {}",
                            e
                        ))
                    })?;

                    extract_wayk_binaries(package_path, &output_dir, *enable_unattended)?;
                }
                BundlePackageType::PowerShellModule => {
                    let mut copy_options = DirCopyOptions::new();
                    copy_options.copy_inside = true;

                    let output_dir = bundle_directory.path().join("PowerShell").join("Modules");

                    create_dir_all(&output_dir).map_err(|e| {
                        WaykCseError::BundleGenerationFailed(format!(
                            "Failed to create PowerShell module bundle folder -> {}",
                            e
                        ))
                    })?;

                    fs_extra::dir::copy(&package_path, &output_dir, &copy_options).map_err(|e| {
                        WaykCseError::BundleGenerationFailed(format!("Failed to copy modules directory -> {}", e))
                    })?;

                    let package_path_folder_name = package_path
                        .file_name()
                        .ok_or_else(|| WaykCseError::BundleGenerationFailed("Invalid PowerShell module path".into()))?;

                    if package_path_folder_name != "WaykNow" {
                        fs_extra::dir::move_dir(
                            output_dir.join(package_path_folder_name),
                            output_dir.join("WaykNow"),
                            &copy_options,
                        )
                        .map_err(|e| {
                            WaykCseError::BundleGenerationFailed(format!(
                                "Failed to rename PowerShell module directory -> {}",
                                e
                            ))
                        })?;
                    }
                }
                BundlePackageType::BrandingZip => {
                    fs::copy(package_path, bundle_directory.path().join("branding.zip")).map_err(|e| {
                        WaykCseError::BundleGenerationFailed(format!("Failed to copy branding.zip -> {}", e))
                    })?;
                }
                BundlePackageType::CustomInitializationScript => {
                    fs::copy(package_path, bundle_directory.path().join("init.ps1")).map_err(|e| {
                        WaykCseError::BundleGenerationFailed(format!("Failed to copy PowerShell init script -> {}", e))
                    })?;
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

fn extract_wayk_binaries(archive_path: &Path, output_directory: &Path, unattended: bool) -> WaykCseResult<()> {
    let zip_file = File::open(archive_path)
        .map_err(|e| WaykCseError::BundleGenerationFailed(format!("Failed to open wayk binaries zip -> {}", e)))?;

    let mut zip = ZipArchive::new(zip_file)
        .map_err(|e| WaykCseError::BundleGenerationFailed(format!("Failed to read wayk binaries zip -> {}", e)))?;

    let required_files: &[&str] = if unattended {
        &["WaykNow.exe", "WaykHost.exe", "NowSession.exe", "NowService.exe"]
    } else {
        &["WaykNow.exe"]
    };

    if !output_directory.exists() {
        create_dir_all(output_directory).map_err(|e| {
            WaykCseError::BundleGenerationFailed(format!("Failed to create WaykNow binaries directory -> {}", e))
        })?;
    }

    for file in required_files.iter().copied() {
        let mut compressed = zip.by_name(file).map_err(|_| {
            WaykCseError::BundleGenerationFailed(format!("Can't find file {} in {}", file, archive_path.display()))
        })?;

        let mut output_file = File::create(output_directory.join(file))
            .map_err(|e| WaykCseError::BundleGenerationFailed(format!("Can't create file {} -> {}", file, e)))?;

        std::io::copy(&mut compressed, &mut output_file)
            .map_err(|e| WaykCseError::BundleGenerationFailed(format!("Failed to extract file {} -> {}", file, e)))?;
    }

    Ok(())
}

fn get_archiver_path() -> WaykCseResult<PathBuf> {
    which::which("7z")
        .map_err(|e| WaykCseError::InvalidEnvironment(
            format!("Failed to find 7z path -> {}", e)
        ))
}

fn compress_bundle(unpacked_bundle_path: &Path, output_path: &Path) -> WaykCseResult<()> {
    let archiver_output = Command::new(get_archiver_path()?)
        .arg("a")
        .arg(format!("{}", output_path.display()))
        .arg(format!("{}", unpacked_bundle_path.join("*").display()))
        .output()
        .map_err(|e| WaykCseError::BundleGenerationFailed(format!("7z invocation failed -> {}", e)))?;

    if !archiver_output.status.success() {
        let stderr = std::str::from_utf8(&archiver_output.stderr).unwrap_or("<INVALID STDERR>");

        return Err(WaykCseError::BundleGenerationFailed(format!(
            "7z packing failed: -> {}",
            stderr
        )));
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs::read_dir;

    use tempfile::tempdir;

    #[test]
    fn test_extract_wayk_binaries_unattended() {
        let temp_dir = tempdir().unwrap();

        extract_wayk_binaries(
            Path::new("tests/data/wayk_now_mock_binaries.zip"),
            temp_dir.path(),
            true,
        )
        .unwrap();

        assert_eq!(read_dir(&temp_dir).unwrap().into_iter().count(), 4);
    }

    #[test]
    fn test_extract_wayk_binaries_non_unattended() {
        let temp_dir = tempdir().unwrap();

        extract_wayk_binaries(
            Path::new("tests/data/wayk_now_mock_binaries.zip"),
            temp_dir.path(),
            false,
        )
        .unwrap();

        assert_eq!(read_dir(&temp_dir).unwrap().into_iter().count(), 1);
    }

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
        packer.add_bundle_package(BundlePackageType::BrandingZip, Path::new("tests/data/branding.zip"));
        packer.add_bundle_package(
            BundlePackageType::PowerShellModule,
            Path::new("tests/data/fake_powershell_module"),
        );
        packer.add_bundle_package(
            BundlePackageType::CustomInitializationScript,
            Path::new("tests/data/fake_init_script.ps1"),
        );
        packer.add_bundle_package(
            BundlePackageType::WaykBinariesArchive {
                bitness: Bitness::X64,
                enable_unattended: false,
            },
            Path::new("tests/data/wayk_now_mock_binaries.zip"),
        );
        packer.add_bundle_package(
            BundlePackageType::WaykBinariesArchive {
                bitness: Bitness::X86,
                enable_unattended: true,
            },
            Path::new("tests/data/wayk_now_mock_binaries.zip"),
        );

        packer.pack(&temp_path).unwrap();

        let output = Command::new(get_archiver_path().unwrap())
            .arg("l")
            .arg(format!("{}", temp_path.display()))
            .output()
            .unwrap();

        let stdout = std::str::from_utf8(&output.stdout).unwrap().to_string();
        assert!(stdout.contains("Wayk_x64"));
        assert!(stdout.contains("Wayk_x86"));
        assert!(stdout.contains("branding.zip"));
        assert!(stdout.contains("init.ps1"));
        assert!(stdout.contains("test_file.txt"));
        assert!(stdout.contains("normal_file.txt"));
    }
}
