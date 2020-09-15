use std::{
    collections::HashMap,
    path::{Path, PathBuf},
};

use rcedit::{RceditResult, ResourceUpdater};
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

pub type ResourcePathcerResult<T> = Result<T, Error>;

pub struct ResourcePatcher {
    resource_updater: ResourceUpdater,
    original_binary_path: Option<PathBuf>,
    icon_path: Option<PathBuf>,
    wayk_bundle_path: Option<PathBuf>,
    product_name: Option<String>,
}

impl ResourcePatcher {
    pub fn new() -> Self {
        ResourcePatcher {
            resource_updater: ResourceUpdater::new(),
            original_binary_path: None,
            icon_path: None,
            wayk_bundle_path: None,
            product_name: None,
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


    pub fn patch(&mut self) -> ResourcePathcerResult<()> {
        if self.original_binary_path.is_none() {
            return Err(Error::ExecutableLoadFailed("original binary path is not set".to_string()));
        }

        self.resource_updater.load(&self.original_binary_path.as_ref().unwrap()).map_err(|e| {
            Error::ExecutableLoadFailed(format!("{}", e))
        })?;

        if let Some(icon_path) = self.icon_path.as_ref() {
            check_rcedit(self.resource_updater.set_icon(icon_path), "Icon patching failed")?;
        }

        if let Some(wayk_bundle_path) = self.wayk_bundle_path.as_ref() {
            check_rcedit(
                self.resource_updater
                    .set_rcdata(WAYK_BUNDLE_RESOURCE_ID, wayk_bundle_path),
                "Wayk bundle patching failed",
            )?;
        }

        if let Some(product_name) = self.product_name.as_ref() {
            check_rcedit(
                self.resource_updater.set_string(PRODUCT_NAME_RESOURCE_ID, product_name),
                "Failed to patch product name",
            )?;
        }

        check_rcedit(self.resource_updater.commit(), "Failed to commit patched binary")?;

        Ok(())
    }
}

impl Default for ResourcePatcher {
    fn default() -> Self {
        Self::new()
    }
}

fn check_rcedit<T>(result: RceditResult<T>, message: &str) -> ResourcePathcerResult<T> {
    result.map_err(|_| Error::ResourcePatchingFailed(message.to_string()))
}
