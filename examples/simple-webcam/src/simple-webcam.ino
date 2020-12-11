#include <esp32cam.h>
#include <Husarnet.h>
#include <WebServer.h>
#include <WiFi.h>

#if __has_include("credentials.h")
#include "credentials.h"
#else

// WiFi credentials
const char *ssid = "wifi-network";
const char *password = "wifi-password";

// Husarnet credentials
const char *hostName = "esp32webcam";
const char *husarnetJoinCode = "fc94:b01d:1803:8dd8:b293:5c7d:7639:932a/xxxxxxxxxxxxxxxxxxx";
const char *dashboardURL = "default";

#endif

// Stream configuration
static const int httpPort = 8000;
static const char *streamPath = "/stream";

// Setup global objects
WebServer server(httpPort);

void handleMjpeg()
{
  // Configure stream
  static struct esp32cam::CameraClass::StreamMjpegConfig mjcfg;
  mjcfg.frameTimeout = 1000; // ms
  mjcfg.minInterval = 100;   // ms
  mjcfg.maxFrames = -1;      // -1 means - send frames until error occurs or client disconnects

  // Actually stream
  auto client = server.client();

  auto startTime = millis();

  int res = esp32cam::Camera.streamMjpeg(client, mjcfg);
  if (res <= 0)
  {
    Serial.printf("Stream error: %d\n", res);
    return;
  }

  auto duration = millis() - startTime;
  Serial.printf("Stream end %d frames, on average %0.2f FPS\n", res, 1000.0 * res / duration);
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  // Configure camera
  esp32cam::Config cfg;
  // cfg.setPins(esp32cam::pins::AiThinker);
  cfg.setPins(esp32cam::Pins{
    D0 : 17,
    D1 : 35,
    D2 : 34,
    D3 : 5,
    D4 : 39,
    D5 : 18,
    D6 : 36,
    D7 : 19,
    XCLK : 27,
    PCLK : 21,
    VSYNC : 22,
    HREF : 26,
    SDA : 25,
    SCL : 23,
    RESET : 15,
    PWDN : -1,
  });
  cfg.setResolution(esp32cam::Resolution::find(320, 240));
  cfg.setBufferCount(4);
  cfg.setJpeg(80);

  bool ok = esp32cam::Camera.begin(cfg);
  if (!ok)
  {
    Serial.println("Camera initialization failed");
  }

  // Connect to Wi-Fi
  Serial.printf("Connecting to %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("done!\r\nLocal IP: %s\r\n", WiFi.localIP().toString().c_str());

  // Start Husarnet
  Husarnet.selfHostedSetup(dashboardURL);
  Husarnet.join(husarnetJoinCode, hostName);
  Husarnet.start();

  // Setup HTTP endpoints
  server.on(streamPath, handleMjpeg);
  server.begin();

  // Output some user friendly data
  Serial.printf("Local URL:    http://%s:%d%s\r\n", WiFi.localIP().toString().c_str(), httpPort, streamPath);
  Serial.printf("Husarnet URL: http://%s:%d%s\r\n", hostName, httpPort, streamPath);
}

void loop()
{
  server.handleClient();
  delay(10);
}