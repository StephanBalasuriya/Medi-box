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
#define PB_OK 2
#define PB_Up 4
#define PB_Down 5

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// functions Declaration
void print_line(String text, int column, int row, int text_size);
void print_time_now();
void update_time();

void update_time_with_check_alarm();
void ring_alarm();
void go_to_menu();
int wait_for_button_press();
void run_mode(int mode);
void set_time();
void set_alarm(int n_alarm);

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

int current_mode = 0;
int max_mode = 5;                                                                                                         // Number of modes
String mode_name[] = {"1 - Set Time", "2 - Set Alarm 1", "3 -Set Alarm 2", "4 - Disable Alarm_1", "5 - Disable Alarm_2"}; // Menu modes

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(Buzzer, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(PB_Cancel, INPUT); // Push button cancel pin
  pinMode(PB_OK, INPUT);
  pinMode(PB_Up, INPUT);
  pinMode(PB_Down, INPUT);

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
  if (digitalRead(PB_OK) == LOW)
  {
    delay(200); // Debounce delay
    Serial.println("Go to menu");
    go_to_menu();
  }
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

void go_to_menu()
{
  display.clearDisplay();
  print_line("Menu", 10, 10, 2);
  delay(2000);
  while (digitalRead(PB_Cancel) == HIGH)
  {
    display.clearDisplay();
    print_line(mode_name[current_mode], 10, 10, 2);

    int pressed_button = wait_for_button_press();
    if (pressed_button == PB_Up)
    {
      delay(200);
      current_mode++;
      current_mode = current_mode % max_mode;
    }
    else if (pressed_button == PB_Down)
    {
      delay(200);
      current_mode--;
      if (current_mode < 0)
      {
        current_mode = max_mode - 1;
      }
    }
    else if (pressed_button == PB_OK)
    {
      delay(200);
      Serial.println("Run mode: " + String(current_mode));
      run_mode(current_mode);
    }
    else if (pressed_button == PB_Cancel)
    {
      delay(200);
      break;
    }
  }
}

int wait_for_button_press()
{
  while (true)
  {
    if (digitalRead(PB_Cancel) == LOW)
    {
      delay(200); // Debounce delay
      return PB_Cancel;
    }
    else if (digitalRead(PB_OK) == LOW)
    {
      delay(200); // Debounce delay
      return PB_OK;
    }
    else if (digitalRead(PB_Up) == LOW)
    {
      delay(200); // Debounce delay
      return PB_Up;
    }
    else if (digitalRead(PB_Down) == LOW)
    {
      delay(200); // Debounce delay
      return PB_Down;
    }

    update_time(); // Update time in the loop
  }
}

void run_mode(int mode)
{
  if (mode == 0)
    set_time();

  else if (mode == 1 || mode == 2)
  {
    set_alarm(mode - 1); // Set alarm 1 or 2 based on the mode
    alarm_enable = true; // Enable the alarm after setting it
  }
  else if (mode == 3 || mode == 4)
  {
    alarm_time[mode - 3].alarm_state = false; // Disable the alarm based on the mode
    if (alarm_time[0].alarm_state == false && alarm_time[1].alarm_state == false)
      alarm_enable = false; // Disable the alarm if all alarms are disabled
  }
}

void set_time()
{
  int temp_days = days;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter days:" + String(temp_days), 10, 10, 2);

    int pressed_button = wait_for_button_press();
    if (pressed_button == PB_Up)
    {
      delay(200);
      temp_days++;
    }
    else if (pressed_button == PB_Down)
    {
      delay(200);
      temp_days--;
      if (temp_days < 0)
      {
        temp_days = 30; // max_day=30;
      }
    }
    else if (pressed_button == PB_OK)
    {
      delay(200);
      days = temp_days;
      break;
    }
    else if (pressed_button == PB_Cancel)
    {
      delay(200);
      break;
    }
  }
  int temp_hours = hours;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter hour:" + String(temp_hours), 10, 10, 2);

    int pressed_button = wait_for_button_press();
    if (pressed_button == PB_Up)
    {
      delay(200);
      temp_hours++;
      // if(temp_hours > 23)
      //   days++;
      temp_hours = temp_hours % 24;
    }
    else if (pressed_button == PB_Down)
    {
      delay(200);
      temp_hours--;
      if (temp_hours < 0)
      {
        // days--;
        temp_hours = 23;
      }
    }
    else if (pressed_button == PB_OK)
    {
      delay(200);
      hours = temp_hours;
      break;
    }
    else if (pressed_button == PB_Cancel)
    {
      delay(200);
      break;
    }
  }

  int temp_minutes = minutes;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter minutes:" + String(temp_minutes), 10, 10, 2);

    int pressed_button = wait_for_button_press();
    if (pressed_button == PB_Up)
    {
      delay(200);
      temp_minutes++;
      temp_minutes = temp_minutes % 60;
    }
    else if (pressed_button == PB_Down)
    {
      delay(200);
      temp_minutes--;
      if (temp_minutes < 0)
      {
        temp_minutes = 59;
      }
    }
    else if (pressed_button == PB_OK)
    {
      delay(200);
      minutes = temp_minutes;
      break;
    }
    else if (pressed_button == PB_Cancel)
    {
      delay(200);
      break;
    }
  }

  int temp_seconds = seconds;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter seconds:" + String(temp_seconds), 10, 10, 2);

    int pressed_button = wait_for_button_press();
    if (pressed_button == PB_Up)
    {
      delay(200);
      temp_seconds++;
      temp_seconds = temp_seconds % 60;
    }
    else if (pressed_button == PB_Down)
    {
      delay(200);
      temp_seconds--;
      if (temp_seconds < 0)
      {
        temp_seconds = 59;
      }
    }
    else if (pressed_button == PB_OK)
    {
      delay(200);
      seconds = temp_seconds;
      break;
    }
    else if (pressed_button == PB_Cancel)
    {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time set to:\n" + String(days) + ":" + String(hours) + ":" + String(minutes) + ":" + String(seconds), 10, 10, 2);
  delay(1000);
}

void set_alarm(int n_alarm)
{
  int temp_hours = alarm_time[n_alarm].hours;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter hour:" + String(temp_hours), 10, 10, 2);

    int pressed_button = wait_for_button_press();
    if (pressed_button == PB_Up)
    {
      delay(200);
      temp_hours++;
      // if(temp_hours > 23)
      //   days++;
      temp_hours = temp_hours % 24;
    }
    else if (pressed_button == PB_Down)
    {
      delay(200);
      temp_hours--;
      if (temp_hours < 0)
      {
        // days--;
        temp_hours = 23;
      }
    }
    else if (pressed_button == PB_OK)
    {
      delay(200);
      alarm_time[n_alarm].hours = temp_hours;
      break;
    }
    else if (pressed_button == PB_Cancel)
    {
      delay(200);
      break;
    }
  }

  int temp_minutes = alarm_time[n_alarm].minutes;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter minutes:" + String(temp_minutes), 10, 10, 2);

    int pressed_button = wait_for_button_press();
    if (pressed_button == PB_Up)
    {
      delay(200);
      temp_minutes++;
      temp_minutes = temp_minutes % 60;
    }
    else if (pressed_button == PB_Down)
    {
      delay(200);
      temp_minutes--;
      if (temp_minutes < 0)
      {
        temp_minutes = 59;
      }
    }
    else if (pressed_button == PB_OK)
    {
      delay(200);
      alarm_time[n_alarm].minutes = temp_minutes;
      break;
    }
    else if (pressed_button == PB_Cancel)
    {
      delay(200);
      break;
    }
  }

  int temp_seconds = alarm_time[n_alarm].seconds;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter seconds:" + String(temp_seconds), 10, 10, 2);

    int pressed_button = wait_for_button_press();
    if (pressed_button == PB_Up)
    {
      delay(200);
      temp_seconds++;
      temp_seconds = temp_seconds % 60;
    }
    else if (pressed_button == PB_Down)
    {
      delay(200);
      temp_seconds--;
      if (temp_seconds < 0)
      {
        temp_seconds = 59;
      }
    }
    else if (pressed_button == PB_OK)
    {
      delay(200);
      alarm_time[n_alarm].seconds = temp_seconds;
      break;
    }
    else if (pressed_button == PB_Cancel)
    {
      delay(200);
      break;
    }
  }
  // Set the alarm state to true (enabled) after setting the time
  alarm_time[n_alarm].alarm_state = true;

  display.clearDisplay();
  print_line("Alarm " + String(n_alarm) + " set to:\n" + String(alarm_time[n_alarm].hours) + ":" + String(alarm_time[n_alarm].minutes) + ":" + String(alarm_time[n_alarm].seconds), 10, 10, 2);
}