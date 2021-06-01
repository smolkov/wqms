
use serde::{Deserialize, Serialize};
use crate::Result;
use crate::Integration;
use std::time::{Duration,SystemTime};
use crate::brocker;

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Fsr {
    pub value: f64,
}

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub enum SensorModel {
    NDIR1,
    NDIR2,
    CODO,
    TNB,
}

/// Fsr from fsr
impl From<f64> for Fsr {
    fn from(fsr :f64) -> Self {
        Fsr {
            value: fsr,
        }
    }
}
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub enum SensorState{
    STOP,
    MEASUREMENT,
    JUSTIFICATION,
    INTEGRATION,
}


#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Sensor {
    pub name: String,
    pub state:SensorState, 
    pub model:SensorModel, 
    pub activ: bool,
    pub fsr: u32,
    pub area: u32,
    pub lifizero: u32,
    pub broken:bool,
    pub integration: Integration,
    pub integration_end: SystemTime,
    pub integration_start: SystemTime,
    pub justification_end: SystemTime,
    pub justification_start: SystemTime,
}


impl Sensor{
    pub fn new(name: &str,model:SensorModel) -> Sensor {
        Sensor{
            name: name.to_owned(),
            state: SensorState::STOP,
            model: model,
            activ: true,
            fsr: 0,
            area: 0,
            lifizero: 0,
            broken: true,
            integration: Integration::default(),
            integration_start:SystemTime::now(),
            integration_end:SystemTime::now(),
            justification_start:SystemTime::now(),
            justification_end:SystemTime::now(),
        }
    }
    pub fn fsr(&mut self) -> Result<u32> {
        Ok(self.fsr)
    }
    pub fn area(&mut self) -> Result<u32> {
        Ok(self.area)
    }
    pub fn start(&mut self,integration:&Integration ) -> Result<()> {
        brocker::sensor_start(&self.name)?;
        if self.activ {
            self.state = SensorState::MEASUREMENT;
            self.integration = integration.clone();
        }
        Ok(())
    }
    pub fn stop(&mut self) -> Result<()> {
        brocker::sensor_stop(&self.name)?;
        self.state = SensorState::STOP;
        Ok(())
    }
    pub fn justification(&mut self ) -> Result<()> {
        brocker::sensor_justification(&self.name)?;
        if self.is_start()? {
            self.justification_start = SystemTime::now();
            self.justification_end = self.justification_start  + Duration::from_secs(self.integration.justification);
        }
        Ok(())
    }
    pub fn integration(&mut self) -> Result<()> {
        brocker::sensor_integration(&self.name)?;
        if self.is_start()? {
            self.integration_start = SystemTime::now();
        }
        Ok(())
    }
    pub fn is_start(&self) -> Result<bool> {
        Ok(self.state != SensorState::STOP)
    }
    pub fn justification_done(&mut self) -> Result<bool> {
        Ok(SystemTime::now()>self.justification_end)
    }
    pub fn integration_done(&mut self) -> Result<bool> {
        let elapsed = self.integration_start.elapsed()?.as_secs();
        
        let done = if !self.is_start()? || elapsed > self.integration.max_stop {
            self.integration_end =  SystemTime::now();
           true
        } else if elapsed > self.integration.min_stop {
            if self.fsr()? - self.lifizero < self.integration.stop_treshold {
                self.integration_end =  SystemTime::now();
                true
            }else {
                false
            }
        }else {
            false
        };
        Ok(done)
        
    }
    
    // pub fn lifezero(&mut self) -> Result<f64> {
        // self.state = SensorState::JUSTIFICATION;
        // Ok(self.lifezero)
    // }
}
