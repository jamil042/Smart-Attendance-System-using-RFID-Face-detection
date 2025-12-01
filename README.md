
# Smart Attendance System using RFID & Face detection

The Smart Attendance System is a dual authentication security device designed to ensure accurate and reliable attendance verification. By combining RFID card scanning with face recognition, the system eliminates proxy/fake attendance and grants access only when both validations succeed.

This project showcases how embedded systems + IoT + automation can be applied in real-world environments such as classrooms, offices, and secured zones.
<br><br>
## 🚀 Features

✔ Dual-authentication: RFID + Face Recognition
<br>
✔ Prevents proxy attendance & unauthorized access
<br>
✔ Automatic attendance logging in real-time
<br>
✔ Supports multiple registered users
<br>
✔ LCD status display + LED + buzzer notifications
<br>
✔ Low power consumption, portable design
<br>
✔ Expandable for IoT/cloud integration for online record storage


<br><br>
## 🔧 Hardware Components


| Component                 | Description                                          |
| ------------------------- | ---------------------------------------------------- |
| Arduino UNO               | Central controller for RFID scanning and system flow |
| ESP32-CAM                 | Face recognition module                              |
| RC522 RFID Reader         | Reads RFID card unique UID                           |
| LCD Display (16×2 / I2C)  | Shows live status messages                           |
| LEDs (Red, Green)         | Visual authentication feedback                       |
| Buzzer                    | Audio response for verify/deny signals               |
| Jumper Wires + Breadboard | Circuit connections                                  |
| Power Source (USB/5V)     | External or portable                                 |

<br><br>

## ⚙ Working Methodology


| Step                             | Description                                                             |
| -------------------------------- | ----------------------------------------------------------------------- |
| **Step 1 – Hardware Setup**      | Connect Arduino with RFID, LCD, LEDs, buzzer; test modules individually |
| **Step 2 – RFID Verification**   | Scan card, validate UID, move to next step if correct                   |
| **Step 3 – Face Recognition**    | ESP32-CAM captures face & compares with trained dataset                 |
| **Step 4 – Access + Attendance** | Dual-auth success → log attendance, green LED, buzzer                   |
| **Step 5 – Final Assembly**      | Power via USB/5V, optional Li-ion battery for portability               |


<br><br>
## 📈 Future Improvements


🔹 Cloud attendance sync using Wi-Fi / Firebase / MQTT
<br>
🔹 Web Dashboard for admin monitoring
<br>
🔹 Battery powered enclosure + compact PCB design
<br>
🔹 Mobile app notifications for entry tracking
<br><br>
## 📜 License


This project is open-source and free to use under MIT License.
<br><br>
## 🤝 Contributing


Pull requests are welcome! Feel free to improve the system or add new features.

📩 Contact

For queries, suggestions, or collaboration:

