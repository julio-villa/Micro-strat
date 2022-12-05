// microStrat Guitar Program

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "app_timer.h"
#include "nrf_delay.h"
#include "nrfx_saadc.h"
#include "nrfx_pwm.h"

#include "microbit_v2.h"


// Digital GPIO inputs - Effects 
#define TOUCH0 EDGE_P11
#define TOUCH1 EDGE_P12

// Analog ADC configuration inputs - Notes
#define ANALOG_TOUCH0 NRF_SAADC_INPUT_AIN0
#define ANALOG_TOUCH1 NRF_SAADC_INPUT_AIN1
#define ANALOG_TOUCH2 NRF_SAADC_INPUT_AIN2
#define ANALOG_TOUCH3 NRF_SAADC_INPUT_AIN4

// ADC channel configurations
// These are ADC channel numbers that can be used in ADC calls
#define ADC_TOUCH_CHANNEL0  0
#define ADC_TOUCH_CHANNEL1  1
#define ADC_TOUCH_CHANNEL2  2
#define ADC_TOUCH_CHANNEL3  3

// Timers
APP_TIMER_DEF(sample_timer);
APP_TIMER_DEF(effects_timer);

// Function prototypes
static void gpio_init(void);
static void adc_init(void);
static float adc_sample_blocking(uint8_t channel);

// PWM configuration
static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);

// Holds a pre-computed sine wave
#define SINE_BUFFER_SIZE 100
uint16_t sine_buffer[SINE_BUFFER_SIZE] = {0};

// Sample data configurations
// Note: this is a 32 kB buffer (about 25% of RAM)
#define SAMPLING_FREQUENCY 17000 // 17 kHz sampling rate
#define BUFFER_SIZE 5000 // one second worth of data
uint16_t samples[BUFFER_SIZE] = {0}; // stores PWM duty cycle values

int touch_count = 0; // How many notes are being played at a time
bool spacey_on = false;
bool tremolo_on = false;
int sfreq = 17000;

nrfx_pwm_config_t config = {
    .output_pins = {EDGE_P8,NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED},
    .base_clock = NRF_PWM_CLK_16MHz,
    .count_mode = NRF_PWM_MODE_UP,
    .top_value = 16000000/SAMPLING_FREQUENCY/2,
    .load_mode = NRF_PWM_LOAD_COMMON,
    .step_mode = NRF_PWM_STEP_AUTO,
  };

static void pwm_init(void) {
  // Initialize the PWM
  nrfx_pwm_init(&PWM_INST, &config, NULL);
}

static void gpio_init(void) {

  // Initialize input GPIO pin
    nrf_gpio_cfg_input(TOUCH0, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(TOUCH1, NRF_GPIO_PIN_NOPULL);

    // Initialize pins
    // Microphone pin MUST be high drive
    nrf_gpio_pin_dir_set(LED_MIC, NRF_GPIO_PIN_DIR_OUTPUT);
    nrf_gpio_cfg(LED_MIC, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0H1, NRF_GPIO_PIN_NOSENSE);

    // Enable microphone - We are not using this lol
    nrf_gpio_pin_set(LED_MIC);
}


static void compute_sine_wave(uint16_t max_value, int touches) {
  for (int i=0; i<SINE_BUFFER_SIZE; i++) {
    // what percent into the sine wave are we?
    float percent = (float)i / (float)SINE_BUFFER_SIZE;

    // get sine value
    float twopi = 2.0*3.14159;
    float radians = twopi * percent;
    float sine_value = sin(radians);

    // translate from "-1 to 1" into "0 to 2"
    float abs_sine = sine_value + 1.0;

    // scale from "0 to 2" into "0 to 1"
    float mag_1_sine = abs_sine / 2.0;

    // scale to the max_value desired ("0 to max_value")
    // and truncate to an integer value
    uint16_t value = (uint16_t)(max_value * mag_1_sine);

    // save value in buffer
    printf("%d \n", touches);
    if(touches == 0){
      sine_buffer[i] = value * 5;
    }
    else{
      sine_buffer[i] = ((int)((value * 2) / touches));
    }
  }
}

// Sound Signal Generation -> What kind ofsound will be played
static void play_note(uint16_t frequency, int samp_freq, bool trem_on) {

  // determine number of sine values to "step" per played sample
  // units are (sine-values/cycle) * (cycles/second) / (samples/second) = (sine-values/sample)
  float step_size = (float)SINE_BUFFER_SIZE * (float)frequency / (float)samp_freq;
  
  //Tremolo setup
  static float tremrate = 0.5;  
  static float depth = 0.5; 
  static float t = 0.0;
 
  float i = 0;
  int j = 0;

  while(j < BUFFER_SIZE){
    // tremolo modulation calculation
    float tremolo = 1.0 - (depth*(2*sinf(t)+0.5));
    t += (tremrate * 0.002);
    if (t > 6.28318531) t -= 6.28318531;

    // Caculate samples based on what effects are turned on/off - tremolo and spacey
    if(trem_on && !spacey_on){
      samples[j] = sine_buffer[(int)i] * tremolo;
    }
    else if(spacey_on && !trem_on){
      samples[j] = 10 * sqrt((sine_buffer[(int)i]));
    }
    else if(spacey_on && trem_on){
      samples[j] = 10 * sqrt((sine_buffer[(int)i])) * tremolo;
    }
    else{
      samples[j] = (sine_buffer[(int)i]);
    }
    j++;
    i = i + step_size;
    if(i>=SINE_BUFFER_SIZE){
      i -= SINE_BUFFER_SIZE;
    }
  }
    nrf_pwm_sequence_t pwm_sequence = {
    .values.p_common = samples,
    .length = BUFFER_SIZE,
    .repeats = 1,
    .end_delay = 0,
  };
  nrfx_pwm_simple_playback(&PWM_INST, &pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);
}

// Check if a touch-event occurred
bool touch(float v){
  if(v > 3.15){
    return true;
  }
  else {
    return false;
  }
}
bool touched(uint32_t pin){
  if(nrf_gpio_pin_read(pin) > 0){
    return true;
  }
  else {
    return false;
  }
}

// Arrays of max/min frequencies per "string"
int notes_min[4] = {82, 110, 146, 196};
int notes_max[4] = {246, 329, 440, 587};

// Callback for detecting input on effects sensors
static void effects_timer_callback(void* _unused) {

  bool capacitive0 = touched(TOUCH0);
  bool capacitive1 = touched(TOUCH1);

  // Spacey Effect
  if(capacitive0){
    spacey_on = !spacey_on;
  }
  // Tremolo Effect 
  if(capacitive1){
    tremolo_on = !tremolo_on;
  }
}

static void sample_timer_callback(void* _unused) {
  touch_count = 0;

  float t0 = adc_sample_blocking(ADC_TOUCH_CHANNEL0);
  float t1 = adc_sample_blocking(ADC_TOUCH_CHANNEL1);
  float t2 = adc_sample_blocking(ADC_TOUCH_CHANNEL2);
  float t3 = adc_sample_blocking(ADC_TOUCH_CHANNEL3);

  printf("%f \n", t3);

  float touch_array[4] = {t3,t2,t1,t0};

  // Sound Production upon Touch Event
  if(touch_array[0] > 0.15 || touch_array[1] > 0.15 || touch_array[2] > 0.15 || touch_array[3] > 0.15){
    for(int i = 0; i < 4; i++){
      if(touch_array[i] > 0.15){
        touch_count++;
        play_note(notes_max[i] - ((touch_array[i]-0.15) * notes_min[i]), sfreq, tremolo_on);
      }
    }
  }
  else{
    // If nothing is being touched, do not play anything
    nrfx_pwm_stop(&PWM_INST, true);
  }
}

static void saadc_event_callback(nrfx_saadc_evt_t const* _unused) {
  // don't care about saadc events
  // ignore this function
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

  // Initialize touch sensor channels
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

  return ((float)adc_counts * 3.6/4096.0);
  // return voltage measurement
}

int main(void) {
  printf("Board started!\n");
  
  // initialize GPIO
  gpio_init();

  // initalize PWM
  pwm_init();

  // initialize ADC
  adc_init();

  compute_sine_wave(config.top_value - 1, touch_count);

  // initialize app timers
  app_timer_init();
  app_timer_create(&sample_timer, APP_TIMER_MODE_REPEATED, sample_timer_callback);
  app_timer_create(&effects_timer, APP_TIMER_MODE_REPEATED, effects_timer_callback);

  // start timer
  app_timer_start(sample_timer, 3276/650, NULL);
  app_timer_start(effects_timer, 3276 * 2, NULL);
  
  // loop forever
  while (1) {
    // Don't put any code in here. Instead put periodic code in `sample_timer_callback()`
    nrf_delay_ms(1000);
  }
}

