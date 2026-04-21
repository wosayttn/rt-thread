# NuMaker-M3334KI

## 1. Introduction

The NuMaker-M3334KI is an evaluation board based on the Nuvoton NuMicro® **M3334KIGAE** microcontroller (M3331 series, Arm® Cortex®-M33) with an on-board Nu-Link2-Me debugger/programmer.

### 1.1 Hardware Specifications

| Item | Specification |
| :--- | :--- |
| MCU | M3334KIGAE (Arm® Cortex®-M33) |
| Operating Frequency | 180 MHz |
| On-chip Flash | 512 KB (Dual Bank) |
| On-chip SRAM | 320 KB |
| Interfaces | Arduino UNO, USB 2.0 High-Speed Type-C (DRP), SWD |

### 1.2 On-board Devices

| Device | Description | Status |
| :--- | :--- | :--- |
| LEDs & Buttons | On-board GPIO | Supported |
| HSUSB | High-Speed USB 2.0 DRP (Host / Device) | Supported |
| NuTFT V1.3 | ILI9341 LCD (`spi2`) + ADC Touch (`eadc0`) + QSPI Flash | Supported |
| NuFUN V2.0 | Extended peripheral shield | Supported |

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

1. Set **ICESW2** switches on the NuMaker-M3334KI board to **ON**, then connect the board to your PC via USB Type-C to enumerate the **NuMicro MCU** virtual drive and **Nu-Link2 Virtual COM Port**.
2. Drag and drop `rtthread.bin` onto the **NuMicro MCU** virtual drive (or flash via Keil MDK5 / `Nuvoton ICP Program`).
3. Open the Virtual COM Port (`UART0`, **115200 8-N-1**, no flow control) to access the RT-Thread FinSH/MSH prompt (`msh >`).

- [Download NuMicro ICP Programming Tool](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG1320200721113745)

## 5. Resources

- [Quick Start — NuMaker-M3334KI](https://www.nuvoton.com/board/numaker-m3334ki/)
- [User Manual — NuMaker-M3334KI](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG132026032603222854)
- [Schematic, PCB & Gerber File — NuMaker-M3334KI](https://www.nuvoton.com/resource-download.jsp?tp_GUID=HL102026032603280495)
- [Nuvoton Direct — Buy NuMaker-M3334KI](https://direct.nuvoton.com/search?options%5Bprefix%5D=last&q=NuMaker-M3334KI)
