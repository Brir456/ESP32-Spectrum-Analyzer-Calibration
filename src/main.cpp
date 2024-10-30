/*

Code inspired from here: https://github.com/s-marley/ESP32_FFT_VU/tree/master
The the code for each band width was generated using this spreadsheet: https://github.com/s-marley/ESP32_FFT_VU/blob/master/FFT.xlsx

You can use this file to measure the average amplitude of each band of the analyzer (10 in total) for any given sample. I recommend using white noise for calibrating.

 */

#include <Arduino.h>
#include <arduinoFFT.h>
#include <driver/adc.h>

#define SAMPLES 1024        // Must be a power of 2
#define SAMPLING_FREQ 40000 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AMPLITUDE 1000      // Depending on your audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define NUM_BANDS 10        // To change this, you will need to change the bunch of if statements describing the mapping from bins to bands
#define NOISE 500           // Used as a crude noise filter, values below this are ignored
#define NUM_ITERATIONS 100  // Number of iterations used to measure white noise input for averaging

// Sampling and FFT stuff
unsigned int sampling_period_us;
int oldBarHeights[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int bandValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long avgBarHeights[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float coefficient[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

double vReal[SAMPLES];
int vRealInt[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQ);

void setup()
{
  Serial.begin(115200);

  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));

  delay(2000);
  Serial.println("Setup complete");
}

void loop()
{
  for (int i = 0; i < NUM_ITERATIONS; i++)
  {

    for (int i = 0; i < NUM_BANDS; i++)
    {
      bandValues[i] = 0;
    }

    // Sample the audio pin
    for (int i = 0; i < SAMPLES; i++)
    {
      newTime = micros();
      esp_err_t r = adc2_get_raw(ADC2_CHANNEL_2, ADC_WIDTH_12Bit, &vRealInt[i]);
      vReal[i] = vRealInt[i];
      vImag[i] = 0;
      while ((micros() - newTime) < sampling_period_us)
      { /* chill */
      }
    }

    // Compute FFT
    FFT.dcRemoval();
    FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Weigh data
    FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);                 // Compute FFT
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);                   // Compute magnitudes

    // Analyse FFT results
    for (int i = 2; i < (SAMPLES / 2); i++) // In case you're not using 10 bands, replace this loop with the code generated from this spreadsheet: https://github.com/s-marley/ESP32_FFT_VU/blob/master/FFT.xlsx
    {                                       // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.
      if (vReal[i] > NOISE)
      { // Add a crude noise filter
        if (i <= 3)
        {
          bandValues[0] += (int)vReal[i];
        }
        if (i > 3 && i <= 5)
        {
          bandValues[1] += (int)vReal[i];
        }
        if (i > 5 && i <= 9)
        {
          bandValues[2] += (int)vReal[i];
        }
        if (i > 9 && i <= 15)
        {
          bandValues[3] += (int)vReal[i];
        }
        if (i > 15 && i <= 26)
        {
          bandValues[4] += (int)vReal[i];
        }
        if (i > 26 && i <= 45)
        {
          bandValues[5] += (int)vReal[i];
        }
        if (i > 45 && i <= 79)
        {
          bandValues[6] += (int)vReal[i];
        }
        if (i > 79 && i <= 138)
        {
          bandValues[7] += (int)vReal[i];
        }
        if (i > 138 && i <= 242)
        {
          bandValues[8] += (int)vReal[i];
        }
        if (i > 242)
        {
          bandValues[9] += ((int)vReal[i]);
        }
      }
    }

    for (byte band = 0; band < NUM_BANDS; band++)
    {
      int barHeight = bandValues[band];
      barHeight = (oldBarHeights[band] + barHeight * coefficient[band]) / 2;
      oldBarHeights[band] = barHeight;
      avgBarHeights[band] += barHeight;
    }
  }

  for (int i = 0; i < NUM_BANDS; i++)
  {
    float avg = avgBarHeights[i] / NUM_ITERATIONS;
    Serial.println("Band" + String(i) + ": " + String(avg));
    avgBarHeights[i] = 0;
  }
  delay(1000);
}