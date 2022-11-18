// Breadboard example app
//
// Read from multiple analog sensors and control an RGB LED

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "app_timer.h"
#include "nrf_delay.h"
#include "nrfx_saadc.h"

#include "microbit_v2.h"

// Digital outputs
// Breakout pins 13, 14, and 15
// These are GPIO pin numbers that can be used in nrf_gpio_* calls
#define LED_RED   EDGE_P13
#define LED_GREEN EDGE_P14
#define LED_BLUE  EDGE_P15

#define TOUCH0 EDGE_P11
#define TOUCH1 EDGE_P12
#define TOUCH2  EDGE_P13
#define TOUCH3 EDGE_P14



// Digital inputs
// Breakout pin 16
// These are GPIO pin numbers that can be used in nrf_gpio_* calls
#define SWITCH_IN EDGE_P16

// Analog inputs
// Breakout pins 1 and 2
// These are GPIO pin numbers that can be used in ADC configurations
// AIN1 is breakout pin 1. AIN2 is breakout pin 2.
#define ANALOG_TOUCH0  NRF_SAADC_INPUT_AIN0
#define ANALOG_TOUCH1 NRF_SAADC_INPUT_AIN1
#define ANALOG_TOUCH2 NRF_SAADC_INPUT_AIN2
#define ANALOG_TOUCH3 NRF_SAADC_INPUT_AIN4

// ADC channel configurations
// These are ADC channel numbers that can be used in ADC calls
#define ADC_TOUCH_CHANNEL0  0
#define ADC_TOUCH_CHANNEL1  1
#define ADC_TOUCH_CHANNEL2  2
#define ADC_TOUCH_CHANNEL3  3

// Global variables
APP_TIMER_DEF(sample_timer);

// Function prototypes
static void gpio_init(void);
static void adc_init(void);
static float adc_sample_blocking(uint8_t channel);


bool touch(float v){
  if(v > 3.15){
    return true;
  }
  else {
    return false;
  }
}

static void sample_timer_callback(void* _unused) {
  // Do things periodically here
  // TODO
  float t0 = adc_sample_blocking(ADC_TOUCH_CHANNEL0);
  float t1 = adc_sample_blocking(ADC_TOUCH_CHANNEL1);
  float t2 = adc_sample_blocking(ADC_TOUCH_CHANNEL2);
  float t3 = adc_sample_blocking(ADC_TOUCH_CHANNEL3);

  float touch_array[4] = {t0,t1,t2,t3};

  if(touch(t0) || touch(t1) || touch(t2) || touch(t3)){
    for(int i = 0; i < 4; i++){
      if(touch(touch_array[i])){
        printf("Sensor %d being touched \n", i);
      }
      else{
        continue;
      }
    }
  }
  else{
    printf("No sensor being touched \n");
  }
}

static void saadc_event_callback(nrfx_saadc_evt_t const* _unused) {
  // don't care about saadc events
  // ignore this function
}

static void gpio_init(void) {
  // Initialize output pins
  // TODO
  	nrf_gpio_cfg_output(LED_RED);

  // Set LEDs off initially
  // TODO
  	
    nrf_gpio_pin_set(LED_RED);

    // nrf_gpio_pin_clear(LED_RED);
    // nrf_gpio_pin_clear(LED_BLUE);
    // nrf_gpio_pin_clear(LED_GREEN);
  // Initialize input pin

    nrf_gpio_cfg_input(TOUCH0, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(TOUCH1, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(TOUCH2, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(TOUCH3, NRF_GPIO_PIN_NOPULL);
  // TODO
  
}

static void adc_init(void) {
  // Initialize the SAADC
  nrfx_saadc_config_t saadc_config = {
    .resolution = NRF_SAADC_RESOLUTION_12BIT,
    .oversample = NRF_SAADC_OVERSAMPLE_DISABLED,
    .interrupt_priority = 4,
    .low_power_mode = false,
  };
  ret_code_t error_code = nrfx_saadc_init(&saadc_config, saadc_event_callback);
  APP_ERROR_CHECK(error_code);

  // Initialize touch sensor channel
  nrf_saadc_channel_config_t touch_channel_config0 = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(ANALOG_TOUCH0);
  error_code = nrfx_saadc_channel_init(ADC_TOUCH_CHANNEL0, &touch_channel_config0);
  APP_ERROR_CHECK(error_code);

  nrf_saadc_channel_config_t touch_channel_config1 = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(ANALOG_TOUCH1);
  error_code = nrfx_saadc_channel_init(ADC_TOUCH_CHANNEL1, &touch_channel_config1);
  APP_ERROR_CHECK(error_code);

  nrf_saadc_channel_config_t touch_channel_config2 = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(ANALOG_TOUCH2);
  error_code = nrfx_saadc_channel_init(ADC_TOUCH_CHANNEL2, &touch_channel_config2);
  APP_ERROR_CHECK(error_code);

  nrf_saadc_channel_config_t touch_channel_config3 = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(ANALOG_TOUCH3);
  error_code = nrfx_saadc_channel_init(ADC_TOUCH_CHANNEL3, &touch_channel_config3);
  APP_ERROR_CHECK(error_code);

}

static float adc_sample_blocking(uint8_t channel) {
  // read ADC counts (0-4095)
  // this function blocks until the sample is ready
  int16_t adc_counts = 0;
  ret_code_t error_code = nrfx_saadc_sample_convert(channel, &adc_counts);
  APP_ERROR_CHECK(error_code);

  // convert ADC counts to volts
  // 12-bit ADC with range from 0 to 3.6 Volts
  // TODO
  return ((float)adc_counts * 3.6/4096.0);

  // return voltage measurement
}



int main(void) {
  printf("Board started!\n");
  
  // initialize GPIO
  gpio_init();

  // initialize ADC
  adc_init();

  // initialize app timers
  app_timer_init();
  app_timer_create(&sample_timer, APP_TIMER_MODE_REPEATED, sample_timer_callback);

  // start timer
  // change the rate to whatever you want
  app_timer_start(sample_timer, 32768/25, NULL);

  // loop forever
  while (1) {
    // Don't put any code in here. Instead put periodic code in `sample_timer_callback()`
    nrf_delay_ms(1000);
  }
}

