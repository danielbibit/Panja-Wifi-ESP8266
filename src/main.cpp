#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"

WiFiServer server(9999);

const String BOARD_NAME = "wifi";
const int BOARD_VERSION = 0;
const String BOARD_HOSTNAME = "ESP-1";

const char* HOST = "panja-server";
const uint16_t PORT = 9997;

const bool DEBUG = true;

unsigned long last_connected;
const int RESTART_TIME = 4 * 60 * 1000;

String message[10];

// Function prototypes
void send_to_server(String message);
String json_builder(String action, String argument);
bool parse_command(String s);
void serial_flush();
void debug(const char* c);

void setup(){
	ESP.wdtEnable(10000);
	Serial.begin(19200);

	WiFiManager wifiManager;
	wifiManager.setDebugOutput(DEBUG);

	// WiFi.setPhyMode(WIFI_PHY_MODE_11G); // Test this to try solve b/g/n problem
	WiFi.hostname(BOARD_HOSTNAME);

	if(!wifiManager.autoConnect()){
		debug("failed to connect and hit timeout");
		ESP.reset();
	}

	server.begin();

	send_to_server(json_builder("alive", "exordial_board"));
	serial_flush();
	Serial.println("0;server;status;0");
}

void loop(){
	if(WiFi.status() == WL_CONNECTED){
		last_connected = millis();
	}else if(millis() - last_connected > RESTART_TIME){
		ESP.reset();
	}

	server.setNoDelay(true);
	WiFiClient client = server.available();

	if (client){
		client.setNoDelay(true);
		debug("CLIENT CONNECTED");
		while (client.connected()){
			if (client.available()){
				debug("CLIENT AVAILABLE");
				String line = client.readStringUntil('\n');
				Serial.println(line);

				client.print(json_builder("response", "ok"));
				client.stop();
				debug("CLIENT DESCONNECTED");
				break;
			}
		}
	}

	if(Serial.available()){
		String string = Serial.readStringUntil('\n');
		send_to_server(string);
	}

	delay(1);
}

void send_to_server(String message){
	WiFiClient client_sender;

	int time_out = 3000;

	client_sender.setTimeout(time_out);
	while(!client_sender.connect(HOST, PORT)){
		delay(1);
	}

	client_sender.print(message);

	String response = client_sender.readStringUntil('\n');
	Serial.println(response);

	client_sender.stop();
}

bool parse_command(String s){
	int n = 0;

	for(int i = 0; i < 10; i++){
		message[i] = "";
	}

	for(int i = 0; i < (int)s.length(); i++){
		if(s.charAt(i) == ';'){
			n += 1;
		}else{
			message[n].concat(s.charAt(i));
		}
	}

	return n >= 3 ? true : false;
}

String json_builder(String action, String argument){
	String string = "{\"version\":";
	string.concat(BOARD_VERSION);
	string.concat(", \"name\" : \"");
	string.concat(BOARD_NAME);
	string.concat("\", \"action\" : \"");
	string.concat(action);
	string.concat("\", \"argument\" : \"");
	string.concat(argument);
	string.concat("\"}");

	return string;
}

void serial_flush(){
	while(Serial.available() > 0){
		char t = Serial.read();
	}
}

void debug(const char* c){
	if(DEBUG){
		Serial.print("D: ");
		Serial.println(c);
	}
}
