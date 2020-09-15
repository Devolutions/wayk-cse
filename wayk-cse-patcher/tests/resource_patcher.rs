use std::{fs::File, io, path::Path};

use tempfile::{Builder as TempFileBuilder, TempPath};
use wayk_cse_patcher::resource_patcher::ResourcePatcher;

fn copy_test_binary() -> TempPath {
    let mut temp_file = TempFileBuilder::new().suffix(".exe").tempfile().unwrap();
    let mut original = File::open("tests/data/MockCseDummy.exe").unwrap();
    io::copy(&mut original, &mut temp_file).unwrap();
    temp_file.into_temp_path()
}

fn patch(binary_path: &Path) {
    let mut updater = ResourcePatcher::new();
    updater.set_original_binary_path(binary_path);
    updater.set_icon_path(Path::new("tests/data/cse_icon.ico"));
    updater.set_wayk_bundle_path(Path::new("tests/data/wayk_bundle_fake.7z"));
    updater.patch().unwrap();
}

#[test]
fn patching_test() {
    let binary_path = copy_test_binary();
    patch(&binary_path);
    let patched_file = File::open(&binary_path).unwrap();
    assert_eq!(patched_file.metadata().unwrap().len(), 12288);
}
