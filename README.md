<h1>Sentio: Mood-Based Home Automation and Journaling</h1>

<p>
Sentio is an ESP32-based IoT prototype that detects a user’s mood from physiological signals
(HRV, GSR, temperature, activity) and adapts the environment with ambient lights and servo-driven
devices (diffuser, fan) while journaling mood trends for wellbeing insights.
</p>

<hr />

<h2>Description</h2>
<p>
This project fuses multiple sensors—two potentiometers (simulated HRV and GSR), a DS18B20 digital
temperature sensor, and an MPU6050 accelerometer—with simple threshold logic to classify emotions:
Happy, Sad, Angry, Fear, Surprise, Disgust, Neutral. The detected mood appears on an I2C LCD and
drives ambient LEDs and two servos that simulate an aroma diffuser and a fan. A push button triggers
each detection, and a lightweight journal counts emotions across sessions for quick summaries.
</p>

<h2>Features</h2>
<ul>
  <li>Multi-sensor emotion detection (HRV, GSR, temperature, activity).</li>
  <li>Mood-based automation: warm/cool/fresh lighting and servo actions (diffuser, fan).</li>
  <li>On-device journaling with periodic summaries in the serial console.</li>
  <li>One-button operation with real-time LCD feedback.</li>
  <li>Works on real hardware or in Wokwi simulation.</li>
</ul>

<h2>Hardware</h2>
<ul>
  <li><strong>Controller:</strong> ESP32 DevKit C</li>
  <li><strong>Sensors:</strong> 2× Potentiometers (HRV, GSR), DS18B20 (Temp, 1‑Wire), MPU6050 (Activity, I2C)</li>
  <li><strong>Outputs:</strong> 7 emotion LEDs, 3 ambient LEDs, 2× Servos (diffuser, fan), Push Button</li>
  <li><strong>Display:</strong> 16×2 I2C LCD (address 0x27)</li>
</ul>

<h2>Getting Started</h2>
<ol>
  <li>Install the Arduino IDE and the ESP32 board support package.</li>
  <li>Add libraries: <em>ESP32Servo</em>, <em>OneWire</em>, <em>DallasTemperature</em>, <em>MPU6050</em> (or compatible), <em>LiquidCrystal_I2C</em>.</li>
  <li>Wire the circuit:
    <ul>
      <li>I2C (LCD &amp; MPU6050): SDA=GPIO21, SCL=GPIO22</li>
      <li>DS18B20 (1‑Wire): DQ=GPIO32</li>
      <li>Potentiometers (HRV=GPIO34, GSR=GPIO35)</li>
      <li>Servos: Diffuser=GPIO17, Fan=GPIO18</li>
      <li>Emotion LEDs: 12, 13, 14, 27, 26, 25, 23; Ambient LEDs: 2, 15, 16; Button: 4</li>
    </ul>
  </li>
  <li>Open the sketch, select the correct ESP32 board and port, and upload.</li>
  <li>Open Serial Monitor at 115200, adjust potentiometers/move MPU6050, then press the button to detect mood.</li>
</ol>

<h2>How It Works</h2>
<ol>
  <li><strong>Read sensors:</strong> Analog (HRV, GSR), DS18B20 via 1‑Wire, MPU6050 via I2C.</li>
  <li><strong>Classify mood:</strong> Compare values to per‑emotion thresholds and pick the best match.</li>
  <li><strong>Actuate:</strong> Set emotion LED, drive ambient lighting; run diffuser or fan servos as needed.</li>
  <li><strong>Display &amp; log:</strong> Show mood and key readings on LCD; append counts to the journal and print to Serial.</li>
  <li><strong>Summarize:</strong> Every few detections, print a concise journal summary to the Serial console.</li>
</ol>
