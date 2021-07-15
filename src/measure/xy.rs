use serde::{Serialize, Deserialize};
use crate::config::Statistic;
use crate::config::XyStream as Stream;
use crate::Result;
// use std::time::{SystemTime};




#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct MeasData {
    pub value: f64,
    pub outlier: bool,
    // pub signal: Signal,
}

/// Humidity from adc 16bit
impl From<f64> for MeasData {
    fn from(value :f64) -> Self {
        MeasData {
            value: value,
            outlier: false,
        }
    }
}
/// Measurement data result
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct MeasReplicate {
    pub time : u64,
    pub value: f64,
    pub data:  Vec<MeasData>,
    pub cv: f64,
    pub deviation: f64,
}


impl MeasReplicate {
    pub fn calculate(&mut self) {
        let mut sum: f64 = 0.0;
        for data in self.data.as_slice() {
            sum = sum + data.value;
            // println!("VAL:{}",data.value);
        }
        let count = match self.data.len() {
            positive if positive > 0 => positive,
            _ => 1,
        };
        self.value  = sum / count as f64;
        let variance = self.data.iter().map(|data| {
            let diff = self.value - data.value;
            diff * diff
            }).sum::<f64>() / count as f64;
        self.deviation = variance.sqrt();
        self.cv = 100. * self.deviation / self.value; 
    }
    pub fn statistic(&mut self,stat:&Statistic) -> bool {
        let mut need_value = true;
        self.calculate();
        if self.data.len() >= stat.replicates.into() {
            need_value = false; 
        } 
        !need_value 
    }
    pub fn add(&mut self,val:f64 ) -> Result<()> {
        self.data.push(MeasData::from(val));
        self.calculate();
        Ok(())
    }
    pub fn new() -> MeasReplicate {
        MeasReplicate{
            time: 0,
            value: 0.0,
            data:  Vec::new(),
            cv: 0.0,
            deviation:0.0,
        }
    }
}

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Measurement {
    pub stream: u64,
    pub is_tic: bool,
    pub replicat: u8,
    pub tc: MeasReplicate,
    pub tic: MeasReplicate,
    pub toc: MeasReplicate,
    pub tnb: MeasReplicate,
    pub codo: MeasReplicate,

}

impl Measurement {
    pub fn new(stream:u64) -> Measurement {
        Measurement{
        stream:stream,
        is_tic: false,
        replicat:0,
        tc:MeasReplicate::new(),
        tic:MeasReplicate::new(),
        toc:MeasReplicate::new(),
        tnb:MeasReplicate::new(),
        codo:MeasReplicate::new(),
        }
    }
    pub fn next_measurement(&mut self,stream:&Stream) -> Result<bool> {
        let mut need_measurement = false;
        self.is_tic = if self.replicat != 0 && stream.tic.activ && !self.is_tic {
            need_measurement = true;
            true
        } else {
            false
        };
        if ! self.is_tic {
            self.replicat +=1;
        }
        if stream.tc.activ && !self.tc.statistic(&stream.measurement) {
            need_measurement = true;
        } 
        if stream.tic.activ && !self.tic.statistic(&stream.measurement) {
            need_measurement = true;
        }
        if stream.toc.activ && !self.toc.statistic(&stream.measurement) {
            need_measurement = true;
        }
        if stream.tnb.activ && !self.tic.statistic(&stream.measurement) {
            need_measurement = true;
        } 
        if stream.codo.activ && !self.tic.statistic(&stream.measurement) {
            need_measurement = true;
        } 
        Ok(need_measurement)
    }
}



#[derive(Clone, Deserialize, Serialize, PartialEq, Debug)]
pub struct CalPoint {
    pub solution :Solution,
    pub measurement: Measurement,
}

impl CalPoint {
    pub fn new(solution:Solution,measurement: Measurement) -> CalPoint {
        CalPoint{ solution,measurement}
    }
}

#[derive(Clone, Deserialize, Serialize, PartialEq, Debug)]
pub struct Calibration {
    pub stream: Stream,
    pub solutions: Vec<Solution>,
    pub points: Vec<CalPoint>,
}


impl Calibration {
    pub fn new(stream:Stream) -> Calibration {
        let solutions = stream.solution.iter().map(|sol|sol.clone()).collect();
        let points = Vec::new();
        Calibration {stream, solutions, points}
    }
    pub fn add_measurement(&mut self,measurement:Measurement) -> Result<()> {
        match self.solutions.pop() {
            Some(sol) => self.points.push(CalPoint::new(sol,measurement)),
            None => {}
        }
        Ok(())
    }
    pub fn next_point(&mut self) -> Result<bool> {

        Ok(!self.solutions.is_empty())
    }

}