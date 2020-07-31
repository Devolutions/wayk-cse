use std::{
    collections::HashMap,
    path::{Path, PathBuf},
};

use rcedit::{RceditResult, ResourceUpdater};

use crate::error::{WaykCseError, WaykCseResult};

const WAYK_BUNDLE_RESOURCE_ID: u32 = 102;

#[derive(PartialEq, Eq, Hash, Copy, Clone)]
pub enum WaykCseOption {
    EnableUnattendedService,
    WaykDataPath,
    WaykSystemPath,
    WaykExtractionPath,
    WaykProductName,
    EnableWaykAutoClean,
}

impl WaykCseOption {
    pub fn get_string_resource_id(self) -> u32 {
        match self {
            WaykCseOption::EnableUnattendedService => 103,
            WaykCseOption::WaykDataPath => 104,
            WaykCseOption::WaykSystemPath => 105,
            WaykCseOption::WaykExtractionPath => 106,
            WaykCseOption::WaykProductName => 107,
            WaykCseOption::EnableWaykAutoClean => 108,
        }
    }
}

pub struct ResourcePatcher {
    resource_updater: ResourceUpdater,
    original_binary_path: Option<PathBuf>,
    icon_path: Option<PathBuf>,
    wayk_bundle_path: Option<PathBuf>,
    wayk_cse_options: HashMap<WaykCseOption, String>,
}

impl ResourcePatcher {
    pub fn new() -> Self {
        ResourcePatcher {
            resource_updater: ResourceUpdater::new(),
            original_binary_path: None,
            icon_path: None,
            wayk_bundle_path: None,
            wayk_cse_options: HashMap::new(),
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

    pub fn set_wayk_cse_option(&mut self, option: WaykCseOption, value: &str) -> &mut Self {
        self.wayk_cse_options.insert(option, value.into());
        self
    }

    pub fn patch(&mut self) -> WaykCseResult<()> {
        if self.original_binary_path.is_none() {
            return Err(WaykCseError::ResourcePatchingFailed(
                "Original binary path is not set".into(),
            ));
        }

        check_rcedit(
            self.resource_updater.load(&self.original_binary_path.as_ref().unwrap()),
            "WaykDummy load failed",
        )?;

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

        for (option, value) in &self.wayk_cse_options {
            let id = option.get_string_resource_id();
            check_rcedit(
                self.resource_updater.set_string(id, &value),
                &format!("Failed to set resource with id {}", id),
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

fn check_rcedit<T>(result: RceditResult<T>, message: &str) -> WaykCseResult<T> {
    result.map_err(|e| WaykCseError::ResourcePatchingFailed(format!("{}: {}", message, e)))
}
