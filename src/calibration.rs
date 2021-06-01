use serde::{Serialize, Deserialize};
use crate::{Stream,Measurement,Solution,Result};

/// Linear
/// An indicator calibration and adjust
///
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Linear {
    pub slope : f64,
    pub intercept: f64,
    pub min: f64,
    pub max: f64,
}

impl Linear {
    pub fn adjust(&self, value:f64) -> f64 {
        value/self.slope - self.intercept
    }
}

/// Polygon calibration

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub struct Polygon {
    points: Vec<Linear>,
}

impl Polygon {
    pub fn adjust(&self, value:f64) -> f64 {
        value/self.points[0].slope - self.points[0].intercept
    }
}

/// Calibration
/// An indicator calibration and adjust
///
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
pub enum  Adjustment{
    None,
    Lineal(Linear),
    Polygon(Polygon),
}

impl Adjustment {
    pub fn adjust(&self, fsr: f64) -> f64 {
        match self {
            Adjustment::None => fsr,
            Adjustment::Lineal(lineal) => lineal.adjust(fsr),
            Adjustment::Polygon(polygon) => polygon.adjust(fsr),
        }
    }
}

impl Linear {
    pub fn new(slope:f64, intercept:f64) -> Linear {
        Self {
            slope : slope,
            intercept : intercept,
            min: 0.0,
            max: 500.0,
        }
    }
}

impl Default for Linear {
    fn default() -> Self {
        Self{
            slope : 1.0,
            intercept : 0.0,
            min: 0.0,
            max: 500.0,
        }
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


// /// Calibration parameter
// #[derive(Clone, Deserialize, Serialize, PartialEq, Debug)]
// pub struct CalibrationConfig {
//     pub interval: Duration,
// /// Statistic  replicates,outliers...usw
//     pub statistic: Statistic,
// ///Füllzeit Kalibrierlösung[s]
//     pub filltime: Duration,
// /// Autocalibration
//     pub autocal: bool,
// /// Autocalibration
//     pub solution: Vec<Solution>,
// }

// impl Default for CalibrationSetting {
//     fn default()-> Self {
//         Self{
//             interval:  Duration::from_secs(14400),
//             statistic: Statistic::default(),
//             filltime:  Duration::from_secs(10),
//             autocal:   false,
//             solution:  vec![
//                 Solution::new(),
//             ]
//         }
//     }
// }

// #[derive(Clone, Deserialize, Serialize, PartialEq, Debug)]
// pub enum CalibrationMutation {
//     Setting(CalibrationSetting),
//     Interval(Duration),
//     Statistic(Statistic),
//     FillTime(Duration),
//     Autocal(bool),
//     Solutions(Vec<Solution>),
//     Solution(usize,Solution),
//     SolutionAdd(Solution),
//     SolutionRemove(usize)
// }
