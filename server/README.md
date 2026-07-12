# Inference server

Flask server that receives a JPEG image from the ESP32-CAM over HTTP and
returns a single-character waste category code.

## Setup

```bash
cd server
pip install -r requirements.txt
```

Place `waste_classifier.h5` in this folder (see [model/README.md](../model/README.md)
for how to get/regenerate it — it's not committed directly to this repo,
see the note in the main README about model file hosting).

## Run

```bash
python laptop_server_final.py
```

You should see:

```
[SERVER] Model loaded
[SERVER] Confidence threshold : 60%
Running on http://0.0.0.0:5000
```

Find your machine's LAN IP (`ipconfig` on Windows, `ifconfig`/`ip a` on
Mac/Linux) and update `SERVER_URL` in `esp32cam/esp32cam_clean/esp32cam_clean.ino`
to point at `http://<your-ip>:5000/classify`.

## API

### `POST /classify`
Body: raw JPEG bytes.
Response: single character —
`0`=wet, `1`=dry, `2`=plastic, `3`=biomedical, `4`=recyclable, `9`=low confidence (fallback)

### `GET /health`
Returns `OK` if the model loaded successfully, `NO_MODEL` otherwise.
