/***************************************************************************
 *
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 * Copyright NXP B.V. 2015. All rights reserved
 *
***************************************************************************/
#include "contiki.h"
#include "sys/etimer.h"
#include "lib/sensors.h"
#include "button-sensor.h" 
#include <AppHardwareApi.h>

/*---------------------------------------------------------------------------*/
/* LOCAL DEFINITIONS                                                         */
/*---------------------------------------------------------------------------*/
//#define DEBUG 
#ifdef DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

typedef enum{
  APP_E_BUTTON_SW0  = 0,
  APP_E_BUTTON_SW1,
  APP_E_BUTTON_SW2,
  APP_E_BUTTON_SW3,
  APP_E_BUTTON_SW4,
  APP_E_BUTTON_NUM    /* Number of buttons */
  } app_e_button_t;

/* Mapping of DIO port connections to buttons. Use as mask to get button value */
#define APP_PORT_BUTTON_SW0   (8)
#define APP_PORT_BUTTON_SW1		(11)
#define APP_PORT_BUTTON_SW2		(12)
#define APP_PORT_BUTTON_SW3		(17)
#define APP_PORT_BUTTON_SW4		(1)

/* Definition of port masks based on button mapping */ 
#define APP_BUTTONS_DIO_MASK  ((1 << APP_PORT_BUTTON_SW0) | \
                               (1 << APP_PORT_BUTTON_SW1) | \
                               (1 << APP_PORT_BUTTON_SW2) | \
                               (1 << APP_PORT_BUTTON_SW3) | \
                               (1 << APP_PORT_BUTTON_SW4))

#define KEY_SAMPLE_TIME (CLOCK_SECOND/20)

typedef enum {
  BUTTONS_STATUS_NOT_INIT = 0,
  BUTTONS_STATUS_INIT,
  BUTTONS_STATUS_NOT_ACTIVE = BUTTONS_STATUS_INIT,
  BUTTONS_STATUS_ACTIVE 
} buttons_status_t;

/*---------------------------------------------------------------------------*/
/* LOCAL DATA DEFINITIONS                                                    */
/*---------------------------------------------------------------------------*/
const struct sensors_sensor         button_sensor;
volatile static buttons_status_t    buttons_status = BUTTONS_STATUS_NOT_INIT;
static int                          key_value = 0;   
static uint8 key_map[] = {APP_PORT_BUTTON_SW0,  //APP_E_BUTTON_SW0
                          APP_PORT_BUTTON_SW1,  //APP_E_BUTTON_SW1
                          APP_PORT_BUTTON_SW2,  //APP_E_BUTTON_SW2
                          APP_PORT_BUTTON_SW3,  //APP_E_BUTTON_SW3
                          APP_PORT_BUTTON_SW4}; //APP_E_BUTTON_SW4


/*---------------------------------------------------------------------------*/
/* LOCAL FUNCTION PROTOTYPES                                                 */
/*---------------------------------------------------------------------------*/
PROCESS(key_sampling, "Key sample");
static int get_key_value(void);

/*---------------------------------------------------------------------------*/
/* PUBLIC FUNCTIONS                                                          */
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
  if (type == SENSORS_HW_INIT) {
    /* Called from sensor thread when started.
       Configure DIO lines with buttons connected as input */
    vAHI_DioSetDirection(APP_BUTTONS_DIO_MASK, 0);
    /* Turn on pull-ups for DIO lines with buttons connected */
    vAHI_DioSetPullup(APP_BUTTONS_DIO_MASK, 0);
    PRINTF("HW_INIT BUTTONS (0x%x)\n", APP_BUTTONS_DIO_MASK);
    /* Configure debounce timer. Do not run it yet. */ 
    buttons_status = BUTTONS_STATUS_INIT;
    process_start(&key_sampling, NULL);
    return 1;  
  } 
  else if (type == SENSORS_ACTIVE) {
    if (buttons_status != BUTTONS_STATUS_NOT_INIT) {
    	if (value){
        /* Button sensor activated */
        PRINTF("BUTTONS ACTIVATED\n");
        buttons_status = BUTTONS_STATUS_ACTIVE;
  		} else {
        /* Button sensor de-activated */
        PRINTF("BUTTONS DE-ACTIVATED\n");
        buttons_status = BUTTONS_STATUS_NOT_ACTIVE;
      }
      process_post(&key_sampling, PROCESS_EVENT_MSG, (void *)&buttons_status);
      return 1;
    }      
    else {
      /* Buttons must be intialised before being (de)-activated */
 		  PRINTF("ERROR: NO HW_INIT BUTTONS\n");
 		  return 0;
    }
  }
  /* Non valid type */
  else {		
    return 0;
	}
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  if (type == SENSORS_ACTIVE) {
    return (buttons_status == BUTTONS_STATUS_ACTIVE);
  }
  else if (type == SENSORS_READY) {
    return (buttons_status != BUTTONS_STATUS_NOT_INIT);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  /* type: Not defined for the buttons interface
  */
  return key_value;
}
/*---------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS                                                           */
/*---------------------------------------------------------------------------*/
static int get_key_value(void)
{
  /* Function returns the actual key value. Pressed key will return '1' */
  int io_value = ~u32AHI_DioReadInput() & APP_BUTTONS_DIO_MASK; 
  int k = 0;
  int key = 0;
  
  while (k < APP_E_BUTTON_NUM) {
    if (io_value & (1 << key_map[k])) {
      key |= (1 << k);
    }
    k++;
  }
  return key;
}

/* Process takes care of detecting key changes */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(key_sampling, ev, data)
{
  PROCESS_BEGIN();
  static struct etimer et;
  static int previous_key_value = 0;
  static char debounce_check = 0;
  int current_key_value;
  
  etimer_set(&et, CLOCK_SECOND/50);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL((ev == PROCESS_EVENT_TIMER) || (ev == PROCESS_EVENT_MSG));
    if (ev == PROCESS_EVENT_TIMER) {
      /* Handle sensor reading.   */
      PRINTF("Key sample\n");
      current_key_value = get_key_value(); 
      if (debounce_check != 0) {
        /* Check if key remained constant */
        if (previous_key_value == current_key_value) {
          sensors_changed(&button_sensor);
          key_value = current_key_value;
          debounce_check = 0;
        } else {
          /* Bouncing */
          previous_key_value = current_key_value;
        }
      }
      else {
        /* Check for new key change */
        if (current_key_value != previous_key_value) {
          previous_key_value = current_key_value;
          debounce_check = 1;   
        }     
      }
      etimer_reset(&et);
    }
    else {
      /* ev == PROCESS_EVENT_MSG */
      if (*(int*)data == BUTTONS_STATUS_NOT_ACTIVE) {
        /* Stop sampling */
        etimer_stop(&et);
      }
      else if ((*(int*)data == BUTTONS_STATUS_ACTIVE)) {
        /* restart sampling */
        etimer_restart(&et);
      }
    }
  }
  PROCESS_END();
}


/*---------------------------------------------------------------------------*/
/* Sensor defintion for sensor module */
SENSORS_SENSOR(button_sensor, BUTTON_SENSOR, value, configure, status);
/*---------------------------------------------------------------------------*/
