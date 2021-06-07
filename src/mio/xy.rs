use super::{
    Vessel,
    Pump,
    Valve,
    Fluid,
    Relay,
    Autosampler,
    Stirrer,
    Furnace,
    TicPort,
    Cooler,
    Humidity,
    Airflow,
    Pressure,
    Sensor,
};
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Deserialize, Serialize,Default)]
pub struct XySettings {
    pub furnace: Vessel,
    pub ticport: Vessel,
    pub xhold: u32,
    pub yhold: u32,
    pub zhold: u32,
    pub xmax: u32,
    pub ymax: u32,
    pub zmax: u32,
    pub xcurrent: u32,
    pub ycurrent: u32,
    pub zcurrent: u32,
    pub stirrer_current: u32,
    pub stirrer_delay: u32,
    pub stirrer_current_delution: u32,
    pub stirrer_delay_delution: u32,
    pub rinse: u32,
    pub air: u32,
    pub air_cod: u32,
}

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct XYSystem {
    pub settings: XySettings,
    pub condensat_pump: Pump,
    pub sample_pump: Vec<Pump>,
    pub humidity_valve: Valve,
    pub tic_valve: Valve,
    pub injection_valve: Valve,
    pub tocdirect_valve: Valve,
    pub fluid: Vec<Fluid>,
    pub relay: Vec<Relay>,
    pub autosampler:Autosampler,
    pub stirrer: Stirrer,
    pub furnace: Furnace,
    pub ticport: TicPort,
    pub cooler: Cooler,
    pub humidity: Humidity,
    pub airflow: Airflow,
    pub pressure: Pressure,
    pub ndir1: Sensor,
    pub ndir2: Sensor,
    pub codo: Sensor,
    pub tnb: Sensor,
}



// impl XYSystem {
//     pub fn new(settings:XySettings) -> XYSystem {
//         // crate::start();
//         let condensat_pump = Pump::new("condensat_pump");
//         let sample_pump = vec![Pump::new("sample1_pump"), Pump::new("sample2_pump"), Pump::new("sample3_pump"), Pump::new("sample4_pump"), Pump::new("sample5_pump"), Pump::new("sample6_pump")];
//         let humidity_valve = Valve::new("humidity_valve");
//         let tic_valve = Valve::new("tic_valve");
//         let tocdirect_valve = Valve::new("tocdirect_valve");
//         let injection_valve = Valve::new("injection_valve");
//         let fluid = vec![Fluid::new("fluid_sensor1")];
//         let relay = vec![Relay::new("relay1")];
//         let autosampler = Autosampler::new();
//         let stirrer = Stirrer::new("stirrer");
//         let furnace = Furnace::new();
//         let ticport = TicPort::new();
//         let cooler = Cooler::new();
//         let humidity = Humidity::new();
//         let airflow = Airflow::new();
//         let pressure = Pressure::new();
//         let ndir1 = Sensor::new("ndir1",SensorModel::NDIR1);
//         let ndir2 = Sensor::new("ndir2",SensorModel::NDIR2);
//         let tnb   = Sensor::new("tnb",SensorModel::TNB);
//         let codo  = Sensor::new("codo",SensorModel::CODO);
//         XYSystem{settings,
//             condensat_pump,sample_pump,humidity_valve,
//             tic_valve,injection_valve,tocdirect_valve,fluid,relay,
//             autosampler,stirrer,furnace,ticport,cooler,humidity,airflow,pressure,
//             ndir1,ndir2,codo,tnb
//         }
//     }
// }