#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h> // For storing Wi-Fi credentials persistently
#include <DNSServer.h>

// Define your LED pin
#define LED_PIN 2
const byte DNS_PORT = 53;

// Define your Access Point SSID and Password
const char* ap_ssid = "pip_1";
const char* ap_password = "thisispip";

// Create a web server on port 80
IPAddress apIP(192, 168, 4, 1)
DNSServer dnsServer;
WebServer server(80);

// Preferences to store Wi-Fi credentials
Preferences preferences;

String stored_ssid;
String stored_password;

bool isConnected = false;

void handleRoot() {
  String html = "<html><body><h1>Wi-Fi Configuration</h1>"
                "<form action=\"/connect\" method=\"POST\">"
                "SSID: <input type=\"text\" name=\"ssid\"><br>"
                "Password: <input type=\"text\" name=\"password\"><br>"
                "<input type=\"submit\" value=\"Connect\">"
                "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void handleConnect() {
  // Get Wi-Fi credentials from the form
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  Serial.println("In handle connect: receied ssid: " + ssid);
  Serial.println("In handle connect: recevied pw: " + password);

  // Store credentials in Preferences
  preferences.begin("wifi-creds", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();

  // Try to connect to the new Wi-Fi credentials
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Attempting to connect to new Wi-Fi credentials...");

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print("waiting.");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi!");
    digitalWrite(LED_PIN, HIGH); // Turn on LED to show success
    server.send(200, "text/html", "Connected! ESP will reboot.");
    delay(2000);
    ESP.restart(); // TODO: Might not be needed
  } else {
    server.send(200, "text/html", "Failed to connect. Please try again.");
  }
}

void startAccessPoint() {
  // Create Access Point
  WiFi.disconnect(true);  // Disconnect from any station mode connection
  WiFi.mode(WIFI_AP);     // Force Wi-Fi into Access Point mode

  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  // Start DNS server to redirect all queries to our IP
  dnsServer.start(DNS_PORT, "*", apIP);

  // Start Web Server to handle Wi-Fi credentials input
  server.on("/", handleRoot);
  server.on("/connect", handleConnect);
  server.onNotFound(handleNotFound); // Handle undefined routes
  server.begin();

  Serial.println("Access Point started. Connect to " + String(ap_ssid));
  Serial.println("IP Address: 192.168.4.1");

  while (!isConnected) {
    dnsServer.processNextRequest();  // Process DNS requests to redirect all to the IP
    server.handleClient(); // Handle client requests
  }
}

void setup() {
  Serial.begin(115200);
  delay(5000); // Add this to ensure time for Serial Monitor to connect

  // Setup onboard LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Turn off LED initially

  // Load Wi-Fi credentials from memory
  preferences.begin("wifi-creds", false);  // Namespace: "wifi-creds"
  stored_ssid = preferences.getString("ssid", "");
  stored_password = preferences.getString("password", "");
  preferences.end();

  Serial.println("stored_ssid: " + stored_ssid);
  Serial.println("stored_password: " + stored_password);

  if (stored_ssid == "" || stored_password == "") {
    Serial.println("No stored credentials found. Starting AP mode...");
    startAccessPoint();
  } else {
    // Try to connect to the saved Wi-Fi credentials
    WiFi.begin(stored_ssid.c_str(), stored_password.c_str());
    Serial.println("Attempting to connect to saved Wi-Fi...");

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
      delay(500);
      Serial.print(".");
      retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to Wi-Fi!");
      digitalWrite(LED_PIN, HIGH);  // Turn on LED to show success
      isConnected = true;
    } else {
      Serial.println("Failed to connect to saved Wi-Fi. Starting AP mode...");
      startAccessPoint();
    }
  }
}

void loop() {
  // Nothing in the loop, as everything is handled in event-driven handlers
}
