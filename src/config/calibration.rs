use serde::{Serialize, Deserialize};

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

