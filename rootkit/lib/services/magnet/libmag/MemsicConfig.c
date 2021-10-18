/*****************************************************************************
* Copyright (C), 2016, MEMSIC Inc.
* File Name		: MemsicConfig.c
* Author		: MEMSIC Inc.		
* Version		: 1.0	Data: 2015/06/18
* Description	: This file is the configuration file for MEMSIC algorithm. 
* History		: 1.Data		: 
* 			  	  2.Author		: 
*			      3.Modification: By Guopu on 2015/08/06	
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "MemsicConfig.h"
#include <string.h>
#include <unistd.h>

#define SAVE_CAL_DATA	1
#if SAVE_CAL_DATA
#include <fcntl.h>
//#include <utils/Log.h>
#define NVM_SIZE		1024 
#define NVM_PATH		"/data/misc/sensor/magpara"  

#endif
static float fMagSP[9] = {1.0f,0.0f,0.0f, 0.0f,1.0f,0.0f, 0.0f,0.0f,1.0f};                         
		
#if SAVE_CAL_DATA
int ReadDataFromFile(float *pa);
int SaveDataInFile(float *pa, float *pb);
#endif
								
/*******************************************************************************************
* Function Name	: SetMagSensorPara
* Description	: Set the magnetic sensor parameter.
********************************************************************************************/
int SetMagSensorPara(float *pa,float *pb)
{
	int i;	
	float magHP[4] = {0.0f,0.0f,0.0f,0.5f};
	
	for(i=0;i<9;i++){
		pa[i] = fMagSP[i];
	}

   // //ALOGE(" memsic save SetMagSensorPara  111111\n");
	#if SAVE_CAL_DATA
	if(ReadDataFromFile(magHP)<0){
		////ALOGE("memsic save memsicalgoinit config.c %s: fail to load the calibration file\n",__FUNCTION__);
	}
	#endif
	
	// //ALOGE("SetMagSensorPara  2222\n");
	for(i=0;i<4;i++){
		pb[i] = magHP[i];
		//printf("memsicalgoinit pb[%d] = %f\n",i,pb[i]);
	}
	
	return 0;
}

/*******************************************************************************************
* Function Name	: SaveMagSensorPara
* Description	: Save the magnetic sensor parameter.
********************************************************************************************/
int SaveMagSensorPara(float *pa,float *pb)
{
	
	 //  //ALOGE(" 20180824 memesic save  4444\n");
	#if SAVE_CAL_DATA
	if(SaveDataInFile(pa,pb)<0){
		////ALOGE(" 20180824 memsicalgo config.c %s: fail to save the calibration file\n",__FUNCTION__);
	}
	#endif
	
	return 1;
}

/*******************************************************************************************
* Function Name	: SetOriSensorPara
* Description	: Set the orientation sensor parameter
********************************************************************************************/
int SetOriSensorPara(float *pa, float *pb, int *pc)
{
	pa[0] = 10;		//FirstLevel	= can not be more than 15
	pa[1] = 18;		//SecondLevel	= can not be more than 35
	pa[2] = 30;		//ThirdLevel	= can not be more than 50
	pa[3] = 0.025;	//dCalPDistLOne	
	pa[4] = 0.07;	//dCalPDistLTwo	
	pa[5] = 0.85;   //dPointToCenterLow
	pa[6] = 1.15;	//dPointToCenterHigh
	pa[7] = 1.4;	//dMaxPiontsDistance

	pb[0] = 0.1;	//dSatAccVar 	= threshold for saturation judgment
	pb[1] = 0.00002;//dSatMagVar 	= threshold for saturation judgment
	pb[2] = 0.0002;	//dMovAccVar 	= threshold for move judgment
	pb[3] = 0.0001;	//dMovMagVar 	= threshold for move judgment

	pc[0] = 20;		//delay_ms 		= sampling interval	
	pc[1] = 15;		//yawFirstLen	= can not be more than 31, must be odd number.
	pc[2] = 10;		//yawFirstLen	= can not be more than 30
	pc[3] = 2;		//corMagLen		= can not be more than 20
	
	
	return 1;
}
/*****************************************************************
* Description:
*****************************************************************/
#if SAVE_CAL_DATA
int SaveDataInFile(float *pa, float *pb)
{
	int fd = -1;
	char buf[NVM_SIZE];
	
	   ////ALOGE("  20180824 memesic save  55555\n");

	fd = open(NVM_PATH, O_WRONLY | O_CREAT, 0666);
	if (fd < 0){
		////ALOGE(" 20180824 memsicpara config.c %s: fail to open %s\n", __FUNCTION__, NVM_PATH);
		return -1;
	}

	memset(buf, '\0', NVM_SIZE);
	sprintf(buf, "%f %f %f %f\r\n",pa[0], pa[1],pa[2],pb[0]);
	write(fd, buf, strlen(buf));
	close(fd);

	
	////ALOGE("20180824  memesic save  66666\n");
	////ALOGE("memsicpara config.c %s: save data %f\t %f\t %f\t %f\t\n", __FUNCTION__, pa[0], pa[1], pa[2],pb[0]);

	return fd;
}
#endif
/*****************************************************************
* Description:
*****************************************************************/
#if SAVE_CAL_DATA
int ReadDataFromFile(float *pa)
{
	int fd;
	int n;
	char buf[NVM_SIZE];

	fd = open(NVM_PATH, O_RDONLY);
	if (fd == -1){
	//	//ALOGE(" 20180824 memsicpara config.c %s: fail to open %s\n", __FUNCTION__, NVM_PATH);
		return -1;
	}

	n = read(fd, buf, NVM_SIZE);
	if (n <= 0){
	//	//ALOGE("20180824 memsicpara config.c %s: there is no data in file %s\n", __FUNCTION__, NVM_PATH);
		close(fd);
		return -1;
	}

	sscanf(buf, "%f %f %f %f\r\n",&pa[0], &pa[1], &pa[2],&pa[3]);
	close(fd);

	////ALOGE("memsicpara config.c %s: read data %f\t %f\t %f\t %f\t\n", __FUNCTION__, pa[0], pa[1], pa[2], pa[3]);

	return fd;
}
#endif
