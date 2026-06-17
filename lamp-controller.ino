#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// --- KONFIGURASI WIFI ---
const char* ssid     = "MyDevice";
const char* password = "87654321";

// --- KONFIGURASI 3 CHANNEL PIN LED ---
const int LED_PINS[] = {2, 3, 4}; // GPIO untuk Channel 1, Channel 2, Channel 3
const int NUM_CHANNELS = 3;

// Struktur data status untuk setiap channel
struct LedChannel {
  bool state;
  int brightness;
};

LedChannel leds[NUM_CHANNELS] = {
  {false, 128}, // Channel 1
  {false, 128}, // Channel 2
  {false, 128}  // Channel 3
};

WebServer server(80);

// --- HALAMAN WEB DENGAN DESIGN MODERN PURIST (TOSKA NEON) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NEO-CTRL // MATRIX</title>
    <style>
        :root {
            --bg-color: #0a0f1d;
            --panel-bg: #111827;
            --primary-neon: #00f2fe;
            --primary-glow: rgba(0, 242, 254, 0.3);
            --text-color: #e5e7eb;
            --text-muted: #6b7280;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, sans-serif; }
        body { background-color: var(--bg-color); color: var(--text-color); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }
        .container { width: 100%; max-width: 450px; background: var(--panel-bg); border: 1px solid rgba(0, 242, 254, 0.15); border-radius: 16px; padding: 30px; box-shadow: 0 20px 40px rgba(0,0,0,0.5); }
        header { text-align: center; margin-bottom: 30px; border-bottom: 1px solid #1f293d; padding-bottom: 15px; }
        header h1 { font-size: 1.4rem; color: #ffffff; text-shadow: 0 0 10px var(--primary-glow); letter-spacing: 2px; }
        header p { font-size: 0.7rem; color: var(--text-muted); letter-spacing: 3px; margin-top: 5px; }
        
        .channels-container { display: flex; flex-direction: column; gap: 25px; }
        .channel-card { background: rgba(255,255,255,0.02); border: 1px solid #1f293d; padding: 20px; border-radius: 12px; display: flex; flex-direction: column; gap: 15px; }
        .channel-title { font-size: 0.85rem; font-weight: bold; text-transform: uppercase; color: #ffffff; letter-spacing: 1px; }
        
        /* Button */
        .btn-neon { background: transparent; border: 1px solid var(--primary-neon); color: #ffffff; padding: 12px; font-size: 0.9rem; font-weight: 600; border-radius: 6px; cursor: pointer; transition: all 0.3s; text-transform: uppercase; width: 100%; }
        .btn-neon:hover { background: rgba(0, 242, 254, 0.1); box-shadow: 0 0 15px var(--primary-glow); }
        .btn-neon.active { background: var(--primary-neon); color: var(--bg-color); box-shadow: 0 0 20px var(--primary-neon); font-weight: 700; }
        
        /* Slider */
        .slider-group { display: flex; flex-direction: column; gap: 5px; }
        .slider { -webkit-appearance: none; width: 100%; height: 5px; border-radius: 3px; background: #1f293d; outline: none; }
        .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 16px; height: 16px; border-radius: 50%; background: var(--primary-neon); cursor: pointer; box-shadow: 0 0 8px var(--primary-neon); }
        .value-display { font-family: monospace; font-size: 0.9rem; color: var(--primary-neon); text-align: right; }
    </style>
</head>
<body>
<div class="container">
    <header>
        <h1>NEO-MATRIX CONTROL</h1>
        <p>MAIN MULTI-CHANNEL BUS</p>
    </header>
    
    <div class="channels-container">
        <div id="led-nodes"></div>
    </div>
</div>

<script>
    const TOTAL_CHANNELS = 3;
    let localState = [];

    // Loop otomatis untuk menggambar UI secara modular (Channel 1 sampai 3)
    const nodesContainer = document.getElementById('led-nodes');
    for (let i = 0; i < TOTAL_CHANNELS; i++) {
        nodesContainer.innerHTML += `
            <div class="channel-card">
                <div class="channel-title">CHANNEL 0${i+1}</div>
                <button id="btn-${i}" class="btn-neon" onclick="togglePower(${i})">CONNECTING...</button>
                <div class="slider-group">
                    <input type="range" id="slide-${i}" class="slider" min="0" max="255" oninput="updateDisplayVal(${i}, this.value)" onchange="sendDimmer(${i}, this.value)">
                    <div class="value-display" id="val-${i}">0</div>
                </div>
            </div>
        `;
    }

    // Ambil sinkronisasi data awal dari alat
    document.addEventListener("DOMContentLoaded", () => {
        fetch('/status')
        .then(res => res.json())
        .then(data => {
            localState = data;
            for(let i=0; i<TOTAL_CHANNELS; i++) {
                document.getElementById(`slide-${i}`).value = localState[i].brightness;
                document.getElementById(`val-${i}`).innerText = localState[i].brightness;
                updateButtonUI(i, localState[i].state);
            }
        });
    });

    function updateButtonUI(ch, state) {
        const btn = document.getElementById(`btn-${ch}`);
        if(state) {
            btn.innerText = `CH 0${ch+1} // ACTIVE`;
            btn.classList.add('active');
        } else {
            btn.innerText = `CH 0${ch+1} // STANDBY`;
            btn.classList.remove('active');
        }
    }

    function updateDisplayVal(ch, val) {
        document.getElementById(`val-${ch}`).innerText = val;
    }

    function sendData(ch, param, value) {
        fetch(`/update?ch=${ch}&${param}=${value}`).catch(err => console.error(err));
    }

    function togglePower(ch) {
        localState[ch].state = !localState[ch].state;
        updateButtonUI(ch, localState[ch].state);
        sendData(ch, 'state', localState[ch].state ? 1 : 0);
    }

    function sendDimmer(ch, val) {
        localState[ch].brightness = parseInt(val);
        sendData(ch, 'val', val);
    }
</script>
</body>
</html>
)rawliteral";

// --- WEB SERVER ROUTE HANDLERS ---

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleStatus() {
  String json = "[";
  for (int i = 0; i < NUM_CHANNELS; i++) {
    json += "{\"state\":" + String(leds[i].state ? "true" : "false") + ",\"brightness\":" + String(leds[i].brightness) + "}";
    if (i < NUM_CHANNELS - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleUpdate() {
  if (server.hasArg("ch")) {
    int ch = server.arg("ch").toInt();
    
    if (ch >= 0 && ch < NUM_CHANNELS) {
      if (server.hasArg("state")) {
        leds[ch].state = server.arg("state").toInt();
      }
      if (server.hasArg("val")) {
        leds[ch].brightness = server.arg("val").toInt();
      }

      // Eksekusi output fisik berdasarkan index channel array
      if (leds[ch].state) {
        analogWrite(LED_PINS[ch], leds[ch].brightness);
      } else {
        analogWrite(LED_PINS[ch], 0);
      }
    }
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  
  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    analogWrite(LED_PINS[i], 0); 
  }


  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/update", handleUpdate);

  server.begin();
}

void loop() {
  server.handleClient();
  delay(2); 
}