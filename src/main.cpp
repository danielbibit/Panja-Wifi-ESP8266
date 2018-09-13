#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "WiFiManager.h"

ESP8266WebServer server(80);

const String BOARD_NAME = "WIFI";
const int BOARD_VERSION = 0;
const String BOARD_HOSTNAME = "esp-1";
const String KEY = "mykey";

String SERVER = "http://panja-server:5000";

const bool DEBUG = false;

unsigned long last_connected;
const int RESTART_TIME = 4 * 60 * 1000;

void debug(String string){
    if(DEBUG){
        Serial.println(string);
    }
}

// authenticate request
int process_request(){
    if(server.method() != HTTP_GET){
        server.send(405, "text/plain", "Invalid method");
        return 0;
    }else if(server.arg("key") != KEY){
        server.send(401, "text/plain", "Auth error");
        return 0;
    }else if(server.arg("key") == ""){
        server.send(400, "text/plain", "Bad request");
        return 0; 
    }else{
        return 1;
    }
}

void handleDirection(String path){

}
// ?
void handleNotFound(){
    String path = server.uri();
    int index = path.indexOf("/robot");
    if (index >= 0) {
        handleDirection(path);
    }else{
        server.send(404, "text/plain", "Service not found !");
    }
}

// /
void handleRoot() {
    server.send(200, "text/plain", "Hi! I'm a panja wifi module !");
}

// /config //must have authentication
void handleConfig(){
    if(process_request()){
        if(server.arg("server") != ""){
            SERVER = server.arg("server");
            server.send(
                200,
                "text/plain", 
                "Until reboot, I'll send data to " + server.arg("panja-server")
            );
        }else{
            server.send(400, "text/plain", "I dont understand");
        }
    }
}

// /sync //must have authentication
void handleSync(){
    if(process_request()){
        server.send(200, "text/plain", "ok, sync");
        Serial.println("0;server;status;0");
    }
}

// /control //must have authentication
void handleControl(){
    if(process_request()){
        if(server.arg("action") == "" || server.arg("args") == ""){
            server.send(400, "text/plain", "Bad request bro");
        }else{
            String command = "0;server;";
            command += server.arg("action");
            command += ";";
            command += server.arg("args");
            Serial.println(command);
            server.send(200, "text/plain", "ok");
        }
    }
}

void send_to_server(String message, String route){
    HTTPClient http;

    http.begin(SERVER + route);
    http.addHeader("Content-Type", "application/json"); //Server only accepts json
    int httpCode = http.POST(message); //Server only accepts POST

    if(httpCode > 0){
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            debug(payload);
        }
    }else{
        debug("Something went wrong sending the message");
    }

    http.end();
}

void setup(){
    // Serial.begin(115200);
    Serial.begin(19200);
    
    WiFiManager wifiManager;
	wifiManager.setDebugOutput(DEBUG);

	WiFi.hostname(BOARD_HOSTNAME);

	if(!wifiManager.autoConnect()){
		ESP.reset();
	}

    server.onNotFound(handleNotFound);

    server.on("/", handleRoot);

    server.on("/sync", handleSync);

    server.on("/config", handleConfig);

    server.on("/control", handleControl);
    
    server.begin();
    
    debug("HTTP server started");
}

void loop(){
    server.handleClient();

    if(Serial.available()){
		String string = Serial.readStringUntil('\n');
        send_to_server(string, "/modules");
	}

    delay(1);
}