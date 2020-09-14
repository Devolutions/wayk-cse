use std::fmt::Write;

use log::info;
use version_compare::{CompOp, VersionCompare};

use crate::error::{WaykCseResult, WaykCseError};

#[derive(Debug, Clone, Copy)]
pub struct NowVersion {
    quad: [u32; 4],
}

impl NowVersion {
    pub fn from_quad(quad: [u32; 4]) -> Self {
        Self { quad }
    }

    pub fn as_triple(&self) -> String {
        format!("{}.{}.{}", self.quad[0], self.quad[1], self.quad[2])
    }

    pub fn as_quad(&self) -> String {
        let mut version = self.as_triple();
        write!(&mut version, ".{}", self.quad[3]).unwrap();
        version
    }
}