/*

   Written by Drew André using Daniel Garcia and Mark Kriegsman's FastLED library.

   Edited by Zombodotcom
*/


#include <Arduino.h>
#include <FastLED.h>
#include <SoftwareSerial.h>
#define SERIAL_TIMEOUT 5
//#include <MSGEQ7.h> // to be built

const uint8_t DATA_PIN = 7;
const uint8_t NUM_LEDS = 144;
int HALF_POS = (NUM_LEDS * 0.5) - 1;
//const uint8_t HALF_POS = 14;
CRGB leds[NUM_LEDS];
const uint8_t FRAMES_PER_SECOND = 300;

// MSGEQ7 SETUP and SMOOTHING (eventually replace by MSGEQ7 class)
const uint8_t AUDIO_LEFT_PIN = 0, AUDIO_RIGHT_PIN = 1;
const uint8_t MSGEQ7_STROBE_PIN = 4, MSGEQ7_RESET_PIN = 5;
float lowPass_audio = 0.15;
uint8_t filter_min = 120;
const uint8_t left_filter_max, right_filter_max, filter_max = 1016;
const float EQ[7] = {0, 0, 0.1, 0.1, 0.1, 0.1, 0}; /* EQ[7] = {0, 0, 0, 0, 0, 0, 0}; */
const uint8_t sensitive_min = filter_min - (filter_min * .15);
const float FILTER_MIN[7] = {filter_min, filter_min, filter_min, filter_min, filter_min, filter_min, filter_min};
uint8_t new_left, new_right, prev_left, prev_right, prev_value, left_value, right_value;
uint8_t left_volume, right_volume, mono_volume;
uint8_t left[7], right[7], mono[7];
uint8_t mapped_left[7], mapped_right[7], full_mapped[14], mapped[7], full_flex[7];
float left_factor, right_factor, mono_factor, full_factor;
float half_MAPPED_AMPLITUDE, half_MAPPED_LEFT_AMP, half_MAPPED_RIGHT_AMP;
uint8_t prev_left_amp, prev_right_amp;




//NOISE SETUP
static uint16_t x, y, z, dist;
float scale = 20.00; //1 - ~4000 (shimmery, zoomed out)
//float SPEED = 1.00f;  //1-100 (fast)
uint8_t ioffset, joffset;

// HSV SETUP
int BRIGHTNESS = 255;
int saturation = 255;
uint8_t hue, ihue, encoderHue;
float fhue;
uint8_t spectrumWidth = 36;
float HUEcorrection = /*floor(255 / NUM_LEDS);*/ 1.7; // *ALMOST* hue range fits on strip length
float half_HUEcorrection = HUEcorrection * 2;
// unnecessary math, to be fixed
const uint8_t HUEZ[14] = {240, 200, 160, 120, 80, 40, 0, 0, 40, 80, 120, 160, 200, 240};

// OTHER
uint8_t ledindex, segment, INDEX, i, k, band;
float divFactor;

// EFFECT: radiate (stereo)
uint8_t zero_l, three_l, six_l, zero_r, three_r, six_r;

// EFFECT: flex (mono)
uint8_t current_hue, current_brightness, next_hue, next_brightness, pos, point;
// EFFECT: flex (stereo)
uint8_t left_current_brightness, left_next_brightness, right_current_brightness, right_next_brightness;
uint8_t left_next_hue, right_next_hue, left_current_sat, right_current_sat, left_next_sat, right_next_sat;
uint8_t left_pos, right_pos, left_point, right_point;

// EFFECT: audio data (mono and stereo)
const uint8_t segmentSize = 32;
const uint8_t half_segmentSize = floor(segmentSize * 0.5);
float non_mirror_divFactor = 1.00 / segmentSize;
const uint8_t segmentEnd[7] = {31, 22, 22, 22, 22, 13, 10};

// EFFECT: fire2012 (Daniel Garcia and Mark Kriegsman)
bool gReverseDirection = false;
const uint8_t COOLING  = 170, SPARKING = 10; // Higher chance = more roaring fire, Default = 120

int MILLISECONDS = 5;




// wake up the MSGEQ7
void InitMSGEQ7() {
  analogReference(DEFAULT);              // change
  pinMode(MSGEQ7_RESET_PIN,  OUTPUT);
  pinMode(MSGEQ7_STROBE_PIN, OUTPUT);
  pinMode(AUDIO_LEFT_PIN,   INPUT);
  pinMode(AUDIO_RIGHT_PIN,  INPUT);
  digitalWrite(MSGEQ7_RESET_PIN, LOW);
  digitalWrite(MSGEQ7_STROBE_PIN, LOW);
  //RESET MSGEQ7
  digitalWrite(MSGEQ7_RESET_PIN,  HIGH);
  delay(1);
  digitalWrite(MSGEQ7_RESET_PIN,  LOW);
  digitalWrite(MSGEQ7_STROBE_PIN, HIGH);
  delay(1);
  READ_AUDIO();
}

void InitWS2812B() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  //FastLED.setTemperature(Candle);

  for (int i = 0; i < NUM_LEDS; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
}




void welcome() {
  //intro animation for box button and leds
  delay(500);
}



void radiate() {
   READ_AUDIO();
  //int SPEED = mono[0] * 0.004;
  //HALF_POS = beatsin8(40, 30 + SPEED, 80 + SPEED);
  MILLISECONDS  = 0;
  //lowPass_audio = 0.10;
  //filter_min    = 100;

  //  EVERY_N_MILLISECONDS(50) {
  //    hue++;
  //    if ( hue > 255) hue = 0;
  //  }


  zero_l  = left[0];
  three_l = left[3];
  six_l   = left[6];

  zero_r  = right[0];
  three_r = right[3];
  six_r   = right[6];

  leds[HALF_POS] = CRGB(zero_l, three_l, six_l);
  leds[HALF_POS + 1] = CRGB(zero_r, three_r, six_r);
  //leds[HALF_POS].fadeToBlackBy(30);


  EVERY_N_MILLISECONDS(11) {
    for (int i = NUM_LEDS - 1; i > HALF_POS + 1; i--) {
      leds[i].blue = leds[i - 1].blue;
    }
    for (int i = 0; i < HALF_POS; i++) {
      leds[i].blue = leds[i + 1].blue;
    }
  }
  EVERY_N_MILLISECONDS(27) {
    for (int i = NUM_LEDS - 1; i > HALF_POS + 1; i--) {
      leds[i].green = leds[i - 1].green;
    }
    for (int i = 0; i < HALF_POS; i++) {
      leds[i].green = leds[i + 1].green;
    }
  }
  EVERY_N_MILLISECONDS(52) {
    for (int i = NUM_LEDS - 1; i > HALF_POS + 1; i--) {
      leds[i].red = leds[i - 1].red;
    }
    for (int i = 0; i < HALF_POS; i++) {
      leds[i].red = leds[i + 1].red;
    }
  }

  EVERY_N_MILLISECONDS(2) {
    //blur1d(leds, NUM_LEDS, 1);
  }
  FastLED.show();
}

void READ_AUDIO() {

  prev_left  = left_volume;
  prev_right = right_volume;

  prev_left_amp  = half_MAPPED_LEFT_AMP;
  prev_right_amp = half_MAPPED_RIGHT_AMP;

  left_volume  = 0.0;
  right_volume = 0.0;
  mono_volume  = 0;
  left_factor  = 0.0;
  right_factor = 0.0;
  mono_factor  = 0;

  digitalWrite(MSGEQ7_RESET_PIN, HIGH);
  digitalWrite(MSGEQ7_RESET_PIN, LOW);
  delayMicroseconds(1);

  for (int band = 0; band < 7; band++)
  {

    filter_min = FILTER_MIN[band];
    digitalWrite(MSGEQ7_STROBE_PIN, LOW);
    delayMicroseconds(36);

    prev_value  = left[band];
    left_value  = analogRead(AUDIO_LEFT_PIN);
    //left_value  -= (left_value * EQ[band]);
    left_value  = constrain(left_value, filter_min, filter_max);
    left_value  = map(left_value, filter_min, filter_max, 0, 255);
    left[band]  = prev_value + (left_value - prev_value) * lowPass_audio;
    left_volume += left[band];


    prev_value   = right[band];
    right_value  = analogRead(AUDIO_RIGHT_PIN);
    //right_value  -= (right_value * EQ[band]);
    right_value  = constrain(right_value, filter_min, filter_max);
    right_value  = map(right_value, filter_min, filter_max, 0, 255);
    right[band]  = prev_value + (right_value - prev_value) * lowPass_audio;
    right_volume += right[band];

    digitalWrite(MSGEQ7_STROBE_PIN, HIGH);
    delayMicroseconds(1);

    mono[band]   = (left[band] + right[band]) * 0.5;
    mono_volume += mono[band];

    //IF DEF is (this effect) then do these extra/specfic tasks *stereo_vu*
  }

  //mono_volume  /= 7;
  //left_volume  /= 7;
  //right_volume /= 7;
  //  new_left = (left_volume  / 7);
  //  new_left = prev_left + (new_left - prev_left) * 0.005;
  //  new_right = (right_volume / 7);
  //  new_right = prev_right + (new_right - prev_right) * 0.005;
  //
  //  left_filter_max  = new_left;
  //  right_filter_max = new_right;


  left_factor   = float(HALF_POS) / left_volume;
  right_factor  = float(HALF_POS) / right_volume;
  mono_factor   = float(HALF_POS) / mono_volume;
  //if(mono_factor < 0.01) mono_factor = 0;
  full_factor   = float(NUM_LEDS) / mono_volume;

  for (int band = 0; band < 7; band++)
  {

    full_flex[band] = mono[band] * full_factor;

    mapped[band]    = mono[band] * mono_factor;

    //    if (half_MAPPED_AMP < 2) {
    //      half_MAPPED_AMP -= 2;
    //    }
    //    mapped[band] = half_MAPPED_AMP;

    half_MAPPED_LEFT_AMP = floor(left[band]  * left_factor);
    //    if (half_MAPPED_LEFT_AMP < 3) {
    //      half_MAPPED_LEFT_AMP -= 3;
    //    }
    mapped_left[band]    = half_MAPPED_LEFT_AMP;

    half_MAPPED_RIGHT_AMP = floor(right[band] * right_factor);
    //    if (half_MAPPED_RIGHT_AMP < 2) {
    //      half_MAPPED_RIGHT_AMP -= 2;
    //    }
    mapped_right[band]    = half_MAPPED_RIGHT_AMP;


    //full_mapped[band]     = left[6 - band]  * left_factor;
    //full_mapped[band + 7] = right[band] * right_factor;

    // }

  }
}

void setup() {
  delay(500);
  //---PROCESS HARDWARE SERIAL COMMANDS AND ARGS
  InitWS2812B();
  InitMSGEQ7();
  welcome();
  Serial.begin(9600);
}

void loop() {

 radiate();


}
