# NuMaker-M467HJ

## 1. Introduction

The NuMaker-M467HJ is an evaluation board based on the Nuvoton NuMicro® **M467HJHAN** microcontroller (Arm® Cortex®-M4F) with an on-board Nu-Link2-Me debugger/programmer. It features HyperRAM, Ethernet, audio codec, CAN-FD, USB OTG, microSD, EBI LCD, and CCAP camera interfaces.

### 1.1 Hardware Specifications

| Item | Specification |
| :--- | :--- |
| MCU | M467HJHAN (Arm® Cortex®-M4F) |
| Operating Frequency | 200 MHz |
| On-chip Flash | 1 MB (Dual Bank) |
| On-chip SRAM | 512 KB |
| Crypto Engine | TRNG, PRNG, AES, SHA/HMAC, RSA, ECC |
| Ethernet | 10/100 Mbps RMII (RTL8201F PHY) |
| Interfaces | Arduino UNO, RJ45 Ethernet, USB 1.1/2.0 OTG, microSD, 3.5 mm Audio, CAN-FD, EBI LCD, CCAP Camera, SWD |

### 1.2 On-board Devices

| Device | Description | Status |
| :--- | :--- | :--- |
| HyperRAM | W956A8MBYA5I (64 Mb) | Supported |
| Ethernet PHY | RTL8201F | Supported |
| Audio Codec | NAU88L22 | Supported |
| Thermal Sensor | NCT7717U | Supported |
| EBI LCD Panels | SSD1963 / FSA506 (`ebi0`) + ILI2130 / ST1663I Touch (`i2c1`) | Supported |
| CCAP Sensors | HM1055 / ADV7280 (`i2c0`) | Supported |

## 2. Supported Toolchains

| Toolchain | Tested Version | Linker Script |
| :--- | :--- | :--- |
| **Arm GCC** | Arm GNU Toolchain 13.3.1 (RT-Thread Env v2.1+) | `linking_scripts/link.ld` |
| **Keil MDK5** | Arm Compiler 6 (`armclang` V6.22) | `linking_scripts/link.sct` |

> Install `Nu-Link_Keil_Driver` when debugging/flashing from Keil MDK5.

## 3. Build Instructions

The default `.config` enables the minimal baseline (`UART0`, `PDMA`, `GPIO`, `PIN`, `MSH`). Use `menuconfig` to enable additional peripherals.

```bash
# Arm GCC build
scons -j16

# Generate Keil MDK5 project (Windows Env)
set RTT_CC=keil
scons --target=mdk5
```

## 4. Flashing & Serial Console

1. Set **Nu-Link2-Me** DIP switches No. 1 and 2 to **ON**, then connect the board to your PC via USB to enumerate the **NuMicro MCU** virtual drive and **Nu-Link2 Virtual COM Port**.
2. Drag and drop `rtthread.bin` onto the **NuMicro MCU** virtual drive (or flash via Keil MDK5 / `Nuvoton ICP Program`).
3. Open the Virtual COM Port (`UART0`, **115200 8-N-1**, no flow control) to access the RT-Thread FinSH/MSH prompt (`msh >`).

- [Download NuMicro ICP Programming Tool](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG1320200721113745)

## 5. Resources

- [Quick Start — NuMaker-M467HJ](https://www.nuvoton.com/board/numaker-m467hj/)
- [User Manual — NuMaker-M467HJ](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG132022072705350809)
- [Schematic, PCB & Gerber File — NuMaker-M467HJ](https://www.nuvoton.com/resource-download.jsp?tp_GUID=EV012022072705411161)
- [Nuvoton Direct — Buy NuMaker-M467HJ](https://direct.nuvoton.com/search?options%5Bprefix%5D=last&q=NuMaker-M467HJ)
