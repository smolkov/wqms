// use std::fs;
// use std::io;
// use std::io;
use std::path::PathBuf;
use structopt::StructOpt;

/// ✇ init
#[derive(Debug, StructOpt)]
pub struct Init {
    /// 🔧 replace
    #[structopt(name = "replace", long = "replace")]
    replace: bool,
    /// 🔧 git
    #[structopt(name = "git", long = "git")]
    git: bool,
}
/// Init
impl Init {
    /// is replace on setup
    #[inline]
    pub fn replace(&self) -> bool {
        self.replace
    }
    /// is git setup
    #[inline]
    pub fn git(&self) -> bool {
        self.git
    }
}

/// ✇ set prop signal
#[derive(Debug, StructOpt)]
pub struct SetProp {
    //⏱ interval in seconds
    #[structopt(short = "d", long = "name")]
    name: PathBuf,
    ///🔌 hardware connection address
    #[structopt(
        short = "a",
        long = "address",
        default_value = "tcp:host=192.168.66.59,port=6666"
    )]
    value: String,
}

/// ✇ get property value
#[derive(Debug, StructOpt)]
pub struct GetProp {
    /// 🔧 name
    #[structopt(short = "d", long = "name")]
    name: PathBuf,
}
/// ✇ list show workspace
#[derive(Debug, StructOpt)]
pub struct List {}

///📢 Commands
#[derive(StructOpt, Debug)]
pub enum Cmd {
    /// ✇ Hold
    Hold,
    /// ✇ Measurement
    Measurement,
    /// ✇ Calibration
    Calibration,
    /// Online
    Online,
}

///automata command argument
#[derive(Debug, StructOpt)]
#[structopt(name = "tocxy", about = "🧰tocxy console interface usage.")]
pub struct Args {
    /// Verbose mode (-v, -vv, -vvv)
    #[structopt(short, long, parse(from_occurrences))]
    pub verbose: u8,
    ///📢 commands
    #[structopt(subcommand, about = "📢automata commands list")]
    pub cmd: Cmd,
}