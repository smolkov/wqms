/// Monitor gear pump normally used for solution sampling.
///
///
use crate::Result;
use serde::{Deserialize, Serialize};
use super::interface::*;

use std::path::PathBuf;
use std::fs;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
enum State{
    Start,
    Stop
}

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Pump {
    path :PathBuf,
}

impl From<Interface> for Pump{
    fn from(drv:Interface) -> Pump {
        Pump{
            path: drv.path.to_path_buf()
        }
    }
}

// use std::convert::TryFrom;
// impl TryFrom<Interface> for Pump {
//     type Error = anyhow::Error;
//     fn try_from(iface: Interface) -> Result<Self> {
//         let mut iface = iface;
//         iface.set_itype(IType::GearPump)?;
//         iface.set_iclass(IClass::Pump)?;
//         Ok(Self{
//             path:iface.path,
//         })
//     }
// }
impl Pump {
    pub fn new(path:&str) -> Pump {
        Pump { 
            path: PathBuf::from(path),
           }
    }
    pub fn start(&mut self) -> Result<()> {
        fs::write(self.path.join("state"), b"1")?;
        Ok(())
    }
    pub fn stop(&mut self) -> Result<()> {
        fs::write(self.path.join("state"), b"0")?;
        Ok(())
    }
}


