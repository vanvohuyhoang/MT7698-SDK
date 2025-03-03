/* Copyright Statement:
*
* (C) 2005-2017  MediaTek Inc. All rights reserved.
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
* Without the prior written permission of MediaTek and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
* You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
* if you have agreed to and been bound by the applicable license agreement with
* MediaTek ("License Agreement") and been granted explicit permission to do so within
* the License Agreement ("Permitted User").  If you are not a Permitted User,
* please cease any access or use of MediaTek Software immediately.
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
* ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
* WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/


#ifndef _PATCH_CONFIG_RAM_H
#define _PATCH_CONFIG_RAM_H

#include "patch_config.h"

/*  Add 5 retries for Auth and Assoc frames. */
/*Patch size 308 Bytes */
#undef MT7687_BORA00005044
#define MT7687_BORA00005044						1


/*EAPOL Retry issue with NetGear AP AMAZON DEMO Issue*/
/*Patch Size 2256 bytes*/
#undef MT7687_BORA00007020
#define MT7687_BORA00007020						1


/*Semd EAPOL and DHCP Packets on Basic Rate*/
/*Patch size 216 Bytes*/
#undef MT7687_BORA00007373
#define MT7687_BORA00007373						1


/*Enable RSSI threshold*/
/*Patch Size - 580 bytes*/
#ifdef MT7687_BORA00006495
#undef MT7687_BORA00006495
#endif
#define MT7687_BORA00006495				1


/*Country Code display fix in in Beacon Country IE*/
/*Patch Size : 16 Bytes*/
#define MT7687_BORA00007277				1

/*Country Code Configuration*/
/* patch size: 840 Bytes */
#undef MT7687_BORA00007497
#define MT7687_BORA00007497 						1

//Patch Optimizations
/*patch size : reduced Code size by 10276 bytes*/
#define MT7687_BORA00007544             1

/*Country Code Configuration - Channellist to TxPower mapping*/
/*Patch Size 232 bytes*/
#define MT7687_BORA00007879 						0

/*MT7682 Extended Inband command expansion*/
/*Patch Size 40 bytes*/
#define MT7682_Ext_Inband_Expansion 						1
/* low Rx Throughput in power save */
/*  Patch Size  256Bytes*/
	/*Enable Later related to Low power throughput*/
#undef MT7687_BORA00005202
#define MT7687_BORA00005202						1

/* Issue :[Homekit][N9] Lost response from N9 with switch opmode repeatedly */
/*Patch Size  0Bytes*/
#undef MT7687_BORA00005852
#define MT7687_BORA00005852						1

/*Patch Size 1700  Bytes*/
#define MT7687_BORA00008227					1
/* New feature: Tx Raw 802.11 packet */
/* Patch Size :  520 bytes */
#undef MT7687_BORA00005865
#define MT7687_BORA00005865                            1

/*Won't add to SCAN Table if SSID is null*/
/*patch Size :  0Bytes*/
/* Anil patch disabled as IOT_CFG80211_SUPPORT flag is disabled in ROM */
#undef MT7687_BORA00007121
#define MT7687_BORA00007121				0

/*Mixed mode fix for STA after BSSID Channel Cmd*/
/*patch Size :  480 Bytes*/
#undef MT7687_BORA00007195
#define MT7687_BORA00007195				1

/*40Mhz Link-up fail after BSSID Channel Cmd*/
/*pacth Size : 72  Bytes*/
#undef MT7687_BORA00007212
#define MT7687_BORA00007212			 1


/* STA fail to connect AP in hidden-ssid mode */
/*Patch Size:  68 Bytes */
#undef MT7687_BORA00007280
#define MT7687_BORA00007280            1

/* Smart connection Regression Failure Fix */
/*Patch Size: 260 Bytes */
#undef MT7687_BORA00007317
#define MT7687_BORA00007317			1

/* Send probe req with BSSID sent during scan */
/*Patch Size:   1072 Bytes */
#undef MT7687_BORA00007445
#define MT7687_BORA00007445 		1
/* WiFi command event implementation */
/*Patch Size  356 Bytes*/

#undef MT7687_BORA00004905
#define MT7687_BORA00004905						1

/*Beacon Loss Time configuration and when beacon loss occurs
the connection will be maintained if data traffic exists*/
/*pacth Size :  152Bytes*/
#define MT7687_BORA00007132				1

/*TGn 4.2.10 APUT sending continuous MC data even if TIB =0*/
/*Patch Size :408B*/
/*AP Mode Will be enabled once AP Mode is done*/
#ifdef MT7687_BORA00005322
#undef MT7687_BORA00005322
#endif
#define MT7687_BORA00005322                     1

/* Add PS POLL handling in AP mode  */
/* Patch Size :1120 bytes*/
#ifdef MT7687_BORA00005053
#undef MT7687_BORA00005053
#endif
#define MT7687_BORA00005053							1

/*TGn 4.2.47 failing. APUT sends more than 1 packet for PS POLL*/
/*Patch Size: 1324 Bytes*/
#ifdef MT7687_BORA00005209
#undef MT7687_BORA00005209
#endif
#define MT7687_BORA00005209							1

/* AP(20MHz, channel 6) asserts when receiving coex-2040 mgmt frame from associated STA */
/* PATCH SIZE :  144 bytes */
#ifdef MT7687_BORA00005270
#undef MT7687_BORA00005270
#endif
#define MT7687_BORA00005270							1

/* Enabled Tbtt interrupt in AP mode */
/* Sending Broadcast Packets on Tbtt instead of Pre TBtt in AP mode*/
/*Patch Size: 604 Bytes */
#ifdef MT7687_BORA00004881
#undef MT7687_BORA00004881
#endif
#define MT7687_BORA00004881 						1

/*Drop class3 frames from un-authenticated stations*/
/*Patch Size: 56 Bytes */
#define MT7687_BORA00007483 						1

/*Rate Adaptation */
/*Patch Size: 10.83KB*/
/*RA_CTRL defined in module.mk*/

/* disable hexdump from wtbl inband cmd prints */
/* patch size: 296 Bytes */
#define HAL_WTBL1_HEXDUMP_DISABLE					1

/*MT7682 Fix for 40 Mhz*/
/* patch size: 16 Bytes */
#define MT7682_BORA00008292					1

/* management pkt filter */
/* patch size: 556bytes */
/*Feature enable Later*/
#ifdef MT7687_BORA00006676
#undef MT7687_BORA00006676
#endif
#define MT7687_BORA00006676							1
/*Set bandwidth inband command fix*/
/* patch size: 136 Bytes */
#define MT7682_BORA00008220                         1

#ifdef MT7687_BORA00006736
#undef MT7687_BORA00006736
#endif
/*Patch Size  8 bytes*/
#define MT7687_BORA00006736				1

/*Support for get STA List*/
/*Patch Size  584 bytes*/
#define MT7687_BORA00008232				1

/*auto connect feature*/
/*Patch Size  56*/
#define MT7687_BORA00008271				0

//DUT connect AP successfully, when AP change security, DUT happen crash.
//SQC issue  WSAP00016818
//Patch Size=32 bytes
#define MT7687_BORA00007992							1


/* BSS Co-exist2040 support in AP mode */
/* PATCH SIZE : 3268 bytes */
#ifdef MT7687_BORA00005151
#undef MT7687_BORA00005151
#endif
#define MT7687_BORA00005151     1

/*Fix _config_disconnect_sta: STA disconnection Fail */
/* PATCH SIZE : 16 bytes */
#define MT7687_BORA00008293     1


/*Low Power Patch Size  2856 bytes*/
#define MT7687_BORA00008228				1

#if (MT7687_BORA00008228 == 1)
#ifdef MT7687_BORA00004984
#undef MT7687_BORA00004984
#endif
#define MT7687_BORA00004984 					1

#ifdef MT7687_BORA00005308
#undef MT7687_BORA00005308
#endif
#define MT7687_BORA00005308			1
#endif

/*MAC CR Access in HAL Functions VIA Inband commands Read/Write*/
/*Patch Size  7148 Bytes*/
#define MT7687_BORA00008296							0

/*Fix for Smart connection assert*/
/* PATCH SIZE : -16 bytes */
#define MT7687_BORA00008312 	1

/*[mt7682 STA mode]boot up in station mode, the system has more than 50% possibility  stopped after inband BSSID command with log "MlmeAux.CentralCh = 11"*/
/*patch Size 48 bytes*/
#define WSAP00020506			1

/*[MT7682 STA mode]some wifi event handler can't work*/
/*patch size 208 bytes*/
#define WSAP00020512 			1
/*Enable Interrupt based support*/
/*Size 750b*/
#define INTERRUPT_BASED_SUPPORT	1

/*CM4 hangs when switch mode from STA to AP if STA is connected.*/
/*Size 32b*/
#define MT7682_WSAP00020526			1

/*Add FW Reload Support*/
/*size: 656b*/
#define MT7682_WSAP00020527			1

#define N9_DELAY 			1

/* BA not updated in wtbl2*/
/* size: 320 bytes */
#define MT7682_WSAP00020534 	1
/*Low Power Enter to sleep after 200ms of TX/RX*/
/*PATCH Size 136 Bytes*/
#define MT7687_BORA00008058		1

/*Tx power values are taken from Flash Addr*/
/*Patch Size: 72b*/
#define MT7682_WSAP00020532			1


/*AP Functionality not proper*/
/*Size = 64b*/
#define MT7682_WSAP00020539			1
/*[WSAP00020009][[Wifi Task][MT7682][MT7686]TX AC Queue mapping correction*/
/*Patch Size: 24b*/
#define MT7682_WSAP00020009			1

/* [MT7682/86 WifiTask] BA Reorder queue should be flushed when system ran out of pbuf's */
/* patch size: 160 bytes */
#define MT7682_WSAP00020553		1

/*fix for only 20Mhz rates after 40Mhz connection*/
/*	Patch Size 60Bytes */
#define MT7682_WSAP00020564				1

/*Tx power API Support*/
/*	Patch Size 288Bytes */
#define MT7687_WSAP00020513			    1

/*Power save BUg Bix*/
/*Patch Size 0 bytes*/
#ifdef MT7687_BORA00006507
#undef MT7687_BORA00006507
#endif
#define MT7687_BORA00006507				1


/*[MT7682] Low power LP Ownership transfer and Inband command*/
/*Patch Size   1040Bytes*/
#define MT7687_BORA00008519				1

#ifdef MT5932_SINGLE_CONTEXT
/* [MT7682 AP]Fix for intrabss tcp fail */
/* Patch size 496 Bytes */
#define MT7687_WSAP00020857				1
#endif
/*send wireless event to Upper layer if auth timeout or assoc timeout*/
/*Patch size  128 bytes*/
#define MT7687_BORA00008636				1

/*Increase the BAR Retry count to FIx the TCP THroughput drop isse with INTEL N7260 STATION IN AP  Mode*/
/*Patch Size 48 Bytes*/
#define MT7687_WSAP00021419				1

/*[MT7682] Send Beacons using direct PSE access*/
/*Patch Size   592Bytes*/
#define MT7682_WSAP00021568				1

/*Fix for 40Mhz WPA2PSK Fail in STA Mode*/
/*Patch Size  16Bytes*/
#define MT7682_WSAP00021598				1
/*Partial Scan feature*/
/*Patch Size 3328 bytes*/
#ifdef MT7687_BORA00004843
#undef MT7687_BORA00004843
#endif
#define MT7687_BORA00004843				1

/* SmartConnect support in SoftAP mode */
/*Patch Size 616 bytes*/
#ifdef MT7687_BORA00005974
#undef MT7687_BORA00005974
#endif
#define MT7687_BORA00005974				1

//Amazon release feature DTIM skip factor
/*Patch Size  72 bytes*/
#define MT7687_BORA00007979								1


/*remove the security when sta link down*/
/*Patch Size  80  bytes*/
#define MT7682_WSAP00023002  0
/* Support to block wfw task in power-save mode  */
/* Kernel flags enabled: configUSE_QUEUE_SETS */
/* Patch Size: 672 bytes (Patch size + new os code  due to flags) */
#define MT7682_WSAP00022987				1

/*shared wep support in ap mode*/
/*patch size 1320 bytes*/
#define MT7682_WSAP00022988  1

/*Add locks for CM4 low power Assert*/
/*Patch Size  60  bytes*/
#define MT7682_WSAP00023077  1


/*Add changes in WifiTask for GDMA related changes.*/
/*Patch Size : 496b*/
#define MT7682_WSAP00020997 	1

/*Register EINT interrupt*/
/*Patch Size  56 bytes*/
#define MT7682_WSAP00023897  1

/*Disble MAC Rx using Inband command*/
/*Patch Size 80 bytes*/
#define MT7682_WSAP00024036			1


/*Disable a band in country region*/
/*Size: -64b*/
#define MT7682_WSAP00023851 		1


/*Assertion inside PeerAddBAReqAction*/
/*Size 72bytes*/
#define MT7682_WSAP00023928		1


/*RxFilter value shown as 0 when STA in powersave*/
/*Size:88b*/
#define MT7682_WSAP00024154  1


/*fix the issue that mc bc data frame will affect connection*/
/*Size: 288bytes*/
#define MT7682_WSAP00024104 1

/*Get AP Radio state through inband command*/
/*Size 24b*/
#define MT7682_WSAP00024358		1

/*Fix crash in OpMode Change*/
/*Size : 8b*/
#define MT7682_WSAP00024419	1

/*bc frame will affect getting the ip*/
/*Size : 20bytes*/
#define MT7682_WSAP00024415 0

/*Add Inband query Statistic Support */
/*pacth Size :  424Bytes*/
#ifdef MT7682_WSAP00033668
#undef MT7682_WSAP00033668
#endif
#define MT7682_WSAP00033668             1

/*Changing Powersave Periodic Exec timing to 10 sec */
/*Size bytes 440 bytes*/
#define MT7682_WSAP00024125  1

/*Avoid Waiting for Beacon*/
/*patch Size :  544 Bytes*/
#define MT7682_WSAP00027933				1

/*Enable IP Packet filter to Drop Broadcast ARP for other devices*/
/*Patch Size 832 bytes*/
#define MT7682_WSAP00028247				1

/*Crash fix for MT7686*/
/*Patch Size 312 bytes*/
#define MT7686_WSAP00028628				1

/*Print timetaken from bootup to first packet*/
/*Patch Size 112 bytes*/
#define MT7682_WSAP00029427				1

/*retransmit for eapol key*/
/*patch size  bytes*/
#define MT7682_WSAP00029144  1

/*wsc support*/
#define MT7682_WSAP00029590 1
/*Wait for N9 wake up before Enque to MAC*/
/*Patch Size 60 Bytes*/
#define MT7682_WSAP00028366 			1

/*Wake up if in sleep before channel switch in case of fallback*/
/*Patch Size 32 bytes*/
#define MT7682_WSAP00028381				1

/*40Mhz Beacon issue on Reload*/
/*patch size  0 bytes*/
#define MT7682_WSAP00029574  1

/*disable wifi task auto reconnect, this should be triggered by upplayer*/
/*patch size  0 bytes*/
#define MT7682_WSAP00029619 1

/*Mpu init crash for MT7686 and MT7682*/
/*Patch size 1008 Bytes*/
#define MT7682_WSAP00030353 		1

/*not send linkdown event when first boot up*/
/*Patch size 10 Bytes*/
#define MT7682_WSAP00030414  1


/*add debug log for wtbl*/
/*Patch size 0 Bytes*/
#define MT7682_WSAP00030969  0

/*Disable LDPC - Not supported*/
/*Patch size  32Bytes*/
#define MT7682_WSAP00030970	1


/*TCP Crash*/
/* Patch Size 0 Bytes*/
#define MT7682_WSAP00030986		1

/*Fix issues in Legacy powersave.*/
/*Size : 0b*/
#define MT7682_WSAP00030486		1


/*reload Crash*/
/* Patch Size 0 Bytes*/
#define MT7686_WSAP00031017 1

/*insert the bss to the scan table by rssi*/
/* Patch Size 0 Bytes*/
#define MT7682_WSAP00031035 1

/*Management/action Frame handling in PSMode 2 */
/*Patch Size    136 Bytes*/
#define MT7682_WSAP00030356 			1
/*AP Mode crash while opmode switch*/
/*patch size  4 Bytes*/
#define MT7682_WSAP00030993				1

/*Action frame handing in PSMode 2*/
/*Patch Size 8 Bytes*/
#define MT7682_WSAP00031159				1

/*CLI hang when change to AP mode from STA in PSMode 2*/
/*Size : 368b*/
#define MT7682_WSAP00031081  						0

/*APUT Crash on TCP Bidirectional data*/
/*Patch size 8 Bytes*/
#define MT7682_WSAP00031201							1

/*Overnight Crash issue fix (DLINK DIR 655)*/
/*Patch size 128 Bytes*/
#define MT7682_WSAP00031072							1

/*Wake up on service period expire*/
/*Patch Size  -16Bytes*/
#define MT7682_WSAP00031032							1

/*Set PSMode Dynamic*/
/*Patch Size  144Bytes*/
#define MT7682_WSAP00031776 						1

/* DUT connect to AP, AP switch CH1~13 ,DUT assert */
/*Patch Size 132 Bytes*/
#define MT7686_WSAP00031080                         1

/* Timeout/Crash issue fix */
/*Patch Size  224Bytes*/
#define MT7682_WSAP00031029			1

#ifdef MTK_WIFI_REPEATER_ENABLE 

/* APCLI Related fixes for SQC*/
/* Patch size: 4.9KB */
/*APCLI  Mode Disable*/
#ifdef MT7687_BORA00005562
#undef MT7687_BORA00005562
#endif
#define MT7687_BORA00005562			1

/*Dual Interface related changes */
/* Patch Size : 452KB */
#ifdef MT7687_BORA00005606
#undef MT7687_BORA00005606
#endif
#define MT7687_BORA00005606 			1

/*APCLI security related changes*/
/* Patch Size : 600bytes*/
	/*APCLI Not Present*/
#ifdef MT7687_BORA00005874
#undef MT7687_BORA00005874
#endif
#define MT7687_BORA00005874				1


/* Security is mixed mode, DUT APCLI some case connect will FAIL */
/* Patch Size : 436bytes*/
/*APCLI Not Present*/
#ifdef MT7687_BORA00005933
#undef MT7687_BORA00005933
#endif
#define MT7687_BORA00005933				1


/*APLCI LINK STATUS*/
/* Patch Size 28bytes*/
/*APCLI Not Present */
#ifdef MT7687_BORA00006432
#undef MT7687_BORA00006432
#endif

#define MT7687_BORA00006432				1

/*APCLI LINK DOWN EVENT to CM4 enable*/
/*pacth Size :  52Bytes*/
	/*APCLI Not needed*/
#ifdef MT7687_BORA00006502
#undef MT7687_BORA00006502
#endif
#define MT7687_BORA00006502				1

/* In APCLI Mode Correct the Max Num supported STA cmd*/
/*Patch Size: 16Bytes */
#define MT7687_BORA00007301			1

/*APCLI 40Mhz Auto up/below Selection*/
/*patch Size :  468Bytes*/
#define MT7687_BORA00007468				1

/*APCLI ALL Channel Scan Support*/
/*patch Size :  1064 Bytes*/
#define MT7687_BORA00007469				1

//[MT7697][Repeater mode][N9 Memory][WIFI Function] RootAP set WPA-PSK TKIP or WPA-PSK AES+TKIP, After DUT start connect RootAP and send beacon, always happen crash.
//SQC issue WSAP00016923
//Patch size =4 bytes
#define MT7687_BORA00008042							1
/*[MT7687][Repeater][SDK][Letv]If 7687 connected 8 STA in repeater mode, it's popup "password error" when the 9th STA (the IPhone device)connecting.*/
/*Patch Size 8 Bytes*/
#define MT7687_BORA00008365                         1

#define MT7686_WSAP00031481				1

/*send beaocn to host during scan under repeater mode*/
#define MT7682_WSAP00033157  1

/*apcli wep support*/
#define MT7682_WSAP00033313 1

/*handle APMDU eapol key under repeater mode*/
/*Patch size 152 Bytes*/
#define MT7682_WSAP00033437  1
#endif

/*fix crash when send ap start comand to host*/
#define MT7682_WSAP00033302 1

/*APCLI Opmode Switch Crash Fix + APCLI All channel scan flag reset during linkup*/
/*Patch Size 296Bytes*/
#define MT7682_WSAP00033426 1


/*[7682][Wifi-Task]1st TX packet optimization : Remove extra prints*/
/*Patch Size -48 Bytes*/
#define MT7682_WSAP00033453 1


/*AP mode crash at bootup dur to NULL pointer accessing*/
/*Patch size 10Bytes*/
#define MT7682_WSAP00033571 				1

#ifdef MTK_WIFI_REPEATER_ENABLE
/*Fix APCLI connection issues*/
/*Patch size 416Bytes*/
#define MT7682_WSAP00033569	 1
#endif

/*reset mic error count to zero after 60seconds*/
/* Patch Size 14 Bytes*/
#define MT7682_WSAP00033025 1

/* [7682][Wifi-Task] Add Apple IE to Beacon  */
/*Patch Size  56Bytes*/
#define MT7682_WSAP00033687  1

/*black list support*/
/*Patch Size 62 Bytes*/
#define MT7682_WSAP00035445 1

/*cippher error report to upper layer*/
#define MT7682_WSAP00035446 1

/* Crash Fix for Rx packet data handling during linkdown */
/*Patch Size  16Bytes*/
#define MT7682_WSAP00033582  1

/*TCP Crash issue Fix when AP is doing fragmentation*/
/*Patch Size--16 Bytes*/
#define MT7682_WSAP00035625		1

/*IP-ARP Filter Enabled*/
/*Patch Size  112Bytes*/
#define MT7682_WSAP00036065		1

/*Fix for [Crash] wfw: ASSERT get_regulatory_class */
/*Patch Size  184Bytes*/
#define MT7682_WSAP00036054		1

/*Fix fot no IP in APCLI Mode for Iphone */
/*Patch Size  32Bytes*/
#define MT7682_WSAP00036240		1

/*[MT7686 E2] [Wifi task] Add Assert when WTBL not working */
/*Patch Size  8Bytes*/
#define MT7682_WSAP00037219		1

/*Fix ASSERT issue during Opmode Change */
/*Patch Size  Bytes - Update later*/
#define MT7682_WSAP00037040		1

/*To stop scan if a Connection command is received*/
/*Patch Size  Bytes - 24Bytes*/
#define MT7682_WSAP00039149		1

/*WMM feature for 11b mode*/
/*Patch Size 192 Bytes*/
#define MT7682_WSAP00021643		1

#ifdef MTK_WIFI_REPEATER_ENABLE
/*Avoid Base chanel scan if connection fails*/
/*Patch Size  64Bytes*/
#define MT7682_WSAP00039609		1
#endif

/*Fix for Broadlink Crash issue*/
/*Patch Size -8 Bytes*/
#define MT7682_WSAP00039186		1

/*Coverity Scan Fixes*/
/*Patch Size  Bytes 0*/
#define MT7682_WSAP00040306		1
/*Patch Size  Bytes 32*/
#define MT7682_WSAP00040327		1
/*Patch Size  Bytes 0*/
#define MT7682_WSAP00040332		1
/*Patch Size  Bytes 16*/
#define MT7682_WSAP00040344		1
/*Patch Size  Bytes 0*/
#define MT7682_WSAP00040303 	1

/* Adding Rx Dscr in Mgmt pkt filter */
/* patch Size 208 Bytes */
#define MT7682_WSAP00040706     1

/*Fix for AMSDU Throughput drop*/
/*Patch Size 52 Bytes*/
#define MT7682_WSAP00040507		1

/*Coverity scan fix -Leak*/
/*Patch Size  Bytes 8*/
#define MT7682_WSAP00040334		1

/* Temperature Recalibration TX-RX Handling */
/*Patch Size 384 Bytes*/
#define MT7682_WSAP00040931     0


#ifdef MTK_WIFI_WPS_ENABLE
/*receive wsc done after disassoc*/
/*Patch Size 62 Bytes*/
#define MT7682_WSAP00040971  1

/*large wsc m2 msg dropped by mlme */
/*Patch Size 1856 Bytes*/
#define MT7682_WSAP00040492  1
#endif

/*Increase the Driver own wait time */
/*Patch Size  Bytes 0*/
#define MT7682_WSAP00041423	 1

/*Fix for UDP throughput display results  */
/*Patch Size  56 Bytes */
#define MT7682_WSAP00041741	 1

/*Support BWCS config */
/*Patch Size  184 Bytes */
#define MT7682_WSAP00062784 1

/*KRACK KeyReplay Attack avoidance for WPA2 , to fix issue in Group Key Update*/
/*Patch size 392bytes*/
#define MT7687_BORA00011572					1

/*Fix for Broadlink crash while receiving MGMT packet of 511 bytes */
/* patch Size  208Bytes */
#define MT7682_WSAP00061676     1


/* Fix for [Coverity scan] CID: 186609, 175845*/
/*file: middleware/MTK/wifi_stack/src/patch_e2/rom_patch.c*/
/*patch Size 0 bytes*/
#define MT7682_WSAP00066924		1


/*Fix for SDK Build Warning During Code Inspection*/
//WSAP00063145,WSAP00063146
/*patch Size 0 bytes*/
#define MT7682_WSAP00063145		1

/* Ali SDS System Crash while reboot */
/* patch Size  88Bytes */
#define MT7682_WSAP00067385     1


/* Send ap stop inband twice cause memory leak	     */
/*Send Opmode switch just after AP stop , cause crash*/
/* patch Size  240 Bytes */
#define MT7682_WSAP00070467  1

//[MT7682][AIT/Quadrant] Power Save issue :Long duration wake up with TP link AP 
//Size 464 bytes
#define MT7682_WSAP00070254					1

/*Remove while loop on lp flag as it was causing the task to hang */
/*patch size 1416 bytes ->this size is inclusive of the Free RTOS API addition for group wait*/
#define MT7682_WSAP00062869					1


/*Add Flow control retry logic*/
//1288bytes -> this size is inclusive of the Free RTOS API addition for group wait
#define MT7682_WSAP00069512					1

/*Add inband command and Event to upper layer */
//248bytes
#define MT7682_WSAP00069397		     		        1


/*always enable rx vector group3 */
/*patch size   32 bytes*/
#define MT7682_WSAP00070999  1

/*fail at join wait beacon if ap not send beacon */
/*patch size   24 bytes*/
#define MT7682_WSAP00071220  0

/*rsnie pairwise cipher not same as ap */
/*patch size    744 bytes*/
#define MT7682_WSAP00071303  1


/*New Inband Cmd support for system sleep control */
/*Patch Size  Bytes 224 Bytes*/
#define MT7682_WSAP00043052	 1


/*fix get  the  wrong txpower by txpower inband */
/*Patch Size  4 Bytes*/
#define MT7682_WSAP00073584 1


/*Avoid setting frequency offset as suggested by DE */
/*Patch Size  -72 Bytes*/
#define MT7682_WSAP00074091			1


/* the large packet sent to the upper layer by frame fitler is not correct */
/*Patch size : 60 bytes*/
#define MT7682_WSAP00074061		1


/* __RTMPSetTimer can not change timer period correctly  */
/*Patch size : 4 bytes*/
#define MT7682_WSAP00074255		1

/*Configure the Scan dwell time using inband command*/
/*Patch size  144 bytes*/
#define MT7687_BORA00008760			1

/*Reset PN number when install pairwise key*/
/*Patch size  120 bytes*/
#define MT7682_WSAP00074317  1

/*scan dwell time support in AP mode*/
/*Patch size  16 bytes*/
#define MT7682_WSAP00074453  1


/*Add support for single SKU in 2.4GHz*/
//Size 484 bytes
#define MT7687_BORA00007905							1

/*Configure the TX BW Window size via NVRAM change RX WinSize max as 4*/
/* Patch Size   1136*/
#define MT7682_WSAP00074890	1


/*use defalut power per rate table when txPowr.bin not been written to flash*/
/* Patch Size   25bytes*/
#define MT7682_WSAP00075685 1


/*cli for change different rate*/
/* Patch Size  112 bytes*/
#define MT7682_WSAP00076044 1

/*when start ap and reload ap , do not allow other comand send to wifi task */
/* patch Size  264Bytes */
#define MT7682_WSAP00075683 1


/*Take the default value of Coex mgmt frame support as seen in beacon */
/*Patch size : 16 bytes*/
#define MT7682_WSAP00073712			1

/*ignore enter low power while under foce wake up flow*/
/*Patch size : 96 bytes*/

#define MT7682_WSAP00079101  1


//system crash after connect to Wi-Fi AP with secure as WEP.
//SQC issue  WSAP00078403
//Patch Size=640 bytes
#define MT7682_WSAP00078403                         1

//[IAR]DUT dump while connect WPA encrypted wifi ap
//SQC issue  WSAP00078403
//Patch Size=1024 bytes
#define MT7682_WSAP00078726                         1


#ifdef MTK_WIFI_REPEATER_ENABLE
/*discard ap port connection if apcli not conencted under repeater mode*/
/*Patch size : 112 bytes*/
#define MT7682_WSAP00079114 1
#endif

/*when central channel not match the AP's, reset scan table and connect again*/
/*Patch size : 336 bytes*/
#define MT7682_WSAP00079625 1


/*[WSAP00079102] [SDK4.7.0][MT7682]Always ping fail after set PSMODE = 2*/
#define MT7682_WSAP00079102   1


/*[GVA-1449] [FreeRTOS_V10][MT7682]System crash while auto test*/
#define MT7682_GVA1449     1

/*after switch opmode ,sometimes occur cipper error*/
/*Patch size : 72 bytes*/
#define MT7682_GVA_568  1

/*TPLINKSP-43 if ap in 40M, sta 20M only, sta SKU not meeting expectations*/
/*Patch size : 72 bytes*/
#define MT7682_GVA_2074  1

/*Add DPD enable control in scan and connect stage*/
/*Patch size : */
#define MT7682_GVA_1783  1

/*AP mode unicast forward will sometime cause assert*/
/*Patch size : */
#define MT7682_GVA_3490  1

/*AP mode: the max tx power invalid of set country code cmd*/
/*Patch 70 bytes : */
#define MT7682_GVA_4315  1

/*system dump when disconnect state tx data*/
/*Patch 200 bytes : */
#define MT7682_GVA_4381  1

/*system dump when stability test*/
/*Patch 10 bytes : */
#define MT7682_GVA_4382  1

/*for single rate test tool*/
#define MT7682_GVA_4386  1

/*Dequeue fail when received one more out of order packet*/
#define AW7698_GVA_8779	 1

/*Reset RFCR which set after wifi connected*/
#define MT7682_GVA_9453  1

/*Add NULL check for mpdu_blk */
#define AW7698_GVA_10551 1

/**for solution udp rx issue,  rootcuase is beacon country code IE's tx power=0*/
#define MT7686_GVA_12033 1

/* Disable 802.11e in STA mode */
#define MT7686_GVA_13441 1

/* Disable 802.11e in AP mode */
#define MT7686_GVA_13445 1

#endif/*_PATCH_CONFIG_RAM_H*/
