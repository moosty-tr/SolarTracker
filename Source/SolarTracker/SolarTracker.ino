#include <WiFi.h>
#include <time.h>  // Time functions for ESP32
#include <math.h>
#include <SolarCalculator.h>
#include <TimeLib.h>

// Replace with your network credentials
const char* ssid     = "SUPERONLINE_WiFi_4684";
const char* password = "KvCyye6s6nAf";

// NTP server details
const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 0; // 3 * 60 * 60; 
const int daylightOffset_sec = 0;       // No daylight saving currently
int utc_offset = 3;
// Your geographical location (Ankara, Turkey)
const float latitude = 39.991536758134075;  
const float longitude = 32.622165021691444;

// Constants for solar position calculation
const float MY_PI = 3.14159265358979323846;
const float degToRad = MY_PI / 180.0;
const float radToDeg = 180.0 / MY_PI;

double az, el;                    // Horizontal coordinates, in degrees

// Function to connect to Wi-Fi
void setupWifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

// Function to initialize and get the time from NTP server
void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for NTP time sync...");
  delay(5000); // Wait for NTP sync

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  } else {
    Serial.println(&timeinfo, "Time: %A, %B %d %Y %H:%M:%S");
  }
}

// Convert the date and time to Julian Day
double calculateJulianDay(int year, int month, int day, int hour, int minute, int second) {
  if (month <= 2) {
    year -= 1;
    month += 12;
  }
  int A = year / 100;
  int B = 2 - A + (A / 4);
  double JD = (int)(365.25 * (year + 4716)) + (int)(30.6001 * (month + 1)) + day + B - 1524.5;
  JD += (hour + (minute / 60.0) + (second / 3600.0)) / 24.0;
  Serial.print("Julian Day:");
  Serial.println(JD);
  return JD;
}

// Calculate the equation of time (in minutes)
double calculateEquationOfTime(double D) {
  double g = (357.529 + 0.98560028 * D) * degToRad;
  double C = (1.9148 * sin(g) + 0.0200 * sin(2 * g) + 0.0003 * sin(3 * g)) * degToRad;
  double L0 = (280.46646 + 0.98564736 * D) * degToRad;
  double eot = 4 * ((L0 + C) - (MY_PI / 2)); // Equation of time in minutes
  return eot * radToDeg;  // Convert to degrees
}

// Calculate solar declination, equation of time, and other parameters
void calculateSunPosition(float latitude, float longitude, int year, int month, int day, int hour, int minute, int second) {
  double JD = calculateJulianDay(year, month, day, hour, minute, second);
  double D = JD - 2451545.0; // Days since J2000.0
  
  // Mean anomaly of the Sun
  double g = 357.529 + 0.98560028 * D;
  g = fmod(g, 360.0);
  g = g * degToRad;

  // Equation of center
  double C = 1.9148 * sin(g) + 0.0200 * sin(2 * g) + 0.0003 * sin(3 * g);

  // Mean longitude of the Sun
  double L0 = 280.46646 + 0.98564736 * D;
  L0 = fmod(L0, 360.0);
  L0 = L0 * degToRad;

  // True longitude of the Sun
  double lambda = L0 + C * degToRad;

  // Obliquity of the ecliptic
  double epsilon = 23.439 - 0.00000036 * D;
  epsilon = epsilon * degToRad;

  // Declination of the Sun
  double declination = asin(sin(epsilon) * sin(lambda));

  // Equation of time
  double equationOfTime = calculateEquationOfTime(D);  // In degrees

  // Time of solar noon at longitude
  double solarNoonOffset = 4.0 * (longitude - equationOfTime); // Adjust for equation of time
  double solarTime = hour + (minute / 60.0) + (second / 3600.0) + solarNoonOffset / 60.0;
  double hourAngle = (solarTime - 12.0) * 15.0 * degToRad;  // Hour angle in radians

  // Solar zenith angle
  double zenith = acos(sin(latitude * degToRad) * sin(declination) + cos(latitude * degToRad) * cos(declination) * cos(hourAngle));

  // Solar azimuth angle
  double azimuth = atan2(-sin(hourAngle), (cos(hourAngle) * sin(latitude * degToRad) - tan(declination) * cos(latitude * degToRad)));

  // Convert results from radians to degrees
  zenith = zenith * radToDeg;
  azimuth = azimuth * radToDeg;

  // Apply atmospheric refraction correction (approximately 0.57 degrees for the horizon)
  if (zenith > 90) { // Only apply if the sun is below the horizon
    zenith += 0.57; // Refraction correction in degrees
  }

  // Ensure azimuth is within 0 to 360 degrees
  if (azimuth < 0) {
    azimuth += 360.0;
  }

  // Output calculated values
  Serial.print("Zenith: ");
  Serial.println(zenith);
  Serial.print("Azimuth: ");
  Serial.println(azimuth);
}

void setup() {
  Serial.begin(115200);
  setupWifi();
  setupTime();
}



void loop() {
  // Get current time
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int year = timeinfo.tm_year + 1900;
    int month = timeinfo.tm_mon + 1;
    int day = timeinfo.tm_mday;
    int hour = timeinfo.tm_hour;
    int minute = timeinfo.tm_min;
    int second = timeinfo.tm_sec;

    Serial.print("Currrent Time:");
    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.print(second);
    Serial.println(" (UTC)");

    setTime(hour,minute,second,day, month, year);
    time_t utc = now();
    // Calculate the sun's position
    //calculateSunPosition(latitude, longitude, year, month, day, hour, minute, second);
    calcHorizontalCoordinates(utc, latitude, longitude, az, el);

    Serial.print("Solar Azimuth Angle :");
    Serial.println(az);
    Serial.print("Solar Elevation Angle :");
    Serial.println(el);
    
  } else {
    Serial.println("Failed to obtain time in loop");
  }

  delay(60000); // Update every minute
}