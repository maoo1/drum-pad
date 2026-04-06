# Lion Dance Drum: Interactive ESP32 Digital Instrument

A quiet, portable, and responsive electronic Lion Dance drum designed for practice in noise-sensitive environments. This project uses the ESP32's internal capacitive touch sensors to detect strikes on a cardboard drum and triggers high-fidelity audio samples on a laptop via UDP over Wi-Fi.

## Features
*   **5-Zone Sensing:** Two center pads (for mute logic), two rim pads, and one mode-switch pad.
*   **Mute Logic:** Imitates traditional drumming by triggering a "muted" sound when one center pad is held while the other is struck.
*   **Dual Mode:** Toggle between "Drum" and "Cymbal" sound kits.
*   **Adaptive Baseline Algorithm:** Automatically adjusts touch thresholds to account for environmental noise and grounding shifts.
---

## Hardware Requirements
*   **Microcontroller:** ESP32 (e.g., DOIT DevKit V1).
*   **Sensors:** Copper foil tape.
*   **Structure:** Cardboard box (roughly drum-shaped).
*   **Wiring:** 22AWG jumper wires and soldering iron.
*   **Connection:** A micro-USB cable (used for power and as a common ground plane).

### Pin Mapping (Touch Sensors)
| Pad Position | ESP32 Pin |
| :--- | :--- |
| Center Pad A | GPIO 13 (T4) |
| Center Pad B | GPIO 15 (T3) |
| Rim Pad A | GPIO 12 (T5) |
| Rim Pad B | GPIO 33 (T8) |
| Mode Toggle | GPIO 27 (T7) |

---

## Software Requirements
1.  **Arduino IDE** or **PlatformIO**.
2.  **Python 3.x** installed on your laptop.
3.  Python libraries: `socket` (standard library).

---

## Installation & Setup

### 1. Hardware Assembly
1.  Cut five pieces of copper tape.
2.  Apply them to your cardboard structure. For the most authentic feel, place the "Rim" pads on the curved sides of the box.
3.  Divide the center area into two distinct copper zones (Pad A and Pad B) to allow for the muting gesture.
4.  Solder jumper wires from the copper tape to the corresponding ESP32 GPIO pins.
5.  **Note on Grounding:** Keep the ESP32 plugged into your laptop via USB. The cable acts as a ground plane, which is necessary for stable capacitive readings.

### 2. ESP32 Firmware
1.  Open the provided firmware file in your editor.
2.  Update the `laptopIP` variable to match your laptop's current IP address on the Wi-Fi network.
3.  Upload the code to your ESP32.
4.  Open the Serial Monitor (115200 baud) to ensure the device is connecting to the Wi-Fi Access Point and showing raw sensor values.

### 3. Laptop Receiver (Python)
1.  Place your audio samples (`.wav` format) in the same directory as the Python script. Feel free to use the samples in the github repo, as they're from a real lion dance drum.
2.  Ensure the filenames in the Python script match your recordings (e.g., `center.wav`, `rim.wav`, `mute.wav`).
3.  Run the script:
    ```bash
    python listen.py
    ```

---

## Usage
1.  **Center Hit:** Strike either Pad A or Pad B.
2.  **Mute Hit:** Hold Pad A with one hand and strike Pad B with the other (or vice-versa).
3.  **Rim Hit:** Strike the side pads.
4.  **Change Instrument:** Tap the Mode pad to toggle between the Drum kit and the Cymbal kit.

---

## Reproducibility Notes
*   **Threshold Tuning:** Capacitance varies based on the size of your copper tape and the thickness of your cardboard. Use the `printDebugInfo()` function in the code to view raw values via UDP. Adjust the `touchThresholds` array in the firmware if the pads are too sensitive or not responsive enough.
*   **Adaptive Baseline:** The code includes a drift-compensation algorithm. If the sensors become "wonky" upon startup, leave the drum untouched for 3 seconds to allow the baselines to calibrate to the room's ambient capacitance.

