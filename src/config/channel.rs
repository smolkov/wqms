use serde::{Serialize, Deserialize};
use super::{Adjustment,Linear,Integration};




#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Channel {
    pub activ:  bool,
    pub name:   String,
    pub unit:   String,
    pub sensor: String,
    pub min:    f64,
    pub max:    f64,
    pub value:  f64,
    pub integration: Integration,
    pub adjust:  Adjustment,
}

/// Channel
impl Channel {
    pub fn new() -> Channel {
        Channel {
            activ : false,
            name: "CH".to_owned(),
            unit:"mg/l".to_owned(),
            sensor: "ndir1".to_owned(),
            min: 0.0,
            max: 0.0,
            value: 0.0,
            integration: Integration::default(),
            adjust: Adjustment::Lineal(Linear::new(1.0,0.0)),
        }
    }
}
