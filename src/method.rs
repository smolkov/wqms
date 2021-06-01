use serde_derive::{Deserialize, Serialize};
use std::time::{Duration};
use crate::measurement::Measurement;



//     pub stream:      Stream,
//     pub measurement: MeasurementSetting,
//     pub calibration: CalibrationSetting,
//     pub single:      SingleSetting,
//     pub check:       CheckSetting,
// } v

// impl Method {
//     pub new() -> Method {
//        Method {
//            stream: Stream::new(1),
//            measurement: MeasurementSetting::default(),
//            calibration: CalibrationSetting::default(),
//            single: SingleSetting::default(),
//            check: CheckSetting::default(),
//        } 
//     }
// }


// pub struct Method<T> where 
//     T: Clone
// {
//     value: T, 
//     ch: Channel<T>,
//     delay: Option<Duration>,
//     volume: Volume,
// }


// impl <T>Method<T> where 
//     T: Clone,
// {
    
//     pub fn new(id:&mut Identificator,value:T) -> Method<T> {
//         Method{
//             value: value,
//             ch: Channel::new(id.chanel_next().clone(),value.clone()),
//             delay: None,
//             volume: Volume::new(100),
//         }
//     }
// }

