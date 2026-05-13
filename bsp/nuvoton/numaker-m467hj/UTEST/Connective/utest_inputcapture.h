/******************************************************************************
*
* @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
******************************************************************************/
#ifndef __UTEST_INPUTCAPTURE_H__
#define __UTEST_INPUTCAPTURE_H__

void nu_init_inputcapture_test(void);
void nu_deinit_inputcapture_test(void);
void nu_capture_basic_test(char *pcDeviceName);
void nu_capture_watermark_test(char *pcDeviceName);
void nu_capture_multichannel_test(char *pcDeviceName0, char *pcDeviceName1, char *pcDeviceName2);

#endif /* __UTEST_INPUTCAPTURE_H__ */