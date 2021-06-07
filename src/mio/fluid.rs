/// Fluid sensor interface
use std::path::{ PathBuf};
use serde::{Serialize, Deserialize};
// use async_trait::async_trait;
use super::{*};

// use std::convert::TryFrom;
/// interface transfer
// impl TryFrom<Interface> for Fluid {
//     fn try_from(iface: Interface) -> Result<Self> {
//         iface.set_itype(IType::Fluid)?;
//         Ok(Self{
//             path:iface.path,
//         })
//     }
// }


#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Fluid {
    path:PathBuf,
}


impl From<&Interface> for Fluid {
    #[inline]
    fn from(device:&Interface) -> Fluid {
        Fluid{
            path: device.path.to_path_buf()
        }
    }
}
impl From<&Fluid> for digital::DigIN {
    #[inline]
    fn from(fluid:&Fluid) -> digital::DigIN {
        digital::DigIN{
            path: fluid.path.to_path_buf()
        }
    }
}
impl Fluid {
    pub fn empty(&self) -> Result<bool> {
        digital::DigIN::from(self).is_high()
    }

    pub fn full(&self ) -> Result<bool> {
        digital::DigIN::from(self).is_low()
    }
}

