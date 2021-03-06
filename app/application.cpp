#include "SmingCore.h"
#include <CarCommand.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
	#define WIFI_SSID "PleaseEnterSSID" // Put you SSID and Password here
	#define WIFI_PWD "PleaseEnterPass"
#endif

int currentRbootSlot =0;

int blinkPart = 0;
enum class BlinkState {
	NOT_CONNECTED = 0,
	CONNECTED_AP_ONLY = 1,
	CONNECTED_BOTH = 2,
	AT_LEASET_ONE_CONNECTED =3
};
#define LED_PIN 2 // GPIO2

///** this is the definitions of the states that our program uses */
//State notConnected = State(noopUpdate);  //no operation
//State apOnly = State(fadeEnter, fadeUpdate, NULL);  //this state fades the LEDs in
//State connectedBoth = State(flashUpdate);  //this state flashes the leds FLASH_ITERATIONS times at 1000/FLASH_INTERVAL
//State atLeastOne = State(circleMotionUpdate); //show the circular animation
//
///** the state machine controls which of the states get attention and execution time */
//FSM stateMachine = FSM(noop); //initialize state machine, start in state: noop

BlinkState led0State = BlinkState::NOT_CONNECTED;
Timer blinkStateTimer;

//#ifndef OTA_SERVER
//	#define OTA_SERVER "xxx2" // Put you OTA server URL Here
//#endif

#define VERSION_CONFIG_FILE ".version" // leading point for security reasons :)

//http://www.smartarduino.com/2wd-wifi-rc-smart-car-with-nodemcu-shield-for-esp-12e_p94572.html
//https://smartarduino.gitbooks.io/user-manual-for-wifi-car-by-nodemcu-doitcar-/content/31_code_for_ap_case_on_doitcar.html
//--GPIO Define
//2     function initGPIO()
//3     --1,2EN     D1 GPIO5
//4     --3,4EN     D2 GPIO4
//5     --1A  ~2A   D3 GPIO0
//6     --3A  ~4A   D4 GPIO2

HttpServer server;
//FTPServer ftp;
TelnetServer telnet;
Timer msgTimer;

//PINS
#define leftMotorPwmPin 4
#define rightMotorPwmPin 5
#define leftMotorDirPin 2
#define rightMotorDirPin 0

CarCommand carCommand(leftMotorPwmPin, rightMotorPwmPin,  leftMotorDirPin, rightMotorDirPin);
//void changeBlinkState(BlinkState state);

String projName = "";
String thisBuildVersion = "";
String romFileName = "";
String spiffFileName = "";

rBootHttpUpdate* otaUpdater = 0;
Timer keepAliveTimer;

void OtaUpdate_CallBack(bool result) {
	
	Serial.println("In callback...");
	if(result == true) {
		// success
		uint8 slot;
		slot = rboot_get_current_rom();
		if (slot == 0) slot = 1; else slot = 0;
		// set to boot new rom and then reboot
		Serial.printf("Firmware updated, rebooting to rom %d...\r\n", slot);
		rboot_set_current_rom(slot);
		System.restart();
	} else {
		// fail
		Serial.println("Firmware update failed!");
	}
}

//void OtaUpdate() {
//
//	uint8 slot;
//	rboot_config bootconf;
//
//	// need a clean object, otherwise if run before and failed will not run again
//	if (otaUpdater) delete otaUpdater;
//	otaUpdater = new rBootHttpUpdate();
//
//	// select rom slot to flash
//	bootconf = rboot_get_config();
//	slot = bootconf.current_rom;
//	if (slot == 0) slot = 1; else slot = 0;
//
//	String romUrl = String(OTA_SERVER) + "/" + romFileName;
//	String spiffyUrl = String(OTA_SERVER) + "/" + spiffFileName;
//
//	debugf("Current rom slot is %d, OTA update romurl=%s, spiffUrl=%s", slot, romUrl.c_str(), spiffyUrl.c_str());
//
//#ifndef RBOOT_TWO_ROMS
//	// flash rom to position indicated in the rBoot config rom table
//	otaUpdater->addItem(bootconf.roms[slot], romUrl);
//#else
//	// flash appropriate rom
//	if (slot == 0) {
//		otaUpdater->addItem(bootconf.roms[slot], romUrl);
//	} else {
//		otaUpdater->addItem(bootconf.roms[slot], ROM_1_URL);
//	}
//#endif
//
//#ifndef DISABLE_SPIFFS
//	// use user supplied values (defaults for 4mb flash in makefile)
//	if (slot == 0) {
//		otaUpdater->addItem(RBOOT_SPIFFS_0, spiffyUrl);
//	} else {
//		otaUpdater->addItem(RBOOT_SPIFFS_1, spiffyUrl);
//	}
//#endif
//
//	// request switch and reboot on success
//	//otaUpdater->switchToRom(slot);
//	// and/or set a callback (called on failure or success without switching requested)
//	otaUpdater->setCallback(OtaUpdate_CallBack);
//
//	// start update
//	otaUpdater->start();
//}

void Switch() {
	uint8 before, after;
	before = rboot_get_current_rom();
	if (before == 0) after = 1; else after = 0;
	Serial.printf("Swapping from rom %d to rom %d.\r\n", before, after);
	rboot_set_current_rom(after);
	Serial.println("Restarting...\r\n");
	System.restart();
}

void ShowInfo() {
    Serial.printf("\r\nSDK: v%s\r\n", system_get_sdk_version());
    Serial.printf("Free Heap: %d\r\n", system_get_free_heap_size());
    Serial.printf("CPU Frequency: %d MHz\r\n", system_get_cpu_freq());
    Serial.printf("System Chip ID: %x\r\n", system_get_chip_id());
    Serial.printf("SPI Flash ID: %x\r\n", spi_flash_get_id());
    Serial.printf("Current Rom is: %d\r\n", rboot_get_current_rom());
//    Serial.printf("OTA Server URL: %s", OTA_SERVER);
    //Serial.printf("SPI Flash Size: %d\r\n", (1 << ((spi_flash_get_id() >> 16) & 0xff)));
}

/*
 * return 1 if v1 > v2
 * return 0 if v1 = v2
 * return -1 if v1 < v2
 */

int cmpVersion(String& v1, String& v2)
{
	Vector<int> v1Data;
	splitString(v1, '.', v1Data);
	Vector<int> v2Data;
	splitString(v2, '.', v2Data);
    int i;
//    int oct_v1[4], oct_v2[4];
//    sscanf(v1, "%d.%d.%d.%d", &oct_v1[0], &oct_v1[1], &oct_v1[2], &oct_v1[3]);
//    sscanf(v2, "%d.%d.%d.%d", &oct_v2[0], &oct_v2[1], &oct_v2[2], &oct_v2[3]);

    for (i = 0; i < 4; i++) {
        if (v1Data.elementAt(i) > v2Data.elementAt(i))
            return 1;
        else if (v1Data.elementAt(i) < v2Data.elementAt(i))
            return -1;
    }

    return 0;
}

void onDataSent(HttpClient& client, bool successful)
{
	if (!successful)
	{
		Serial.println("OTA Check Failed - could not get data from server");
	}

	if (successful)
		Serial.println("OTA Check Success sent");
	else
		Serial.println("OTA Check Failed");

	String response = client.getResponseString();
//	Serial.println("Server response: '" + response + "'");
	if (response.length() > 0)
	{
		DynamicJsonBuffer jsonBuffer;
		char* jsonString = new char[response.length() + 1];
		response.toCharArray(jsonString, response.length() + 1, 0);
		JsonObject& root = jsonBuffer.parseObject(jsonString);
		JsonArray& roms = root["roms"];
//		Vector<String> romsList;
		for (int i = 0; i < roms.size(); i++) {
			String tmpProjName = roms[i]["name"].asString();
			if (tmpProjName.equals(projName)) {

				romFileName = roms[i]["file"].asString();
				spiffFileName =  roms[i]["spiff"].asString();
				String tmpVersion = roms[i]["version"].asString();

				Serial.printf("Rom for proj: %s\r\nVersion: %s\r\nrom: %s\r\nspiff: %s\r\n", projName.c_str(), tmpVersion.c_str(), romFileName.c_str(), spiffFileName.c_str());
				debugf("Server version is %s", tmpVersion.c_str());
				if (cmpVersion(thisBuildVersion, tmpVersion) == -1) {
					debugf("Need update because current build is %s, and server is %s", thisBuildVersion.c_str(), tmpVersion.c_str());
				}
				break;
			}
		}
	}
}

HttpClient testOTA;

bool checkNeedsOTAUpdate() {
	if(!fileExist(VERSION_CONFIG_FILE)){
		debugf("There is no version file in this ESP, exiting");
		return false;
	}

	DynamicJsonBuffer jsonBuffer;
	int size = fileGetSize(VERSION_CONFIG_FILE);
	char* jsonString = new char[size + 1];
	fileGetContent(VERSION_CONFIG_FILE, jsonString, size + 1);
	JsonObject& root = jsonBuffer.parseObject(jsonString);
	JsonObject& rom = root["rom"];
	projName = String((const char*)rom["name"]);
	thisBuildVersion = String((const char*)rom["version"]);
	debugf("Current proj is %s, current version is %s", projName.c_str(), thisBuildVersion.c_str());
//	String ota = String(OTA_SERVER) + "/versions.txt";
//	testOTA.downloadString( ota, onDataSent);
}

void serialCallBack(Stream& stream, char arrivedChar, unsigned short availableCharsCount) {
	if (arrivedChar == '\n') {
		char str[availableCharsCount];
		for (int i = 0; i < availableCharsCount; i++) {
			str[i] = stream.read();
			if (str[i] == '\r' || str[i] == '\n') {
				str[i] = '\0';
			}
		}
		
		if (!strcmp(str, "connect")) {
			// connect to wifi
			WifiStation.config(WIFI_SSID, WIFI_PWD);
			WifiStation.enable(true);
		} else if (!strcmp(str, "ip")) {
			Serial.printf("ip: %s mac: %s\r\n", WifiStation.getIP().toString().c_str(), WifiStation.getMAC().c_str());
		} else if (!strcmp(str, "ota")) {
//			OtaUpdate();
		} else if (!strcmp(str, "switch")) {
			Switch();
		} else if (!strcmp(str, "restart")) {
			System.restart();
		} else if (!strcmp(str, "ls")) {
			Vector<String> files = fileList();
			Serial.printf("filecount %d\r\n", files.count());
			for (unsigned int i = 0; i < files.count(); i++) {
				Serial.println(files[i]);
			}
		} else if (!strcmp(str, "cat")) {
			Vector<String> files = fileList();
			if (files.count() > 0) {
				Serial.printf("dumping file %s:\r\n", files[0].c_str());
				Serial.println(fileGetContent(files[0]));
			} else {
				Serial.println("Empty spiffs!");
			}
		} else if (!strcmp(str, "info")) {
			ShowInfo();
		} else if (!strcmp(str, "help")) {
			Serial.println();
			Serial.println("available commands:");
			Serial.println("  help - display this message");
			Serial.println("  ip - show current ip address");
			Serial.println("  connect - connect to wifi");
			Serial.println("  restart - restart the esp8266");
			Serial.println("  switch - switch to the other rom and reboot");
			Serial.println("  ota - perform ota update, switch rom and reboot");
			Serial.println("  info - show esp8266 info");
#ifndef DISABLE_SPIFFS
			Serial.println("  ls - list files in spiffs");
			Serial.println("  cat - show first file in spiffs");
#endif
			Serial.println();
		}
		//test ota needed
		else if (!strcmp(str, "t")) {
			checkNeedsOTAUpdate();
		}
		else {
			Serial.println("unknown command");
		}
	}
}

void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
	auto &vars = tmpl->variables();
	//vars["counter"] = String(counter);
	response.sendTemplate(tmpl); // this template object will be deleted automatically
}

void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

int msgCount = 0;

void wsConnected(WebSocket& socket)
{
	Serial.printf("Socket connected\r\n");
//	changeBlinkState(BlinkState::AT_LEASET_ONE_CONNECTED);
}

void wsMessageReceived(WebSocket& socket, const String& message)
{
	Serial.printf("WebSocket message received:\r\n%s\r\n", message.c_str());
	String response = "Echo: " + message;
//	socket.sendString(response);
}

void wsBinaryReceived(WebSocket& socket, uint8_t* data, size_t size)
{
	Serial.printf("Websocket binary data recieved, size: %d\r\n", size);
}

void wsDisconnected(WebSocket& socket)
{
	Serial.printf("Socket disconnected");
//	if (WifiStation.isConnected()) {
//		changeBlinkState(BlinkState::CONNECTED_BOTH);
//	} else {
//		changeBlinkState(BlinkState::CONNECTED_AP_ONLY);
//	}
}

void processApplicationCommands(String commandLine, CommandOutput* commandOutput)
{
	commandOutput->printf("This command is handle by the application\r\n");
}

void handleCarCallbacks(String cmd) {
	debugf("cmd from car is: %s", cmd.c_str());
}

void initCarCommands()
{
	carCommand.initCommand();
	commandHandler.registerCommand(CommandDelegate("example","Example Command from Class","Application",processApplicationCommands));
	carCommand.setOnPublishDelegate(handleCarCallbacks);
}

void StartServers()
{
	debugf("Starting Server");
	server.listen(80);
	server.addPath("/", onIndex);
	server.setDefaultHandler(onFile);

	// Web Sockets configuration
	server.enableWebSockets(true);
	server.commandProcessing(true,"command");

	server.setWebSocketConnectionHandler(wsConnected);
	server.setWebSocketMessageHandler(wsMessageReceived);
	server.setWebSocketBinaryHandler(wsBinaryReceived);
	server.setWebSocketDisconnectionHandler(wsDisconnected);

	Serial.println("\r\n=== WEB SERVER STARTED ===");
	Serial.println(WifiStation.getIP());
	Serial.println(WifiAccessPoint.getIP());

	Serial.println("==============================\r\n");

//	// Start FTP server
//	ftp.listen(21);
//	ftp.addUser("me", "123"); // FTP account
//
//	Serial.println("\r\n=== FTP SERVER STARTED ===");
//	Serial.println("==============================\r\n");

//	telnet.listen(23);
//	telnet.enableDebug(true);

	Serial.println("\r\n=== TelnetServer SERVER STARTED ===");
	Serial.println("==============================\r\n");

	initCarCommands();
}

//void changeBlinkState(BlinkState state) {
////	debugf("New state found %s", state.)
//	blinkStateTimer.stop();
//	led0State = state;
//	blinkPart =0;
//	blinkStateTimer.startOnce();
//}
//
//void blinkStateLedUpdate() {
////	if (blinkPart == -1) {
////		return;
////	}
//	int shortPause = 250;
//	int longPause = 450;
//
//	if (led0State == BlinkState::NOT_CONNECTED) {
//		//blink every 50ms
//		switch (blinkPart) {
//			case 0:
//				digitalWrite(LED_PIN, HIGH);
//				blinkPart++;
//				blinkStateTimer.initializeMs(400, blinkStateLedUpdate).startOnce();
//				break;
//			case 1:
//				digitalWrite(LED_PIN, LOW);
//				blinkPart=0;
//				blinkStateTimer.initializeMs(400, blinkStateLedUpdate).startOnce();
//				break;
//		}
//	}
//	else if (led0State == BlinkState::CONNECTED_AP_ONLY) {
//		//blink 1 short 1 long
//		switch (blinkPart) {
//			case 0:
//				digitalWrite(LED_PIN, HIGH);
//				blinkPart++;
//				blinkStateTimer.initializeMs(shortPause, blinkStateLedUpdate).startOnce();
//				break;
//			case 1:
//				digitalWrite(LED_PIN, LOW);
//				blinkPart++;
//				blinkStateTimer.initializeMs(shortPause, blinkStateLedUpdate).startOnce();
//				break;
//			case 2:
//				digitalWrite(LED_PIN, HIGH);
//				blinkPart++;
//				blinkStateTimer.initializeMs(longPause, blinkStateLedUpdate).startOnce();
//				break;
//			case 3:
//				digitalWrite(LED_PIN, LOW);
//				blinkPart=0;
//				blinkStateTimer.initializeMs(longPause, blinkStateLedUpdate).startOnce();
//				break;
//		}
//	}
//	else if (led0State == BlinkState::CONNECTED_BOTH) {
//	//blink 1 long 1 short
//	switch (blinkPart) {
//			case 0:
//				digitalWrite(LED_PIN, HIGH);
//				blinkPart++;
//				blinkStateTimer.initializeMs(longPause, blinkStateLedUpdate).startOnce();
//				break;
//			case 1:
//				digitalWrite(LED_PIN, LOW);
//				blinkPart++;
//				blinkStateTimer.initializeMs(longPause, blinkStateLedUpdate).startOnce();
//				break;
//			case 2:
//				digitalWrite(LED_PIN, HIGH);
//				blinkPart++;
//				blinkStateTimer.initializeMs(shortPause, blinkStateLedUpdate).startOnce();
//				break;
//			case 3:
//				digitalWrite(LED_PIN, LOW);
//				blinkPart=0;
//				blinkStateTimer.initializeMs(shortPause, blinkStateLedUpdate).startOnce();
//				break;
//		}
//	}
//	else if (led0State == BlinkState::AT_LEASET_ONE_CONNECTED) {
//			//constant
//		digitalWrite(LED_PIN, LOW);
//		}
//}


void connectOk() {
	debugf("Connected to STA ip=%s", WifiStation.getIP().toString().c_str());
//	checkNeedsOTAUpdate();
//	changeBlinkState(BlinkState::CONNECTED_BOTH);
}

void connectFail() {
	Serial.println("Not Connectd to Station !!!, turning station mode network OFF");
	WifiStation.enable(false);
}

Timer onetime;

void updateWebSockets(String cmd) {
	WebSocketsList &clients = server.getActiveWebSockets();
	for (int i = 0; i < clients.count(); i++) {
		clients[i].sendString(cmd);
	}
}

void updateTime() {
	updateWebSockets("heart");
}

void setupSpiffs() {
	// mount spiffs
	currentRbootSlot = rboot_get_current_rom();
	#ifndef DISABLE_SPIFFS
		if (currentRbootSlot == 0) {
	#ifdef RBOOT_SPIFFS_0
			debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
			spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000 - 0x40200000, SPIFF_SIZE);
	#else
			debugf("trying to mount spiffs at %x, length %d", 0x40300000, SPIFF_SIZE);
			spiffs_mount_manual(0x40300000 - 0x40200000, SPIFF_SIZE);
	#endif
		} else {
	#ifdef RBOOT_SPIFFS_1
			debugf("trying to mount spiffs at %x, length %d", RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
			spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000 - 0x40200000, SPIFF_SIZE);
	#else
			debugf("trying to mount spiffs at %x, length %d", 0x40500000, SPIFF_SIZE);
			spiffs_mount_manual(0x40500000 - 0x40200000, SPIFF_SIZE);
	#endif
		}
	#else
		debugf("spiffs disabled");
	#endif
}

void init() {
	
	Serial.begin(115200); // 115200 by default
	Serial.systemDebugOutput(true); // Debug output to serial
	setupSpiffs();
	
	pinMode(LED_PIN, OUTPUT);
//	WifiStation.enable(false);
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.waitConnection(connectOk, 20, connectFail);

	WifiAccessPoint.enable(true);
	WifiAccessPoint.config("SmartCar", "", AUTH_OPEN, false, 11, 200);

	Serial.printf("\r\nCurrently running rom %d.\r\n", currentRbootSlot);
	Serial.println("Type 'help' and press enter for instructions.");
	Serial.println();
	
	commandHandler.registerSystemCommands();
	Debug.setDebug(Serial);
	Serial.systemDebugOutput(true); // Enable debug output to serial
//	Serial.commandProcessing(true);
	Serial.setCallback(serialCallBack);

//	onetime.initializeMs(10000, StartServers).startOnce();
	//Change CPU freq. to 160MHZ
	System.setCpuFrequency(eCF_160MHz);


//	blinkStateTimer.setCallback(blinkStateLedUpdate);
//	blinkStateTimer.setIntervalMs(120);
//	blinkStateTimer.start(true);

//	// Run WEB server on system ready
//	System.onReady(StartServers);
	StartServers();
}
