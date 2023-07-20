#if ! (ESP8266 || ESP32 )
  #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting
#endif

char ssid[] = "Kama";             // your network SSID (name)
char pass[] = "12345678";         // your network password

char user[]         = "user";          // MySQL user login username
char password[]     = "1234";          // MySQL user login password

#define MYSQL_DEBUG_PORT      Serial

// Debug Level from 0 to 4
#define _MYSQL_LOGLEVEL_      1

#include <MySQL_Generic.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
//#include <BLEEddystoneURL.h>
//#include <BLEEddystoneTLM.h>
//#include <BLEBeacon.h>

#define USING_HOST_NAME     false

IPAddress SQL_server(192, 168, 4, 197);

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

int scanTime = 1; //In seconds
BLEScan *pBLEScan;

char * mybeacon = "ESP32";

uint16_t server_port = 3306;    //3306;

const int id = 1;

char default_database[] = "esp32";           //"test_arduino";
char default_table[]    = "item";          //"test_arduino";

MySQL_Connection conn((Client *)&client);
MySQL_Query *query_mem;
MySQL_Query sql_query = MySQL_Query(&conn);
int value = 0;
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    //비콘값을 출력한다
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      //여기다가 비콘이 수신한 정보를 출력하겠다!
      //녹칸다가 가진 비콘은 Holy-IOT이름을 가지고 있고
      //이것만 검색하겠다!
      
      if(strcmp(advertisedDevice.getName().c_str(),mybeacon) == 0){
        int ble_rssi = advertisedDevice.getRSSI();
        //자신이 쓰는 비콘의 tx파워를 직접 알아내야하는 상황
        int8_t ble_tx_power = -61; //녹칸다의 비콘의 TX파워
        float dist = pow(10,((ble_tx_power-ble_rssi)/20.0)); //미터(m)
        value = ble_rssi - 3;
      }
    }
};

void setup()
{
  Serial.begin(115200);
  
  //beacon setup
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
  
  //query setup
  MYSQL_DISPLAY1("\nStarting Basic_Insert_ESP on", ARDUINO_BOARD);
  MYSQL_DISPLAY(MYSQL_MARIADB_GENERIC_VERSION);
  // Begin WiFi section
  MYSQL_DISPLAY1("Connecting to", ssid);  
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    MYSQL_DISPLAY0(".");
  }
  // print out info about the connection:
  MYSQL_DISPLAY1("Connected to network. My IP address is:", WiFi.localIP());
  MYSQL_DISPLAY3("Connecting to SQL Server @", SQL_server, ", Port =", server_port);
  MYSQL_DISPLAY5("User =", user, ", PW =", password, ", DB =", default_database);
}


String query = "";
String INSERT_SQL = "";

void loop()
{ 
  //scanTime타임동안 검색한다
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  INSERT_SQL = String("INSERT INTO ") + "esp32.item(value_1) VALUES ('" + value +"')";
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  
  if (conn.connectNonBlocking(SQL_server, server_port, user, password) != RESULT_FAIL)
  {
    delay(500);
    runInsert();
    conn.close();                     // close the connection
  } 
  else 
  {
    MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
  }

  delay(1000);
}

void runInsert()
{
  // Initiate the query class instance
  MySQL_Query query_mem = MySQL_Query(&conn);

  if (conn.connected())
  {
    MYSQL_DISPLAY(INSERT_SQL);
    
    // Execute the query
    // KH, check if valid before fetching
    if ( !query_mem.execute(INSERT_SQL.c_str()) )
    {
      MYSQL_DISPLAY("Insert error");
    }
    else
    {
      MYSQL_DISPLAY("Data Inserted.");
    }
  }
  else
  {
    MYSQL_DISPLAY("Disconnected from Server. Can't insert.");
  }
}
