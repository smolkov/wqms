pub mod interface;
pub mod digital;
pub mod airflow;
pub mod humidity;
pub mod pressure;
pub mod sensor;
pub mod stirrer;
pub mod axis;
pub mod ticport;
pub mod furnace;
pub mod lamp;
pub mod pump;
pub mod relay;
pub mod valve;
pub mod cooler;
pub mod fluid;
pub mod xy;
pub mod loopdev;
pub mod uv;


pub use interface::*;
pub use pump::Pump;
pub use lamp::Lamp;
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
pub use sensor::{Sensor,SensorModel};
pub use axis::{Axis};
pub use xy::Xy;
pub use loopdev::Loop;
pub use uv::Uv;
use crate::Result;

use std::path::{PathBuf};
use serde::{Deserialize, Serialize};
use std::fs;
// pub fn simulation() -> Result<XYSystem> {
    // Ok(XYSystem::new(XySettings::default()))
// }


#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct Mio{
    pub path : PathBuf,
}


impl Mio {
    pub fn new(path: &str) -> Result<Mio> {
        let path = PathBuf::from(path);
        let mio = Mio{path};
        if !mio.path.is_dir() {
            log::info!("mio [{}] mount", mio.path.display());
            fs::create_dir_all(&mio.path)?;
        }
        Ok(mio)
    }
    pub fn create_interface(&mut self,name:&str) -> Result<Interface> {
        let path = self.path.join(name);
        if !path.is_dir() {
            log::info!("mio create new device {} in {}",name,path.display());
            fs::create_dir_all(&path)?;
        }else{
            log::info!("interface {} in {} alreadey exist",name ,self.path.as_path().display());
        }
        Ok(Interface::from(path))
    }
    pub fn unmount(&mut self) -> Result<()>{
        log::info!("mio unmount {}",self.path.display());
        Ok(())
    }
}

