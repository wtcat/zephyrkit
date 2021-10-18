/* This sample code teaches developer how to operate the MEMSIC alogrithm library
 * to get Yaw, Pitch and Roll respectively. if developer want to use it as a part
 * of his codes, he need to declare and define 3 functions by himself:
 * ReadPara;
 * Savepara;
 * please NOTE it is ONLY a sample code and can be changed freely by developers.
 */
#include "MemsicAlgo.h"
#include "MemsicCompass.h"

#define A_LAYOUT	0
#define M_LAYOUT	0




/* These variables are used to save the magnetic sensor calibrated data. */	
float calMagX, calMagY, calMagZ;	

/* These variables are used to save the calibrated orientation output. */	
float fAzimuth, fPitch, fRoll;

/* This variable is used to save the calibration accuracy. */	
int iAccuracy;



/**
 * @brief Convert the sensor coordinate to right-front-up coordinate system;
 */
void acc_coordinate_raw_to_real(int layout, float *in, float *out);

/**
 * @brief Convert the sensor coordinate to right-front-up coordinate system;
 */
void mag_coordinate_raw_to_real(int layout, float *in, float *out);



void mag_coordinate_raw_to_real(int layout, float *in, float *out)
{
    if ((!out) || (!in)) {
		    return;
    }

    switch (layout) {
        case 0:
            //x'=-y y'=x z'=z
            out[0] = -in[1];
            out[1] = in[0];
            out[2] = in[2];
            break;
        case 1:
            //x'=x y'=y z'=z
            out[0] = in[0];
            out[1] = in[1];
            out[2] = in[2];
            break;
        case 2:
            //x'=y y'=-x z'=z
            out[0] = in[1];
            out[1] = -in[0];
            out[2] = in[2];
            break;
        case 3:
            //x'=-x y'=-y z'=z
            out[0] = -in[0];
            out[1] = -in[1];
            out[2] = in[2];
            break;
        case 4:
            //x'=y y'=x z'=-z
            out[0] = in[1];
            out[1] = in[0];
            out[2] = -in[2];
            break;
        case 5:
            //x'=x y'=-y z'=-z
            out[0] = in[0];
            out[1] =  -in[1];
            out[2] = -in[2];
            break;
        case 6:
            //x'=-y y'=-x z'=-z
            out[0] = -in[1];
            out[1] = -in[0];
            out[2] = -in[2];
            break;
        case 7:
            //x'=-x y'=y z'=-z
            out[0] = -in[0];
            out[1] = in[1];
            out[2] = -in[2];
            break;
        default:
		        //x'=x y'=y z'=z
		        out[0] = in[0];
		        out[1] = in[1];
		        out[2] = in[2];
		        break;
    }
	
}


/*******************************************************************************
* Function Name  : main
* Description    : the main function
*******************************************************************************/
int main(void)
{
	int i;
	
	/* This variable is used to store the accelerometer and magnetometer raw data.
	 * please NOTE that the data is going to store here MUST has been transmitted 
	 * to match the Right-Handed coordinate sytem already.
	 */
	float acc_raw_data[3] = {0.0f};	//accelerometer field vector, unit is g

    float mag_raw_out[3]={0.0,0.0,0.0};///unit is gauss

	float ori[3] = {0.0f};	
	static int algoRst = 1;

	
	 float fMagSP[9] = {1.0,0.0,0.0,
	0.0,1.0,0.0,
	0.0,0.0,1.0};


    float pa[8] ={0.0f};
    float pb[4] ={0.0f};
    int   pc[4] ={0};
	
    pa[0] = 18;		//FirstLevel	= can not be more than 15
    pa[1] = 25;		//SecondLevel	= can not be more than 35
    pa[2] = 35;		//ThirdLevel	= can not be more than 5
 	pa[3] = 0.025;	//dCalPDistLOne	
	pa[4] = 0.08;	//dCalPDistLTwo
    pa[5] = 0.85;   //dPointToCenterLow
    pa[6] = 1.15;	//dPointToCenterHigh dd
    pa[7] = 1.4;	//dMaxPiontsDistance

    pb[0] = 0.1;	//dSatAccVar 	= threshold for saturation judgment
    pb[1] = 0.00002;//dSatMagVar 	= threshold for saturation judgment
    pb[2] = 0.0002;	//dMovAccVar 	= threshold for move judgment
    pb[3] = 0.0001;	//dMovMagVar 	= threshold for move judgment

    pc[0] = 20;		//delay_ms 		= sampling interval	
    pc[1] = 15;		//yawFirstLen	= can not be more than 31, must be odd number.
    pc[2] = 10;		//yawFirstLen	= can not be more than 30
    pc[3] = 2;		//corMagLen		= can not be more than 20
	

    AlgoInitial(fMagSP,pa,pb,pc);
	
	while(1)
	{
		/* get the acc raw data, unit is g*/
		acc_raw_data[0] = ;		
		acc_raw_data[1] = ;	
		acc_raw_data[2] = ;		
				
		/* get the mag raw data, unit is gauss */	
		mag_raw_data[0] = ;		
		mag_raw_data[1] = ;	
		mag_raw_data[2] = ;	
		
		printf("befor input algo acc_raw %f %f %f \r\n",acc_raw_data[0],acc_raw_data[1],acc_raw_data[2]);
		printf("befor input algo mag_raw %f %f %f \r\n",mag_raw_data[0],mag_raw_data[1],mag_raw_data[2]);
	
		layout=1 //地磁安装方向 可以选择0-7 根据实际方向来调整，不知道选择哪个方向时，可以一个个来尝试，如果方向对了，fAzimuth为指南北时为0度
		mag_coordinate_raw_to_real(layout,mag_raw_data,mag_raw_out);//
		MainAlgorithmProcess(acc_raw_data,mag_raw_out,mag_raw_out,algoRst,0);
		algoRst = 0;

		/* get corrected mag data */
		GetCalMag(caliMag);
		
		/* calibrated magnetic sensor output */
		calMagX = caliMag[0];	//unit is gauss
		calMagY = caliMag[1];	//unit is gauss
		calMagZ = caliMag[2];	//unit is gauss
		
		
		/* get corrected ori data */
		GetCalOri(ori);
		
		/* Get the fAzimuth Pitch Roll Accuracy for the eCompass */
		fAzimuth  = ori[0];
		fPitch    = ori[1];
		fRoll     = ori[2];	
	

  
	
	}
}