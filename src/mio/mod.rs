pub mod airflow;
pub mod humidity;
pub mod pressure;
pub mod sensor;
pub mod stirrer;
pub mod autosampler;
pub mod ticport;
pub mod furnace;
pub mod pump;
pub mod relay;
pub mod valve;
pub mod cooler;
pub mod fluid;
pub mod vessel;
pub mod xy;


pub use pump::Pump;
pub use fluid::Fluid;
pub use relay::Relay;
pub use ticport::TicPort;
pub use furnace::Furnace;
pub use cooler::Cooler;
pub use stirrer::Stirrer;
pub use sensor::Fsr;
pub use valve::Valve;
pub use airflow::Airflow;
pub use humidity::Humidity;
pub use pressure::Pressure;
pub use vessel::Vessel;
pub use sensor::{Sensor,SensorModel};
pub use autosampler::{Autosampler,Speed,SpeedLevel};
pub use xy::{XYSystem,XySettings};
use crate::Result;



pub fn simulation() -> Result<XYSystem> {
    Ok(XYSystem::new(XySettings::default()))
}