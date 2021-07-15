/// Relay
// use std::prelude::*;
use std::fs;
// use async_trait::async_trait;
use std::path::PathBuf;
use super::*;
use serde::{Serialize, Deserialize};

pub const BOOLEXP: &'static str = "boolexp";


// use std::convert::TryFrom;
// impl TryFrom<Interface> for Relay {
//     type Error = Error;
//     fn try_from(iface: Interface) -> Result<Self> {
//         iface.set_itype(IType::Relay)?;
//         Ok(Self{
//             path:iface.path,
//         })
//     }
// }

 /// Relay simulation
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Relay {
    path: PathBuf,
}


impl From<&Interface> for Relay {
    #[inline]
    fn from(device:&Interface) -> Relay {
        Relay{
            path: device.path.to_path_buf()
        }
    }
}

impl Relay {
    pub fn is_open(&self) -> Result<bool> {
        match fs::read_to_string(self.path.join("value"))?.as_str() {
            "1" => Ok(false),
            _ => Ok(true),
        }
    }
    pub fn open(&mut self)  -> Result<()> {
        fs::write(self.path.join("value"), b"1")?;
        Ok(())
    }
    pub fn close(&mut self) -> Result<()> {
        fs::write(self.path.join("value"), b"0")?;
        Ok(())
    }
    pub fn boolexp(&self) -> Result<String> {
        let boolexp = fs::read_to_string(self.path.join(BOOLEXP))?;
        Ok(boolexp) 
    }
    pub fn set_boolexp(&self,boolexp:String) -> Result<()> {
        fs::write(self.path.join(BOOLEXP),boolexp.as_bytes())?;
        Ok(())
    }
}