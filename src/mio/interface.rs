/// Interface information 
/// 
use std::fs;
// use sysfs_gpio::{Direction,Pin};
use std::path::{ Path,PathBuf};
use std::fmt;

// use async_trait::async_trait;
use super::*;

pub const LABEL: &'static str       = "label";
pub const UNIT: &'static str        = "unit";
pub const DESCRIPTION: &'static str = "description";
pub const MODEL: &'static str       = "model";
pub const ERROR: &'static str       = "error";
pub const TYPE: &'static str        = "type";
pub const CLASS: &'static str       = "class";
pub const SUBSYSTEM: &'static str       = "class";

// pub const SIMULATE: &'static str = "simulate";
use std::collections::BTreeSet;
use serde::{Serialize,Deserialize};

pub enum IType {
    DigOUT,
    DigIN,
    ADC,
    UART,
    Relay,
    Valve,
    Fluid,
    Lamp,
    GearPump,
    Furnace,
    Temperatur,
    Humidity,
    Airflow,
    Pressure,
    NDir,
    Zirox,
    NO5,
    Axis,
    Injection,
    Autosampler,
    Stirrer,
    Cooler,
    Unknown
}


// impl From<u8> for IType {
//     fn from(value: u8) -> Self {
//         match value {
//             1 => IType::DigIN,
//             2 => IType::DigOUT,
//             3 => IType::ADC,
//             4 => IType::UART,
//             5 => IType::Relay,
//             6 => IType::Valve,
//             7 => IType::Fluid,
//             8 => IType::Lamp,
//             9 => IType::GearPump,
//            10 => IType::ImpulsPump,
//            11 => IType::Furnace,
//            12 => IType::Airflow,
//            13 => IType::Pressure,
//            14 => IType::Temperatur,
//            15 => IType::Humidity,
//            21 => IType::NDir,
//            22 => IType::Zirox,
//            23 => IType::NO5,
//            41 => IType::Axis,
//            42 => IType::Injection,
//            51 => IType::Stirrer,
//            110 => IType::Autosampler,
//            _ =>   IType::Unknown
//         }
//     }
// }

impl From<IType> for u8 {
    fn from(hid:IType) -> u8 {
        hid.into()
    }
}

impl From<&str> for IType {
    fn from(value: &str) -> Self {
        match value {
            "digin"           => IType::DigIN,
            "digout"          => IType::DigOUT,
            "adc"             => IType::ADC,
            "uart"            => IType::UART,
            "relay"           => IType::Relay,
            "valve"           => IType::Valve,
            "fluid"           => IType::Fluid,
            "lamp"            => IType::Lamp,
            "pump"            => IType::GearPump,
            "furnace"         => IType::Furnace,
            "airflow"         => IType::Airflow,
            "pressure"        => IType::Pressure,
            "temperatur"      => IType::Temperatur,
            "humidity"        => IType::Humidity,
            "ndir"            => IType::NDir,
            "zirox"           => IType::Zirox,
            "no5"             => IType::NO5,
            "axis"            => IType::Axis,
            "injection"       => IType::Injection,
            "stirrer"         => IType::Stirrer,
            "autosampler"     => IType::Autosampler,
            "cooler"          => IType::Cooler,
            _                 => IType::Unknown,
        }
    }
}

impl From<String> for IType {
    fn from(value: String) -> Self {
        IType::from(value.as_str())
    }
}

impl fmt::Display for IType {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            IType::DigIN      => return write!(f,"digin"),          
            IType::DigOUT     => return write!(f,"digout"),           
            IType::ADC        => return write!(f,"adc"),    
            IType::UART       => return write!(f,"uart"),     
            IType::Relay      => return write!(f,"relay"),      
            IType::Valve      => return write!(f,"valve"),      
            IType::Fluid      => return write!(f,"fluid"),      
            IType::Lamp       => return write!(f,"lamp"),     
            IType::GearPump   => return write!(f,"pump"),         
            IType::Furnace    => return write!(f,"furnace"),        
            IType::Airflow    => return write!(f,"airflow"),        
            IType::Pressure   => return write!(f,"pressure"),         
            IType::Temperatur => return write!(f,"temperatur"),           
            IType::Humidity   => return write!(f,"humidity"),         
            IType::NDir       => return write!(f,"ndir"),     
            IType::Zirox      => return write!(f,"zirox"),      
            IType::NO5        => return write!(f,"no5"),    
            IType::Axis       => return write!(f,"axis"),     
            IType::Injection  => return write!(f,"injection"),          
            IType::Stirrer    => return write!(f,"stirrer"),        
            IType::Autosampler=> return write!(f,"autosamler"),         
            IType::Cooler     => return write!(f,"cooler"),         
            IType::Unknown    => return write!(f,"unknown"),        
        }
    }
}
/// iclass 
pub enum IClass {
    Unclassified,
    DigOUT,
    DigIN,
    AOut,
    Sensor,
    Valve,
    Pump,
}
impl From<&str> for IClass {
    fn from(value: &str) -> Self {
        match value {
            "unclassified"    => IClass::Unclassified,
            "digout"          => IClass::DigOUT,
            "digin"           => IClass::DigIN,
            "analogout"       => IClass::AOut,
            "sensor"          => IClass::Sensor,
            "valve"           => IClass::Valve,
            "pump"            => IClass::Pump,
            _                 => IClass::Unclassified,
        }
    }
}

impl From<String> for IClass {
    fn from(value: String) -> Self {
        IClass::from(value.as_str())
    }
}

impl fmt::Display for IClass {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            IClass::Unclassified => return write!(f,"unclassified"),          
            IClass::DigOUT       => return write!(f,"digout"),           
            IClass::DigIN        => return write!(f,"digin"),    
            IClass::AOut         => return write!(f,"analogout"),     
            IClass::Sensor       => return write!(f,"sensor"),      
            IClass::Valve        => return write!(f,"valve"),      
            IClass::Pump         => return write!(f,"pump"),      
        }
    }
}


/// mio interface 
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Interface {
    pub path: PathBuf,
}

impl From<&Path> for Interface {
    fn from(path:&Path) -> Interface {
        Interface{path:path.to_path_buf()}
    }
}
impl From<PathBuf> for Interface {
    fn from(path:PathBuf) -> Interface {
        Interface{path:path.to_path_buf()}
    }
}
impl From<Interface> for PathBuf {
    fn from(device:Interface) -> PathBuf {
        device.path
    }
}
impl AsRef<Interface> for Interface {
    fn as_ref(&self) -> &Interface {
        self
    }
}
/// mio interface 
impl Interface {
    pub fn create(path:&Path ) -> Result<Interface> {
        if !path.is_dir() {
            fs::create_dir(path)?;
            fs::write(path.join("enabled"), b"0")?;
            //  🗷 - unchecked 
            //  🗹 - checked 
            fs::write(path.join("check"), format!("🗹").as_bytes())?;
            fs::write(path.join(TYPE), format!("{}",IType::Unknown).as_bytes())?;
            fs::write(path.join(CLASS), format!("{}",IClass::Unclassified).as_bytes())?;
        }
        Ok(Interface{path:path.to_path_buf()})
    }
    // pub fn driver(&self,pid:u32) -> Result<Driver> {
        // let drv = Driver::open(self.path.as_path(),pid)?;
        // Ok(drv)
    // }
    pub fn label(&self) -> Result<String> {
        let label = fs::read_to_string(self.path.join(LABEL))?;
        Ok(label)
    }
    pub fn unit(&self) -> Result<String> {
        let unit= fs::read_to_string(self.path.join(UNIT))?;
        Ok(unit)
    }
    pub fn description(&self)-> Result<String> {
        let desc= fs::read_to_string(self.path.join(DESCRIPTION))?;
        Ok(desc) 
    }
    pub fn model(&self) -> Result<String> {
        let unit= fs::read_to_string(self.path.join(MODEL))?;
        Ok(unit)
    }
    pub fn device_type(&self) -> Result<IType> {
        let hid = fs::read_to_string(self.path.join(TYPE))?;
        Ok(hid.into())
    }
    pub fn enabled(&self) -> Result<bool> {
        match fs::read_to_string(self.path.join("enabled"))?.as_str() {
            "1" => Ok(true),
            _ => Ok(false),
        }
    }

    // pub fn driver(&self) -> Result<bool> {
        // match fs::read_to_string(self.path.join("driver"))?.as_str() {
            // "1" => Ok(true),
            // _ => Ok(false),
        // }
    // }
    pub fn is_error(&self) -> Result<bool> {
        if self.path.join(ERROR).is_file() {
            Ok(true)
        }else {
            Ok(false)
        }
    }
    pub fn error(&self) -> Result<String> {
        let error = fs::read_to_string(self.path.join(ERROR))?;
        Ok(error)
    }    
    pub fn set_itype(&self,itype:IType) -> Result<()> {
        fs::write(self.path.join(TYPE), format!("{}",itype))?;
        Ok(())
    }
    pub fn set_iclass(&mut self,iclass:IClass) -> Result<()> {
        fs::write(self.path.join(CLASS), format!("{}",iclass))?;
        let mut p = self.path.to_path_buf();
        if p.pop() && p.pop() && p.is_dir() {
            let class = p.join("class").join(format!("{}",iclass)).join(self.path.as_path().file_name().unwrap());
            fs::hard_link(self.path.as_path(), &class)?; 
        }
        Ok(())
    }
}

fn bool_true() -> bool {
    true
}


#[derive(Debug, Clone, PartialEq,Serialize, Deserialize)]
pub struct IfaceConfig {
    pub hid: u64,
    #[serde(default)]
    pub names: BTreeSet<String>,
    #[serde(default = "bool_true")]
    pub export: bool,
    pub user: Option<String>,
    pub group: Option<String>,
    pub mode: Option<u32>,
}
