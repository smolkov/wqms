use std::path::PathBuf;

use super::{
    // Humidity,
    Airflow,
    Axis,
    Cooler,
    Fluid,
    Furnace,
    Mio,
    Pump,
    Relay,
    // Pressure,
    Sensor,
    Stirrer,
    TicPort,
    // Vessel,
    Valve,
};
use crate::Result;
use serde::{Deserialize, Serialize};

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct Xy {
    path: PathBuf,
    pub sample_pump: [Pump; 6],
    pub fluid: [Fluid; 6],
    pub condensat_pump: Pump,
    pub humidity_valve: Valve,
    pub tic_valve: Valve,
    pub injection_valve: Valve,
    pub tocdirect_valve: Valve,
    pub relay: Vec<Relay>,
    pub x: Axis,
    pub y: Axis,
    pub inj: Axis,
    pub stirrer: Stirrer,
    pub furnace: Furnace,
    pub ticport: TicPort,
    pub cooler: Cooler,
    // pub humidity: Humidity,
    pub airflow: Airflow,
    // pub pressure: Pressure,
    pub ndir1: Sensor,
    pub ndir2: Sensor,
    pub codo: Sensor,
    pub tnb: Sensor,
}

impl Xy {
    pub fn new(mio: &mut Mio) -> Result<Xy> {
        // let gp1 = Pump::from(mio.create_interface("gp1")?);
        let path = mio.path.to_path_buf();
        let gp1 = Pump::from(mio.create_interface("gp1")?);
        let gp2 = Pump::from(mio.create_interface("gp2")?);
        let gp3 = Pump::from(mio.create_interface("gp3")?);
        let gp4 = Pump::from(mio.create_interface("gp4")?);
        let gp5 = Pump::from(mio.create_interface("gp5")?);
        let gp6 = Pump::from(mio.create_interface("gp6")?);

        let fluid1 = Fluid::from(mio.create_interface("fluid1")?);
        let fluid2 = Fluid::from(mio.create_interface("fluid2")?);
        let fluid3 = Fluid::from(mio.create_interface("fluid3")?);
        let fluid4 = Fluid::from(mio.create_interface("fluid4")?);
        let fluid5 = Fluid::from(mio.create_interface("fluid5")?);
        let fluid6 = Fluid::from(mio.create_interface("fluid6")?);

        let sample_pump = [gp1, gp2, gp3, gp4, gp5, gp6];
        let fluid = [fluid1, fluid2, fluid3, fluid4, fluid5, fluid6];
        let condensat_pump = Pump::from(mio.create_interface("condenz_gp")?);
        let humidity_valve = Valve::from(mio.create_interface("humidity_valve")?);
        let tic_valve = Valve::from(mio.create_interface("tic_valve")?);
        let injection_valve = Valve::from(mio.create_interface("injection_valve")?);
        let tocdirect_valve = Valve::from(mio.create_interface("tocdirect_valve")?);
        let relay = Vec::new();
        let x = Axis::from(mio.create_interface("x")?);
        let y = Axis::from(mio.create_interface("y")?);
        let inj = Axis::from(mio.create_interface("inj")?);

        let stirrer = Stirrer::from(mio.create_interface("stirrer")?);
        let furnace = Furnace::from(mio.create_interface("furnace")?);
        let ticport = TicPort::from(mio.create_interface("ticport")?);
        let cooler = Cooler::from(mio.create_interface("cooler")?);
        let airflow = Airflow::from(mio.create_interface("airflow")?);
        let ndir1 = Sensor::from(mio.create_interface("ndir1")?);
        let ndir2 = Sensor::from(mio.create_interface("ndir2")?);
        let codo = Sensor::from(mio.create_interface("codo")?);
        let tnb = Sensor::from(mio.create_interface("tnb")?);
        Ok(Xy {
            path,
            sample_pump,
            fluid,
            condensat_pump,
            humidity_valve,
            tic_valve,
            injection_valve,
            tocdirect_valve,
            relay,
            x,
            y,
            inj,
            stirrer,
            furnace,
            ticport,
            cooler,
            airflow,
            ndir1,
            ndir2,
            codo,
            tnb,
        })
    }
}
