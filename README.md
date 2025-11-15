# AERAS – Accessible E-Rickshaw Automation System

<div align="center">

![Badge](https://img.shields.io/badge/AERAS-IOTRIX_Hackathon-blue?style=for-the-badge)
![Badge](https://img.shields.io/badge/ESP32-Wokwi_Simulation-green?style=for-the-badge)
![Badge](https://img.shields.io/badge/Backend-Node.js-brightgreen?style=for-the-badge)
![Badge](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)

### **An app-less, location-based ride request system for elderly and special needs individuals**

**No Smartphone Required • Physical Interface • Real-time Coordination**

</div>

---

## 🚀 Quick Start

> ⚠️ **Note:** The backend currently runs locally (`localhost`). A public deployment was not configured due to time constraints and Ngrok issues. To demo the system, please run the backend locally.

### 1. Start Backend Server
```bash
cd backend
npm install
node server.js
````

### 2. Access Web Interfaces (Local)

* **Rickshaw UI:** `rickshaw.html`
* **Admin Dashboard:** `admin.html`

### 3. Run Wokwi Simulation

1. Open **Wokwi ESP32 Simulator**
2. Copy our ESP32 code
3. Add components (Ultrasonic, LEDs, Button, Buzzer, OLED)
4. Click **Start Simulation**

---

## 📋 Table of Contents

* [Problem Statement](#🎯-problem-statement)
* [Solution](#💡-solution)
* [Features](#✨-key-features)
* [Setup Instructions](#🛠️-setup-instructions)
* [Hardware Setup](#🔌-hardware-architecture)
* [API Documentation](#📡-api-documentation)
* [Test Cases](#✅-test-cases-implemented)
* [Workflow](#🚀-complete-workflow)
* [Innovation](#🎯-innovation-highlights)
* [Team](#👥-team)
* [License](#📄-license)
* [Acknowledgments](#🙏-acknowledgments)

---

## 🎯 Problem Statement

### **Target Users**

* Senior Citizens (≥60)
* Autistic & Special Needs Individuals

### **Challenges**

Traditional ride-hailing apps create difficulties due to:

* Complex interfaces
* Small touch targets
* Digital literacy requirements
* Visual/cognitive limitations

### **Mission**

Enable **app-less ride requests** through a physical location block with multi-sensor verification.

---

## 💡 Solution

AERAS is a **hardware + software system** connecting users, rickshaw pullers, and administrators.

### 🎮 Physical Interface

* Ultrasonic presence detection
* Laser/LDR verification
* LED indicators
* Buzzer confirmation
* OLED status displays

### 🔧 Technical Stack

* **ESP32** + sensors + OLED
* **Node.js** + Express backend
* **HTML/CSS/JS** dashboards
* **Wokwi** simulation

---

## ✨ Key Features

### **For Users**

* Zero learning curve
* Multi-sensor security
* LEDs + OLED feedback
* Audio confirmation
* No smartphone needed

### **For Rickshaw Pullers**

* Web dashboard
* Accept/reject rides
* Points reward system
* Navigation guidance

### **For Administrators**

* Live monitoring
* Analytics dashboard
* Puller leaderboard
* Reward management

---

## 🛠️ Setup Instructions

### Backend Setup

```bash
cd backend
npm install express cors
node server.js
```

Visit: `http://localhost:3000` in your browser.
Expected output:
`AERAS Backend Server is active`

### Web Interfaces

* **Rickshaw UI:** `rickshaw.html`
* **Admin Dashboard:** `admin.html`

### Wokwi Simulation

* Copy ESP32 code into a new Wokwi project
* Add the necessary components
* Start the simulation

---

## 🔌 Hardware Architecture

```
User Interface → ESP32 → Node.js Backend → Web Dashboards
     ↑               ↑                   ↑                ↑
Sensors         WiFi/HTTP           REST API        Rickshaw Panel
OLED/Buzzer                        Real-time Sync   Admin Panel
```

### Component Overview

| Component         | Purpose            | Pin(s)         |
| ----------------- | ------------------ | -------------- |
| ESP32             | Main controller    | —              |
| Ultrasonic Sensor | Detect user        | TRIG 4, ECHO 2 |
| Button            | Laser verification | 18             |
| LDR               | Light sensing      | 34             |
| Red LED           | Error/Timeout      | 14             |
| Yellow LED        | Ride offer         | 12             |
| Green LED         | Accepted           | 13             |
| Buzzer            | Alerts             | 27             |
| OLED              | Status display     | SDA 21, SCL 22 |

---

## 📡 API Documentation

| Method | Endpoint               | Description         | Body                              |
| ------ | ---------------------- | ------------------- | --------------------------------- |
| POST   | `/request-ride`        | Create ride request | `{pickup, destination, pullerId}` |
| GET    | `/rides`               | Get all rides       | -                                 |
| GET    | `/ride-status/:id`     | Check ride status   | -                                 |
| POST   | `/accept-ride/:id`     | Accept ride         | -                                 |
| POST   | `/confirm-pickup/:id`  | Confirm pickup      | -                                 |
| POST   | `/confirm-dropoff/:id` | Complete ride       | -                                 |

### Example: Create Ride

```javascript
await fetch('http://localhost:3000/request-ride', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({
    pickup: "CUET Campus",
    destination: "Pahartoli",
    pullerId: "puller_001"
  })
});
```

---

## ✅ Test Cases Implemented

### Hardware Tests (40 marks)

* Ultrasonic detection
* LDR + laser verification
* Button + buzzer
* LED indicators
* OLED display
* Web sync
* GPS & points system

### Software Tests (25 marks)

* Rider alert
* Real-time sync
* Admin monitoring
* Point management
* Database design

### Integration Tests (15 marks)

* End-to-end journey
* Edge cases

---

## 🚀 Complete Workflow

### User Experience

1. Stand on the location block
2. Ultrasonic verifies presence
3. Laser/LDR authentication
4. Automatic ride request
5. LEDs show progress
6. Ride completes → Points awarded

### Puller Experience

1. Receive ride alert
2. Accept ride
3. Navigate to pickup
4. Confirm pickup
5. Confirm dropoff
6. Earn points

---

## 🎯 Innovation Highlights

### 🏆 Social Impact

* Accessible for the elderly
* Boosts rickshaw puller income
* Inclusive design

### 🔬 Technical Innovations

* Multi-sensor fusion
* Real-time synchronization
* State-machine workflow
* Complete Wokwi simulation

### 💡 Unique Features

* Fully app-less
* Physical human interaction
* Audio-visual feedback
* Reward system

---

## 👥 Team

**Team Name:** RetroByte
**Hackathon:** IOTRIX – CUET
**Track:** Hardware + Software Integration

### Members

* **Ashraf Khan Shihab** – Software Specialist
* **Mohammad Sadaf Mahmud** – Hardware Specialist
* **Fariyad Fardin Meah** – Documentation & Presentation

### Contact

* Email: **[ashrafshihab35@gmail.com](mailto:ashrafshihab35@gmail.com)**
* GitHub: **[https://github.com/0-shihab-0](https://github.com/0-shihab-0)**

---

## 📄 License

This project is licensed under the **MIT License**.

---

## 🙏 Acknowledgments

* IOTRIX Organizers – CUET
* Wokwi Team
* Node.js Community
* Project Mentors

---

<div align="center">

### 🏆 IOTRIX Hackathon Submission

**Department of ETE – CUET**
Making transportation accessible for everyone. 🚗💨

</div>
