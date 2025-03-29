#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// functions Declaration
void print_line(String text, int column, int row, int text_size);
void print_time_now();
void update_time();

// Global Variables
int hours = 0;
int minutes = 0;
int seconds = 0;
int days = 0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  print_line(" Welcome\n    to\n  Medibox", 10, 10, 2);
}

void loop()
{
  update_time();
  display.clearDisplay();
  print_time_now();
}

void print_line(String text, int column, int row, int text_size)
{
  display.clearDisplay();

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row); //(Column , row)
  display.println(text);
  display.display();
  delay(2000);
}

void print_time_now()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0); //(Column , row)
  display.print("Time Now: ");
  display.setTextSize(2);
  display.setCursor(0, 20); //(Column , row)
  display.print(days);
  display.print(":");
  display.print(hours);
  display.print(":");
  display.print(minutes);
  display.print(":");
  display.print(seconds);
  display.display();
}

void update_time()
{
  timeNow = millis();
  if (timeNow - timeLast > 1000)
  {
    timeLast = timeNow;
    seconds++;
    if (seconds > 59)
    {
      seconds = 0;
      minutes++;
      if (minutes > 59)
      {
        minutes = 0;
        hours++;
        if (hours > 23)
        {
          hours = 0;
          days++;
        }
      }
    }
  }
}