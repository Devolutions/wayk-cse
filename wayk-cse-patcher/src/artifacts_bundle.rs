use std::{
    fs::File,
    path::Path,
    io::{self, Write},
};

use zip::{
    ZipArchive,
    result::ZipError
};
use thiserror::Error;


#[derive(Error, Debug)]
pub enum Error {
    #[error("IO error detected during artifacts bundle processing ({0})")]
    IoError(#[from] io::Error),
    #[error("Failed to  parse zip file ({0})")]
    InvalidZipFile(#[from] ZipError),
    #[error("File {0} is missing inside artifacts bundle zip")]
    MissingFile(String),
}

pub type ArtifactsBundleResult<T> = Result<T, Error>;


pub struct ArtifactsBundle {
    archive: ZipArchive<File>,
}

impl ArtifactsBundle {
    pub fn open(path: &Path) -> ArtifactsBundleResult<Self> {
        let file = File::open(path)?;
        let archive = ZipArchive::new(file)?;
        Ok(Self { archive })
    }

    pub fn extract_wayk_now_binary<W: Write>(&mut self, writer: &mut W) -> ArtifactsBundleResult<u64> {
        const WAYK_NOW_FILE_NAME: &str = "WaykNow.exe";

        let mut compressed = self.archive.by_name(WAYK_NOW_FILE_NAME).map_err(|_| {
            Error::MissingFile(WAYK_NOW_FILE_NAME.to_string())
        })?;

        Ok(io::copy(&mut compressed, writer)?)
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

        let mut bundle = ArtifactsBundle::open(
            Path::new("tests/data/wayk_now_mock_binaries.zip")
        ).unwrap();

        let size = bundle.extract_wayk_now_binary(&mut file).unwrap();

        assert_eq!(size, 3);

        file.seek(SeekFrom::Start(0)).unwrap();

        let expected = b"123".to_vec();

        let mut actual = Vec::new();
        file.read_to_end(&mut actual).unwrap();

        assert_eq!(actual, expected)
    }
}