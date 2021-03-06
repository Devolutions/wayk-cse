use std::{fs::File, io, path::Path};

use tempfile::{Builder as TempFileBuilder, TempPath, NamedTempFile};
use wayk_cse_patcher::resource_patcher::ResourcePatcher;

const WAYK_CSE_DUMMY_PATH: &str = "tests/data/MockCseDummy.exe";

fn copy_test_binary() -> TempPath {
    let mut temp_file = TempFileBuilder::new().suffix(".exe").tempfile().unwrap();
    let mut original = File::open(WAYK_CSE_DUMMY_PATH).unwrap();
    io::copy(&mut original, &mut temp_file).unwrap();
    temp_file.into_temp_path()
}

fn patch(binary_path: &Path) -> NamedTempFile {
    let output_path = TempFileBuilder::new().prefix("Outfile").suffix(".exe").tempfile().unwrap();

    let mut updater = ResourcePatcher::new();
    updater.set_original_binary_path(binary_path);
    updater.set_output_path(output_path.path());
    updater.set_icon_path(Path::new("tests/data/cse_icon.ico"));
    updater.set_wayk_bundle_path(Path::new("tests/data/wayk_bundle_fake.7z"));
    updater.patch().unwrap();

    output_path
}

#[test]
fn patching_test() {
    let original = File::open(WAYK_CSE_DUMMY_PATH).unwrap();

    let binary_path = copy_test_binary();
    let patched_file = patch(&binary_path);

    assert!(patched_file.as_file().metadata().unwrap().len() > original.metadata().unwrap().len());
}
