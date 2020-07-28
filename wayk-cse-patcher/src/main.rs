use log::error;

use wayk_cse_patcher::patcher::WaykCsePatcher;

fn main() {
    env_logger::from_env(env_logger::Env::default().default_filter_or("info")).init();

    if let Err(err) = WaykCsePatcher::new().run() {
        error!("Wayk CSE patcher failed: {}", err);
    }
}
