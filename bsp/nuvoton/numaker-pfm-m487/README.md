# NuMaker-PFM-M487

## 1. Introduction

The NuMaker-PFM-M487 is an evaluation board based on the Nuvoton NuMicro® **M487JIDAE** microcontroller (Arm® Cortex®-M4F) with an on-board Nu-Link-Me debugger/programmer. It provides Ethernet, USB OTG, microSD, SPI Flash, and audio codec interfaces.

### 1.1 Hardware Specifications

| Item | Specification |
| :--- | :--- |
| MCU | M487JIDAE (Arm® Cortex®-M4F) |
| Operating Frequency | 192 MHz |
| On-chip Flash | 512 KB (Dual Bank) |
| On-chip SRAM | 160 KB |
| Crypto Engine | DES, 3DES, AES, SHA/HMAC, ECC |
| Ethernet | 10/100 Mbps RMII (IP101GR PHY) |
| Interfaces | Arduino UNO, RJ45 Ethernet, USB 1.1/2.0 OTG, microSD, 3.5 mm Audio, SWD |

### 1.2 On-board Devices

| Device | Description | Status |
| :--- | :--- | :--- |
| SPI Flash | W25Q32 (32 Mb) | Supported |
| Ethernet PHY | IP101GR | Supported |
| Audio Codec | NAU88L25 (MIC & earphone) | Supported |

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

1. Set **ISW1** switches on the NuMaker-PFM-M487 board to **ON**, then connect the board to your PC via USB to enumerate the **NuMicro MCU** virtual drive and **Nu-Link Virtual COM Port**.
2. Drag and drop `rtthread.bin` onto the **NuMicro MCU** virtual drive (or flash via Keil MDK5 / `Nuvoton ICP Program`).
3. Open the Virtual COM Port (`UART0`, **115200 8-N-1**, no flow control) to access the RT-Thread FinSH/MSH prompt (`msh >`).

- [Download NuMicro ICP Programming Tool](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG1320200721113745)

## 5. Resources

- [Evaluation Board Index — M4 Series](https://www.nuvoton.com/tool-and-software/evaluation-board/general/m4-evaluation-board/)
- [User Manual — NuMaker-PFM-M487](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG0120180420144451)
- [Schematic, PCB & Gerber File — NuMaker-PFM-M487](https://www.nuvoton.com/resource-download.jsp?tp_GUID=HL0120180420145943)
- [Nuvoton Direct — Buy NuMaker-PFM-M487](https://direct.nuvoton.com/search?options%5Bprefix%5D=last&q=NuMaker-PFM-M487)
