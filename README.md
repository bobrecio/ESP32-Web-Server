# ESP32-S3 Web Server Starter

A small ESP32-S3 PlatformIO project that turns the board into a tiny web server with:

- LittleFS static file hosting
- API endpoints under `/api/...`
- Basic device dashboard
- Hosted file listing
- Reboot endpoint
- Onboard RGB status LED
- WiFi credentials kept out of Git

## Project Structure

```text
.
├── platformio.ini
├── src/
│   └── main.cpp
├── data/
│   ├── index.html
│   ├── app.js
│   └── style.css
├── include/
│   └── secrets.example.h
└── .github/
    └── workflows/
        └── platformio.yml
```

## Setup

Clone the repo:

```bash
git clone <your-repo-url>
cd esp32-s3-web-server
```

Copy the secrets example:

```bash
cp include/secrets.example.h include/secrets.h
```

Edit `include/secrets.h`:

```cpp
#pragma once

#define WIFI_SSID "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"
```

## Build and Upload

Upload firmware:

```bash
pio run -t upload
```

Upload LittleFS web files:

```bash
pio run -t uploadfs
```

Open serial monitor:

```bash
pio device monitor -b 115200
```

Look for:

```text
Connected. Open: http://192.168.x.x
```

Then open that IP address in your browser.

## API Endpoints

```text
GET /api/info
GET /api/status
GET /api/files
GET /api/color?r=0&g=255&b=0&brightness=25
GET /api/reboot
```

## Notes

The onboard RGB pin is set to GPIO `48`.

If your board does not light the RGB LED, try changing this in `src/main.cpp`:

```cpp
#define RGB_PIN 48
```

Common alternatives are `38` or `18`, depending on the ESP32-S3 board.

## GitHub

This repo includes a GitHub Actions workflow that runs a PlatformIO build on pushes and pull requests.

It does **not** include your real `include/secrets.h`.
