#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// FreeRTOS Task Handles
TaskHandle_t TaskGetArd_Handle;
TaskHandle_t TaskDisplay_Handle;

// Mutex for protecting sensorValues between tasks
SemaphoreHandle_t sensorMutex;

// OLED configuration
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledInit = false;

// forward
void TaskDisplay(void *pvParameters);

// Data variables
struct SensorData
{
  int airQuality;
  int Gas;
  int pm1;
  int pm2_5;
  int pm10;
} sensorValues;

// forward declare display update (SensorData is defined above)
void updateDisplay(SensorData data);

#define ard_node Serial2 // Use Serial1 for Mega communication

void setup()
{
  Serial.begin(115200);
  ard_node.begin(115200, SERIAL_8N1, 16, 17); // TX = GPIO 17, RX = GPIO 16

  // Initialize serial interfaces
  // Initialize I2C for OLED (ESP32 default SDA=21, SCL=22)
  Wire.begin(21, 22);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
  }
  else
  {
    display.clearDisplay();

    // Create mutex for sensor data
    sensorMutex = xSemaphoreCreateMutex();
    if (sensorMutex == NULL)
    {
      Serial.println("Failed to create sensor mutex");
    }
    display.display();
    oledInit = true;
  }

  // FreeRTOS task to read from Arduino
  xTaskCreatePinnedToCore(TaskGetArd, "TaskGetArd", 4096, NULL, 1, &TaskGetArd_Handle, 1);
  // FreeRTOS task to refresh OLED display
  xTaskCreatePinnedToCore(TaskDisplay, "TaskDisplay", 4096, NULL, 1, &TaskDisplay_Handle, 1);
}

void loop() { delay(1000); }

// FreeRTOS Tasks
void TaskGetArd(void *pvParameters)
{
  while (1)
  {
    getArd();                             // Get data from Arduino
    vTaskDelay(100 / portTICK_PERIOD_MS); // Check every 100ms
  }
}

// Function to get data from Arduino (using reference logic)
void getArd()
{
  String SS = ""; // Initialize string to store received data
  while (ard_node.available() > 0)
  {                           // While data is available from Arduino via Serial1 (ard_node)
    char S = ard_node.read(); // Read a character
    SS += S;                  // Append character to string
    delay(5);                 // Wait for 5ms
  }

  if (SS.length() > 0)
  {                                                      // If data received
    Serial.println("Received data from Arduino: " + SS); // Print the received data

    // Extracting sensor values from the received comma-separated data
    // New expected format: airQuality,Gas,pm1,pm2_5,pm10,
    SensorData tmp;
    tmp.airQuality = getSplitValue(SS, ',', 0).toInt(); // Air Quality
    tmp.Gas = getSplitValue(SS, ',', 1).toInt();        // Gas
    tmp.pm1 = getSplitValue(SS, ',', 2).toInt();        // PM1
    tmp.pm2_5 = getSplitValue(SS, ',', 3).toInt();      // PM2.5
    tmp.pm10 = getSplitValue(SS, ',', 4).toInt();       // PM10

    ard_node.flush(); // Clear the Serial buffer

    // copy parsed values into shared sensorValues under mutex
    if (sensorMutex != NULL)
    {
      if (xSemaphoreTake(sensorMutex, (TickType_t)10) == pdTRUE)
      {
        sensorValues = tmp;
        xSemaphoreGive(sensorMutex);
      }
    }
    else
    {
      sensorValues = tmp;
    }
  }
}

// Task: periodically read sensorValues and update the OLED
void TaskDisplay(void *pvParameters)
{
  (void)pvParameters;
  SensorData localCopy;
  while (1)
  {
    // copy sensor values under mutex
    if (sensorMutex != NULL)
    {
      if (xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE)
      {
        localCopy = sensorValues;
        xSemaphoreGive(sensorMutex);
      }
    }
    else
    {
      localCopy = sensorValues;
    }

    // update OLED with the copied values
    if (oledInit)
    {
      updateDisplay(localCopy);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // update every 1 second
  }
}

// Update OLED display with latest sensor values
void updateDisplay(struct SensorData data)
{
  if (!oledInit)
    return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Air Quality Monitor");
  display.println();

  display.setTextSize(1);
  display.print("AQI: ");
  display.println(data.airQuality);
  display.print("Gas: ");
  display.println(data.Gas);
  display.print("PM1: ");
  display.println(data.pm1);
  display.print("PM2.5: ");
  display.println(data.pm2_5);
  display.print("PM10: ");
  display.println(data.pm10);

  display.display();
}

// Function to split a string based on a separator and return a specific index value
String getSplitValue(String data, char separator, int index)
{
  int found = 0;                    // Initialize found counter
  int strIndex[] = {0, -1};         // Initialize string index array
  int maxIndex = data.length() - 1; // Get the maximum index of the string

  for (int i = 0; i <= maxIndex && found <= index; i++)
  { // Loop through the string
    if (data.charAt(i) == separator || i == maxIndex)
    {                                            // If separator or end of string is found
      found++;                                   // Increment the found counter
      strIndex[0] = strIndex[1] + 1;             // Set the start index of the substring
      strIndex[1] = (i == maxIndex) ? i + 1 : i; // Set the end index of the substring
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : ""; // Return the substring
}
