#include <WiFi.h>
#include <Update.h>
#include <HTTPClient.h>

WiFiClient client;

// Variables to validate
// response from Git
int contentLength = 0;
bool isValidContentType = false;

// Your SSID and PSWD that the chip needs
// to connect to
const char* SSID = "Viettel Post";
const char* PSWD = "2010#ctbc";

// Git Repository Config
String host = "raw.githubusercontent.com";
int port = 443; // HTTPS port
String bin = "/tungbach1990/esp32cam-cpp/main/build/esp32.esp32.esp32cam/newOTA.ino.bin"; // path to the binary file
String token = "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";


// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

// OTA Logic
void execOTA() {
  Serial.println("Connecting to: " + String(host));
  
  // Connect to Git
  if (client.connect(host.c_str(), port)) {
    Serial.println("Fetching Bin: " + String(bin));

    // Get the contents of the binary file
    String authorizationHeader = (token.length() > 0) ? "Bearer " + String(token) : "";
    client.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n" +
                 (authorizationHeader.length() > 0 ? "Authorization: " + authorizationHeader + "\r\n" : "") +
                 "\r\n");
    Serial.println(String("GET ") + bin + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n" +
                 (authorizationHeader.length() > 0 ? "Authorization: " + authorizationHeader + "\r\n" : "") +
                 "\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        client.stop();
        return;
      }
    }

    while (client.available()) {
      String line = client.readStringUntil('\n');
      line.trim();

      if (!line.length()) {
        break;
      }

      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non-200 status code from server. Exiting OTA Update.");
          break;
        }
      }

      if (line.startsWith("Content-Length: ")) {
        contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
  }

  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  if (contentLength && isValidContentType) {
    bool canBegin = Update.begin(contentLength);

    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Be patient!");
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      Serial.println("Not enough space to begin OTA");
      client.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    client.flush();
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("Connecting to " + String(SSID));

  WiFi.begin(SSID, PSWD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("Connected to " + String(SSID));
  execOTA();
}

void loop() {
  // Your loop code here
}
