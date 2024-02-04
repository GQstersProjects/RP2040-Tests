// Define Servo PWM Values & Stopped Variable
// Yes I know servo libraries exist - Manual PWM allows the same code between servos and motor drivers.
int endpoint = 32; // Multiplier for servo endpoints. Should be between 30 and 60 for modded servos, up to 180 for ESC.
int speed = 8 * endpoint;
int neutral = 4915; // Servo Neutral Point
int lneutral = neutral;
int rneutral = neutral;
int heartbeat = 0;

// Libraries. Updated for Version 1.0.6
#include <esp32-hal-ledc.h> 
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"
//#include "dl_lib.h"  //1.0.2
#include "dl_lib_matrix3d.h" //1.0.6         // this is gone and not being replaced... code is broken rn.

// Stream Encoding
typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;
static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t capture_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

    size_t out_len, out_width, out_height;
    uint8_t * out_buf;
    bool s;
    {
        size_t fb_len = 0;
        if(fb->format == PIXFORMAT_JPEG){
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        } else {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
        esp_camera_fb_return(fb);
        return res;
    }

    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix) {
        esp_camera_fb_return(fb);
        Serial.println("dl_matrix3du_alloc failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    out_buf = image_matrix->item;
    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;

    s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
    esp_camera_fb_return(fb);
    if(!s){
        dl_matrix3du_free(image_matrix);
        Serial.println("to rgb888 failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    jpg_chunking_t jchunk = {req, 0};
    s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
    dl_matrix3du_free(image_matrix);
    if(!s){
        Serial.println("JPEG compression failed");
        return ESP_FAIL;
    }
    return res;
}

static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    dl_matrix3du_t *image_matrix = NULL;

    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
             {
                if(fb->format != PIXFORMAT_JPEG){
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if(!jpeg_converted){
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                } else {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            }
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }
        // int64_t fr_end = esp_timer_get_time();
        // int64_t frame_time = fr_end - last_frame;
        // last_frame = fr_end;
        // frame_time /= 1000;
        // Serial.printf("MJPG: %uB %ums (%.1ffps)\n",
        //     (uint32_t)(_jpg_buf_len),
        //     (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time           
        // );
    }
    last_frame = 0;
    return res;
}


// Control Handling from Server
// Setup states of motion for Scout
enum state {fwd,rev,stp};
state actstate = stp;

static esp_err_t cmd_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char variable[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;

    heartbeat = millis();
    
    if(!strcmp(variable, "framesize")) 
    {
        Serial.println("framesize");
        if(s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
    }
    else if(!strcmp(variable, "quality")) 
    {
      Serial.println("quality");
      res = s->set_quality(s, val);
    }
    //Remote Control
    else if(!strcmp(variable, "flash")) 
    {
      ledcWrite(7,val);
    }
    else if(!strcmp(variable, "speed")) 
    {
      if      (val > 8) val = 8;
      else if (val <   0) val = 0;       
      speed = val * endpoint;
    }
    else if(!strcmp(variable, "ltrim")) 
    {
      if      (val > 192) val = 192;
      else if (val < -192) val = -192;
      lneutral = neutral+val; 
      ledcWrite(3,lneutral);
    }
    else if(!strcmp(variable, "rtrim")) 
    {
      if      (val > 192) val = 192;
      else if (val < -192) val = -192;
      rneutral = neutral-val;
      ledcWrite(4,rneutral);
    }
    else if(!strcmp(variable, "car")) {  
      if (val==1) {
        Serial.println("Forward");
        actstate = fwd; // Set state to modify left & right behavior while moving.   
        ledcWrite(3,lneutral+speed);  // GPIO pin 12 - Left Servo adds speed to neutral value
        ledcWrite(4,rneutral-speed);  // GPIO pin 13 - Right Servo subtracts speed from neutral value
      }
      else if (val==2) {
        Serial.println("Backward");  
        actstate = rev; // Set state to modify left & right behavior while moving.      
        ledcWrite(3,lneutral-speed);  // GPIO pin 12 Left Servo
        ledcWrite(4,rneutral+speed);  // GPIO pin 13 Right Servo        
      }
      else if (val==3) {
        Serial.println("TurnLeft");
        if      (actstate == fwd) { ledcWrite(3,lneutral+speed*.33); ledcWrite(4,rneutral-speed); }
        else if (actstate == rev) { ledcWrite(3,lneutral-speed*.33); ledcWrite(4,rneutral+speed); }
        else                      { ledcWrite(3,lneutral-speed*.75); ledcWrite(4,rneutral-speed*.75); }          
      }
      else if (val==4) {
        Serial.println("TurnRight");
        if      (actstate == fwd) { ledcWrite(3,lneutral+speed); ledcWrite(4,rneutral-speed*.33); }
        else if (actstate == rev) { ledcWrite(3,lneutral-speed); ledcWrite(4,rneutral+speed*.33); }
        else                      { ledcWrite(3,lneutral+speed*.75); ledcWrite(4,rneutral+speed*.75); }        
      }
      else {
        Serial.println("Stop");
        actstate = stp;
        ledcWrite(3,lneutral); // Send no pulse to disable servo (no drift).
        ledcWrite(4,rneutral); // Send no pulse to disable servo (no drift).       
      } 
    }        
    else 
    { 
      Serial.println("variable");
      res = -1; 
    }

    if(res){ return httpd_resp_send_500(req); }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req){
    static char json_response[1024]; 

    sensor_t * s = esp_camera_sensor_get();
    char * p = json_response;
    *p++ = '{';

    p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p+=sprintf(p, "\"quality\":%u,", s->status.quality);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

// Front End / GUI Webpage
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Tank!</title>
<style>
body, html {
    height: 100%;
    width: 100%;
    margin: 0;
    padding: 0;
    font-family: Arial;
    background: #006666;
}

/* Style the tab */
.tab {
    overflow: hidden;
    border: 1px solid #ccc;
    background-color: #009999;
}

/* Style the buttons inside the tab */
.tab button {
    background-color: inherit;
    float: left;
    border: none;
    outline: none;
    cursor: pointer;
    padding: 14px 16px;
    transition: 0.3s;
    font-size: 17px;
}

/* Change background color of buttons on hover */
.tab button:hover {
    background-color: #cc66ff;
}

/* Create an active/current tablink class */
.tab button.active {
    background-color: #cc99ff;
}

/* Style the tab content */
.tabcontent {
    display: none;
    padding: 0; /* Adjusted padding to 0 */
    border: 1px solid #ccc;
    border-top: none;
    height: 100%; /* Adjust height as needed */
    -webkit-animation: fadeEffect 1s;
    animation: fadeEffect 1s;
    box-sizing: border-box; /* Include padding within height calculation */
}

/* Fade in tabs */
@-webkit-keyframes fadeEffect {
    from {opacity: 0;}
    to {opacity: 1;}
}

@keyframes fadeEffect {
    from {opacity: 0;}
    to {opacity: 1;}
}


</style>
</head>
<body>

<div class="tab">
  <button class="tablinks" onclick="openCity(event, 'Viewer')" id="defaultOpen">Viewer</button>
  <button class="tablinks" onclick="openCity(event, 'Settings')">Settings</button>
</div>

<div id="Viewer" class="tabcontent">
  <section id="mainViewer">
    <figure>
      <div id="stream-container" class="image-container">
        <img id="stream" src="" style="transform: rotate(270deg);">
      </div>
    </figure>
  </section>
</div>

<div id="Settings" class="tabcontent">
  <section id="mainSettings">
    <figure>
    </figure>
    <section id="buttons">
      <table>
        <tr><td align="center">
        <tr><td align="center">Speed</td><td align="center" colspan="2"><input type="range" id="speed" min="0" max="8" value="8" onchange="try{fetch(document.location.origin+'/control?var=speed&val='+this.value);}catch(e){}"></td></tr>
        <tr><td align="center">Left Trim</td><td align="center" colspan="2"><input type="range" id="ltrim" min="-192" max="192" value="0" onchange="try{fetch(document.location.origin+'/control?var=ltrim&val='+this.value);}catch(e){}"></td></tr>
        <tr><td align="center">Right Trim</td><td align="center" colspan="2"><input type="range" id="rtrim" min="-192" max="192" value="0" onchange="try{fetch(document.location.origin+'/control?var=rtrim&val='+this.value);}catch(e){}"></td></tr>
        <tr><td align="center">Lights</td><td align="center" colspan="2"><input type="range" id="flash" min="0" max="255" value="0" onchange="try{fetch(document.location.origin+'/control?var=flash&val='+this.value);}catch(e){}"></td></tr>
        <tr><td align="center">Quality</td><td align="center" colspan="2"><input type="range" id="quality" min="10" max="63" value="10" onchange="try{fetch(document.location.origin+'/control?var=quality&val='+this.value);}catch(e){}"></td></tr>
        <tr><td align="center">Resolution</td><td align="center" colspan="2"><input type="range" id="framesize" min="0" max="6" value="5" onchange="try{fetch(document.location.origin+'/control?var=framesize&val='+this.value);}catch(e){}"></td></tr>
        <tr><td align="center">Rotation</td><td align="center" colspan="2"><input type="button" value="Rotate Cam" id="rotate" onclick="rotateImg()"></td></tr>
    </table>
    </section>
  </section>
</div>


<script>
  // Function for the tabs
  function openCity(evt, cityName) {
  var i, tabcontent, tablinks;
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
      tabcontent[i].style.display = "none";
  }
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
      tablinks[i].className = tablinks[i].className.replace(" active", "");
  }
  document.getElementById(cityName).style.display = "block";
  evt.currentTarget.className += " active";
  }

  // Get the element with id="defaultOpen" and click on it
  document.getElementById("defaultOpen").click();




  // Functions to control streaming

  var source = document.getElementById('stream');
  source.src = document.location.origin+':81/stream';
  //Camera Rotation Fix
  var rotation = 0;
  function rotateImg() {
      rotation += 90;
      if (rotation === 360) {rotation = 0;}
      document.querySelector("#stream").style.transform = `rotate(${rotation}deg)`;
  }
  // Functions for Controls via Keypress
  var keyforward=0;
  var keybackward=0; 
  var keyleft=0 ;
  var keyright=0;
  // Emulate Keypress with Touch
  var fwdpress = new KeyboardEvent('keydown', {'keyCode':38, 'which':38});
  var fwdrelease = new KeyboardEvent('keyup', {'keyCode':38, 'which':38});
  var backpress = new KeyboardEvent('keydown', {'keyCode':40, 'which':40});
  var backrelease = new KeyboardEvent('keyup', {'keyCode':40, 'which':40});
  var leftpress = new KeyboardEvent('keydown', {'keyCode':37, 'which':37});
  var leftrelease = new KeyboardEvent('keyup', {'keyCode':37, 'which':37});
  var rightpress = new KeyboardEvent('keydown', {'keyCode':39, 'which':39});
  var rightrelease = new KeyboardEvent('keyup', {'keyCode':39, 'which':39});

  //Keypress Events
  document.addEventListener('keydown',function(keyon){
      keyon.preventDefault();
      if ((keyon.keyCode == '38') && (!keybackward) && (!keyforward)) {keyforward = 1;}
      else if ((keyon.keyCode == '40') && (!keyforward) && (!keybackward)){keybackward = 1;}
      else if ((keyon.keyCode == '37') && (!keyright) && (!keyleft)){keyleft = 1;}
      else if ((keyon.keyCode == '39') && (!keyleft) && (!keyright)){keyright = 1;}
      });
      //KeyRelease Events
      document.addEventListener('keyup',function(keyoff){
      if ((keyoff.keyCode == '38') || (keyoff.keyCode == '40')) {keyforward = 0;keybackward = 0;}
      else if ((keyoff.keyCode == '37') || (keyoff.keyCode == '39')) {keyleft = 0;keyright = 0;}
      });
      //Send Commands to Scout
      var currentcommand=0;
      var oldcommand=0;
      var commandcounter=0;
      window.setInterval(function(){
      if (((keyforward) && (keyleft)) || ((keybackward) && (keyleft)) || (keyleft)) {currentcommand = 3;} // Turn Left
      else if (((keyforward) && (keyright)) || ((keybackward) && (keyright)) || (keyright)) {currentcommand = 4;} // Turn Right
      else if (keyforward) {currentcommand = 1;} //Set Direction Forward
      else if (keybackward) {currentcommand = 2;} // Set Direction Backward
      else {currentcommand = 5;} // Stop
      if ((currentcommand != oldcommand) || (commandcounter > 30)){ // Limit commands to only send on new input or 3 second interval.
          try{fetch(document.location.origin+'/control?var=car&val='+currentcommand);}catch(e){} // Send the command for controls. 
          oldcommand = currentcommand;
          commandcounter = 0;}
      else {commandcounter++;} //Increment non-send command counter.
      }, 100);    
</script>

</body>
</html>


)rawliteral";

static esp_err_t index_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

// Finally, if all is well with the camera, encoding, and all else, here it is, the actual camera server.
// If it works, use your new camera robot to grab a beer from the fridge using function Request.Fridge("beer","buschlite")
void startCameraServer()
{

    // Set LED to off
    ledcWrite(7, 0); // Turn off LED

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t status_uri = {
        .uri       = "/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };

   httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };
    
    Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }

    for(;;) {
      delay (500);
      if ((heartbeat+4000) < millis() ) { // Check if command signal has been sent within the last 4 seconds.
        Serial.printf("Stopping. Disconnected for: '%d'ms\n", millis() - heartbeat); // Notify and stop motors if no signal.
        actstate = stp;
        ledcWrite(3,lneutral);
        ledcWrite(4,rneutral);
      }
    }
}
