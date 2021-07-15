/// Lamp UV
// use std::prelude::*;
use std::fs;
use std::path::{PathBuf};
// use async_trait::async_trait;
// use std::time::{Duration,Instant};
use super::Interface;
use serde::{Deserialize, Serialize};
use crate::Result;
// use std::convert::TryFrom;
/// interface transfer

// impl TryFrom<Interface> for Lamp {
//     type Error = Error;
//     fn try_from(iface: Interface) -> Result<Self> {
//         iface.set_itype(IType::Lamp)?;
//         Ok(Self{
//             path:iface.path,
//         })
//     }
// }


/// Lamp interface
 #[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Lamp {
    path: PathBuf,
}

impl From<Interface> for Lamp{
    fn from(drv:Interface) -> Lamp {
        Lamp{
            path: drv.path.to_path_buf()
        }
    }
}

impl Lamp {
    pub fn radiation_on(&mut self)  -> Result<()> {
        fs::write(self.path.join("value"), b"1")?;
        Ok(())
    }
    pub fn radiation_off(&mut self) -> Result<()> {
        fs::write(self.path.join("value"), b"0")?;
        Ok(())
    }
}


// pub fn create(path:&Path) -> Result<Lamp> {
//     let drv = device::create(path,IType::Lamp)?;
//     fs::write(path.join("radiation"), b"0")?;
//     Ok(drv.into())
// }