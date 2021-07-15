use super::{
    Airflow, Cooler, Fluid, Furnace, Pump,Sensor, Stirrer,Mio,
    TicPort, Valve
};
use serde::{Deserialize, Serialize};
use crate::Result;

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct Loop {
    pub humidity_valve: Valve,
    pub furnace: Furnace,
    pub ticport: TicPort,
    pub injection_block: Valve,
    pub tic_valve: Valve,
    /// Gaskalibrierung Injection Ventil
    pub y4y5: Valve,
    /// Gaskalibrierung outlet Ventil
    pub y4y6: Valve,
    /// Probestrom Ventil
    pub sample_valve: [Valve; 6],
    /// Control ventil
    pub y3y9: Valve,
    /// Einseuerung
    pub stripping_valve: [Valve; 6],
    /// Einseuerung
    pub y5y9: Valve,
    ///TOC strip GAZ
    pub y6y1: Valve,
    pub condensat_pump: Pump,
    pub pump: [Pump; 6],
    pub fluid: [Fluid; 6],
    pub gp10: Pump,
    pub stirrer: Stirrer,
    pub cooler: Cooler,
    pub airflow: Airflow,
    pub ndir1: Sensor,
    pub ndir2: Sensor,
    pub codo: Sensor,
    pub tnb: Sensor,
}

impl Loop {
    pub fn new(mio:&mut Mio) -> Result<Loop> {
        // let gp1 = Pump::from(mio.create_interface("gp1")?);
        let humidity_valve = Valve::from(mio.create_interface("humidity_valve")?);
        let furnace = Furnace::from(mio.create_interface("furnace")?);
        let ticport = TicPort::from(mio.create_interface("ticport")?);
        let injection_block = Valve::from(mio.create_interface("injection_block")?);
        let tic_valve = Valve::from(mio.create_interface("tic_valve")?);

        let y4y5 = Valve::from(mio.create_interface("y4y5")?);
        let y4y6 = Valve::from(mio.create_interface("y4y6")?);
        let sample_valve = [ Valve::from(mio.create_interface("y3y1")?), Valve::from(mio.create_interface("y3y2")?), Valve::from(mio.create_interface("y3y3")?), Valve::from(mio.create_interface("y3y4")?), Valve::from(mio.create_interface("y3y5")?), Valve::from(mio.create_interface("y3y6")?)];
        let y3y9 = Valve::from(mio.create_interface("y3y9")?);
        let stripping_valve = [ Valve::from(mio.create_interface("y5y1")?), Valve::from(mio.create_interface("y5y2")?), Valve::from(mio.create_interface("y5y3")?), Valve::from(mio.create_interface("y5y4")?), Valve::from(mio.create_interface("y5y5")?), Valve::from(mio.create_interface("y5y6")?)];

        let y5y9 =  Valve::from(mio.create_interface("y5y9")?); 
        let y6y1 =  Valve::from(mio.create_interface("y6y1")?); 
        let condensat_pump = Pump::from(mio.create_interface("condenz_gp")?);
        let pump = [ Pump::from(mio.create_interface("gp1")?), Pump::from(mio.create_interface("gp2")?), Pump::from(mio.create_interface("gp3")?), Pump::from(mio.create_interface("gp4")?), Pump::from(mio.create_interface("gp5")?), Pump::from(mio.create_interface("gp6")?)];
        let fluid = [Fluid::from(mio.create_interface("fluid1")?),Fluid::from(mio.create_interface("fluid2")?),Fluid::from(mio.create_interface("fluid3")?),Fluid::from(mio.create_interface("fluid4")?),Fluid::from(mio.create_interface("fluid5")?),Fluid::from(mio.create_interface("fluid6")?)];
        let gp10 = Pump::from(mio.create_interface("gp10")?);

        let stirrer = Stirrer::from(mio.create_interface("stirrer")?);
        let cooler = Cooler::from(mio.create_interface("cooler")?);
        let airflow = Airflow::from(mio.create_interface("airflow")?);
        let ndir1 = Sensor::from(mio.create_interface("ndir1")?);
        let ndir2 = Sensor::from(mio.create_interface("ndir2")?);
        let codo= Sensor::from(mio.create_interface("codo")?);
        let tnb = Sensor::from(mio.create_interface("tnb")?);
        Ok(Loop{
            humidity_valve,furnace,ticport,injection_block,tic_valve,y4y5,y4y6,sample_valve,y3y9,stripping_valve,y5y9,y6y1,condensat_pump,pump,fluid,gp10,stirrer,cooler,airflow,ndir1,ndir2,codo,tnb
        })
    }
}