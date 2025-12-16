#include <WiFi.h>
#include <HardwareSerial.h> 

const char* ssid = "Dialog 4G 732";
const char* password = "F27c5dd5";

WiFiServer server(80);

// --- Serial2 for Arduino Communication ---
#define RXD2 16
#define TXD2 17

// --- PIN DEFINITIONS ---
#define LIGHT_PIN 4          // CHANGED: From 2 to 4 (Safer for boot)
#define GATE_TRIGGER_PIN 23
#define FIRE_ALARM_PIN 27          

// --- Global Variables ---
String lightState = "off";   // Renamed for clarity
int currentFloor = 0; 
int minFloor = 0;    
int maxFloor = 9;    
bool isFireAlarmActive = false;
bool isGateOpen = false;

void setup() {
  // 1. Debug Serial
  Serial.begin(115200);

  // 2. Communication Serial
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Light Control Pin
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW); // Start OFF

  // Gate Trigger Pin
  pinMode(GATE_TRIGGER_PIN, OUTPUT);
  digitalWrite(GATE_TRIGGER_PIN, HIGH); 

  // Fire Alarm Output
  pinMode(FIRE_ALARM_PIN, OUTPUT);
  digitalWrite(FIRE_ALARM_PIN, LOW); 

  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi connection lost. Attempting to reconnect...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000); 
    return;
  }

  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    String header = ""; 
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (header.length() < 200) header += c; 
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            
            // --- LOGIC SECTION ---
            
            // Light Logic (Updated to use LIGHT_PIN)
            if (header.indexOf("GET /light/on") >= 0) {
              Serial.println("COMMAND RECEIVED: Light ON");
              digitalWrite(LIGHT_PIN, HIGH);
              lightState = "on";
            } else if (header.indexOf("GET /light/off") >= 0) {
              Serial.println("COMMAND RECEIVED: Light OFF");
              digitalWrite(LIGHT_PIN, LOW);
              lightState = "off";
            }

            // Fire
            if (header.indexOf("GET /fire/on") >= 0) {
              isFireAlarmActive = true;
              digitalWrite(FIRE_ALARM_PIN, HIGH);
            } else if (header.indexOf("GET /fire/off") >= 0) {
              isFireAlarmActive = false;
              digitalWrite(FIRE_ALARM_PIN, LOW);
            }

            // Gate
            if (header.indexOf("GET /gate/open") >= 0) {
              isGateOpen = true;
              digitalWrite(GATE_TRIGGER_PIN, LOW);
              delay(200); 
              digitalWrite(GATE_TRIGGER_PIN, HIGH);
            } else if (header.indexOf("GET /gate/close") >= 0) {
              isGateOpen = false;
            }

            // Elevator Logic
            if (header.indexOf("GET /elevator/goto/") >= 0) {
              int pos1 = header.indexOf("GET /elevator/goto/");
              int pos2 = header.indexOf(" ", pos1 + 19); 
              String numStr = header.substring(pos1 + 19, pos2);
              int targetFloor = numStr.toInt();
              
              if (targetFloor >= minFloor && targetFloor <= maxFloor) {
                currentFloor = targetFloor;
                Serial.print("[ESP32 DEBUG] Sending floor to Arduino: ");
                Serial.println(targetFloor);
                Serial2.println(targetFloor); 
              }
            }

            // --- HTML RESPONSE SECTION ---
            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            client.println("<!DOCTYPE html><html lang=\"en\"><head>");
            client.println("<meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
            client.println("<title>Service Building Control</title>");
            client.println("<script src=\"https://cdn.tailwindcss.com\"></script>");
            client.println("<link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap\" rel=\"stylesheet\">");
            
            client.println("<style>");
            client.println("body, html { -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; font-family: 'Inter', sans-serif; background-image: linear-gradient(rgba(0, 0, 0, 0.75), rgba(0, 0, 0, 0.75)), url('https://images.unsplash.com/photo-1486406146926-c627a92ad1ab?q=80&w=2070&auto=format&fit=crop'); background-size: cover; background-attachment: fixed; }");
            client.println("@keyframes shimmer { 0% { background-position: -200% center; } 100% { background-position: 200% center; } }");
            client.println(".anim-text { background: linear-gradient(to right, #ffffff 0%, #60a5fa 50%, #ffffff 100%); background-size: 200% auto; color: #000; background-clip: text; -webkit-background-clip: text; -webkit-text-fill-color: transparent; animation: shimmer 3s linear infinite; }");
            client.println("</style>");

            if (header.indexOf("/gate") >= 0) {
                client.println("<script>document.addEventListener('DOMContentLoaded', (event) => { const c = document.getElementById('gate-countdown'); if (c) { let s = 3; c.innerText = `Closing in ${s}...`; const i = setInterval(() => { s--; if (s > 0) { c.innerText = `Closing in ${s}...`; } else { c.innerText = 'Resetting...'; clearInterval(i); window.location.href = '/gate/close'; } }, 1000); } });</script>");
            }
            
            client.println("</head>");
            client.println("<body class=\"text-gray-100 flex flex-col min-h-screen\">");
            
            client.println("<div class=\"w-full max-w-7xl p-6 mx-auto\">");
            client.println("<h1 class=\"text-3xl font-bold text-center mb-4 p-4 rounded-2xl bg-gray-700/30 backdrop-blur-sm shadow-lg\"><a href='/' class='anim-text'>Service Building Control</a></h1>");
            client.println("</div>");
            
            client.println("<div class=\"w-full max-w-7xl p-4 mx-auto flex-grow flex flex-col justify-center\">");

            // --- PAGE: LIGHTING ---
            if (header.indexOf("GET /light") >= 0) {
                if (lightState == "off") {
                    client.println("<div class=\"max-w-2xl mx-auto bg-gray-800/70 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-8 flex flex-col items-center space-y-6\">");
                    client.println("  <div class=\"text-gray-600\"><svg class=\"h-24 w-24\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\" stroke-width=\"2\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M9.663 17h4.673M12 3v1m6.364 1.636l-.707.707M21 12h-1M4 12H3m3.343-5.657l-.707-.707m2.828 9.9a5 5 0 117.072 0l-.548.547A3.374 3.374 0 0014 18.469V19a2 2 0 11-4 0v-.531c0-.895-.356-1.754-.988-2.386l-.548-.547z\" /></svg></div>");
                    client.println("  <h2 class=\"text-2xl font-semibold text-white\">Main Circuit (Lighting)</h2>");
                    client.println("  <p class=\"text-gray-400\">Currently: <span class=\"font-bold text-gray-400\">OFF</span></p>");
                    client.println("  <a href=\"/light/on\" class=\"block w-full text-center bg-green-500 hover:bg-green-600 text-white font-bold py-4 px-4 rounded-lg shadow-md transition\">Turn ON</a>");
                    client.println("</div>");
                } else {
                    client.println("<div class=\"max-w-2xl mx-auto bg-gray-800/70 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-8 flex flex-col items-center space-y-6\">");
                    client.println("  <div class=\"text-yellow-300\"><svg class=\"h-24 w-24\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\" stroke-width=\"2\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M9.663 17h4.673M12 3v1m6.364 1.636l-.707.707M21 12h-1M4 12H3m3.343-5.657l-.707-.707m2.828 9.9a5 5 0 117.072 0l-.548.547A3.374 3.374 0 0014 18.469V19a2 2 0 11-4 0v-.531c0-.895-.356-1.754-.988-2.386l-.548-.547z\" /></svg></div>");
                    client.println("  <h2 class=\"text-2xl font-semibold text-white\">Main Circuit (Lighting)</h2>");
                    client.println("  <p class=\"text-gray-400\">Currently: <span class=\"font-bold text-yellow-300\">ON</span></p>");
                    client.println("  <a href=\"/light/off\" class=\"block w-full text-center bg-red-500 hover:bg-red-600 text-white font-bold py-4 px-4 rounded-lg shadow-md transition\">Turn OFF</a>");
                    client.println("</div>");
                }
                client.println("<br><div class='text-center'><a href='/' class='text-gray-300 underline hover:text-white'>&larr; Back to Home</a></div>");
            }

            // --- PAGE: ELEVATOR ---
            else if (header.indexOf("GET /elevator") >= 0) {
                 client.println("<div class=\"max-w-2xl mx-auto bg-gray-800/70 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-8 flex flex-col items-center space-y-6\">");
                 client.println("  <div class=\"text-blue-400\"><svg class=\"h-24 w-24\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\" stroke-width=\"2\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M8 9l4-4 4 4m0 6l-4 4-4-4\" /></svg></div>");
                 client.println("  <h2 class=\"text-2xl font-semibold text-white\">Elevator Control</h2>");
                 client.println("  <p class=\"text-gray-400 mb-4\">Current Floor: <span class=\"font-bold text-blue-300\">" + String(currentFloor) + "</span></p>");
                 client.println("  <div class=\"grid grid-cols-4 gap-4 w-full\">");
                 
                 for (int i = minFloor; i <= maxFloor; i++) {
                     if (i == currentFloor) {
                         client.println("<span class=\"text-center bg-blue-700 text-white font-bold py-4 px-2 rounded-lg shadow-md\">" + String(i) + "</span>");
                     } else {
                         client.println("<a href=\"/elevator/goto/" + String(i) + "\" class=\"text-center bg-blue-500 hover:bg-blue-600 text-white font-bold py-4 px-2 rounded-lg shadow-md transition\">" + String(i) + "</a>");
                     }
                 }
                 client.println("  </div>");
                 client.println("</div>");
                 client.println("<br><div class='text-center'><a href='/' class='text-gray-300 underline hover:text-white'>&larr; Back to Home</a></div>");
            }

            // --- PAGE: FIRE ALARM ---
            else if (header.indexOf("GET /fire") >= 0) {
                if (isFireAlarmActive) {
                    client.println("<div class=\"max-w-2xl mx-auto bg-orange-600 rounded-2xl shadow-lg p-8 flex flex-col items-center space-y-6\">");
                    client.println("  <div class=\"text-white\"><svg class=\"h-24 w-24\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\" stroke-width=\"2\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M11 5.882V19.24a1.76 1.76 0 01-3.417.592l-2.147-6.15M18 13a3 3 0 100-6M5.436 13.683A4.001 4.001 0 017 6h1.832c4.1 0 7.625-1.234 9.168-3v14c-1.543-1.766-5.067-3-9.168-3H7a3.988 3.988 0 01-1.564-.317z\" /></svg></div>");
                    client.println("  <h2 class=\"text-3xl font-bold text-white\">MANUAL ALARM ON</h2>");
                    client.println("  <p class=\"text-orange-100\">Siren Triggered via Web.</p>");
                    client.println("  <a href=\"/fire/off\" class=\"block w-full text-center bg-gray-100 hover:bg-gray-200 text-orange-700 font-bold py-4 px-4 rounded-lg shadow-md\">Stop Manual Alarm</a>");
                    client.println("</div>");
                } else {
                    client.println("<div class=\"max-w-2xl mx-auto bg-gray-800/70 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-8 flex flex-col items-center space-y-6\">");
                    client.println("  <div class=\"text-green-300\"><svg class=\"h-24 w-24\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\" stroke-width=\"2\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z\" /></svg></div>");
                    client.println("  <h2 class=\"text-2xl font-semibold text-white\">Fire Alarm System</h2>");
                    client.println("  <p class=\"text-gray-300\">Status: <span class=\"font-bold text-green-300\">ALL CLEAR</span></p>");
                    client.println("  <a href=\"/fire/on\" class=\"block w-full text-center bg-red-600 hover:bg-red-700 text-white font-bold py-4 px-4 rounded-lg shadow-md transition\">Trigger Manual Test</a>");
                    client.println("</div>");
                }
                client.println("<br><div class='text-center'><a href='/' class='text-gray-300 underline hover:text-white'>&larr; Back to Home</a></div>");
            }

            // --- PAGE: GATE ---
            else if (header.indexOf("GET /gate") >= 0) {
                client.println("<div class=\"max-w-2xl mx-auto bg-gray-800/70 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-8 flex flex-col items-center space-y-6\">");
                client.println("  <div class=\"text-purple-400\"><svg class=\"h-24 w-24\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\" stroke-width=\"2\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M15 7a2 2 0 012 2m4 0a6 6 0 01-7.743 5.743L11 17H9v2H7v2H4a1 1 0 01-1-1v-2.586a1 1 0 01.293-.707l5.964-5.964A6 6 0 0121 9z\" /></svg></div>");
                client.println("  <h2 class=\"text-2xl font-semibold text-white\">Car Park Gate</h2>");
                if (isGateOpen) {
                    client.println("  <p class=\"text-gray-300\">Status: <span class=\"font-bold text-yellow-300\">Gate Open</span></p>");
                    client.println("  <div id=\"gate-countdown\" class=\"block w-full text-center bg-gray-600 text-white font-bold py-4 px-4 rounded-lg shadow-md animate-pulse\">Initializing...</div>");
                } else {
                    client.println("  <p class=\"text-gray-400\">Send one-time open signal.</p>");
                    client.println("  <a href=\"/gate/open\" class=\"block w-full text-center bg-purple-500 hover:bg-purple-600 text-white font-bold py-4 px-4 rounded-lg shadow-md transition\">Open Gate</a>");
                }
                client.println("</div>");
                client.println("<br><div class='text-center'><a href='/' class='text-gray-300 underline hover:text-white'>&larr; Back to Home</a></div>");
            }

            // --- PAGE: CAMERA ---
            else if (header.indexOf("GET /camera") >= 0) {
                client.println("<div class=\"w-full bg-gray-800/70 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-4\">");
                client.println("  <h3 class=\"text-lg font-semibold text-white mb-3 text-center\">Camera - Live Feed</h3>");
                client.println("  <div class=\"aspect-[16/9] rounded-lg overflow-hidden bg-black\">");
                client.println("    <iframe src=\"http://192.168.8.170\" class=\"w-full h-full\" frameborder=\"0\" allowfullscreen></iframe>");
                client.println("  </div>");
                client.println("</div>");
                client.println("<br><div class='text-center'><a href='/' class='text-gray-300 underline hover:text-white'>&larr; Back to Home</a></div>");
            }
            
            // --- PAGE: ABOUT ---
            else if (header.indexOf("GET /about") >= 0) {
                client.println("<div class=\"max-w-3xl mx-auto bg-gray-800/80 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-10\">");
                client.println("  <h2 class=\"text-3xl font-bold text-white mb-6 border-b border-gray-600 pb-4\">About The Project</h2>");
                client.println("  <div class=\"space-y-4 text-gray-300 text-lg leading-relaxed\">");
                client.println("    <p>Welcome to the <span class=\"text-blue-400 font-semibold\">Service Building Control System</span> (V1.0).</p>");
                client.println("    <ul class=\"list-disc list-inside ml-4 space-y-2\">");
                client.println("      <li><span class=\"text-yellow-300\">Lighting Control:</span> Remote switching.</li>");
                client.println("      <li><span class=\"text-blue-300\">Vertical Transport:</span> Elevator logic.</li>");
                client.println("      <li><span class=\"text-red-400\">Safety:</span> Fire alarm trigger.</li>");
                client.println("      <li><span class=\"text-purple-300\">Access:</span> Gate actuation.</li>");
                client.println("    </ul>");
                client.println("  </div>");
                client.println("</div>");
                client.println("<br><div class='text-center'><a href='/' class='text-gray-300 underline hover:text-white'>&larr; Back to Home</a></div>");
            }

            // --- HOME PAGE (DASHBOARD) ---
            else {
               client.println("<div class=\"grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6\">");
               
               // Light (Dynamic Icon Color Logic)
               client.println("<a href='/light' class=\"bg-gray-800/70 hover:bg-gray-700/80 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-6 flex flex-col items-center space-y-4 transition transform hover:scale-105\">");
               if (lightState == "on") {
                   client.println("<div class=\"text-yellow-300\">");
               } else {
                   client.println("<div class=\"text-gray-500\">");
               }
               client.println("<svg class=\"h-16 w-16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M9.663 17h4.673M12 3v1m6.364 1.636l-.707.707M21 12h-1M4 12H3m3.343-5.657l-.707-.707m2.828 9.9a5 5 0 117.072 0l-.548.547A3.374 3.374 0 0014 18.469V19a2 2 0 11-4 0v-.531c0-.895-.356-1.754-.988-2.386l-.548-.547z\" /></svg></div>");
               client.println("<h2 class=\"text-xl font-bold text-white\">Lighting</h2></a>");

               // Elevator
               client.println("<a href='/elevator' class=\"bg-gray-800/70 hover:bg-gray-700/80 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-6 flex flex-col items-center space-y-4 transition transform hover:scale-105\">");
               client.println("<div class=\"text-blue-400\"><svg class=\"h-16 w-16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M8 9l4-4 4 4m0 6l-4 4-4-4\" /></svg></div>");
               client.println("<h2 class=\"text-xl font-bold text-white\">Elevator</h2></a>");
               
               // Fire
               client.println("<a href='/fire' class=\"bg-gray-800/70 hover:bg-gray-700/80 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-6 flex flex-col items-center space-y-4 transition transform hover:scale-105\">");
               client.println("<div class=\"text-red-500\"><svg class=\"h-16 w-16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M17.657 18.657A8 8 0 016.343 7.343S7 9 9 10c0-2 .5-5 2.986-7C14 5 16.09 5.777 17.656 7.343A7.975 7.975 0 0120 13a7.975 7.975 0 01-2.343 5.657z\" /></svg></div>");
               client.println("<h2 class=\"text-xl font-bold text-white\">Fire Alarm</h2></a>");
               
               // Gate
               client.println("<a href='/gate' class=\"bg-gray-800/70 hover:bg-gray-700/80 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-6 flex flex-col items-center space-y-4 transition transform hover:scale-105\">");
               client.println("<div class=\"text-purple-400\"><svg class=\"h-16 w-16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M15 7a2 2 0 012 2m4 0a6 6 0 01-7.743 5.743L11 17H9v2H7v2H4a1 1 0 01-1-1v-2.586a1 1 0 01.293-.707l5.964-5.964A6 6 0 0121 9z\" /></svg></div>");
               client.println("<h2 class=\"text-xl font-bold text-white\">Car Park Gate</h2></a>");
               
               // Camera
               client.println("<a href='/camera' class=\"bg-gray-800/70 hover:bg-gray-700/80 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-6 flex flex-col items-center space-y-4 transition transform hover:scale-105\">");
               client.println("<div class=\"text-green-300\"><svg class=\"h-16 w-16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M15 10l4.553-2.276A1 1 0 0121 8.618v6.764a1 1 0 01-1.447.894L15 14M5 18h8a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z\" /></svg></div>");
               client.println("<h2 class=\"text-xl font-bold text-white\">Live Camera</h2></a>");
               
               // About
               client.println("<a href='/about' class=\"bg-gray-800/70 hover:bg-gray-700/80 backdrop-blur-sm border border-white/10 rounded-2xl shadow-lg p-6 flex flex-col items-center space-y-4 transition transform hover:scale-105\">");
               client.println("<div class=\"text-gray-300\"><svg class=\"h-16 w-16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z\" /></svg></div>");
               client.println("<h2 class=\"text-xl font-bold text-white\">About</h2></a>");
               
               client.println("</div>");
            }
            
            client.println("</div>");
            client.println("<div class=\"w-full bg-gray-100 p-4 text-center shadow-inner mt-auto\">");
            client.println("  <p class=\"text-gray-700 font-bold\">A project by CEE Department</p>");
            client.println("</div>");

            client.println("</body></html>");
            client.println();
            break; 
            
          } else {
             currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
}