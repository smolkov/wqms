
use serde::{Deserialize, Serialize};

use super::{
    Statistic,
    Channel,
    Solution,
    Vessel,
};



/// Stream 
#[derive(Clone, Deserialize, Serialize, PartialEq, Debug)]
pub struct XyStream {
    pub number: u64,
    pub name : String,
    pub sample: Vessel,
    pub drain: Vessel,
    pub tc_volume: u32,
    pub tic_volume: u32,
    pub air: u32,
    pub sampling_sec: u64, 
    pub striping_sec: u64,
    pub injection_wait: u64,
    pub measurement:Statistic,
    pub calibration:Statistic,
    pub tc: Channel,
    pub tic: Channel,
    pub toc: Channel,
    pub tnb: Channel,
    pub codo: Channel,
    pub solution: Vec<Solution>,
}


impl Default for XyStream {
    fn default()-> Self {
        let sample = Vessel{
            xpos: 1100,
            ypos: 1200,
            needle: 650,
        };
        let drain = Vessel{
            xpos: 2300,
            ypos: 1200,
            needle: 650,
        };
        Self {
            number:1,
            name: "stream1".to_owned(),
            tc_volume: 100,
            tic_volume: 300,
            sample:sample,
            drain:drain,
            air: 50,
            sampling_sec:1,
            striping_sec: 0,
            injection_wait: 2,
            measurement: Statistic::default(),
            calibration: Statistic::default(),
            tc: Channel::new(),
            tic: Channel::new(),
            toc: Channel::new(),
            tnb: Channel::new(),
            codo: Channel::new(),
            solution: vec![Solution::default()],
        }
    }
}

#[derive(Clone,Default,Debug, Deserialize, Serialize)]
pub struct XyConfig {
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
