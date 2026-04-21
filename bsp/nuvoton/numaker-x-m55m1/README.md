# NuMaker-M55M1

## 1. Introduction

The NuMaker-M55M1 is an AI/ML and DSP evaluation board based on the Nuvoton NuMicro® **M55M1** series microcontroller (Arm® Cortex®-M55 + Arm® Ethos™-U55 NPU) with an on-board Nu-Link2-Me debugger/programmer.

### 1.1 Hardware Specifications

| Item | Specification |
| :--- | :--- |
| MCU | M55M1H2LJAE / M55M1K2LJAE (Arm® Cortex®-M55 + Ethos™-U55 NPU) |
| Operating Frequency | 220 MHz |
| On-chip Flash | 2 MB (Dual Bank, APROM @ `0x00100000`) |
| On-chip SRAM | 1.5 MB (ITCM 64 KB + DTCM 128 KB + SRAM 1344 KB + LPSRAM 8 KB) |
| Crypto Engine | TRNG, PRNG, AES, SHA/HMAC, RSA, ECC |
| Ethernet | 10/100 Mbps RMII (RTL8201F PHY) |
| Interfaces | Arduino UNO, mikroBUS, RJ45 Ethernet, USB 1.1/2.0 Type-C (FS/HS), microSD, 3.5 mm Audio, CAN-FD, EBI LCD, CCAP Camera, SWD |

### 1.2 On-board Devices

| Device | Description | Status |
| :--- | :--- | :--- |
| Ethernet PHY | RTL8201F | Supported |
| Audio Codec | NAU88L22 | Supported |
| USB (CherryUSB) | USBD (FS), HSUSBD (HS), HSUSBH (EHCI 480M) | Supported |
| EBI LCD Panels | LT7381 / FT5446, FSA506 / ST1663I | Supported |
| CCAP Sensors | HM1055, ADV7280 | Supported |

## 2. Supported Toolchains

| Toolchain | Tested Version | Linker Script |
| :--- | :--- | :--- |
| **Arm GCC** | Arm GNU Toolchain 13.3.1 (RT-Thread Env v2.1+) | `linking_scripts/link.ld` |
| **Keil MDK5** | Arm Compiler 6 (`armclang` V6.22) | `linking_scripts/link.scatter` |

> Install `Nu-Link_Keil_Driver` when debugging/flashing from Keil MDK5.

## 3. Build Instructions

Use `menuconfig` to enable or disable on-chip peripherals and packages.

```bash
# Arm GCC build
scons -j16

# Generate Keil MDK5 project (Windows Env)
set RTT_CC=keil
scons --target=mdk5
```

## 4. Flashing & Serial Console

1. Set **Nu-Link2-Me** DIP switches No. 1 and 2 to **ON**, then connect the board to your PC via USB to enumerate the **NuMicro MCU** virtual drive and **Nu-Link2 Virtual COM Port**.
2. Drag and drop `rtthread.bin` onto the **NuMicro MCU** virtual drive (or flash to `0x00100000` via Keil MDK5 / `Nuvoton ICP Program`).
3. Open the Virtual COM Port (`UART0` on `PB12`/`PB13`, **115200 8-N-1**, no flow control) to access the RT-Thread FinSH/MSH prompt (`msh >`).

- [Download NuMicro ICP Programming Tool](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG1320200721113745)

## 5. Resources

- [Quick Start — NuMaker-X-M55M1D](https://www.nuvoton.com/board/numaker-x-m55m1d/?index=2)
- [User Manual — NuMaker-X-M55M1D](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG132025091911464523)
- [Schematic, PCB & Gerber File — NuMaker-X-M55M1D](https://www.nuvoton.com/resource-download.jsp?tp_GUID=HL102025092207032917)
- [Nuvoton Direct — Buy NuMaker-X-M55M1D](https://direct.nuvoton.com/search?options%5Bprefix%5D=last&q=NuMaker-X-M55M1D)
- [AIoT-HMI-UVC Demo Video](https://www.youtube.com/watch?v=Ycnhv7C_lWM)
