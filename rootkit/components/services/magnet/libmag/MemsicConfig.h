/*****************************************************************************
* Copyright (C), 2016, MEMSIC Inc.
* File Name		: Config.h
* Author		: Yan Guopu		Version: 1.0	Data: 2014/12/11
* Description	: This file is the head file of config.c. It provides the 
* 			      interface function declarations of the lib. 
* History		: 1. Data			: 
* 			  	  2. Author			: 
*			  	  3. Modification	:	
*****************************************************************************/
#ifndef _MEMSICCONFIG_H_
#define _MEMSICCONFIG_H_

int SetMagSensorPara(float *pa,float *pb);

int SaveMagSensorPara(float *pa,float *pb);
/* Enable this flag to enable uImage operation */
#define ENABLE_UIMG_SUPPORT

// Enable the following macro to see debug out 
//#define DD_MMC_DEBUG

#ifdef DD_MMC_DEBUG

#ifndef ENABLE_UIMG_SUPPORT
#define DD_MMC_MSG_0(level,msg)          	 MSG(MSG_SSID_SNS,DBG_##level##_PRIO, "MMC - " msg)
#define DD_MMC_MSG_1(level,msg,p1)       	 MSG_1(MSG_SSID_SNS,DBG_##level##_PRIO, "MMC - " msg,p1)
#define DD_MMC_MSG_2(level,msg,p1,p2)    	 MSG_2(MSG_SSID_SNS,DBG_##level##_PRIO, "MMC - " msg,p1,p2)
#define DD_MMC_MSG_3(level,msg,p1,p2,p3)      MSG_3(MSG_SSID_SNS,DBG_##level##_PRIO, "MMC - " msg,p1,p2,p3)
#else 
#define DD_MMC_MSG_0(level,msg)          	 UMSG(MSG_SSID_SNS,DBG_##level##_PRIO, "MMC - " msg)
#define DD_MMC_MSG_1(level,msg,p1)       	 UMSG_1(MSG_SSID_SNS,DBG_##level##_PRIO, "MMC - " msg,p1)
#define DD_MMC_MSG_2(level,msg,p1,p2)    	 UMSG_2(MSG_SSID_SNS,DBG_##level##_PRIO, "MMC - " msg,p1,p2)
#define DD_MMC_MSG_3(level,msg,p1,p2,p3)	  UMSG_3(MSG_SSID_SNS,DBG_##level##_PRIO, "MMC - " msg,p1,p2,p3)
#endif /*End of if not*/

#else /* DD_ORI_DEBUG */
#define DD_MMC_MSG_0(level,msg)
#define DD_MMC_MSG_0(level,msg)
#define DD_MMC_MSG_1(level,msg,p1)
#define DD_MMC_MSG_2(level,msg,p1,p2)
#define DD_MMC_MSG_3(level,msg,p1,p2,p3)
#endif /* End of DD_ORI6_DEBUG */

#endif // _MEMSICCONFIG_H_
