#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define Buzzer 18
#define LED 19
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define PB_Cancel 23     // Push button cancel pin

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// functions Declaration
void print_line(String text, int column, int row, int text_size);
void print_time_now();
void update_time();

void update_time_with_check_alarm();
void ring_alarm();

// Global Variables
int hours = 0;
int minutes = 0;
int seconds = 0;
int days = 0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enable = true; // true = alarm on, false = alarm off
// Define the alarm_time_t structure
struct alarm_time_t
{
  bool alarm_state; // true = alarm on, false = alarm off
  // int days;
  int hours;
  int minutes;
  int seconds;
};

constexpr int n_alarm = 2; // for 2 alarms
alarm_time_t alarm_time[n_alarm] = {
    {
        true, // alarm state
        0,
        0,
        5,
    },               // alarm 1
    {false, 0, 0, 0} // alarm 2
};

int melody[] = {262, 294, 330, 349, 392, 440, 494, 523}; // Simple tones

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(Buzzer, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(PB_Cancel, INPUT); // Push button cancel pin

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
  display.clearDisplay();
  update_time_with_check_alarm();
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

void update_time_with_check_alarm()
{
  update_time();
  print_time_now();

  if (alarm_enable)
  {
    bool alarms_diabled = false; // Flag to check if all alarms are disabled
    for (int i = 0; i < n_alarm; i++)
    {
      if (alarm_time[i].alarm_state)
      {
        if (alarm_time[i].hours == hours && alarm_time[i].minutes == minutes && alarm_time[i].seconds == seconds)

        {
          // Trigger the alarm
          Serial.println("Alarm Triggered!");

          ring_alarm();
          alarm_time[i].alarm_state = false; // Disable the alarm after it rings
          alarms_diabled = true;             // Set the flag to true if at least one alarm is enabled
        }
        else
        {
          alarms_diabled = false; // Set the flag to false if at least one alarm is enabled
        }
      }
    }
    if (alarms_diabled)
      alarm_enable = false; // Disable the alarm if all alarms are disabled
    else
      alarm_enable = true; // Enable the alarm if at least one alarm is enabled
  }
  Serial.print("Alarm state: ");
  Serial.println(alarm_enable ? "ON" : "OFF");
}

void ring_alarm()
{
  print_line(" Medicine\n   Time!", 10, 10, 2);
  bool break_happen = false;
  while (break_happen == false && digitalRead(PB_Cancel) == HIGH)
  {

    digitalWrite(LED, HIGH);
    // Ring the buzzer
    for (int i = 0; i < 8; i++)
    {
      if (digitalRead(PB_Cancel) == LOW)
      {
        delay(200); // Debounce delay
        break_happen = true;
        digitalWrite(LED, LOW);
        ledcWriteTone(0, 0); // Stop sound
        break;               // Exit the loop if the button is pressed
      }
      ledcAttachPin(Buzzer, 0);    // Attach the pin to channel 0
      ledcSetup(0, melody[i], 8);  // The frequency setup line
      ledcWriteTone(0, melody[i]); // Play tone at given frequency
      delay(500);
      ledcWriteTone(0, 0); // Stop sound
    }
  }

  display.clearDisplay();
}