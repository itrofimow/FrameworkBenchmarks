use axum::{
    http::{header, HeaderMap, HeaderName, HeaderValue, StatusCode},
    response::IntoResponse,
    routing::get,
    Json, Router,
};
use dotenv::dotenv;
use tower_http::set_header::SetResponseHeaderLayer;

mod models_common;
mod server;

use self::models_common::Message;

const CUSTOM_HEADERS: [HeaderName; 3] = [
    HeaderName::from_static("x-my-first-custom-header"),
    HeaderName::from_static("x-my-second-custom-header"),
    HeaderName::from_static("x-my-third-custom-header"),
];

const RESPONSES: [&str; 4] = [
    "Hello, World",
    "Hello, World1",
    "Hello, World2",
    "Hello, World3",
];

pub async fn plaintext(headers: HeaderMap) -> &'static str {
    let mut count = 0;
    for hdr in CUSTOM_HEADERS {
        if headers.contains_key(hdr) {
            count += 1;
        }
    }

    RESPONSES[count]
}

pub async fn json() -> impl IntoResponse {
    let message = Message {
        message: "Hello, World!",
    };

    (StatusCode::OK, Json(message))
}

//#[tokio::main]
#[tokio::main(flavor = "current_thread")]
async fn main() {
    dotenv().ok();

    let server_header_value = HeaderValue::from_static("Axum");

    let app = Router::new()
        .route("/plaintext", get(plaintext))
        .route("/json", get(json))
        .layer(SetResponseHeaderLayer::if_not_present(
            header::SERVER,
            server_header_value,
        ));

    server::builder()
        .http1_pipeline_flush(true)
        .serve(app.into_make_service())
        .await
        .unwrap();
}
