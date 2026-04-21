# NuMaker-M2L31KI

## 1. Introduction

The NuMaker-M2L31KI is an ultra-low-power evaluation board based on the Nuvoton NuMicro® **M2L31** series microcontroller (`M2L31KIDAE` / `M2L31KGDAE`, Arm® Cortex®-M23) with an on-board Nu-Link2-Me debugger/programmer. It integrates CAN-FD, USB 2.0 Full-Speed OTG, low-power peripherals (LPUART, LPI2C, LPSPI, LPADC, LPPDMA), and Arduino UNO expansion headers.

### 1.1 Hardware Specifications

| Item | Specification |
| :--- | :--- |
| MCU | M2L31KIDAE / M2L31KGDAE (Arm® Cortex®-M23) |
| Operating Frequency | 72 MHz |
| On-chip Flash | 512 KB (RRAM) |
| On-chip SRAM | 168 KB (64 KB default linker baseline) |
| Crypto Engine | PRNG, AES, DES/3DES, SHA/HMAC, CRC |
| Interfaces | Arduino UNO, USB 2.0 Full-Speed OTG, CAN-FD, SWD |

### 1.2 On-board Devices

| Device | Description | Status |
| :--- | :--- | :--- |
| LEDs & Buttons | On-board GPIO (`PB14` LED) | Supported |
| CAN-FD | On-chip CAN-FD0/1 | Supported |
| Low-Power Peripherals | LPUART, LPI2C, LPSPI, LPADC, LPPDMA | Supported |

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

1. Set the **Nu-Link2-Me** DIP switches to **ON**, then connect the board to your PC via USB to enumerate the **NuMicro MCU** virtual drive and **Nu-Link2 Virtual COM Port**.
2. Drag and drop `rtthread.bin` onto the **NuMicro MCU** virtual drive (or flash via Keil MDK5 / `Nuvoton ICP Program`).
3. Open the Virtual COM Port (`UART0` on `PB12`/`PB13`, **115200 8-N-1**, no flow control) to access the RT-Thread FinSH/MSH prompt (`msh >`).

- [Download NuMicro ICP Programming Tool](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG1320200721113745)

## 5. Resources

- [Quick Start — NuMaker-M2L31KI](https://www.nuvoton.com/board/numaker-m2l31ki/)
- [User Manual — NuMaker-M2L31KI](https://www.nuvoton.com/resource-download.jsp?tp_GUID=UG132024062009492968)
- [Schematic, PCB & Gerber File — NuMaker-M2L31KI](https://www.nuvoton.com/resource-download.jsp?tp_GUID=HL102024061911124000)
- [Nuvoton Direct — Buy NuMaker-M2L31KI](https://direct.nuvoton.com/search?options%5Bprefix%5D=last&q=NuMaker-M2L31KI)
