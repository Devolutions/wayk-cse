use std::{
    fs::File,
    io::Read,
    path::{Path, PathBuf},
};

use log::info;

use lief::{Binary, LiefResult};
use thiserror::Error;

const WAYK_BUNDLE_RESOURCE_ID: u32 = 102;
const PRODUCT_NAME_RESOURCE_ID: u32 = 103;

#[derive(Error, Debug)]
pub enum Error {
    #[error("Failed to load executable ({0})")]
    ExecutableLoadFailed(String),
    #[error("Failed to patch resource ({0})")]
    ResourcePatchingFailed(String),
}

pub type ResourcePatcherResult<T> = Result<T, Error>;

pub struct ResourcePatcher {
    original_binary_path: Option<PathBuf>,
    icon_path: Option<PathBuf>,
    wayk_bundle_path: Option<PathBuf>,
    product_name: Option<String>,
    output_path: Option<PathBuf>,
}

impl ResourcePatcher {
    pub fn new() -> Self {
        ResourcePatcher {
            original_binary_path: None,
            icon_path: None,
            wayk_bundle_path: None,
            product_name: None,
            output_path: None,
        }
    }

    pub fn set_original_binary_path(&mut self, path: &Path) -> &mut Self {
        self.original_binary_path = Some(path.into());
        self
    }

    pub fn set_icon_path(&mut self, path: &Path) -> &mut Self {
        self.icon_path = Some(path.into());
        self
    }

    pub fn set_wayk_bundle_path(&mut self, path: &Path) -> &mut Self {
        self.wayk_bundle_path = Some(path.into());
        self
    }

    pub fn set_product_name(&mut self, name: &str) -> &mut Self {
        self.product_name = Some(name.to_string());
        self
    }

    pub fn set_output_path(&mut self, path: &Path) -> &mut Self {
        self.output_path = Some(path.to_path_buf());
        self
    }

    pub fn patch(&mut self) -> ResourcePatcherResult<()> {


        if self.original_binary_path.is_none() {
            return Err(Error::ExecutableLoadFailed(
                "original binary path is not set".to_string(),
            ));
        }

        let original_binary_path = std::mem::replace(&mut self.original_binary_path, None).unwrap();

        let binary = Binary::new(original_binary_path).map_err(|err| {
            Error::ResourcePatchingFailed(format!("Failed to parse binary file:{:?}", err))
        })?;

        let resource_manager = binary.resource_manager().map_err(|err| {
            Error::ResourcePatchingFailed(format!("Failed to create resource manager:{:?}", err))
        })?;

        if let Some(icon_path) = self.icon_path.take() {
            check_lief(resource_manager.set_icon(icon_path), "Icon setting failed")?;
        }

        if let Some(wayk_bundle_path) = self.wayk_bundle_path.as_ref() {
            let mut file = File::open(wayk_bundle_path).unwrap();
            let mut data = Vec::new();

            file.read_to_end(&mut data).map_err(|_| {
                Error::ResourcePatchingFailed("Failed to read content of Wayk bundle".to_owned())
            })?;
            check_lief(
                resource_manager.set_rcdata(data, WAYK_BUNDLE_RESOURCE_ID),
                "Wayk bundle patching failed",
            )?;
        }

        if let Some(product_name) = self.product_name.take() {
            check_lief(
                resource_manager.set_string(product_name, PRODUCT_NAME_RESOURCE_ID),
                "Failed to patch product name",
            )?;
        }

        if let Some(output_path) = self.output_path.take() {
            check_lief(binary.build(output_path), "Failed to create binary file")?
        }

        Ok(())
    }
}

impl Default for ResourcePatcher {
    fn default() -> Self {
        Self::new()
    }
}


fn check_lief<T>(result: LiefResult<T>, message: &str) -> ResourcePatcherResult<T> {
    result
        .map_err(|err| Error::ResourcePatchingFailed(format!("{}:{:?}", message.to_string(), err)))
}
