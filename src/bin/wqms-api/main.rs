use tera::Tera;
use tide::prelude::*;
use tide_tera::prelude::*;
mod pages;
mod state;
mod handles;

use state::State;

pub type Request = tide::Request<State>;

#[async_std::main]
async fn main() -> tide::Result<()> {
    tide::log::start();
    let client = redis::Client::open("redis://127.0.0.1/").unwrap();
    let mut tera = Tera::new("www/templates/**/*")?;
    tera.autoescape_on(vec!["html"]);
         
    let mut app = tide::with_state(State{client,tera});
    let secret = "pdxeaSj/Pfzn07tl8QvJVnVeHIFeAcslGgDTLG3iCC1KcapbE8aSirn8l6nryOqs
    pX5rJ6f/IbVDK6eGxEygXQ==";
    app.with(tide::sessions::SessionMiddleware::new( tide::sessions::MemoryStore::new(), secret.as_bytes(),));

   // Redirect hackers to YouTube.
    app.at("/.env").get(tide::Redirect::new("https://www.youtube.com/watch?v=dQw4w9WgXcQ"));

    // app.at("/").get(tide::Redirect::new("/welcome"));
    // Views
    app.at("/").get(pages::index);
    app.at("/wifi").get(pages::wifi::index);

    app.at("/public").serve_dir("www/public/")?;
    app.listen("127.0.0.1:8000").await?;
    Ok(())
}