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
#include "net/ip/uip.h"
#include "rich.h"
#include "rest-engine.h"
#include "sys/ctimer.h"
#include <stdio.h> 
#include <AppHardwareApi.h>

static void ct_callback(void*ptr);
static void put_post_led_toggle_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

static char content[REST_MAX_CHUNK_SIZE];
static int content_len = 0;
static struct ctimer ct;


#define CONTENT_PRINTF(...) { if(content_len < sizeof(content)) content_len += snprintf(content+content_len, sizeof(content)-content_len, __VA_ARGS__); }

/* On dongle, LEDs are connected anti-parallel to DIO pins. */
#define LED1  16
#define LED2  17

#define TOGGLE_TIME CLOCK_SECOND
/*---------------------------------------------------------------------------*/
PROCESS(start_app, "START_APP");
AUTOSTART_PROCESSES(&start_app);
/*---------------------------------------------------------------------------*/

/* Call back for led toggle timer to toggle leds */
static void ct_callback(void*ptr)
{
  static uint8 toggle_status = 0;
  if (toggle_status) {
      vAHI_DioSetOutput(1<<LED1, 1<<LED2);            /* Only LED1 on */
  } else {
      vAHI_DioSetOutput(1<<LED2, 1<<LED1);            /* Only LED2 on */
  }
  ctimer_restart(&ct);
  toggle_status ^= 0x01;
}

/*********** RICH sensor/ resource ************************************************/
RESOURCE(resource_led_toggle, 
         "title=\"Led_toggle\"",
         NULL,
         put_post_led_toggle_handler,
         put_post_led_toggle_handler,
         NULL);
static void
put_post_led_toggle_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  static int led_state = 0;
  const uint8_t *request_content;
  int request_content_len;
  unsigned int accept = -1;

  /* Given the way the LEDs are connected to the DIO ports, the LEDs are controlled via direct DIO access. */
  content_len = 0;
  switch(led_state)
  {
    case(0):
      ctimer_stop(&ct);
      vAHI_DioSetOutput(1<<LED1, 1<<LED2);            /* Only LED1 on */
      CONTENT_PRINTF("Message from resource: Green LED on");
      led_state = 1;
      break;
    case(1):
      vAHI_DioSetOutput(1<<LED2, 1<<LED1);            /* Only LED2 on */
      CONTENT_PRINTF("Message from resource: Red LED on");
      led_state = 2;
      break;
    case(2):
      vAHI_DioSetOutput(0, (1<<LED1) | (1<< LED2));   /* All LEDS off */
      CONTENT_PRINTF("Message from resource: All LEDs off");
      led_state = 3;
      break;
    case 3:
      /* Both leds toggle */
      CONTENT_PRINTF("Message from resource: LEDs toggle");
      ctimer_restart(&ct);
      led_state = 0;
      default: 
      break;
  }
  /* Return message */
  REST.get_header_accept(request, &accept);
  if(accept == -1 || accept == REST.type.TEXT_PLAIN) {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    REST.set_response_payload(response, (uint8_t *)content, content_len);
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(start_app, ev, data)
{
  PROCESS_BEGIN();
  static int is_coordinator = 0;
 
  /* Switch off dongle leds */
  vAHI_DioSetDirection(0, (1<<LED1) | (1<<LED2));
  vAHI_DioSetOutput(0, (1<<LED1) | (1<<LED2));  /* Default off */
  
  /* Initialise ct timer, but don't let it run yet */
  ctimer_set(&ct, TOGGLE_TIME, ct_callback, NULL);
  ctimer_stop(&ct);
  
  /* Start RICH stack */
  if(is_coordinator) {
    uip_ipaddr_t prefix;
    uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    rich_init(&prefix);
  } else {
    rich_init(NULL);
  }
  printf("Starting RPL node\n");
  
  rest_init_engine();
  rest_activate_resource(&resource_led_toggle, "Dongle/LED-toggle");

  PROCESS_END();
}
