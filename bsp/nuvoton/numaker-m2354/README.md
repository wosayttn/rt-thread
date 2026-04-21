# NuMaker-M2354

## 1. Introduction

The NuMaker-M2354 is a security-focused evaluation board based on the Nuvoton NuMicro® **M2354KJFAE** microcontroller (Arm® Cortex®-M23 with TrustZone®) and an on-board Nu-Link2-Me debugger/programmer.

### 1.1 Hardware Specifications

| Item | Specification |
| :--- | :--- |
| MCU | M2354KJFAE (Arm® Cortex®-M23) |
| Operating Frequency | 96 MHz |
| On-chip Flash | 1 MB (Dual Bank) |
| On-chip SRAM | 256 KB |
| Crypto Engine | PRNG, AES, SHA/HMAC, RSA, ECC |
| Interfaces | Arduino UNO, USB 1.1 OTG, microSD, Segment LCD, SWD |

### 1.2 On-board Devices

| Device | Description | Status |
| :--- | :--- | :--- |
| Wi-Fi Module | ESP8266-ESP12 (AT v1.7) | Supported |
| Segment LCD | On-board SLCD | Supported |

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

1. Set **ISW1** switches on the NuMaker-M2354 board to **ON**, then connect the board to your PC via USB to enumerate the **NuMicro MCU** virtual drive and **Nu-Link2 Virtual COM Port**.
2. Drag and drop `rtthread.bin` onto the **NuMicro MCU** virtual drive (or flash via Keil MDK5 / `Nuvoton ICP Program`).
3. Open the Virtual COM Port (`UART0`, **115200 8-N-1**, no flow control) to access the RT-Thread FinSH/MSH prompt (`msh >`).

- [Download NuMicro ICP Programming Tool](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG1320200721113745)

## 5. Resources

- [Quick Start — NuMaker-M2354](https://www.nuvoton.com/board/numaker-m2354/)
- [User Manual — NuMaker-M2354](https://www.nuvoton.com/resource-download.jsp?tp_GUID=EV0120201230182459)
- [Schematic, PCB & Gerber File — NuMaker-M2354](https://www.nuvoton.com/resource-download.jsp?tp_GUID=HL1020201102161956)
- [Nuvoton Direct — Buy NuMaker-M2354](https://direct.nuvoton.com/search?options%5Bprefix%5D=last&q=NuMaker-M2354)
