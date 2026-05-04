# 🔧 CS423 — Vibration-Based Machine Condition Monitoring System

> ระบบตรวจเช็คสภาพเครื่องจักรด้วยความสั่นสะเทือน  
> วิชา CS423 Internet of Things and Applications

![Platform](https://img.shields.io/badge/Simulator-Wokwi-blue)
![MCU](https://img.shields.io/badge/MCU-ESP32-green)
![Sensor](https://img.shields.io/badge/Sensor-MPU6050-orange)
![Dashboard](https://img.shields.io/badge/Dashboard-Thingsboard-purple)
![Protocol](https://img.shields.io/badge/Protocol-MQTT-red)

---

## 📋 สารบัญ
- [ภาพรวมโครงงาน](#-ภาพรวมโครงงาน)
- [สถาปัตยกรรมระบบ](#-สถาปัตยกรรมระบบ)
- [อุปกรณ์ที่ใช้](#-อุปกรณ์ที่ใช้)
- [โครงสร้างโปรเจค](#-โครงสร้างโปรเจค)
- [การติดตั้งและตั้งค่า](#-การติดตั้งและตั้งค่า)
- [การใช้งาน](#-การใช้งาน)
- [Source Code อธิบาย](#-source-code-อธิบาย)
- [Dashboard Thingsboard](#-dashboard-thingsboard)
- [ผลการทดสอบ](#-ผลการทดสอบ)
- [ผู้จัดทำ](#-ผู้จัดทำ)

---

## 🎯 ภาพรวมโครงงาน

ระบบ IoT ตรวจสอบสภาพเครื่องจักรแบบ Real-time ผ่านการวัดความสั่นสะเทือน โดย ESP32 อ่านค่าจาก MPU6050 (แทน SW-420 ที่ไม่มีใน Wokwi) คำนวณ Vibration Magnitude กรอง Noise ด้วย EMA Filter หากสั่นเกิน 1.3g จะสั่ง Relay ตัดเครื่องอัตโนมัติ และส่งข้อมูลขึ้น Thingsboard ผ่าน MQTT ทุก 500ms

```
[Wokwi: ESP32 + MPU6050] ──MQTT──► [Thingsboard Broker] ──WebSocket──► [Dashboard]
         │                                                                     │
      [Relay]                                                            [Alert Log]
  (ควบคุมเครื่องจักร)                ◄──── MQTT RPC (Downlink) ────── [Control Panel]
```

---

## 🏗️ สถาปัตยกรรมระบบ

| Layer | Component | Technology | หน้าที่ |
|-------|-----------|------------|---------|
| ① Device | ESP32 + MPU6050 + Relay + Potentiometer | Arduino C++ / I2C | วัดสั่น ประมวลผล ควบคุม Relay |
| ② Communication | Wi-Fi → MQTT | PubSubClient / 802.11 | ส่ง JSON ขึ้น Cloud ทุก 500ms |
| ③ Backend/Cloud | Thingsboard MQTT Broker | Thingsboard CE | รับข้อมูล Rule Engine API |
| ④ Database | Time-Series DB | PostgreSQL (built-in) | เก็บ Telemetry + Alert Log |
| ⑤ Frontend | Thingsboard Dashboard | React Widget | Real-time กราฟ Gauge Alert |
| ⑥ AI Agent | Claude AI (Anthropic) | Claude Sonnet | พัฒนา Code Docs Architecture |

---

## 🔩 อุปกรณ์ที่ใช้

| # | อุปกรณ์ | รุ่น | หน้าที่ | Pin ESP32 |
|---|---------|------|---------|-----------|
| 1 | Microcontroller | ESP32 DevKit v1 | ประมวลผลหลัก + Wi-Fi | — |
| 2 | IMU Sensor (แทน SW-420) | MPU6050 | วัดแรงสั่น 3 แกน (±2g) | SDA=21, SCL=22 |
| 3 | Relay Module | 5V Single Channel | ตัด-ต่อไฟเครื่องจักร | GPIO 26 |
| 4 | Potentiometer | Rotary Knob 10kΩ | ปรับ Threshold Manual | GPIO 34 (ADC) |

**Pin Connection:**

| อุปกรณ์ | Pin | ESP32 GPIO | แรงดัน |
|---------|-----|-----------|--------|
| MPU6050 | SDA | GPIO 21 | 3.3V |
| MPU6050 | SCL | GPIO 22 | 3.3V |
| MPU6050 | VCC | 3.3V | 3.3V |
| MPU6050 | GND | GND | 0V |
| Relay | IN | GPIO 26 | 3.3V |
| Relay | VCC | 5V | 5V |
| Potentiometer | SIG | GPIO 34 (ADC) | 3.3V |

---

## 📁 โครงสร้างโปรเจค

```
cs423-vibration-monitor/
├── src/
│   └── main.ino                 ← Source Code หลัก (ESP32 + MQTT + MPU6050)
├── wokwi/
│   ├── diagram.json             ← วงจรจำลอง Wokwi
│   └── libraries.txt            ← Library ที่ต้องใช้
├── docs/
│   ├── CS423_PlatformStack.pdf  ← เอกสาร Platform Stack
│   └── CS423_Full_Report.docx   ← รายงานโครงงานฉบับเต็ม
└── README.md
```

---

## 🚀 การติดตั้งและตั้งค่า

### ขั้นตอนที่ 1 — ตั้งค่า Thingsboard

1. ไปที่ **[thingsboard.cloud](https://thingsboard.cloud)** → สมัครฟรี → Login
2. เมนูซ้าย → **Entities → Devices → "+" → Add new device**
   - Name: `vibration-machine` → กด **Add**
3. คลิก Device → แท็บ **"Credentials"** → คัดลอก **Access Token**

MQTT Connection:
```
Host:     mqtt.thingsboard.cloud
Port:     1883
Username: [Access Token ที่ได้]
Password: (เว้นว่าง)
Topic:    v1/devices/me/telemetry
```

### ขั้นตอนที่ 2 — ทดสอบด้วย MQTTX

1. เปิด MQTTX → New Connection → ใส่ข้อมูลด้านบน → **Connect** (ไฟเขียว = OK)
2. Publish ทดสอบ:
   - Topic: `v1/devices/me/telemetry`
   - Payload: `{"vibration": 1.05, "status": "RUNNING", "ax": 0.1, "ay": 0.05, "az": 1.0}`
3. ตรวจที่ Thingsboard → Device → **Latest telemetry** → ต้องเห็นข้อมูล ✅

### ขั้นตอนที่ 3 — เปิดบน Wokwi

1. ไปที่ **[wokwi.com](https://wokwi.com)** → New Project → ESP32
2. เพิ่ม Libraries ใน `libraries.txt`:
```
PubSubClient
ArduinoJson
MPU6050
```
3. ต่อวงจรตาม Pin Connection ด้านบน

### ขั้นตอนที่ 4 — ใส่ Token แล้ว Run

```cpp
// แก้บรรทัดนี้ใน src/main.ino
const char* mqtt_user = "YOUR_ACCESS_TOKEN_HERE";
```

กด **▶ Run** → Serial Monitor:
```
Connecting WiFi... Connected!
Connecting MQTT... Connected!
=== SYSTEM START ===
✅ MPU6050 Connected
Vib: 0.98 | Status: RUNNING
```

---

## 📖 การใช้งาน

### ตีความ Serial Monitor

| ข้อความ | ความหมาย |
|---------|----------|
| `✅ MPU6050 Connected` | Sensor พร้อมใช้งาน |
| `Vib: 0.98 \| RUNNING` | สั่นปกติ เครื่องทำงาน |
| `⚠️ ABNORMAL → STOP MACHINE` | สั่นเกิน 1.3g → Relay ตัด |
| `✅ NORMAL → MACHINE RUNNING` | กลับปกติ → Relay ต่อ |

### MQTT Payload

```json
{
  "vibration":       1.054,
  "smoothVibration": 1.021,
  "ax":  0.12,
  "ay": -0.05,
  "az":  0.98,
  "status": "RUNNING"
}
```

---

## 💻 Source Code อธิบาย

```cpp
// คำนวณ Vibration Magnitude
float calculateVibration(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);  // ค่าปกติตอนนิ่ง ≈ 1.0g
}

// EMA Filter ลด Noise (alpha=0.7)
float smooth(float input, float prev) {
    return (0.7 * input) + (0.3 * prev);
}

// Control Logic
if (smoothVibration > threshold && machineRunning) {
    digitalWrite(RELAY_PIN, LOW);   // ปิดเครื่อง
    machineRunning = false;
}
```

---

## 📊 Dashboard Thingsboard

1. **Dashboards → "+" → Create** → Title: `Machine Vibration Monitor`
2. กด Edit → Add widget ตามตาราง:

| Widget | ประเภท | Key |
|--------|--------|-----|
| สถานะเครื่อง | Cards → Simple Card | `status` |
| กราฟ Vibration | Charts → Time series | `smoothVibration` |
| Gauge แรงสั่น | Gauges → Radial gauge | `vibration` (0–3) |
| AX / AY / AZ | Gauges → Digital gauge | `ax`, `ay`, `az` |
| Alert Log | Alarm → Alarms table | — |

---

## ✅ ผลการทดสอบ

| Test Case | ผล |
|-----------|-----|
| MPU6050 Connection | ✅ ผ่าน |
| MQTT → Thingsboard | ✅ ผ่าน |
| Vibration ปกติ → Relay ON | ✅ ผ่าน |
| Vibration เกิน → Relay OFF | ✅ ผ่าน |
| Dashboard Real-time | ✅ ผ่าน |
| Alert Trigger | ✅ ผ่าน |
| MQTT Auto-Reconnect | ✅ ผ่าน |

---

## 🤖 AI Agent ที่ใช้

| ส่วนที่ใช้ | Claude AI ช่วย |
|-----------|--------------|
| Arduino Code | MPU6050 Logic, EMA Filter, Relay State Machine |
| Architecture | 6-Layer Design, MQTT Topic |
| Debugging | I2C Error, MQTT Connection |
| Documentation | รายงาน, README, Platform Stack PDF |

---

## 👥 ผู้จัดทำ

| ชื่อ-นามสกุล | รหัสนักศึกษา |
|-------------|-------------|
| [ชื่อ-นามสกุล] | [XXXXXXXXX] |

**ผู้สอน:** อาจารย์ทศพร เวชศิริ  
**วิชา:** CS423 Internet of Things and Applications  
**รูปแบบ:** Wokwi Simulator

