fn main() {
    let dir = cmake::Config::new("..")
        .generator("Ninja")
        .static_crt(true)
        .target("i686-pc-windows-msvc")
        .build_target("WaykCse")
        .build();

    let cse_binary_path = dir.join("build").join("bin").join("WaykCse.exe");
    println!(
        "cargo:rustc-env=WAYK_CSE_PATH={}",
        cse_binary_path.display()
    );
}
