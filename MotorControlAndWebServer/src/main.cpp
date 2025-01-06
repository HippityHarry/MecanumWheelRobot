#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <vector>
#include <iostream>
#include <sstream>

// Wi-Fi credentials
const char* ssid = "MyCar";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

// Car movement constants
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define UP_LEFT 5
#define UP_RIGHT 6
#define DOWN_LEFT 7
#define DOWN_RIGHT 8
#define TURN_LEFT 9
#define TURN_RIGHT 10
#define STOP 0

#define FRONT_RIGHT_MOTOR 0
#define BACK_RIGHT_MOTOR 1
#define FRONT_LEFT_MOTOR 2
#define BACK_LEFT_MOTOR 3

#define FORWARD 1
#define BACKWARD -1

struct MOTOR_PINS {
    int pinRPWM;         // PWM for Right side of the H-bridge
    int pinLPWM;         // PWM for Left side of the H-bridge
    int pwmChannelR;     // PWM channel for Right side
    int pwmChannelL;     // PWM channel for Left side
};

// Motor pins setup
std::vector<MOTOR_PINS> motorPins = {
    {22, 23, 0, 1},   // FRONT_RIGHT_MOTOR (RPWM=22, LPWM=23, PWM channels=0, 1)
    {19, 21, 2, 3},   // BACK_RIGHT_MOTOR  (RPWM=19, LPWM=21, PWM channels=2, 3)
    {25, 26, 4, 5},   // FRONT_LEFT_MOTOR  (RPWM=25, LPWM=26, PWM channels=4, 5)
    {27, 33, 6, 7}    // BACK_LEFT_MOTOR   (RPWM=27, LPWM=33, PWM channels=6, 7)
};

const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;

void rotateMotor(int motorNumber, int motorDirection, int speed = 128);
void processCarMovement(int inputValue);
void setUpPinModes();
void onCarInputWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    .arrows {
      font-size:40px;
      color:red;
    }
    .circularArrows {
      font-size:50px;
      color:blue;
    }
    td.button {
      background-color:black;
      border-radius:25%;
      box-shadow: 5px 5px #888888;
    }
    td.button:active {
      transform: translate(5px,5px);
      box-shadow: none; 
    }

    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
      -webkit-user-select: none; /* Safari */
      -khtml-user-select: none; /* Konqueror HTML */
      -moz-user-select: none; /* Firefox */
      -ms-user-select: none; /* Internet Explorer/Edge */
      user-select: none; /* Non-prefixed version, currently supported by Chrome and Opera */
    }

    .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 15px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }

    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 25px;
      height: 25px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 25px;
      height: 25px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <img id="videoElement" width="640" height="480" />
      </tr> 
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","5")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#11017;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","1")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8679;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","6")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#11016;</span></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","3")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8678;</span></td>
        <td class="button"></td>    
        <td class="button" ontouchstart='sendButtonInput("MoveCar","4")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8680;</span></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","7")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#11019;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","2")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#8681;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","8")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows">&#11018;</span></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","9")' ontouchend='sendButtonInput("MoveCar","0")'><span class="circularArrows">&#8634;</span></td>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","10")' ontouchend='sendButtonInput("MoveCar","0")'><span class="circularArrows">&#8635;</span></td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left"><b>Speed:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="255" value="150" class="slider" id="Speed" oninput='sendButtonInput("Speed",value)'>
          </div>
        </td>
      </tr>        
      <tr>
        <td style="text-align:left"><b>Light:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="255" value="0" class="slider" id="Light" oninput='sendButtonInput("Light",value)'>
          </div>
        </td>   
      </tr>
    </table>
  
    <script>
      // WebSocket for car commands
      var webSocketCarInputUrl = "ws://" + window.location.hostname + "/CarInput";  
      var websocketCarInput;

      // Initialize WebSocket for car control
      function initCarInputWebSocket() 
      {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen = function() {
          var speedButton = document.getElementById("Speed");
          sendButtonInput("Speed", speedButton.value);
          var lightButton = document.getElementById("Light");
          sendButtonInput("Light", lightButton.value);
        };
        websocketCarInput.onclose = function() {
          setTimeout(initCarInputWebSocket, 2000);
        };
        websocketCarInput.onmessage = function(event) {
          console.log("WebSocket message: " + event.data);
        };        
      }
      
      function sendButtonInput(key, value) 
      {
        var data = key + "," + value;
        websocketCarInput.send(data);
      }

      window.onload = initCarInputWebSocket;

      // Prevent touch behavior for table elements
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault();
      });     

      // WebSocket connection to the ESP32-CAM video stream
      let socket = new WebSocket("ws://192.168.4.2:81");

      socket.onmessage = function(event) {
      const videoBlob = new Blob([event.data], {type: 'image/jpeg'}); // Turn the binary data into an image
      const videoElement = document.getElementById("videoElement"); // A video element on your page to show the video
      const videoUrl = URL.createObjectURL(videoBlob);  // Create a URL for the binary data
      videoElement.src = videoUrl;  // Update the src to show the live feed
};

    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", htmlHomePage);
}

int currentSpeed = 128;

// Handle WebSocket events
void onCarInputWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_DATA) {
        String command = String((char*)data);
        int commaIndex = command.indexOf(',');
        String key = command.substring(0, commaIndex);
        int value = command.substring(commaIndex + 1).toInt();
        if (key == "MoveCar") {
            processCarMovement(value);
        } else if (key == "Speed") {
            currentSpeed = value; 
        }
    }
}

void rotateMotor(int motorNumber, int motorDirection, int speed) {
    if (motorDirection == FORWARD) {
        ledcWrite(motorPins[motorNumber].pwmChannelR, speed);
        ledcWrite(motorPins[motorNumber].pwmChannelL, 0);
    } else if (motorDirection == BACKWARD) {
        ledcWrite(motorPins[motorNumber].pwmChannelR, 0);
        ledcWrite(motorPins[motorNumber].pwmChannelL, speed);
    } else {
        ledcWrite(motorPins[motorNumber].pwmChannelR, 0);
        ledcWrite(motorPins[motorNumber].pwmChannelL, 0);
    }
}

// Process movement
void processCarMovement(int inputValue) {
    switch (inputValue) {
        case UP:
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD, currentSpeed);
      break;
      

    case DOWN:
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD, currentSpeed);
      break;

    case LEFT:
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD, currentSpeed);
      break;

    case RIGHT:
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD, currentSpeed);
      break;

    case UP_LEFT:
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD, currentSpeed);  
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      break;
  
    case UP_RIGHT:
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(BACK_LEFT_MOTOR, STOP);  
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD, currentSpeed);
      break;
  
    case DOWN_LEFT:
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(BACK_LEFT_MOTOR, STOP);  
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD, currentSpeed); 
      break;
  
    case DOWN_RIGHT:
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD, currentSpeed);   
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      break;
  
    case TURN_LEFT:
      rotateMotor(FRONT_LEFT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(BACK_LEFT_MOTOR, BACKWARD, currentSpeed);  
      rotateMotor(FRONT_RIGHT_MOTOR, BACKWARD, currentSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, BACKWARD, currentSpeed);
      break;
  
    case TURN_RIGHT:
      rotateMotor(FRONT_LEFT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(BACK_LEFT_MOTOR, FORWARD, currentSpeed);   
      rotateMotor(FRONT_RIGHT_MOTOR, FORWARD, currentSpeed);
      rotateMotor(BACK_RIGHT_MOTOR, FORWARD, currentSpeed);
      break;

    case STOP:
    default:
      rotateMotor(FRONT_LEFT_MOTOR, STOP);
      rotateMotor(BACK_LEFT_MOTOR, STOP);
      rotateMotor(FRONT_RIGHT_MOTOR, STOP);
      rotateMotor(BACK_RIGHT_MOTOR, STOP);
      break;
  }
}

void setUpPinModes() {
    for (int i = 0; i < motorPins.size(); i++) {
        ledcSetup(motorPins[i].pwmChannelR, PWMFreq, PWMResolution);
        ledcAttachPin(motorPins[i].pinRPWM, motorPins[i].pwmChannelR);
        ledcSetup(motorPins[i].pwmChannelL, PWMFreq, PWMResolution);
        ledcAttachPin(motorPins[i].pinLPWM, motorPins[i].pwmChannelL);
        rotateMotor(i, STOP);
    }
}

void setup() {
    setUpPinModes();
    WiFi.softAP(ssid, password);
    server.on("/", HTTP_GET, handleRoot);
    wsCarInput.onEvent(onCarInputWebSocketEvent);
    server.addHandler(&wsCarInput);
    server.begin();
}

void loop() {}
