/**
 * @file esp32_artnet_node.ino
 * @author Mikita Stankiewicz (itsmikita@gmail.com)
 * @brief My take on Art-Net to DMX Node based on ESP32.
 * @version 0.4
 * @date 2022-06-10
 * 
 * @copyright Copyright (c) 2022 Mikita Stankiewicz
 */

#include <FS.h>
#include <SPIFFS.h>
// #include <LittleFS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
// #include <espDMX.h>
// #include <dmx.h>
#include <ArtnetnodeWifi.h>

#define FIRMWARE_VERSION "v0.6"

#define FILESYSTEM_FORMAT_ON_FAIL true

#define HTTP_HOST "rave-art-net.node"
#define HTTP_PORT 80

#define HOTSPOT_SSID "raveartnetnode"
#define HOTSPOT_PASSWORD "raversgonnarave"

#define DHCP true
#define DHCP_STATIC_IP { 192, 168, 0, 1 }
#define DHCP_SUBNET_MASK { 255, 255, 255, 0 }

#define ARTNET_VERSION 0x0200
#define ARTNET_OEM 0x0123
#define ARTNET_PORT 6454
#define ARTNET_BUFFER_SIZE 600
#define ARTNET_REPLY_SIZE 239
#define ARTNET_ARTDMX 0x5000
#define ARTNET_ARTPOLL 0x2000
#define ARTNET_ADDRESS_OFFSET 18

#define LED_RED 23 
#define LED_GREEN 22
#define LED_BLUE 21
#define LED_YELLOW 19

uint8_t mac[ 6 ];
String macAddress;
char nodeName[ 30 ] = "artNetNode";
IPAddress ip( 192, 168, 0, 1 );
IPAddress hotspotIp( 2, 0, 0, 10 );
IPAddress subnetMask( 255, 0, 0, 0 );
IPAddress broadcastIp( ip[ 0 ], 255, 255, 255 );
uint8_t artNetUniverse = 0;
uint8_t artNetSub = 0;
// espDMX dmx( 0 );
// DMX dmx;
unsigned char packetBuffer[ ARTNET_BUFFER_SIZE ];
bool outputScene = 0;
uint16_t outputSceneNum = 0;

WiFiUDP UdpSend;
ArtnetnodeWifi artnetnode;
WebServer server( HTTP_PORT );
File upload;



/**
 * Initialize FileSystem, Serial debug and DMX output.
 * Start WiFi, UDP listener and WebServer.
 * Send an Art-Net response to alert all devices to our presence.
 */
void setup() {
  pinMode( LED_RED, OUTPUT );
  pinMode( LED_GREEN, OUTPUT );
  pinMode( LED_BLUE, OUTPUT );
  pinMode( LED_YELLOW, OUTPUT );

  Serial.begin( 115200 );
  Serial.setDebugOutput( true );
  Serial.println( "Hello, Ravers!" );
  Serial.println( "Firmware Version: " + String( FIRMWARE_VERSION ) );

  if( !SPIFFS.begin( FILESYSTEM_FORMAT_ON_FAIL ) ) {
    error( "FILESYSTEM.begin() — Mount Failed!" );
    return;
  }

  Serial.println();
  Serial.println( "Configuring Access Point..." );
  WiFi.mode( WIFI_AP );
  WiFi.softAP( HOTSPOT_SSID, HOTSPOT_PASSWORD );
  // WiFi.softAPConfig( DHCP_STATIC_IP, DHCP_STATIC_IP, DHCP_SUBNET_MASK );

  // Web Server
  startWebServer();

  // Art-Net node
  artnetnode.setName( "Rave Art-Net Node" );
  artnetnode.setNumPorts( 1 );
  artnetnode.enableDMXOutput( 0 );
  artnetnode.begin();  
  artnetnode.setArtDmxCallback( onDmxFrame );
}

/**
 * Main Application Loop
 */
void loop() {
  artnetnode.read(); // we call the read function inside the loop
  server.handleClient(); // Web Server
}

/**
 * 
 */
void onDmxFrame( uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data ) {
  // Send "break" as a "slow" zero.
  Serial.begin( 56700 );
  Serial.write( ( uint8_t ) 0 );
  delayMicroseconds( 220 );
  Serial.begin( 250000, SERIAL_8N2 );

  Serial.write( ( uint8_t ) 0 ); // Start-Byte
  // send out the buffer
  for (int i = 0; i < length; i++)
  {
    Serial.write( data[ i ] );
  }

}

/**
 * Error Handler
 */
void error( String message ) {
  Serial.println( "ERROR: " + message );
}

/**
 * 
 */
void startWebServer() {
    server.on( "/", HTTP_GET, []() {
    String path = "/index.html";
    if( SPIFFS.exists( path ) ) {
      File file = SPIFFS.open( "/index.html", "r" );
      server.streamFile( file, getContentType( path ) );
      file.close();
    }
  } );
  server.on( "/config", HTTP_GET, []() {
    handleFileRead( server.uri() );
    server.send( 200, "application/json", "{msg:'Read settings'}" );
  } );
  server.on( "/config", HTTP_POST, []() {
    server.send( 200, "application/json", "{msg:'Write settings'}" );
  } );
  server.on( "/status", HTTP_GET, []() {
    server.send( 200, "application/json", "{msg:'Read status'}" );
  } );
  server.on( "/reboot", HTTP_GET, []() {
    server.send( 200, "application/json", "{msg:'Reboot'}" );
  } );
  server.on( "/reset", HTTP_GET, []() {
    server.send( 200, "application/json", "{msg:'Factory Defaults'}" );
  } );
  server.on( "/flash", HTTP_POST, []() {
    server.send( 200, "application/json", "{msg:'Flash firmware OTA'}" );
  }, handleFileUpload );
  server.onNotFound( []() {
    if( !handleFileRead( server.uri() ) ) {
      server.send( 404, "text/plain", "File Not Found" );
    }
  } );
  server.begin();
  Serial.println( "Started Web Server on http://" + WiFi.localIP().toString() );
}

/**
 * Get the Content Type object
 */
String getContentType( String filename ) {
  if( server.hasArg( "download" ) ) return "application/octet-stream";
  else if( filename.endsWith( ".html" ) ) return "text/html";
  else if( filename.endsWith( ".htm" ) ) return "text/html";
  else if( filename.endsWith( ".css" ) ) return "text/css";
  else if( filename.endsWith( ".js" ) ) return "application/javascript";
  else if( filename.endsWith( ".json" ) ) return "application/json";
  else if( filename.endsWith( ".zip" ) ) return "application/x-zip";
  else if( filename.endsWith( ".gz" ) ) return "application/x-gzip";
  else if( filename.endsWith( ".woff2" ) ) return "font/woff2";
  else if( filename.endsWith( ".jpg" ) ) return "image/jpeg";
  else if( filename.endsWith( ".jpeg" ) ) return "image/jpeg";
  else if( filename.endsWith( ".png" ) ) return "image/png";
  else if( filename.endsWith( ".gif" ) ) return "image/gif";
  else if( filename.endsWith( ".ico" ) ) return "image/x-icon";
  else if( filename.endsWith( ".svg" ) ) return "image/svg+xml";
  else return "text/plain";
}

bool handleFileRead( String path ) {
  if( path.endsWith( "/" ) ) {
    path += "index.html";
  }
  if( SPIFFS.exists( path ) ) {
    File file = SPIFFS.open( path, "r" );
    server.streamFile( file, getContentType( path ) );
    file.close();
    return true;
  }
  // else if( path == "/r" ) {
  //   digitalWrite( LED_RED, !digitalRead( LED_RED ) );
  //   return true;
  // }
  // else if( path == "/g" ) {
  //   digitalWrite( LED_GREEN, !digitalRead( LED_GREEN ) );
  //   return true;
  // }
  // else if( path == "/b" ) {
  //   digitalWrite( LED_BLUE, !digitalRead( LED_BLUE ) );
  //   return true;
  // }
  // else if( path == "/y" ) {
  //   digitalWrite( LED_YELLOW, !digitalRead( LED_YELLOW ) );
  //   return true;
  // }
  return false;
}

void handleFileUpload() {
  HTTPUpload& tmp = server.upload();
  if( tmp.status == UPLOAD_FILE_START ) {
    String filename = tmp.filename;
    if( !filename.startsWith( "/" ) ) {
      filename = "/" + filename;
    }
    upload = SPIFFS.open( filename, "w" );
    filename = String();
  }
  else if( tmp.status == UPLOAD_FILE_WRITE ) {
    if( upload ) {
      upload.write( tmp.buf, tmp.currentSize );
    }
  }
  else if( tmp.status == UPLOAD_FILE_END ) {
    if( upload ) {
      upload.close();
    }
  }
}

void handleNotFound() {
  server.send( 404, "text/plain", "Not Found" );
}


