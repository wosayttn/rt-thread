# NuMaker-M5531

## 1. Introduction

The NuMaker-M5531 is a high-performance DSP/IoT evaluation board based on the Nuvoton NuMicro® **M5531** series microcontroller (Arm® Cortex®-M55 with Helium MVE, FPU, I/D-Cache, and MPU) and an on-board Nu-Link2-Me debugger/programmer.

### 1.1 Hardware Specifications

| Item | Specification |
| :--- | :--- |
| MCU | M5531K2LJAE (Arm® Cortex®-M55) |
| Operating Frequency | 220 MHz |
| On-chip Flash | 2 MB (Dual Bank, APROM @ `0x00100000`) |
| On-chip SRAM | 1.5 MB (ITCM 64 KB + DTCM 128 KB + SRAM 1344 KB + LPSRAM 8 KB) |
| Crypto Engine | TRNG, PRNG, AES, SHA/HMAC, RSA, ECC |
| Interfaces | Arduino UNO, USB 1.1/2.0 Type-C (FS/HS), CAN-FD, SWD |

### 1.2 On-board Devices

| Device | Description | Status |
| :--- | :--- | :--- |
| LEDs & Buttons | On-board GPIO | Supported |
| USB (CherryUSB) | USBD (FS), HSUSBD (HS), HSUSBH (EHCI 480M) | Supported |
| NuTFT V1.3 | ILI9341 LCD (`spi2`) + ADC Touch (`eadc0`) + QSPI Flash (`qspi0`) | Supported |

## 2. Supported Toolchains

| Toolchain | Tested Version | Linker Script |
| :--- | :--- | :--- |
| **Arm GCC** | Arm GNU Toolchain 13.3.1 (RT-Thread Env v2.1+) | `linking_scripts/link.ld` |
| **Keil MDK5** | Arm Compiler 6 (`armclang` V6.22) | `linking_scripts/link.scatter` |

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
2. Drag and drop `rtthread.bin` onto the **NuMicro MCU** virtual drive (or flash to `0x00100000` via Keil MDK5 / `Nuvoton ICP Program`).
3. Open the Virtual COM Port (`UART0` on `PB12`/`PB13`, **115200 8-N-1**, no flow control) to access the RT-Thread FinSH/MSH prompt (`msh >`).

- [Download NuMicro ICP Programming Tool](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG1320200721113745)

## 5. Resources

- [Quick Start — NuMaker-M5531](https://www.nuvoton.com/board/numaker-m5531/?index=2)
- [User Manual — NuMaker-M5531](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG132025091610463701)
- [Schematic, PCB & Gerber File — NuMaker-M5531](https://www.nuvoton.com/resource-download.jsp?tp_GUID=HL102025091707512412)
- [Nuvoton Direct — Buy NuMaker-M5531](https://direct.nuvoton.com/search?options%5Bprefix%5D=last&q=NuMaker-M5531)
