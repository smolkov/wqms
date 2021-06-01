// use serde::{Deserialize, Serialize};
use crate::Result;
use crate::Stream;
use crate::mio::XySettings;
use crate::Analyser;

thread_local! {
    pub static CON:redis::Connection = redis::Client::open("redis://127.0.0.1/").unwrap().get_connection().unwrap();
}



pub fn axis_to_pos(axis:&str,pos:u32,speed:u8) -> Result<u32> {
    Ok(pos)
}
pub fn axis_to_sensor(axis:&str) -> Result<u32> {
    Ok(0)
}
pub fn axis_set_current(axis:&str,current:u32) -> Result<()> {
    Ok(())
}
pub fn axis_set_max(axis:&str,max:u32) -> Result<()> {
    Ok(())
}
pub fn axis_position(axis:&str) -> Result<u32> {
    Ok(0)
}
pub fn axis_sensor(axis:&str) -> Result<bool> {
    Ok(true)
}

pub fn pump_start(pump:&str,speed:u16) -> Result<()> {
    Ok(())
}

pub fn pump_stop(pump:&str) -> Result<()> {
    Ok(())
}

pub fn valve_open(valve:&str) -> Result<()> {
    Ok(())
}
pub fn valve_close(valve:&str) -> Result<()> {
    Ok(())
}

pub fn airflow_signal() -> Result<f32> {
    Ok(0.0)
}

pub fn airflow_injection() -> Result<f32> {
    Ok(0.0)
}


pub fn relay_open(relay:&str) -> Result<()> {
    Ok(())
}

pub fn relay_close(relay:&str) -> Result<()> {
    Ok(())
}


pub fn stirrer_start(pump:&str,delay:u32,current:u32) -> Result<()> {
    Ok(())
}

pub fn stirrer_stop(pump:&str) -> Result<()> {
    Ok(())
}
pub fn furnace_on() -> Result<()> {
    Ok(())
}
pub fn furnace_off() -> Result<()> {
    Ok(())
}
pub fn furnace_open() -> Result<()> {
    Ok(())
}
pub fn furnace_close() -> Result<()> {
    Ok(())
}

pub fn ticport_open() -> Result<()> {
    Ok(())
}
pub fn ticport_close() -> Result<()> {
    Ok(())
}

pub fn sensor_start(sensor:&str) -> Result<()> {
    Ok(())
}

pub fn sensor_stop(sensor:&str) -> Result<()> {
    Ok(())
}

pub fn sensor_justification(sensor:&str) -> Result<()> {
    Ok(())
}
pub fn sensor_wait(sensor:&str) -> Result<()> {
    Ok(())
}
pub fn sensor_integration(sensor:&str) -> Result<()> {
    Ok(())
}
pub fn sensor_fsr(sensor:&str) -> Result<f32> {
    Ok(0.0)
}



pub fn stream_read(stream:u8) -> Result<Stream> {
    Ok(Stream::default())
}

pub fn mio_settings() -> Result<XySettings> {
    Ok(XySettings::default())
}

pub fn anayser_read() -> Result<Analyser> {

    Ok(Analyser::new())
}