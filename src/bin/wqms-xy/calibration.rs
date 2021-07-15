use super::measurement;
use tocxy::{Calibration, Result, Stream, mio::XYSystem};
/// Run calibration
/// 
pub fn calibration(xy: &mut XYSystem, stream: &Stream) -> Result<Calibration> {
    let mut calibration = Calibration::new(stream.clone());
    while calibration.next_point()? {
        let measurement = measurement::measurement(xy, stream)?;
        calibration.add_measurement(measurement)?;
    }

    Ok(calibration)
}
pub fn start(xy: &mut XYSystem, stream: &Stream) -> Result<()> {
    let _cal = calibration(xy, stream)?;
    Ok(())
}
