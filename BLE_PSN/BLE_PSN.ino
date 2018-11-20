// Default Arduino includes
#include <Arduino.h>
#include <WiFi.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <ArduinoJson.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <Preferences.h>

const char compileDate[] = __DATE__ " " __TIME__;

/** Unique device name */
char apName[] = "PSN100";
/** Selected network
true = use primary network
false = use secondary network
*/
bool usePrimAP = true;
/** Flag if stored AP credentials are available */
bool hasCredentials = false;
/** Connection status */
volatile bool isConnected = false;
/** Connection change status */
bool connStatusChanged = false;

/**
* Create unique device name from MAC address
**/
void createName() {
	uint8_t baseMac[6];
	// Get MAC address for WiFi station
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	// Write unique name into apName
	sprintf(apName, "PSN100");
}

// List of Service and Characteristic UUIDs

#define SERVICE_UUID  "0000aaaa-ead2-11e7-80c1-9a214cf093ae"
#define WIFI_UUID     "00005555-ead2-11e7-80c1-9a214cf093ae"

String ssidPrim;
String Token;
String pwPrim;

/** Characteristic for digital output */
BLECharacteristic *pCharacteristicWiFi;
/** BLE Advertiser */
BLEAdvertising* pAdvertising;
/** BLE Service */
BLEService *pService;
/** BLE Server */
BLEServer *pServer;

/** Buffer for JSON string */
// MAx size is 51 bytes for frame: 
// {"ssidPrim":"","pwPrim":"","ssidSec":"","pwSec":""}
// + 4 x 32 bytes for 2 SSID's and 2 passwords
StaticJsonBuffer<150> jsonBuffer;

/**
* MyServerCallbacks
* Callbacks for client connection and disconnection
*/
class MyServerCallbacks : public BLEServerCallbacks {
	// TODO this doesn't take into account several clients being connected
	void onConnect(BLEServer* pServer) {
		Serial.println("BLE client connected");
	};

	void onDisconnect(BLEServer* pServer) {
		Serial.println("BLE client disconnected");
		pAdvertising->start();
	}
};

/**
* MyCallbackHandler
* Callbacks for BLE client read/write requests
*/
class MyCallbackHandler : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		std::__cxx11::string value = pCharacteristic->getValue();
		if (value.length() == 0) {
			return;
		}
		Serial.println("Received over BLE: " + String((char *)&value[0]));

		// Decode data
		int keyIndex = 0;
		for (int index = 0; index < value.length(); index++) {
			value[index] = (char)value[index] ^ (char)apName[keyIndex];
			keyIndex++;
			if (keyIndex >= strlen(apName)) keyIndex = 0;
		}

		/** Json object for incoming data */
		JsonObject& jsonIn = jsonBuffer.parseObject((char *)&value[0]);
		if (jsonIn.success()) {
			if (jsonIn.containsKey("ssidPrim") &&
				jsonIn.containsKey("pwPrim") &&
				jsonIn.containsKey("ssidSec") &&
				jsonIn.containsKey("pwSec")) {
				ssidPrim = jsonIn["ssidPrim"].as<String>();
				pwPrim = jsonIn["pwPrim"].as<String>();
				Token = jsonIn["Token"].as<String>();
				
				Preferences preferences;
				preferences.begin("WiFiCred", false);
				preferences.putString("ssidPrim", ssidPrim);
				preferences.putString("Token", Token);
				preferences.putString("pwPrim", pwPrim);
				preferences.putBool("valid", true);
				preferences.end();

				Serial.println("Received over bluetooth:");
				Serial.println("primary SSID: " + ssidPrim + " password: " + pwPrim);
				connStatusChanged = true;
				hasCredentials = true;
			}
			else if (jsonIn.containsKey("erase")) {
				Serial.println("Received erase command");
				Preferences preferences;
				preferences.begin("WiFiCred", false);
				preferences.clear();
				preferences.end();
				connStatusChanged = true;
				hasCredentials = false;
				ssidPrim = "";
				pwPrim = "";
				Token = "";

				int err;
				err = nvs_flash_init();
				Serial.println("nvs_flash_init: " + err);
				err = nvs_flash_erase();
				Serial.println("nvs_flash_erase: " + err);
			}
			else if (jsonIn.containsKey("reset")) {
				WiFi.disconnect();
				esp_restart();
			}
		}
		else {
			Serial.println("Received invalid JSON");
		}
		jsonBuffer.clear();
	};

	void onRead(BLECharacteristic *pCharacteristic) {
		Serial.println("BLE onRead request");
		String wifiCredentials;

		/** Json object for outgoing data */
		JsonObject& jsonOut = jsonBuffer.createObject();
		jsonOut["ssidPrim"] = ssidPrim;
		jsonOut["pwPrim"] = pwPrim;
		jsonOut["Token"] = Token;
		// Convert JSON object into a string
		jsonOut.printTo(wifiCredentials);

		// encode the data
		int keyIndex = 0;
		Serial.println("Stored settings: " + wifiCredentials);
		for (int index = 0; index < wifiCredentials.length(); index++) {
			wifiCredentials[index] = (char)wifiCredentials[index] ^ (char)apName[keyIndex];
			keyIndex++;
			if (keyIndex >= strlen(apName)) keyIndex = 0;
		}
		pCharacteristicWiFi->setValue((uint8_t*)&wifiCredentials[0], wifiCredentials.length());
		jsonBuffer.clear();
	}
};

/**
* initBLE
* Initialize BLE service and characteristic
* Start BLE server and service advertising
*/
void initBLE() {
	// Initialize BLE and set output power
	BLEDevice::init(apName);
	BLEDevice::setPower(ESP_PWR_LVL_P7);

	// Create BLE Server
	pServer = BLEDevice::createServer();

	// Set server callbacks
	pServer->setCallbacks(new MyServerCallbacks());

	// Create BLE Service
	pService = pServer->createService(BLEUUID(SERVICE_UUID), 20);

	// Create BLE Characteristic for WiFi settings
	pCharacteristicWiFi = pService->createCharacteristic(
		BLEUUID(WIFI_UUID),
		// WIFI_UUID,
		BLECharacteristic::PROPERTY_READ |
		BLECharacteristic::PROPERTY_WRITE
	);
	pCharacteristicWiFi->setCallbacks(new MyCallbackHandler());

	// Start the service
	pService->start();

	// Start advertising
	pAdvertising = pServer->getAdvertising();
	pAdvertising->start();
}

/** Callback for receiving IP address from AP */
void gotIP(system_event_id_t event) {
	isConnected = true;
	connStatusChanged = true;
}

/** Callback for connection loss */
void lostCon(system_event_id_t event) {
	isConnected = false;
	connStatusChanged = true;
}

/**
scanWiFi
Scans for available networks
and decides if a switch between
allowed networks makes sense

@return <code>bool</code>
True if at least one allowed network was found
*/
bool scanWiFi() {
	/** RSSI for primary network */
	int8_t rssiPrim;
	/** RSSI for secondary network */
	int8_t rssiSec;
	/** Result of this function */
	bool result = false;

	Serial.println("Start scanning for networks");

	WiFi.disconnect(true);
	WiFi.enableSTA(true);
	WiFi.mode(WIFI_STA);

	// Scan for AP
	int apNum = WiFi.scanNetworks(false, true, false, 1000);
	if (apNum == 0) {
		Serial.println("Found no networks?????");
		return false;
	}

	byte foundAP = 0;
	bool foundPrim = false;

	for (int index = 0; index<apNum; index++) {
		String ssid = WiFi.SSID(index);
		Serial.println("Found AP: " + ssid + " RSSI: " + WiFi.RSSI(index));
		if (!strcmp((const char*)&ssid[0], (const char*)&ssidPrim[0])) {
			Serial.println("Found primary AP");
			foundAP++;
			foundPrim = true;
			rssiPrim = WiFi.RSSI(index);
		}
	}

	switch (foundAP) {
	case 0:
		result = false;
		break;
	case 1:
		if (foundPrim) {
			usePrimAP = true;
		}
		else {
			usePrimAP = false;
		}
		result = true;
		break;
	default:
		Serial.printf("RSSI Prim: %d\n", rssiPrim);
		
		usePrimAP = true; // RSSI of primary network is better
		result = true;
		break;
	}
	return result;
}

/**
* Start connection to AP
*/
void connectWiFi() {
	// Setup callback function for successful connection
	WiFi.onEvent(gotIP, SYSTEM_EVENT_STA_GOT_IP);
	// Setup callback function for lost connection
	WiFi.onEvent(lostCon, SYSTEM_EVENT_STA_DISCONNECTED);

	WiFi.disconnect(true);
	WiFi.enableSTA(true);
	WiFi.mode(WIFI_STA);

	Serial.println();
	Serial.print("Start connection to ");
	if (usePrimAP) {
		Serial.println(ssidPrim);
		WiFi.begin(ssidPrim.c_str(), pwPrim.c_str());
	}
}

void setup() {
	// Create unique device name
	createName();

	// Initialize Serial port
	Serial.begin(115200);
	// Send some device info
	Serial.print("Build: ");
	Serial.println(compileDate);

	Preferences preferences;
	preferences.begin("WiFiCred", false);
	bool hasPref = preferences.getBool("valid", false);
	if (hasPref) {
		ssidPrim = preferences.getString("ssidPrim", "");
		Token = preferences.getString("Token", "");
		pwPrim = preferences.getString("pwPrim", "");

		if (ssidPrim.equals("")
			|| pwPrim.equals("")
			|| Token.equals("")) {
			Serial.println("Found preferences but credentials are invalid");
		}
		else {
			Serial.println("Read from preferences:");
			Serial.println("primary SSID: " + ssidPrim + " password: " + pwPrim);
			Serial.println("Token: " + Token);
			hasCredentials = true;
		}
	}
	else {
		Serial.println("Could not find preferences, need send data over BLE");
	}
	preferences.end();

	// Start BLE server
	initBLE();

	if (hasCredentials) {
		// Check for available AP's
		if (!scanWiFi) {
			Serial.println("Could not find any AP");
		}
		else {
			// If AP was found, start connection
			connectWiFi();
		}
	}
}

void loop() {
	if (connStatusChanged) {
		if (isConnected) {
			Serial.print("Connected to AP: ");
			Serial.print(WiFi.SSID());
			Serial.print(" with IP: ");
			Serial.print(WiFi.localIP());
			Serial.print(" RSSI: ");
			Serial.println(WiFi.RSSI());
		}
		else {
			if (hasCredentials) {
				Serial.println("Lost WiFi connection");
				// Received WiFi credentials
				if (!scanWiFi) { // Check for available AP's
					Serial.println("Could not find any AP");
				}
				else { // If AP was found, start connection
					connectWiFi();
				}
			}
		}
		connStatusChanged = false;
	}
}