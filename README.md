![UNIP KILLER](UNIP%20KILLER%20angular%20cybersecurity%20wordmark.png)

# UNIP KILLER - Phishing via Captive Portal (ESP32 CYD)

Projeto de laboratório feito na UNIP, pra mostrar na prática como funciona
um dos tipos mais comuns de phishing: a rede Wi-Fi falsa (Evil Twin /
captive portal). Roda numa placa ESP32 "Cheap Yellow Display".

**Não captura credencial de ninguém de verdade.** O SSID é
`REDE_INSEGURA_NAO_CONECTE` de propósito, mesmo numa demo ao vivo, ninguém
vai se conectar pensando que é uma rede de verdade. É pra mostrar a tela, não
pra enganar alguém.

## Como funciona

1. O ESP32 abre uma rede Wi-Fi aberta, a isca.
2. Um DNS interno responde qualquer domínio com o próprio IP do ESP32
   (catch-all), então qualquer tentativa de navegar cai direto na página do
   portal. É a mesma técnica que portais legítimos de aeroporto/hotel usam.
3. Um servidor web serve uma página de login parecida com uma de verdade.
4. Se alguém digitar usuário/senha, o ESP32 captura o texto antes de mostrar
   a tela de "confirmação".

O pulo do gato desse ataque: a vítima sabe que está digitando uma senha, só
não sabe que está entregando ela pro atacante, porque a página parece
legítima. É engenharia social pura, depende de a pessoa não checar a rede
ou a URL antes de digitar.

A credencial capturada fica só na RAM da placa (`lastUser`/`lastPass` em
`a.ino`), não grava em flash, EEPROM nem cartão, e some num reset ou queda
de energia. Só fica exposta também no monitor serial (via `Serial.printf`,
pra debug), então vale tirar o cabo USB durante uma demo se quiser ser
rigoroso.

## Estrutura do código

| Arquivo | O que tem |
|---|---|
| `a.ino` | Wi-Fi, DNS, rotas HTTP, captura, toque na tela, a lógica |
| `visual.ino` | Cores, telas do display, HTML/CSS das páginas web, a cara |
| `logo.h` | Bitmap da animação de abertura no display |
| `logo_unip.h` | PNG servido em `/logo.png` na página de login |

## Uso responsável

- Só em ambiente controlado (sala, laboratório, bancada própria), nunca em
  rede de terceiros ou espaço público sem autorização.
- Não troque o SSID por algo que imite uma rede real. A ideia é o contrário:
  deixar bem claro que é pra ninguém conectar.
- Não digite senha de verdade numa demo.

## Como se proteger desse tipo de ataque

- Prefira redes WPA2/WPA3-Enterprise em vez de rede aberta.
- Olhe o cadeado (TLS) antes de digitar qualquer senha.
- Desconfie de Wi-Fi aberto com nome de instituição conhecida.
- MFA resistente a phishing (chave FIDO2/passkey) não cai nessa, porque não
  tem senha pra roubar.
