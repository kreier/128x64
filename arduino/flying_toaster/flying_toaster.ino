// Animated pendant for Adafruit Pro Trinket and SSD1306 OLED display,
// inspired by the After Dark "Flying Toasters" screensaver.
// Triggered with vibration switch between digital pins 3 and 4.

// #include <avr/sleep.h>
// #include <avr/power.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bitmaps.h" // Toaster graphics data is in this header file

#define EXTRAGND   4 // Extra ground pin for vibration switch
#define OLED_DC    5 // OLED control pins are configurable.
#define OLED_CS    6 // These are different from other SSD1306 examples
#define OLED_RESET 8 // because the Pro Trinket has no pin 2 or 7.
// Hardware SPI for Data & Clk -- pins 11 & 13 on Uno or Pro Trinket.
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

#define N_FLYERS   5 // Number of flying things

struct Flyer {       // Array of flying things
  int16_t x, y;      // Top-left position * 16 (for subpixel pos updates)
  int8_t  depth;     // Stacking order is also speed, 12-24 subpixels/frame
  uint8_t frame;     // Animation frame; Toasters cycle 0-3, Toast=255
} flyer[N_FLYERS];

uint32_t startTime;

void setup() {
  Serial.begin(74880);
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  randomSeed(analogRead(2));           // Seed random from unused analog input
  // DDRB  = DDRC  = DDRD  = 0x00;        // Set all pins to inputs and
  // PORTB = PORTC = PORTD = 0xFF;        // enable pullups (for power saving)
  // pinMode(EXTRAGND, OUTPUT);           // Set one pin low to provide a handy
  // digitalWrite(EXTRAGND, LOW);         // ground point for vibration switch
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Init screen
  display.clearDisplay();
  for(uint8_t i=0; i<N_FLYERS; i++) {  // Randomize initial flyer states
    flyer[i].x     = (-32 + random(160)) * 16;
    flyer[i].y     = (-32 + random( 96)) * 16;
    flyer[i].frame = random(3) ? random(4) : 255; // 66% toaster, else toast
    flyer[i].depth = 10 + random(16);             // Speed / stacking order
  }
  qsort(flyer, N_FLYERS, sizeof(struct Flyer), compare); // Sort depths

  // AVR peripherals that aren't used by this code are disabled to further
  // conserve power, and may take certain Arduino functionality with them.
  // If you adapt this code to other projects, may need to re-enable some.
  // power_adc_disable();    // Disable ADC (no analogRead())
  // power_twi_disable();    // Disable I2C (no Wire library)
  // power_usart0_disable(); // Disable UART (no Serial)
  // power_timer1_disable();
  // power_timer2_disable();

  // EICRA = _BV(ISC11); // Falling edge of INT1 (pin 3) generates an interrupt
  // EIMSK = _BV(INT1);  // Enable interrupt (vibration switch wakes from sleep)
  display.display();
  delay(2000); // Pause for 2 seconds
  
  Serial.println("Display initialized.");
  startTime = millis();
}

void loop() {
  uint8_t i, f;
  int16_t x, y;
  boolean resort = false;     // By default, don't re-sort depths

  display.display();          // Update screen to show current positions
  display.clearDisplay();     // Start drawing next frame

  for(i=0; i<N_FLYERS; i++) { // For each flyer...

    // First draw each item...
    f = (flyer[i].frame == 255) ? 4 : (flyer[i].frame++ & 3); // Frame #
    x = flyer[i].x / 16;
    y = flyer[i].y / 16;
    // display.drawBitmap(x, y, (const uint8_t *)pgm_read_word(&mask[f]), 32, 32, BLACK);
    // display.drawBitmap(x, y, (const uint8_t *)pgm_read_word(&img[ f]), 32, 32, WHITE);
    display.drawBitmap(x, y, mask[f], 32, 32, BLACK);
    display.drawBitmap(x, y, img[ f], 32, 32, WHITE);

    // Then update position, checking if item moved off screen...
    flyer[i].x -= flyer[i].depth * 2; // Update position based on depth,
    flyer[i].y += flyer[i].depth;     // for a sort of pseudo-parallax effect.
    if((flyer[i].y >= (64*16)) || (flyer[i].x <= (-32*16))) { // Off screen?
      if(random(7) < 5) {         // Pick random edge; 0-4 = top
        flyer[i].x = random(160) * 16;
        flyer[i].y = -32         * 16;
      } else {                    // 5-6 = right
        flyer[i].x = 128         * 16;
        flyer[i].y = random(64)  * 16;
      }
      flyer[i].frame = random(3) ? random(4) : 255; // 66% toaster, else toast
      flyer[i].depth = 10 + random(16);
      resort = true;
    }
  }
  // If any items were 'rebooted' to new position, re-sort all depths
  if(resort) qsort(flyer, N_FLYERS, sizeof(struct Flyer), compare);

  if((millis() - startTime) >= 15000L) {         // If 15 seconds elapsed...
    display.ssd1306_command(SSD1306_DISPLAYOFF); // Screen off
    // power_spi_disable();                         // Disable remaining periphs
    // power_timer0_disable();
    // set_sleep_mode(SLEEP_MODE_PWR_DOWN);         // Deepest sleep
    // sleep_mode();
    // Execution resumes here on wake.
    // power_spi_enable();                          // Re-enable SPI
    // power_timer0_enable();                       // and millis(), etc.
    display.ssd1306_command(SSD1306_DISPLAYON);  // Main screen turn on
    startTime = millis();                        // Save wake time
  }
}

// Flyer depth comparison function for qsort()
static int compare(const void *a, const void *b) {
  return ((struct Flyer *)a)->depth - ((struct Flyer *)b)->depth;
}

// ISR(INT1_vect) { } // Vibration switch wakeup interrupt