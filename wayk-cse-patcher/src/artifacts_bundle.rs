use std::{
    fs::File,
    path::Path,
    io::{self, Write},
};

use zip::ZipArchive;

use crate::error::{WaykCseResult, WaykCseError};


struct ArtifactsBundle {
    archive: ZipArchive<File>,
}

impl ArtifactsBundle {
    pub fn open(path: &Path) -> WaykCseResult<Self> {
        let file = File::open(path).map_err(|e| {
            WaykCseError::ArtifactsBundleError(format!("Can't open arifacts bundle file: {}",e ))
        })?;

        let archive = ZipArchive::new(file).map_err(|e| {
            WaykCseError::ArtifactsBundleError(format!("Zip parsing error: {}", e))
        })?;

        Ok(Self { archive })
    }

    pub fn extract_wayk_now_binary<W: Write>(&mut self, writer: &mut W) -> WaykCseResult<u64> {
        let mut compressed = self.archive.by_name("WaykNow.exe").map_err(|e| {
            WaykCseError::ArtifactsBundleError(format!("Can't find WaykNow binary: {}", e))
        })?;

        io::copy(&mut compressed, writer).map_err(|e| {
            WaykCseError::ArtifactsBundleError(format!("Can't extract WaykNow binary: {}", e))
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::{Seek, SeekFrom, Read};

    use tempfile::tempfile;

    #[test]
    fn wayk_binary_extracts_successfully() {
        let mut file = tempfile().unwrap();

        let mut bundle = ArtifactsBundle::open(Path::new("tests/data/wayk_now_mock_binaries.zip"))
            .unwrap();

        let size = bundle.extract_wayk_now_binary(&mut file).unwrap();

        assert_eq!(size, 3);

        file.seek(SeekFrom::Start(0));

        let expected = b"123".to_vec();

        let mut actual = Vec::new();
        file.read_to_end(&mut actual).unwrap();

        assert_eq!(actual, expected)
    }
}