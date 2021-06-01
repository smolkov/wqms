/// Pressure sensor
/// Anschlus:  `Analog:IN04`
/// Model:     `presurei877`
///
use serde::{Deserialize, Serialize};
use crate::Result;
/// Presure
///
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Pressure {
    pub value: f32,
    pub broken: bool,
}

// Pressure ad 16bit
// impl From<u16> for Pressure {
//     fn from(adc :u16) -> Self {
//         let mut signal =  adc as f32 / 4096.0 * 5.0;
//         let broken = signal < 1.0 * 4.0 / 5.0;
//         signal = (signal - 1.0)  / (5.0 - 1.0);
//         Pressure{
//             fsr: signal,
//             broken: broken,
//         }
//     }
// }



impl Pressure {
    pub fn new() -> Pressure{
        Pressure{
            value:0.0,
            broken: true,
        }
    }
    pub fn pressure(&mut self) -> Result<f32> {
        Ok(self.value)
    }
}