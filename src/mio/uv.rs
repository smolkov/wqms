///# `QuickTOCuv`  MIO - hardware,simulation
/// 
///   * Ultra
///   * Nitritox
///   * Loop
///   * Biomonitor
/// 
// use std::prelude::*;
// use std::io;
use std::path::PathBuf;
use std::convert::TryFrom;
// use std::stream::Stream;
use super::{
    Pump,
    Valve,
    Lamp,
    Sensor,
    Mio,
};
use serde::{Deserialize, Serialize};
use crate::Result;
// use std::stream;
// use std::time::Duration;
// use std::prelude::*;

// pub const VELOCITY: &'static str = "velocity";
// pub const POSITION: &'static str = "position";
// pub const HOLD: &'static str = "hold";
// pub const MAX: &'static str = "max";
// pub const ENDSCHALTER: &'static str = "endschalter";
// pub const PTP: &'static str = "ptp";
// pub const COMMAND: &'static str = "command";
// pub const GOTO: &'static str = "goto";


#[derive(Clone,Deserialize, Serialize, Debug)]
pub struct Uv {
    path : PathBuf,
    pub lamp:   Lamp,
    pub sampl:  Vec<Valve>,
    pub cal:    Valve,
    pub tic:    Valve,
    pub bypass: Valve,
    pub gp:     Pump,
    pub ndir:   Vec<Sensor>,
}



impl Uv {

    pub fn new(mio:&mut Mio) -> Result<Uv> {
        let path = mio.path.to_path_buf();
        let lamp         = Lamp::from(mio.create_interface("lamp")?);
        let sv1    = Valve::from(mio.create_interface("valve1")?);
        let sv2    = Valve::from(mio.create_interface("valve2")?);
        let sv3    = Valve::from(mio.create_interface("valve3")?);
        let sv4    = Valve::from(mio.create_interface("valve4")?);
        let sv5    = Valve::from(mio.create_interface("valve5")?);
        let sv6    = Valve::from(mio.create_interface("valve6")?);
        let cal    = Valve::from(mio.create_interface("cal")?);
        let tic    = Valve::from(mio.create_interface("tic")?);
        let bypass = Valve::from(mio.create_interface("bypas")?);
        let gp      = Pump::try_from(mio.create_interface("gp")?)?;
        let ndir1  = Sensor::from(mio.create_interface("ndir1")?);
        let ndir2  = Sensor::from(mio.create_interface("ndir2")?);
        let sampl  = vec![sv1, sv2 ,sv3, sv4 ,sv5, sv6];
        let ndir   = vec![ndir1,ndir2];
        Ok(Uv{path,lamp,sampl,cal,tic,bypass,gp,ndir})
    }
    
}