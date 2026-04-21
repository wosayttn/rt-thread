# MA35 DISP with RGB Converters

## Hardware Requirements

| **Converter** | **Base board** | **Daughter board** | **Notes** |
| --- | --- | --- | --- |
| RGB to LDVS (TI DS90C189) | NuMaker-HMI-MA35D1 V2.0 (VDDIO4=1.8v) | RGB TO LVDS 1080P_V2.0 | Must re-work VDDIO4 voltage setting on base board |
| RGB to LDVS (TI DS90C189) | NuMaker-HMI-MA35H0 V1.0 (VDDIO4=1.8v) | RGB TO LVDS 1080P_V2.0 | Must re-work VDDIO4 voltage setting on base board |
| RGB to HDMI (Lontium LT8618) | NuMaker-HMI-MA35D1 V2.0 (VDDIO4=3.3v) | RGB TO HDMI BOARD V3 | |
| RGB to HDMI (Lontium LT8618) | NuMaker-HMI-MA35H0 V1.0 (VDDIO4=3.3v) | RGB TO HDMI BOARD V3 | Slow down pixel clock to 123.75 MHz |
| RGB to MIPI (Lontium LT8918) | NuMaker-HMI-MA35D1 V2.0 (VDDIO4=1.8v) | RGB TO MIPI 1080P V2.0 | Must re-work VDDIO4 voltage setting on base board |
| RGB to eDP (Lontium LT8718) | NuMaker-HMI-MA35D1 V2.0 (VDDIO4=3.3v) | RGB TO eDP BOARD V2 | |
| RGB to VGA (ADV7125KSTZ140) | Dedicated board | N/A | |

## LCD Timing settings

| **Converter** | **HA** | **HSL** | **HFP** | **HBP** | **VA** | **VSL** | **VFP** | **VBP** | **PxlClk(Hz)** |
| --- | ---| --- | --- | --- | --- | --- | --- | --- | --- |
| RGB to LDVS (TI DS90C189) | 1920 | 16 | 32 | 16 | 1200 | 2 | 15 | 18 | 147014400 |
| RGB to MIPI (Lontium LT8918) | 1920 | 16 | 32 | 16 | 1200 | 2 | 15 | 18 | 148000000 |
| RGB to MIPI (Lontium LT8918) | 1080 | 16 | 16 | 32 | 2400 | 2 | 38 | 10 | 148000000 |
| RGB to HDMI (Lontium LT8618) | 1920 | 44 | 88 | 148 | 1080 | 5 | 4 | 36 | 148500000 |
| RGB to eDP (Lontium LT8718) | 1920 | 160 | 48 | 32 | 1080 | 31 | 3 | 5 | 148000000 |
| RGB to VGA (ADV7125KSTZ140) | 1024 | 136 | 24 | 160 | 768 | 6 | 3 | 29 | 65000000 |
