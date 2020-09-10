use thiserror::Error;

/// Main WaykCse patcher error type
#[derive(Error, Debug)]
pub enum WaykCseError {
    #[error("Patching failed: {0}")]
    PatchingError(String),
    #[error("Branding icon extraction failed: {0}")]
    BrandingIconExtractionFailed(String),
    #[error("Resource patching failed: {0}")]
    ResourcePatchingFailed(String),
    #[error("Failed to download the power shell module: {0}")]
    PowerShellModuleDownloadFailed(String),
    #[error("Invalid environment: {0}")]
    InvalidEnvironment(String),
    #[error("Signing failure: {0}")]
    SigningFailed(String),
    #[error("Failed to generate bundle archive: {0}")]
    BundleGenerationFailed(String),
    #[error("Failed to download package: {0}")]
    DownloadFailed(String),
    #[error("Artifacts bundle processing failed: {0}")]
    ArtifactsBundleError(String),
    #[error("Cse options parsing failed: {0}")]
    CseOptionsParsingFailed(String),
    #[error("Error: {0}")]
    Other(String),

}

/// Handy alias for Result with WaykCseError as error type
pub type WaykCseResult<T> = Result<T, WaykCseError>;
