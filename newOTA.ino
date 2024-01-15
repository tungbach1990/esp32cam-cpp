#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>
#include <Crypto.h>
#include <SHA256.h>
#include <LittleFS.h>
#include <ArduinoJson.h>


// Global variables
String internal_tree[100]; // Adjust the array size as needed

// User Variables
const char* ssid = "Viettel Post";
const char* password = "2010#ctbc";
const char* user = "tungbach1990";
const char* repository = "esp32cam-cpp";
const char* token = "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";
const char* default_branch = "main";
const String ignore_files[] = {"/ugit.cpp"}; // Adjust as needed

// Static URLs
String giturl = "https://github.com/" + String(user) + "/" + String(repository);
String call_trees_url = "https://api.github.com/repos/" + String(user) + "/" + String(repository) + "/git/trees/" + String(default_branch) + "?recursive=1";
String raw = "https://raw.githubusercontent.com/" + String(user) + "/" + String(repository) + "/master/";

void pull(String f_path, String raw_url);

void wificonnect(const char* ssid = ssid, const char* password = password);

void pull_all(String tree_url = call_trees_url, String raw_url = raw, bool isconnected = false);

void build_internal_tree();

void add_to_tree(String& dir_item);

String get_hash(String file);

String get_data_hash(String data);

bool is_directory(String path);

String pull_git_tree(String tree_url);

void parse_git_tree(String treeData);

void remove_ignore(String internalTree[], const String ignoreList[] = ignore_files);
void check_ignore(String treeData, const String ignoreList[] = ignore_files);

bool isInIgnoreList(String path, String ignoreList[]);

void remove_item(String item, String internalTree[]);

void update();

void backup(String internalTree[]);


void remove_ignore(String internalTree[], const String ignoreList[]) {
  Serial.println("Removing ignored files from internal tree...");

  for (int i = 0; i < 100; i++) {
    for (int j = 0; j < sizeof(ignoreList) / sizeof(ignoreList[0]); j++) {
      if (internalTree[i] == ignoreList[j]) {
        Serial.println(internalTree[i] + " removed from internal tree");
        internalTree[i] = "";  // Mark the slot as empty
        break;  // Move to the next internalTree item
      }
    }
  }
}


void pull(String f_path, String raw_url) {
  Serial.println("pulling " + f_path + " from GitHub");
  HTTPClient http;
  http.begin(raw_url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    File new_file = LittleFS.open(f_path, "w");
    if (new_file) {
      new_file.print(http.getString());
      new_file.close();
    }
    http.end();
  } else {
    Serial.println("Failed to fetch " + f_path);
    http.end();
  }
}

void wificonnect(const char* ssid, const char* password) {
  Serial.println("Đang kết nối WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối WiFi...");
  }

  Serial.println("WiFi đã kết nối!");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.println("Địa chỉ IP, Mặt nạ mạng, Cổng mặc định, Đang lắng nghe trên...");
  Serial.println(WiFi.localIP());
}

void pull_all(String tree_url, String raw_url, bool isconnected) {
  if (!isconnected) {
    wificonnect();
  }

  // You need to manage the working directory and paths manually.

  // Call functions to pull Git tree, build internal tree, and remove ignored files
  pull_git_tree(tree_url);
  build_internal_tree();
  remove_ignore(internal_tree);

  Serial.println("ignore removed ----------------------");
  for (int i = 0; i < 100; i++) {
    Serial.println(internal_tree[i]);
  }

  // Download and save all files
  // Implement logic to create directories and handle file updates manually
  // For example, using LittleFS

  // Delete files not in GitHub tree
  if (sizeof(internal_tree) > 0) {
    Serial.println("leftover!");
    for (int i = 0; i < 100; i++) {
      // Implement logic to delete files manually
    }
  }

  // Log actions to a file
  File logfile = LittleFS.open("/ugit_log.txt", "w");
  if (logfile) {
    for (int i = 0; i < 100; i++) {
      logfile.println(internal_tree[i]);
    }
    logfile.close();
  }

  delay(10000); // Sleep for 10 seconds
  Serial.println("Resetting device: ESP.restart()");
  ESP.restart();
}

void build_internal_tree() {
  Serial.println("Building internal tree...");
  File root = LittleFS.open("/", "r");

  if (!root) {
    Serial.println("Failed to open root directory");
    return;
  }

  while (File file = root.openNextFile()) {
    add_to_tree(file.name());
    file.close();
  }

  root.close();
}

void add_to_tree(const String& dir_item) {
  // Implementation as per MicroPython code
  Serial.println(dir_item);

  if (dir_item != "/") {
    String subfile_path = dir_item;
    get_hash(subfile_path); // Assuming get_hash function is implemented elsewhere
  }

  // The rest of the function logic needs to be adapted based on your requirements
}
String get_hash(String file) {
  Serial.println(file);

  File f = LittleFS.open(file, "r");
  if (!f) {
    Serial.println("Failed to open file for hashing");
    return "";
  }

  const size_t bufferSize = 1024;
  uint8_t buffer[bufferSize];
  SHA256 sha256;

  while (f.available()) {
    size_t bytesRead = f.read(buffer, bufferSize);
    sha256.update(buffer, bytesRead);
  }

  f.close();

  const size_t hashSize = sha256.hashSize();
  uint8_t hash[hashSize];

  // Manually copy the hash from the internal buffer to our array
  sha256.finalize(hash, hashSize);

  String hashString = "";
  for (int i = 0; i < hashSize; i++) {
    char hex[3];
    sprintf(hex, "%02x", hash[i]);
    hashString += hex;
  }

  return hashString;
}

String get_data_hash(String data) {
  SHA256 sha256;

  // Convert the String to a char array
  char charArray[data.length() + 1];
  data.toCharArray(charArray, sizeof(charArray));

  // Update the hash with the data
  sha256.update(reinterpret_cast<uint8_t*>(charArray), data.length());

  const size_t hashSize = sha256.hashSize();
  uint8_t hash[hashSize];

  // Manually copy the hash from the internal buffer to our array
  sha256.finalize(hash, hashSize);

  // Convert the hash to a hexadecimal string
  String hashString = "";
  for (int i = 0; i < hashSize; i++) {
    char hex[3];
    sprintf(hex, "%02x", hash[i]);
    hashString += hex;
  }

  return hashString;
}

bool is_directory(String path) {
  File file = LittleFS.open(path, "r");

  if (!file) {
    // Unable to open the file, assuming it doesn't exist or is a directory
    return true;
  }

  // Check if it's a directory
  bool isDir = file.isDirectory();
  file.close();

  return isDir;
}

String pull_git_tree(String tree_url) {
  Serial.println("Fetching Git tree...");

  HTTPClient http;
  http.begin(tree_url);

  int httpCode = http.GET();
  String response = "";

  if (httpCode == HTTP_CODE_OK) {
    response = http.getString();
  } else {
    Serial.printf("HTTP request failed with error code %d\n", httpCode);
  }

  http.end();
  return response;
}

void parse_git_tree(String treeData) {
  Serial.println("Parsing Git tree...");

  const size_t bufferSize = 2 * JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 220;
  DynamicJsonDocument jsonDoc(bufferSize);

  DeserializationError error = deserializeJson(jsonDoc, treeData);

  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  // Access parsed values
  JsonArray treeArray = jsonDoc["tree"];

  Serial.println("Dirs:");
  for (JsonObject treeObj : treeArray) {
    String type = treeObj["type"];
    String path = treeObj["path"];

    if (type == "tree") {
      Serial.println(path);
    }
  }

  Serial.println("Files:");
  for (JsonObject treeObj : treeArray) {
    String type = treeObj["type"];
    String path = treeObj["path"];

    if (type == "blob") {
      Serial.println(path);
    }
  }
}

void check_ignore(String treeData, const String ignoreList[]) {
  Serial.println("Checking ignore list...");

  const size_t bufferSize = 2 * JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 220;
  DynamicJsonDocument jsonDoc(bufferSize);

  DeserializationError error = deserializeJson(jsonDoc, treeData);

  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  // Access parsed values
  JsonArray treeArray = jsonDoc["tree"];

  for (JsonObject treeObj : treeArray) {
    String path = treeObj["path"];

    if (isInIgnoreList(path, ignoreList)) {
      Serial.println(path + " is in ignore list");
    } else {
      Serial.println(path + " is not in ignore list");
    }
  }
}

bool isInIgnoreList(String path, const String ignoreList[]) {
  for (int i = 0; i < sizeof(ignoreList) / sizeof(ignoreList[0]); i++) {
    if (path == ignoreList[i]) {
      return true;
    }
  }
  return false;
}

void remove_item(String item, String internalTree[]) {
  Serial.println("Removing item from internal tree...");

  for (int i = 0; i < 100; i++) {
    if (internalTree[i] == item) {
      // The item is found, remove it
      Serial.println(item + " removed from internal tree");
      // You may want to perform additional actions here, such as deleting the file
      internalTree[i] = ""; // Mark the slot as empty
      return; // Assuming there is only one occurrence of the item in the internal tree
    }
  }

  Serial.println(item + " not found in internal tree");
}

void update() {
  Serial.println("Updating ugit.py to the newest version...");

  String rawUrl = "https://raw.githubusercontent.com/turfptax/ugit/master/ugit.py";
  HTTPClient http;

  if (http.begin(rawUrl)) {
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Updating ugit.py...");

      File ugitFile = LittleFS.open("/ugit.py", "w");
      if (ugitFile) {
        ugitFile.print(http.getString());
        ugitFile.close();
        Serial.println("ugit.py updated successfully");
      } else {
        Serial.println("Failed to open ugit.py for writing");
      }
    } else {
      Serial.printf("Failed to fetch ugit.py with error code %d\n", httpCode);
    }

    http.end();
  } else {
    Serial.println("Failed to begin HTTP request for ugit.py");
  }
}

void backup(String internalTree[]) {
  Serial.println("Creating backup...");

  String backupText = "ugit Backup Version 1.0\n\n";

  for (int i = 0; i < 100; i++) {
    if (internalTree[i] != "") {
      File dataFile = LittleFS.open(internalTree[i], "r");

      if (dataFile) {
        backupText += "FN:SHA1" + internalTree[i] + "," + get_hash(internalTree[i]) + "\n";
        backupText += "---" + dataFile.readString() + "---\n";
        dataFile.close();
      } else {
        Serial.println("Failed to open file for backup: " + internalTree[i]);
      }
    }
  }

  File backupFile = LittleFS.open("/ugit.backup", "w");
  if (backupFile) {
    backupFile.print(backupText);
    backupFile.close();
    Serial.println("Backup created successfully");
  } else {
    Serial.println("Failed to open ugit.backup for writing");
  }
}


void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  wificonnect();
}

void loop() {
  // Your main loop code goes here
  backup(internal_tree);
  pull_all();
  // Additional code can be added here as needed
  // This loop will continuously perform the backup and pull operations
  delay(60000); // Delay for 1 minute before the next iteration
}