# Lion Dance Drum: Interactive ESP32 Digital Instrument

A quiet, portable, and responsive electronic Lion Dance drum designed for practice in noise-sensitive environments. This project uses the ESP32's internal capacitive touch sensors to detect strikes on a cardboard drum and triggers high-fidelity audio samples on a laptop via UDP over Wi-Fi.

## Usage
1.  **Center Hit:** Strike either Pad A or Pad B.
2.  **Mute Hit:** Hold Pad A with one hand and strike Pad B with the other (or vice-versa).
3.  **Rim Hit:** Strike the side pads.
4.  **Change Instrument:** Tap the Mode pad to toggle between the Drum kit and the Cymbal kit.
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
2.  Apply them to your cardboard structure. For the most authentic feel, place the "Rim" pads on the curved sides of the box. The rims were put together to the main drum via. hot glue. 
3.  Divide the center area into two distinct copper zones (Pad A and Pad B) to allow for the muting gesture. This was done by divideding the center into two zones by having one big donut shaped cardboard and a smaller circular cardboard that fit in the middle. 
4.  Solder jumper wires from the copper tape to the corresponding ESP32 GPIO pins.
5.  **Note on Grounding:** Keep the ESP32 plugged into your laptop via USB. The cable acts as a ground plane, which is necessary for stable capacitive readings.

This is what the drum should roughly look like - made to mimic a real lion dance drum. 
![copper-lion-dance-drum](https://github.com/user-attachments/assets/3f43aa65-70a7-469a-bbba-bab1ee8d64fe)

This is what my wiring looked like in the back.
![back-setup](https://github.com/user-attachments/assets/f46fcb0c-8c7f-46d1-a594-26a68cc000c1)


### 2. ESP32 Firmware
1.  Open the provided firmware file in your editor.
2.  Update the `laptopIP` variable to match your laptop's current IP address on the Wi-Fi network.
3.  Upload the code to your ESP32.

### 3. Laptop Receiver (Python)
1. Install pygame.
2.  Place your audio samples (`.wav` format) in the same directory as the Python script. Feel free to use the samples in the github repo, as they're from a real lion dance drum.
3.  Ensure the filenames in the Python script match your recordings (e.g., `center.wav`, `rim.wav`, `mute.wav`).
4.  Run the script for debugging. This will allow you to see the raw values, as well as the change in value upon touch:
    ```bash
    python listen.py
    ```
5. Run the script for actual sound:
   ```bash
   python laptop_drum.py
   ```
---

## Reproducibility Notes
*   **Threshold Tuning:** Capacitance varies based on the size of your copper tape and the thickness of your cardboard. Use the `printDebugInfo()` function in the code to view raw values via UDP. Adjust the `touchThresholds` array in the firmware if the pads are too sensitive or not responsive enough.
*   **Adaptive Baseline:** The code includes a drift-compensation algorithm. If the sensors become "wonky" upon startup, leave the drum untouched for 3 seconds to allow the baselines to calibrate to the room's ambient capacitance.

