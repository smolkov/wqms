use std::path::PathBuf;

/// Pressure sensor
/// Anschlus `Analog:IN04`
///
///
use serde::{Serialize, Deserialize};
use super::Interface;
// use crate::Result;
// use crate::io;
// use std::ops::Range;
// use super::error::MioError;

/// für 0..60:   0.230197;
// static A6:f32 = 0.003836617;
/// für 0..60:  -3.616438;
// static A5:f32 = -0.06027397;
/// für 0..60:  22.36370;
// static A4:f32 = 0.3727283;
/// für 0..60: -68.58285;
// static A3:f32 = -1.1430475;
/// für 0..60: 110.3052;
// static A2:f32 = 1.83842;
/// für 0..60: -84.19201;
// static A1:f32 = -1.4032;
/// für 0..60:  23.49542;
// static A0:f32 = 0.39159;




#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Airflow {
   path: PathBuf,
}




impl From<Interface> for Airflow {
    fn from(drv:Interface) -> Airflow {
        Airflow{
            path: drv.path.to_path_buf(),
        }
    }
}
