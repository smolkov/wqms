/// Pressure sensor
/// Anschlus `Analog:IN04`
///
///
use serde::{Serialize, Deserialize};
// use crate::io;
// use std::ops::Range;
// use super::error::MioError;

/// für 0..60:   0.230197;
static A6:f32 = 0.003836617;
/// für 0..60:  -3.616438;
static A5:f32 = -0.06027397;
/// für 0..60:  22.36370;
static A4:f32 = 0.3727283;
/// für 0..60: -68.58285;
static A3:f32 = -1.1430475;
/// für 0..60: 110.3052;
static A2:f32 = 1.83842;
/// für 0..60: -84.19201;
static A1:f32 = -1.4032;
/// für 0..60:  23.49542;
static A0:f32 = 0.39159;





#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Airflow {
    pub input : f32,
    pub output : f32,
    pub broken:bool,
    
}

impl Airflow {
    pub fn new() -> Airflow {
        Airflow{
            input: 0.0,
            output: 0.0,
            broken: false,
        }    
    }

}



// Airflow from adc
// impl From<u16> for Airflow {
//     fn from(adc :u16) -> Self {
//         let mut signal = adc as f32 / 4095.0 * 5.0;
//         let broken = signal> 1.0;
//         signal = (((((A6*signal +A5)*signal +A4) *signal +A3) *signal + A2) * signal + A1) *signal + A0;
//         Airflow {
//             input: signal,
//             broken: broken
//         }
//     }
// }