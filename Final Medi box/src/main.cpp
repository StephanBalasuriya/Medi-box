#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>

#define Buzzer 18
#define LED 19
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define PB_Cancel 23
#define PB_OK 2
#define PB_Up 4
#define PB_Down 5
#define DHT22_PIN 16
#define I2C0_SDA 21 // OLED1
#define I2C0_SCL 22 // OLED1
#define I2C1_SDA 12 // OLED2
#define I2C1_SCL 13 // OLED2
#define OLED_ADDRESS 0x3C
#define NTP_SERVER "pool.ntp.org"

DHTesp dhtSensor;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);   // OLED1 on I2C0
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1); // OLED2 on I2C1

// Global Variables
int hours = 0, minutes = 0, seconds = 0, days = 0, months = 0;
;
long utc_offset = 0; // UTC offset in seconds
bool alarm_enable = true;

struct alarm_time_t
{
  bool alarm_state;
  int hours, minutes, seconds;
  bool snoozed;
  unsigned long snooze_time;
};

constexpr int n_alarm = 2;
alarm_time_t alarm_time[n_alarm] = {
    {true, 0, 0, 5, false, 0}, // Alarm 1
    {false, 0, 0, 0, false, 0} // Alarm 2
};

int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
int current_mode = 0;
int max_mode = 6;
String mode_name[] = {
    "1 - Set Time Zone", "2 - Set Alarm 1", "3 - Set Alarm 2",
    "4 - View Alarms", "5 - Delete Alarm 1", "6 - Delete Alarm 2"};

// Function Declarations
void print_line(Adafruit_SSD1306 &disp, String text, int col, int row, int size);
void print_time_now();
void update_time();
void update_time_with_check_alarm();
void ring_alarm(int alarm_idx);
void go_to_menu();
int wait_for_button_press();
void run_mode(int mode);
void set_time_zone();
void set_alarm(int n_alarm);
void view_alarms();
void delete_alarm(int n_alarm);
void check_temperature_humidity();
void spinner();

// Setup
void setup()
{
  Serial.begin(9600);

  Wire.begin(I2C1_SDA, I2C1_SCL);  // I2C1 for OLED2
  Wire1.begin(I2C0_SDA, I2C0_SCL); // I2C0 for OLED1

  pinMode(Buzzer, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(PB_Cancel, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_Up, INPUT);
  pinMode(PB_Down, INPUT);

  dhtSensor.setup(DHT22_PIN, DHTesp::DHT22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    Serial.println(F("Display 1 failed"));
    for (;;)
      ;
  }
  if (!display2.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    Serial.println(F("Display 2 failed"));
    for (;;)
      ;
  }

  display.display();
  display2.display();
  delay(500);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    display.clearDisplay();
    print_line(display, "Connecting to\n  WiFi...", 10, 10, 2);
    spinner();
  }

  display.clearDisplay();
  print_line(display, "Connected to\n  WiFi", 10, 10, 2);
  delay(1000);
  configTime(utc_offset, 0, NTP_SERVER);

  display.clearDisplay();
  display2.clearDisplay();
  print_line(display, " Welcome\n    to\n  Medibox", 10, 10, 2);
}

// Loop
void loop()
{
  display.clearDisplay();
  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW)
  {
    delay(100);
    Serial.println("Go to menu");
    go_to_menu();
  }
  check_temperature_humidity();
}

void print_line(Adafruit_SSD1306 &disp, String text, int col, int row, int size)
{
  disp.clearDisplay();
  disp.setTextSize(size);
  disp.setTextColor(SSD1306_WHITE);
  disp.setCursor(col, row);
  disp.println(text);
  disp.display();
}

void print_time_now()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 00);
  display.print("Time: ");
  display.setTextSize(2);
  display.setCursor(10, 20);
  if (hours < 10)
    display.print("0");
  display.print(hours);
  display.print(":");
  if (minutes < 10)
    display.print("0");
  display.print(minutes);
  display.print(":");
  if (seconds < 10)
    display.print("0");
  display.print(seconds);

  display.setCursor(10, 40);
  if (months == 1)
    display.print("Jan");
  else if (months == 2)
    display.print("Feb");
  else if (months == 3)
    display.print("Mar");
  else if (months == 4)
    display.print("Apr");
  else if (months == 5)
    display.print("May");
  else if (months == 6)
    display.print("Jun");
  else if (months == 7)
    display.print("Jul");
  else if (months == 8)
    display.print("Aug");
  else if (months == 9)
    display.print("Sep");
  else if (months == 10)
    display.print("Oct");
  else if (months == 11)
    display.print("Nov");
  else if (months == 12)
    display.print("Dec");
  display.print(":");
  display.print(days);

  display.display();
}

void update_time()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  days = timeinfo.tm_mday;
  months = timeinfo.tm_mon + 1;
}

void update_time_with_check_alarm()
{
  update_time();
  print_time_now();

  if (alarm_enable)
  {
    for (int i = 0; i < n_alarm; i++)
    {
      if (alarm_time[i].alarm_state)
      {
        if (alarm_time[i].hours == hours && alarm_time[i].minutes == minutes &&
            alarm_time[i].seconds == seconds && !alarm_time[i].snoozed)
        {
          Serial.println("Alarm " + String(i) + " Triggered!");
          ring_alarm(i);
        }
        if (alarm_time[i].snoozed && millis() - alarm_time[i].snooze_time >= 300000)
        { // 5 min snooze
          alarm_time[i].snoozed = false;
          if (alarm_time[i].hours == hours && alarm_time[i].minutes == minutes &&
              alarm_time[i].seconds == seconds)
          {
            ring_alarm(i);
          }
        }
      }
    }
    alarm_enable = alarm_time[0].alarm_state || alarm_time[1].alarm_state;
    Serial.print("Alarm state: ");
    Serial.println(alarm_enable ? "ON" : "OFF");
  }
}

void ring_alarm(int alarm_idx)
{
  print_line(display, " Medicine\n   Time!\nAlarm " + String(alarm_idx + 1), 10, 10, 2);
  bool stopped = false;
  while (!stopped)
  {
    digitalWrite(LED, HIGH);
    for (int i = 0; i < 8; i++)
    {
      if (digitalRead(PB_Cancel) == LOW)
      { // Stop
        delay(200);
        stopped = true;
        digitalWrite(LED, LOW);
        noTone(Buzzer);
        alarm_time[alarm_idx].alarm_state = false;
        break;
      }
      if (digitalRead(PB_OK) == LOW)
      { // Snooze
        delay(200);
        stopped = true;
        digitalWrite(LED, LOW);
        noTone(Buzzer);
        alarm_time[alarm_idx].snoozed = true;
        alarm_time[alarm_idx].snooze_time = millis();
        print_line(display, "Snoozed 5 min", 10, 10, 2);
        delay(1000);
        break;
      }
      tone(Buzzer, melody[i]);
      delay(500);
      noTone(Buzzer);
      delay(50);
    }
  }
  display.clearDisplay();
}

void go_to_menu()
{
  print_line(display, "Menu", 10, 10, 2);
  delay(1000);
  while (digitalRead(PB_Cancel) == HIGH)
  {
    display.clearDisplay();
    print_line(display, mode_name[current_mode], 10, 10, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_Up)
    {
      delay(100);
      current_mode = (current_mode + 1) % max_mode;
    }
    else if (pressed == PB_Down)
    {
      delay(100);
      current_mode = (current_mode - 1 + max_mode) % max_mode;
    }
    else if (pressed == PB_OK)
    {
      delay(100);
      Serial.println("Run mode: " + String(current_mode));
      run_mode(current_mode);
    }
    else if (pressed == PB_Cancel)
    {
      delay(100);
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
      delay(100);
      return PB_Cancel;
    }
    if (digitalRead(PB_OK) == LOW)
    {
      delay(100);
      return PB_OK;
    }
    if (digitalRead(PB_Up) == LOW)
    {
      delay(100);
      return PB_Up;
    }
    if (digitalRead(PB_Down) == LOW)
    {
      delay(100);
      return PB_Down;
    }
    update_time();
  }
}

void run_mode(int mode)
{
  if (mode == 0)
    set_time_zone();
  else if (mode == 1 || mode == 2)
    set_alarm(mode - 1);
  else if (mode == 3)
    view_alarms();
  else if (mode == 4 || mode == 5)
    delete_alarm(mode - 4);
}

void set_time_zone()
{
  int temp_offset = utc_offset / 3600; // Convert seconds to hours
  while (true)
  {
    print_line(display, "UTC Offset:\n" + String(temp_offset) + "h", 00, 10, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_Up)
    {
      temp_offset = min(temp_offset + 1, 14);
      delay(100);
    }
    else if (pressed == PB_Down)
    {
      temp_offset = max(temp_offset - 1, -12);
      delay(100);
    }
    else if (pressed == PB_OK)
    {
      utc_offset = temp_offset * 3600;
      configTime(utc_offset, 0, NTP_SERVER);
      print_line(display, "Time Zone Set", 10, 10, 2);
      delay(1000);
      break;
    }
    else if (pressed == PB_Cancel)
    {
      delay(100);
      break;
    }
  }
}

void set_alarm(int n_alarm)
{
  int temp_hours = alarm_time[n_alarm].hours;
  while (true)
  {
    print_line(display, "Alarm " + String(n_alarm + 1) + "\nHour: " + String(temp_hours), 10, 10, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_Up)
    {
      temp_hours = (temp_hours + 1) % 24;
      delay(100);
    }
    else if (pressed == PB_Down)
    {
      temp_hours = (temp_hours - 1 + 24) % 24;
      delay(100);
    }
    else if (pressed == PB_OK)
    {
      alarm_time[n_alarm].hours = temp_hours;
      delay(100);
      break;
    }
    else if (pressed == PB_Cancel)
    {

      delay(100);
      return;
    }
  }

  int temp_minutes = alarm_time[n_alarm].minutes;
  while (true)
  {
    print_line(display, "Alarm " + String(n_alarm + 1) + "\nMin: " + String(temp_minutes), 10, 10, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_Up)
    {
      temp_minutes = (temp_minutes + 1) % 60;
      delay(100);
    }
    else if (pressed == PB_Down)
    {
      temp_minutes = (temp_minutes - 1 + 60) % 60;
      delay(100);
    }
    else if (pressed == PB_OK)
    {
      alarm_time[n_alarm].minutes = temp_minutes;
      delay(100);
      break;
    }
    else if (pressed == PB_Cancel)
    {
      delay(100);
      return;
    }
  }

  int temp_seconds = alarm_time[n_alarm].seconds;
  while (true)
  {
    print_line(display, "Alarm " + String(n_alarm + 1) + "\nSec: " + String(temp_seconds), 10, 10, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_Up)
    {
      temp_seconds = (temp_seconds + 1) % 60;
      delay(100);
    }
    else if (pressed == PB_Down)
    {
      temp_seconds = (temp_seconds - 1 + 60) % 60;
      delay(100);
    }
    else if (pressed == PB_OK)
    {
      alarm_time[n_alarm].seconds = temp_seconds;
      alarm_time[n_alarm].alarm_state = true;
      alarm_time[n_alarm].snoozed = false;
      alarm_enable = true;
      print_line(display, "Alarm " + String(n_alarm + 1) + " Set", 10, 10, 2);
      delay(1000);
      break;
    }
    else if (pressed == PB_Cancel)
    {
      delay(100);
      return;
    }
  }
}

void view_alarms()
{
  display.clearDisplay();
  // display.setTextSize(1);
  // display.setCursor(0, 0);
  // display.print("Active Alarms:");
  display.setTextSize(2);
  for (int i = 0; i < n_alarm; i++)
  {
    if (alarm_time[i].alarm_state)
    {
      display.setCursor(0, i * 30);
      display.print("A" + String(i + 1) + ": ");
      if (alarm_time[i].hours < 10)
        display.print("0");
      display.print(alarm_time[i].hours);
      display.print(":");
      if (alarm_time[i].minutes < 10)
        display.print("0");
      display.print(alarm_time[i].minutes);
      display.print(":");
      if (alarm_time[i].seconds < 10)
        display.print("0");
      display.print(alarm_time[i].seconds);
    }
  }
  display.display();
  delay(3000);
}

void delete_alarm(int n_alarm)
{
  alarm_time[n_alarm].alarm_state = false;
  alarm_time[n_alarm].snoozed = false;
  print_line(display, "Alarm " + String(n_alarm + 1) + "\nDeleted", 10, 10, 2);
  delay(1000);
  alarm_enable = alarm_time[0].alarm_state || alarm_time[1].alarm_state;
}

void check_temperature_humidity()
{
  delay(dhtSensor.getMinimumSamplingPeriod());
  float temp = dhtSensor.getTemperature();
  float hum = dhtSensor.getHumidity();

  display2.clearDisplay();
  display2.setTextSize(2);
  display2.setTextColor(SSD1306_WHITE);
  display2.setCursor(0, 0);
  display2.print("Temp: ");
  display2.setCursor(30, 15);
  display2.print(temp);
  display2.print(" C");
  display2.setCursor(0, 30);
  display2.print("Hum: ");
  display2.setCursor(30, 45);
  display2.print(hum);
  display2.print(" %");

  if (temp < 24 || temp > 32 || hum < 65 || hum > 80)
  {
    tone(Buzzer, melody[7]);
    digitalWrite(LED, HIGH);
    delay(500);
    noTone(Buzzer);
    digitalWrite(LED, LOW);
    delay(500);
    print_line(display2, "Warning!\nTemp/Hum\nOut of Range", 10, 10, 2);
    delay(1000);
  }
  display2.display();
}

void spinner()
{
  static int8_t counter = 0;
  const char *glyphs = "\xa1\xa5\xdb";
  display.setCursor(100, 40);
  display.print(glyphs[counter++]);
  if (counter == strlen(glyphs))
    counter = 0;
  display.display();
}