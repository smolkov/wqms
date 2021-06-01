/// Humidity sensor
/// Anschlus Analog:IN04
///
use serde::{Deserialize, Serialize};
use crate::Result;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Humidity {
    pub value: f32,
    pub broken:bool,

}

//Humidity from adc 16bit
// impl From<u16> for Humidity {
//     fn from(adc :u16) -> Self {
//         let mut signal =  adc as f32 / 4096.0 * 5.0;
//         let broken = signal < 0.8 * 4.0 / 5.0;
//         signal = (signal - 0.8)  / (3.6 - 0.8);
//         Humidity {
//             value: signal,
//             broken: broken,
//         }
//     }
// }


impl Humidity {
    pub fn new() -> Humidity{
        Humidity{
            value:0.0,
            broken: true,
        }
    }
}
