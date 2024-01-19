
/**
   AWS S3 OTA Update
   Date: 14th June 2017
   Author: Arvind Ravulavaru <https://github.com/arvindr21>
   Purpose: Perform an OTA update from a bin located in Amazon S3 (HTTP Only)

   Upload:
   Step 1 : Download the sample bin file from the examples folder
   Step 2 : Upload it to your Amazon S3 account, in a bucket of your choice
   Step 3 : Once uploaded, inside S3, select the bin file >> More (button on top of the file list) >> Make Public
   Step 4 : You S3 URL => http://bucket-name.s3.ap-south-1.amazonaws.com/sketch-name.ino.bin
   Step 5 : Build the above URL and fire it either in your browser or curl it `curl -I -v http://bucket-name.ap-south-1.amazonaws.com/sketch-name.ino.bin` to validate the same
   Step 6:  Plug in your SSID, Password, S3 Host and Bin file below

   Build & upload
   Step 1 : Menu > Sketch > Export Compiled Library. The bin file will be saved in the sketch folder (Menu > Sketch > Show Sketch folder)
   Step 2 : Upload bin to S3 and continue the above process

   // Check the bottom of this sketch for sample serial monitor log, during and after successful OTA Update
*/

#include <WiFi.h>
#include <Update.h>
#include <HTTPClient.h>


WiFiClientSecure client;

// Variables to validate
// response from S3
int contentLength = 0;
bool isValidContentType = false;
String version = "0.1.1";

// Your SSID and PSWD that the chip needs
// to connect to
const char* SSID = "Viettel Post";
const char* PSWD = "2010#ctbc";

// Git Repository Config
String  host = "raw.githubusercontent.com";
int port = 443; // HTTPS port
String bin = "/tungbach1990/esp32cam-cpp/main/build/esp32.esp32.esp32wrover/newOTA.ino.bin"; // path to the binary file
String token = "";//"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";
const char* root_ca = \
    "-----BEGIN CERTIFICATE-----\n"
    "MIIHEjCCBfqgAwIBAgIQBE1y13zdpwLdWmfyoju92TANBgkqhkiG9w0BAQsFADBP\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMSkwJwYDVQQDEyBE\n"
    "aWdpQ2VydCBUTFMgUlNBIFNIQTI1NiAyMDIwIENBMTAeFw0yMzAyMjEwMDAwMDBa\n"
    "Fw0yNDAzMjAyMzU5NTlaMGcxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9y\n"
    "bmlhMRYwFAYDVQQHEw1TYW4gRnJhbmNpc2NvMRUwEwYDVQQKEwxHaXRIdWIsIElu\n"
    "Yy4xFDASBgNVBAMMCyouZ2l0aHViLmlvMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A\n"
    "MIIBCgKCAQEAuLBgDhov8bGGS2TsEZ+meb7oh/GIxbRJmxC7yq/qr75UDHhDf8p7\n"
    "TkVbCyQp8bsj/Bmkx2xwSXZT0wkjZbJIe7Ycqgca4nka+Xpe5xb4pkrVOaPiDfdX\n"
    "7+34CHZbUtqL0OYebi/5D5lLalLKNOGkySAz05foenfFAxAmQYJhR6KvxFY/dqI4\n"
    "y7JwrnJ6Q8F+J6Ne1uP256UwcL0qlid6e/tA0ld3ryMSJ0I6xgtqjL26Le4/nxXu\n"
    "YlekppVQr0OwrHa44Q7Z/1bsdFCGtR+WLNGVBeW3BWeTTp7yWjgfp49DWt48V9pI\n"
    "elDGiDgVyJcsLOz4OQk2vRmNA1ZBZgck4wIDAQABo4ID0DCCA8wwHwYDVR0jBBgw\n"
    "FoAUt2ui6qiqhIx56rTaD5iyxZV2ufQwHQYDVR0OBBYEFI0CHHVazcamQXhpKMP3\n"
    "qqeYO9W7MHsGA1UdEQR0MHKCCyouZ2l0aHViLmlvgglnaXRodWIuaW+CDCouZ2l0\n"
    "aHViLmNvbYIKZ2l0aHViLmNvbYIOd3d3LmdpdGh1Yi5jb22CFyouZ2l0aHVidXNl\n"
    "cmNvbnRlbnQuY29tghVnaXRodWJ1c2VyY29udGVudC5jb20wDgYDVR0PAQH/BAQD\n"
    "AgWgMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjCBjwYDVR0fBIGHMIGE\n"
    "MECgPqA8hjpodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vRGlnaUNlcnRUTFNSU0FT\n"
    "SEEyNTYyMDIwQ0ExLTQuY3JsMECgPqA8hjpodHRwOi8vY3JsNC5kaWdpY2VydC5j\n"
    "b20vRGlnaUNlcnRUTFNSU0FTSEEyNTYyMDIwQ0ExLTQuY3JsMD4GA1UdIAQ3MDUw\n"
    "MwYGZ4EMAQICMCkwJwYIKwYBBQUHAgEWG2h0dHA6Ly93d3cuZGlnaWNlcnQuY29t\n"
    "L0NQUzB/BggrBgEFBQcBAQRzMHEwJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRp\n"
    "Z2ljZXJ0LmNvbTBJBggrBgEFBQcwAoY9aHR0cDovL2NhY2VydHMuZGlnaWNlcnQu\n"
    "Y29tL0RpZ2lDZXJ0VExTUlNBU0hBMjU2MjAyMENBMS0xLmNydDAJBgNVHRMEAjAA\n"
    "MIIBfgYKKwYBBAHWeQIEAgSCAW4EggFqAWgAdwB2/4g/Crb7lVHCYcz1h7o0tKTN\n"
    "uyncaEIKn+ZnTFo6dAAAAYZ0gHV7AAAEAwBIMEYCIQCqfmfSO8MxeeVZ/fJzqqBB\n"
    "p+VqeRDUOUBVGyTTOn43ewIhAJT0S27mmGUlpqNiDADP+Jo8C6kYHF+7U6TY74bH\n"
    "XHAaAHYAc9meiRtMlnigIH1HneayxhzQUV5xGSqMa4AQesF3crUAAAGGdIB1agAA\n"
    "BAMARzBFAiEAguB+XQVANBj2MPcJzbz+LBPrkDDOEO3op52jdHUSW3ICIF0fnYdW\n"
    "qvdtmgQNSns13pAppdQWp4/f/jerNYskI7krAHUASLDja9qmRzQP5WoC+p0w6xxS\n"
    "ActW3SyB2bu/qznYhHMAAAGGdIB1SgAABAMARjBEAiAT/wA2qGGHSKZqBAm84z6q\n"
    "E+dGPQZ1aCMY52pFSfcw8QIgP/SciuZG02X2mBO/miDT2hCp4y5d2sc7FE5PThyC\n"
    "pbMwDQYJKoZIhvcNAQELBQADggEBADekGxEin/yfyWcHj6qGE5/gCB1uDI1l+wN5\n"
    "UMZ2ujCQoKQceRMHuVoYjZdMBXGK0CIXxhmiIosD9iyEcWxV3+KZQ2Xl17e3N0zG\n"
    "yOXx2Kd7B13ruBxQpKOO8Ez4uGpyWb5DDoretV6Pnj9aQ2SCzODedvS+phIKBmi7\n"
    "d+FM70tNZ6/2csdrG5xIU6d/7XYYXPD2xkwkU1dX4UKmPa7h9ZPyavopcgE+twbx\n"
    "LxoOkcXsNb/12jOV3iQSDfXDI41AgtFc694KCOjlg+UKizpemE53T5/cq37OqChP\n"
    "qnlPyb6PYIhua/kgbH84ltba1xEDQ9i4UYfOMiNZEzEdSfQ498=\n"
    "-----END CERTIFICATE-----\n";


// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

// OTA Logic
void execOTA() {
  Serial.println("Connecting to: " + String(host));
  client.setCACert(root_ca);
  client.setInsecure();//skip verification
  // Connect to S3
  if (client.connect(host.c_str(), port)) {
    // Connection Succeed.
    // Fecthing the bin
    Serial.println("Fetching Bin: " + String(bin));

    // Get the contents of the bin file
    client.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    // Check what is being sent
    //    Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
    //                 "Host: " + host + "\r\n" +
    //                 "Cache-Control: no-cache\r\n" +
    //                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 50000) {
        Serial.println("Client Timeout !");
        client.stop();
        return;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3

        {{BIN FILE CONTENTS}}

    */
    while (client.available()) {
      // read line till /n
      String line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }
      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }
      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    // Connect to S3 failed
    // May be try?
    // Probably a choppy network?
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
    // retry??
    // execOTA();
  }
  // Check what is the contentLength and if content type is `application/octet-stream`
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));
  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);
    // If yes, begin
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(client);
      Serial.println("after Update.writeStream");
      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
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
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      Serial.println("Not enough space to begin OTA");
      client.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    client.flush();
  }
}
void setup() {
  //Begin Serial
  Serial.begin(115200);
  delay(10);
  Serial.println("Connecting to " + String(SSID));
  // Connect to provided SSID and PSWD
  WiFi.begin(SSID, PSWD);
  // Wait for connection to establish
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); // Keep the serial monitor lit!
    delay(500);
  }
  // Connection Succeed
  Serial.println("");
  Serial.println("Connected to " + String(SSID));
  Serial.println("");
  Serial.println("");
  Serial.println("Running version:" + String(version));
  Serial.println("");
  Serial.println("");
  // Execute OTA Update
  execOTA();
  

}
void loop() {
 sleep(100);
}
/*
   Serial Monitor log for this sketch

   If the OTA succeeded, it would load the preference sketch, with a small modification. i.e.
   Print `OTA Update succeeded!! This is an example sketch : Preferences > StartCounter`
   And then keeps on restarting every 10 seconds, updating the preferences


      rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
      configsip: 0, SPIWP:0x00
      clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
      mode:DIO, clock div:1
      load:0x3fff0008,len:8
      load:0x3fff0010,len:160
      load:0x40078000,len:10632
      load:0x40080000,len:252
      entry 0x40080034
      Connecting to SSID
      ......
      Connected to SSID
      Connecting to: bucket-name.s3.ap-south-1.amazonaws.com
      Fetching Bin: /StartCounter.ino.bin
      Got application/octet-stream payload.
      Got 357280 bytes from server
      contentLength : 357280, isValidContentType : 1
      Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!
      Written : 357280 successfully
      OTA done!
      Update successfully completed. Rebooting.
      ets Jun  8 2016 00:22:57

      rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
      configsip: 0, SPIWP:0x00
      clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
      mode:DIO, clock div:1
      load:0x3fff0008,len:8
      load:0x3fff0010,len:160
      load:0x40078000,len:10632
      load:0x40080000,len:252
      entry 0x40080034

      OTA Update succeeded!! This is an example sketch : Preferences > StartCounter
      Current counter value: 1
      Restarting in 10 seconds...
      E (102534) wifi: esp_wifi_stop 802 wifi is not init
      ets Jun  8 2016 00:22:57

      rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
      configsip: 0, SPIWP:0x00
      clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
      mode:DIO, clock div:1
      load:0x3fff0008,len:8
      load:0x3fff0010,len:160
      load:0x40078000,len:10632
      load:0x40080000,len:252
      entry 0x40080034

      OTA Update succeeded!! This is an example sketch : Preferences > StartCounter
      Current counter value: 2
      Restarting in 10 seconds...

      ....

*/
