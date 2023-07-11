#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"
#include <linux/regulator/consumer.h>

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(PFX fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)   pr_err(fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
                do {    \
                   pr_debug(PFX fmt, ##arg); \
                } while(0)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#define PK_XLOG_INFO(fmt, args...)
#endif

/*
#ifndef BOOL
typedef unsigned char BOOL;
#endif
*/

struct regulator *regVCAMA = NULL;
struct regulator *regVCAMD = NULL;
struct regulator *regVCAMIO = NULL;
struct regulator *regVCAMAF = NULL;
struct regulator *regVCAMD_SUB = NULL;






/* Mark: need to verify whether ISP_MCLK1_EN is required in here //Jessy @2014/06/04*/
extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);

//extern struct platform_device *camerahw_platform_device;
extern struct device *sensor_device;

//yinyapeng ==>
UsedSubCameraType   g_CurrUsedSubCameraName = SUB_NOCAMERA;
UsedMainCameraType  g_CurrUsedMainCameraName = MAIN_NOCAMERA;
//<== end


bool _hwPowerOn(char * powerId, int powerVolt, struct regulator **regVCAM){
    bool ret = false;
	struct regulator * temp = NULL;
		PK_DBG("[_hwPowerOn]before get, powerId:%s, regVCAM: %p\n", powerId, *regVCAM );
	if(*regVCAM == NULL)
		*regVCAM = regulator_get(sensor_device,powerId);
	temp = *regVCAM;

	PK_DBG("[_hwPowerOn]after get, powerId:%s, regVCAM: %p\n", powerId, temp );


 	if(!IS_ERR(temp)){
        if(powerId != CAMERA_POWER_VCAM_IO && regulator_set_voltage(temp , powerVolt,powerVolt)!=0 ){
            PK_DBG("[_hwPowerOn]fail to regulator_set_voltage, powerVolt:%d, powerID: %s\n", powerVolt, powerId);
        }
        if(regulator_enable(temp)!= 0) {
            PK_DBG("[_hwPowerOn]fail to regulator_enable, powerVolt:%d, powerID: %s\n", powerVolt, powerId);
            //regulator_put(regVCAM);
            //regVCAM = NULL;
            return ret;
        }
        ret = true;
    }else
   		 PK_DBG("[_hwPowerOn]IS_ERR_OR_NULL regVCAM %s\n",powerId);

    return ret;
}

bool _hwPowerDown(char * powerId, struct regulator **regVCAM){
    bool ret = false;
	struct regulator * temp = *regVCAM;
    if(!IS_ERR(temp)){
		#if 1
        if(regulator_is_enabled(temp)) {
            PK_DBG("[_hwPowerDown]before disable %s is enabled\n", powerId);
        }
		#endif
		if(regulator_disable(temp)!=0)
			 PK_DBG("[_hwPowerDown]fail to regulator_disable, powerID: %s\n", powerId);
		//for SMT stage, put seems always fail?
       // regulator_put(regVCAM);
       // regVCAM = NULL;
        ret = true;
    } else {
        PK_DBG("[_hwPowerDown]%s fail to power down  due to regVCAM == NULL regVCAM 0x%p\n", powerId,temp );
    }
    return ret;

}


int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{

u32 pinSetIdx = 0;//default main sensor

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4
#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3

#define VOL_2800 2800000
#define VOL_1800 1800000
#define VOL_1500 1500000
#define VOL_1200 1200000
#define VOL_1000 1000000


u32 pinSet[3][8] = {
                        //for main sensor
                     {  CAMERA_CMRST_PIN, // The reset pin of main sensor uses GPIO10 of mt6306, please call mt6306 API to set
                        CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,              /* ON state */
                        GPIO_OUT_ZERO,             /* OFF state */
                        CAMERA_CMPDN_PIN,
                        CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     },
                     //for sub sensor
                     {  CAMERA_CMRST1_PIN,
                        CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        CAMERA_CMPDN1_PIN,
                        CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     },
                     //for main_2 sensor
                     {  GPIO_CAMERA_INVALID,
                        GPIO_CAMERA_INVALID,   /* mode */
                        GPIO_OUT_ONE,               /* ON state */
                        GPIO_OUT_ZERO,              /* OFF state */
                        GPIO_CAMERA_INVALID,
                        GPIO_CAMERA_INVALID,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     }
                   };



    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx){
        pinSetIdx = 0;
    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        pinSetIdx = 1;
    }
    else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
        pinSetIdx = 2;
    }

    //power ON
    if (On) {
	//	if(pinSetIdx == 0)
	//		ISP_MCLK1_EN(1);
	//	else
	//		ISP_MCLK2_EN(1);
      PK_DBG("linmingchuang1 power on\n");

if (pinSetIdx == 1)	{
				
				 switch (g_CurrUsedMainCameraName){
					   case MAIN_S5K5E2:
							   printk(" yinyapeng add find main s5k5e2\n");
						
						        if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMPDN]) {
						            if(mt_set_gpio_mode(pinSet[0][IDX_PS_CMPDN],pinSet[0][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
						            if(mt_set_gpio_dir(pinSet[0][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
						            if(mt_set_gpio_out(pinSet[0][IDX_PS_CMPDN],pinSet[0][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
						             }

				
						        if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMRST]) {
						            if(mt_set_gpio_mode(pinSet[0][IDX_PS_CMRST],pinSet[0][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
						            if(mt_set_gpio_dir(pinSet[0][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
						            if(mt_set_gpio_out(pinSet[0][IDX_PS_CMRST],pinSet[0][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
						        }
								
							   break;
				
					   case MAIN_GC5005:
							   printk(" yinyapeng add find main gc5005\n");

						        if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMPDN]) {
						            if(mt_set_gpio_mode(pinSet[0][IDX_PS_CMPDN],pinSet[0][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
						            if(mt_set_gpio_dir(pinSet[0][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
						            if(mt_set_gpio_out(pinSet[0][IDX_PS_CMPDN],pinSet[0][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
						             }

				
						        if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMRST]) {
						            if(mt_set_gpio_mode(pinSet[0][IDX_PS_CMRST],pinSet[0][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
						            if(mt_set_gpio_dir(pinSet[0][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
						            if(mt_set_gpio_out(pinSet[0][IDX_PS_CMRST],pinSet[0][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
						        }								
							   break;
					#if 0	   
					   case MAIN_HI553:
							   printk(" yinyapeng add find main hi553\n");
							   if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMPDN])
								   mtkcam_gpio_set(0, CAMPDN,pinSet[0][IDX_PS_CMPDN + IDX_PS_OFF]);
				
							   if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMRST])
								   mtkcam_gpio_set(0, CAMRST,pinSet[0][IDX_PS_CMRST + IDX_PS_OFF]); 		   
							   break;
					#endif		   
					   default:
							   printk(" yinyapeng add can not find main camera\n");
			   }

		}
	
//////////////////////////////////////
/////////GC2355 POWERON START/////////
/////////////////////////////////////
		
  if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW, currSensorName)))
        {

PK_DBG("linmingchuang4 power on\n");
PK_DBG("linmingchuang pinSetIdx %d is enabled\n", pinSetIdx);
        #if 0
            mt_set_gpio_mode(GPIO_SPI_MOSI_PIN,GPIO_MODE_00);
            mt_set_gpio_dir(GPIO_SPI_MOSI_PIN,GPIO_DIR_OUT);
            mt_set_gpio_out(GPIO_SPI_MOSI_PIN,GPIO_OUT_ONE);
        #endif
            //First Power Pin low and Reset Pin Low

           if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[1][IDX_PS_CMPDN],pinSet[1][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[1][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[1][IDX_PS_CMPDN],pinSet[1][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }

            if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[1][IDX_PS_CMRST],pinSet[1][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[1][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[1][IDX_PS_CMRST],pinSet[1][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }

             mdelay(1);
           // mdelay(50);



            //VCAM_IO
            if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
                goto _kdCISModulePowerOn_exit_;
            }

            if(TRUE != _hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1800,&regVCAMD_SUB))
            {
                 PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %s \n", SUB_CAMERA_POWER_VCAM_D);
                 goto _kdCISModulePowerOn_exit_;
            }

            mdelay(1);
		            //VCAM_A	
	  if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,&regVCAMA))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n", CAMERA_POWER_VCAM_A);
                goto _kdCISModulePowerOn_exit_;
            }

      	     ISP_MCLK2_EN(1);		

     /*       //AF_VCC
            if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800,&regVCAMAF))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %s \n", CAMERA_POWER_VCAM_AF);
                goto _kdCISModulePowerOn_exit_;
            }


            mdelay(50);
*/

            //enable active sensor
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }


            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }


  }

//////////////////////////////////////
/////////GC2355 POWERON END/////////
/////////////////////////////////////
		

//////////////////////////////////////
/////////GC5005 POWERON  START/////////
/////////////////////////////////////

		//add by xuefeng or GC5005
	  else	if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC5005_MIPI_RAW, currSensorName)))
			  {
	  
	  PK_DBG("linmingchuang4 power on\n");
	  PK_DBG("linmingchuang pinSetIdx %d is enabled\n", pinSetIdx);
				  //First Power Pin low and Reset Pin Low

				 if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMPDN]) {
					  if(mt_set_gpio_mode(pinSet[1][IDX_PS_CMPDN],pinSet[1][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
					  if(mt_set_gpio_dir(pinSet[1][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
					  if(mt_set_gpio_out(pinSet[1][IDX_PS_CMPDN],pinSet[1][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
				  }
				  
				  if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMPDN]) {
					  if(mt_set_gpio_mode(pinSet[0][IDX_PS_CMPDN],pinSet[0][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
					  if(mt_set_gpio_dir(pinSet[0][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
					  if(mt_set_gpio_out(pinSet[0][IDX_PS_CMPDN],pinSet[0][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
				  }
	  
	                           mdelay(2);
		  //   ISP_MCLK1_EN(1);
	  
	  
				  //VCAM_IO
				  if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO))
				  {
					  PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
					  goto _kdCISModulePowerOn_exit_;
				  }
	  
				  mdelay(1);
	  
				    if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,&regVCAMD))
				    {
					 PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %s \n", CAMERA_POWER_VCAM_D);
					 goto _kdCISModulePowerOn_exit_;
				    }

				  mdelay(1);
						  //VCAM_A	  
			if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,&regVCAMA))
				  {
					  PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n", CAMERA_POWER_VCAM_A);
					  goto _kdCISModulePowerOn_exit_;
				  }

	                          ISP_MCLK1_EN(1);
				  mdelay(5);
	  
				  //enable active sensor
				  if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
					  if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
					  if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
					  if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
				  }


				  if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
					  if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[1][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
					  if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
					  if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
				  }
			  }
//////////////////////////////////////
/////////GC5005 POWERON END/////////
/////////////////////////////////////

//////////////////////////////////////
/////////GC2365 POWERON START/////////
/////////////////////////////////////


//add by xuefeng for GC2365 power on
else  if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC2365_MIPI_RAW, currSensorName)))
        {

PK_DBG("linmingchuang4 power on\n");
PK_DBG("linmingchuang pinSetIdx %d is enabled\n", pinSetIdx);
            //First Power Pin low and Reset Pin Low


           if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[1][IDX_PS_CMPDN],pinSet[1][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[1][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[1][IDX_PS_CMPDN],pinSet[1][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }
		   
            if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[1][IDX_PS_CMRST],pinSet[1][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[1][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[1][IDX_PS_CMRST],pinSet[1][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }

         //   mdelay(50);
	//   ISP_MCLK1_EN(1);


            //VCAM_IO
            if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
                goto _kdCISModulePowerOn_exit_;
            }

            mdelay(5);

            if(TRUE != _hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1800,&regVCAMD_SUB))
            {
                 PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %s \n", SUB_CAMERA_POWER_VCAM_D);
                 goto _kdCISModulePowerOn_exit_;
            }

            mdelay(1);
		            //VCAM_A	
	  if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,&regVCAMA))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n", CAMERA_POWER_VCAM_A);
                goto _kdCISModulePowerOn_exit_;
            }

          //  mdelay(10);
          ISP_MCLK2_EN(1);
          mdelay(5);

     /*       //AF_VCC
            if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800,&regVCAMAF))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %s \n", CAMERA_POWER_VCAM_AF);
                goto _kdCISModulePowerOn_exit_;
            }


            mdelay(50);
*/
            //enable active sensor
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
                mdelay(5);
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }


            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[1][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
                mdelay(5);
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}

            }

        }

//////////////////////////////////////
/////////GC2365 POWERON END/////////
/////////////////////////////////////


/////////////////////////////////
////////LINMINGCHUANG  S5K5 POWER ON START///////
/////////////////////////////////
else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_S5K5E2YA_MIPI_RAW, currSensorName))) {

PK_DBG("linmingchuang10 power on\n");	
PK_DBG("linmingchuang pinSetIdx %d is enabled\n", pinSetIdx);
				//mtkcam_gpio_set(pinSetIdx, CAMLDO, 1);
				   printk("yinyapeng add s5k5e2 power on\n");
				/* First Power Pin low and Reset Pin Low */


            if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[1][IDX_PS_CMPDN],pinSet[1][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[1][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[1][IDX_PS_CMPDN],pinSet[1][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }

                        
                if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[0][IDX_PS_CMPDN],pinSet[0][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[0][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[0][IDX_PS_CMPDN],pinSet[0][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }
 

 		           mdelay(1);	
              	          ISP_MCLK1_EN(1);
			   mdelay(2);				  
				/* VCAM_IO */


			    if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,&regVCAMD))
			    {
				 PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %s \n", CAMERA_POWER_VCAM_D);				
				 goto _kdCISModulePowerOn_exit_;			   //VCAM_IO
			    }	

		            mdelay(2);	

				    //VCAM_A	
		        if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,&regVCAMA))
		          {
			       PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n", CAMERA_POWER_VCAM_A);
			       goto _kdCISModulePowerOn_exit_;			
		           }
				
                           mdelay(2);	

			   if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO))  
			    {
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			    }


			    mdelay(2);

			    //mdelay(10);
			
				
            //enable active sensor
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }


     //       mdelay(2);
 

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}	
} 


}
/////////////////////////////////
////////LINMINGCHUANG S5K5 POWER ON END///////
/////////////////////////////////

/////////////////////////////////
////////DEFAULT POWER ON START///////
/////////////////////////////////
        else
        {
PK_DBG("linmingchuang7 power on\n");
            //First Power Pin low and Reset Pin Low
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }

            //VCAM_IO
            if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
                goto _kdCISModulePowerOn_exit_;
            }

            //VCAM_A
            if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,&regVCAMA))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n", CAMERA_POWER_VCAM_A);
                goto _kdCISModulePowerOn_exit_;
            }
            //VCAM_D
            if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_S5K2P8_MIPI_RAW, currSensorName)))
            {
                if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,&regVCAMD))
                {
                     PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                     goto _kdCISModulePowerOn_exit_;
                }
            }
            else if(currSensorName &&(0 == strcmp(SENSOR_DRVNAME_IMX219_MIPI_RAW, currSensorName )))
            {
PK_DBG("linmingchuang8 power on\n");
                if(pinSetIdx == 0 && TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,&regVCAMD))
                {
                     PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                     goto _kdCISModulePowerOn_exit_;
                }
            }
            else { // Main VCAMD max 1.5V
PK_DBG("linmingchuang9 power on\n");
                PK_DBG("[CAMERA SENSOR] before vcamd power on\n");
                if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,&regVCAMD))
                {
                     PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                     goto _kdCISModulePowerOn_exit_;
                }
				PK_DBG("[CAMERA SENSOR] after before vcamd power on regVCAMD=%p\n",&regVCAMD);
            }


             //AF_VCC
            if(TRUE != _hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800,&regVCAMAF))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %s \n", CAMERA_POWER_VCAM_AF);
                goto _kdCISModulePowerOn_exit_;
            }

            mdelay(5);

            //enable active sensor
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }

            mdelay(1);

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }
        }
/////////////////////////////////
////////DEFAULT POWER ON END///////
/////////////////////////////////
		
    }
    else {//power OFF


PK_DBG("linmingchuang1 power off\n");
        PK_DBG("[PowerOFF]pinSetIdx:%d\n", pinSetIdx);

    if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW, currSensorName))){
#if 0
            mt_set_gpio_out(GPIO_SPI_MOSI_PIN,GPIO_OUT_ZERO);
#endif
            //Set Power Pin low and Reset Pin Low
PK_DBG("linmingchuang4 power off\n");
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }

	    ISP_MCLK2_EN(0);

            mdelay(5);

            //VCAM_A
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A,&regVCAMA)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s \n", CAMERA_POWER_VCAM_A);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
   

	mdelay(1);

            if(TRUE != _hwPowerDown(SUB_CAMERA_POWER_VCAM_D,&regVCAMD_SUB))
            {
                 PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %s \n",SUB_CAMERA_POWER_VCAM_D);
                 goto _kdCISModulePowerOn_exit_;
            }


            //VCAM_IO
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }

             mdelay(10);

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }

            /*//AF_VCC
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_AF,&regVCAMAF))
            {
                PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %s \n", CAMERA_POWER_VCAM_AF);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }

*/

	      } 


	else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC5005_MIPI_RAW, currSensorName))){
#if 0
            mt_set_gpio_out(GPIO_SPI_MOSI_PIN,GPIO_OUT_ZERO);
#endif
            //Set Power Pin low and Reset Pin Low
PK_DBG("linmingchuang4 power off\n");
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }

	    ISP_MCLK1_EN(0);
            mdelay(2);


            //VCAM_A
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A,&regVCAMA)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s \n", CAMERA_POWER_VCAM_A);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }
				
	    if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D,&regVCAMD))
	    {
		 PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %s \n",CAMERA_POWER_VCAM_D);
		 goto _kdCISModulePowerOn_exit_;
	    }

            //VCAM_IO
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }

	   mdelay(10);

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }


            /*//AF_VCC
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_AF,&regVCAMAF))
            {
                PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %s \n", CAMERA_POWER_VCAM_AF);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }

*/

	      } 

	else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC2365_MIPI_RAW, currSensorName))){
#if 0
            mt_set_gpio_out(GPIO_SPI_MOSI_PIN,GPIO_OUT_ZERO);
#endif
            //Set Power Pin low and Reset Pin Low
PK_DBG("linmingchuang4 power off\n");
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }

	    ISP_MCLK2_EN(0);

	      mdelay(2);		


            //VCAM_A
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A,&regVCAMA)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s \n", CAMERA_POWER_VCAM_A);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }

            if(TRUE != _hwPowerDown(SUB_CAMERA_POWER_VCAM_D,&regVCAMD_SUB))
            {
                 PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %s \n",SUB_CAMERA_POWER_VCAM_D);
                 goto _kdCISModulePowerOn_exit_;
            }


            //VCAM_IO
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
                PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }

		mdelay(10);

            /*//AF_VCC
            if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_AF,&regVCAMAF))
            {
                PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %s \n", CAMERA_POWER_VCAM_AF);
                //return -EIO;
                goto _kdCISModulePowerOn_exit_;
            }

*/
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }



	      } 
/*****S5K5E2YA poweroff******/	

/////////////////////////////////
////////LINMINGCHUANG  S5K5 POWER OFF START///////
/////////////////////////////////
else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_S5K5E2YA_MIPI_RAW, currSensorName))) {
				PK_DBG("linmingchuang10 power off\n");			
								
	   ISP_MCLK1_EN(0);
	    mdelay(10);
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
            }
		
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
                if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
            }
                mdelay(10);
		
				/*****************************/
				
			    //VCAM_A
			    if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A,&regVCAMA)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s \n", CAMERA_POWER_VCAM_A);
				//return -EIO;
				goto _kdCISModulePowerOn_exit_;
			    }


				
			    if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D,&regVCAMD))
			    {
				 PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %s \n",CAMERA_POWER_VCAM_D);
				 goto _kdCISModulePowerOn_exit_;
			    }

				
			   //VCAM_IO
			    if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s \n", CAMERA_POWER_VCAM_IO);
				//return -EIO;
				goto _kdCISModulePowerOn_exit_;
			    }

				
				mdelay(10);
				/*WITHOUT VAF
				if (TRUE != _hwPowerDownCnt(VCAMAF, mode_name)) {
					PK_DBG ("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %d\n", VCAMAF);  
					goto _kdCISModulePowerOn_exit_;
				}
				*/	
				printk("yinyapeng add s5k5e2 power off\n");
		}

/////////////////////////////////
////////LINMINGCHUANG  S5K5 POWER OFF END///////
/////////////////////////////////

else {
PK_DBG("linmingchuang6 power off\n");
			//Set Power Pin low and Reset Pin Low
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
				if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
				if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");}
			}
	        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
				if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
				if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
			}
			if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_IMX219_MIPI_RAW, currSensorName))){
PK_DBG("linmingchuang7 power off\n");
				if(pinSetIdx == 0 && TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D, &regVCAMD))
				{
					goto _kdCISModulePowerOn_exit_;
				}
			}
		 else {
			    if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D,&regVCAMD))
				{

					goto _kdCISModulePowerOn_exit_;
				}
			}
			//VCAM_A
			if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A,&regVCAMA)){

				goto _kdCISModulePowerOn_exit_;
			}
			//VCAM_IO
			if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)){

	            goto _kdCISModulePowerOn_exit_;
			}
			//AF_VCC
			if(TRUE != _hwPowerDown(CAMERA_POWER_VCAM_AF,&regVCAMAF)){

				goto _kdCISModulePowerOn_exit_;
			}
		}
	          ISP_MCLK1_EN(0);
    }

    return 0;

_kdCISModulePowerOn_exit_:
    return -EIO;

}

EXPORT_SYMBOL(kdCISModulePowerOn);

//!--
//


