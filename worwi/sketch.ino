#include <WiFi.h>
#include <coap-simple.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

// --- LANE AND SENSOR CONFIGURATION ---
#define LANES 4
#define SENSORS_PER_LANE 2 // Sensor 0 = Entrance (IN), Sensor 1 = Exit (OUT)

// Pin configuration for 4 lanes x 2 sensors = 8 sensor pairs
// Structure: [Lane Index][Sensor Index (0=In, 1=Out)]
// Note: Pins 0, 2, 8, 9, 10, 15, 34, 35, 36, 37, 38, 39 were previously used for LEDs 
// and are now available for other uses in this pinout, but are unused here.
int trigPins[LANES][SENSORS_PER_LANE] = {
  {5, 17},  // Lane 1: In (5), Out (17)
  {16, 4},  // Lane 2: In (16), Out (4)
  {18, 19}, // Lane 3: In (18), Out (19)
  {21, 22}  // Lane 4: In (21), Out (22)
};
int echoPins[LANES][SENSORS_PER_LANE] = {
  {25, 26}, // Lane 1: In (25), Out (26)
  {27, 14}, // Lane 2: In (27), Out (14)
  {12, 13}, // Lane 3: In (12), Out (13)
  {32, 33}  // Lane 4: In (32), Out (33)
};

int threshold = 10; // Distance in cm to consider the sensor 'occupied'


// Net vehicle count in the segment (true density: Vehicles_In - Vehicles_Out)
static int laneDensity[LANES] = {0, 0, 0, 0}; 
// Previous state of each sensor (used for edge detection)
static bool sensorOccupiedPrev[LANES][SENSORS_PER_LANE] = {
  {false, false}, {false, false}, {false, false}, {false, false}
};

// CoAP client initialization
WiFiUDP udp;
Coap coap(udp);

// CoAP server details
IPAddress coapServer;
const char* coapHost = "coap.me";
const int coapPort = 5683;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000;

// --- Ultrasonic Distance Measurement Function ---
long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Send a 10us pulse to trigger the sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure the duration of the echo pulse (up to 30ms timeout)
  long duration = pulseIn(echoPin, HIGH, 30000); 
  if (duration == 0) return -1; // Return -1 on timeout (no object detected within range)
  
  // Convert time to distance (cm) = (Duration * Speed of Sound) / 2
  return duration * 0.034 / 2;
}

// --- CoAP Response Handler ---
void callback_response(CoapPacket &packet, IPAddress ip, int port) {
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = '\0';

  Serial.println("\n=== CoAP Response Received ===");
  Serial.printf("From: %s:%d\n", ip.toString().c_str(), port);
  Serial.print("Payload: ");
  Serial.println(p);
  Serial.println("============================\n");
}


// Sends the calculated net vehicle count (density) for all 4 lanes
void sendLaneDensity(int density[]) {
  char payload[256];
  snprintf(payload, sizeof(payload),
           "{\"lane1_density\":%d,\"lane2_density\":%d,\"lane3_density\":%d,\"lane4_density\":%d}",
           density[0], density[1], density[2], density[3]);

  Serial.println("\n--- Sending Density to coap.me ---");
  Serial.print("Payload: ");
  Serial.println(payload);

  coap.put(coapServer, coapPort, "sink", payload);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup pins for 4 lanes, 2 sensors each
  for (int i = 0; i < LANES; i++) {
    for (int s = 0; s < SENSORS_PER_LANE; s++) {
      pinMode(trigPins[i][s], OUTPUT);
      pinMode(echoPins[i][s], INPUT);
    }
  }

  // WiFi connection setup
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Resolve coap.me host
  Serial.print("Resolving coap.me... ");
  if (WiFi.hostByName(coapHost, coapServer)) {
    Serial.print("Resolved to: ");
    Serial.println(coapServer);
  } else {
    Serial.println("Failed to resolve! Using fallback.");
    coapServer.fromString("134.102.218.18");
  }

  // CoAP client setup
  coap.response(callback_response); // It gets called whenever you receive a message from the server
  coap.start();

  Serial.println("\nCoAP Client Started!");
  Serial.println("Counting lane density using 8 sensors...");

  lastSendTime = millis();
}

void loop() {
  coap.loop();

  unsigned long currentTime = millis();
  long distance;
  bool isOccupied;

  // --- Density Counting Logic (The main task) ---
  for (int i = 0; i < LANES; i++) {
    
    distance = getDistance(trigPins[i][0], echoPins[i][0]);
    isOccupied = (distance >= 0 && distance < threshold);

    // Vehicle Counting: If state transitions from FREE to OCCUPIED at Entrance sensor
    if (isOccupied && !sensorOccupiedPrev[i][0]) {
      laneDensity[i]++;
      Serial.printf("Lane %d: VEHICLE ENTERED. Density: %d\n", i + 1, laneDensity[i]);
    }
    // Update the previous state for the Entrance sensor
    sensorOccupiedPrev[i][0] = isOccupied;
    
    delay(20); // Short delay to prevent sensor crosstalk
    
    distance = getDistance(trigPins[i][1], echoPins[i][1]);
    isOccupied = (distance >= 0 && distance < threshold);

    // Vehicle Counting: If state transitions from OCCUPIED to FREE at Exit sensor
    if (!isOccupied && sensorOccupiedPrev[i][1]) {
      
      laneDensity[i] = (laneDensity[i] > 0) ? laneDensity[i] - 1 : 0; 
      Serial.printf("Lane %d: VEHICLE EXITED. Density: %d\n", i + 1, laneDensity[i]);
    }
    // Update the previous state for the Exit sensor
    sensorOccupiedPrev[i][1] = isOccupied;
  }
  
  // Print Current Density Status for Monitoring
  Serial.println("----------------------------------------");
  for (int i = 0; i < LANES; i++) {
    Serial.printf("LANE %d DENSITY: %d Vehicles ", 
                   i + 1, laneDensity[i]);
  }
  Serial.println("----------------------------------------");


  // --- CoAP Transmission ---
  if (currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;
    sendLaneDensity(laneDensity);
  }

  delay(1000);
}