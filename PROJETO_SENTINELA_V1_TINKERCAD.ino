#include <Adafruit_LiquidCrystal.h>

const int PINO_PIR = 2;
const int PINO_BOTAO_MODO = 4;
const int PINO_LED_ALERTA = 5;
const int PINO_LED_LUZ = 6;
const int PINO_BOTAO_TELA = 7;
const int PINO_BUZZER = 8;
const int PINO_LED_ARMADO = 13;
const int PINO_LDR = A0;
const int PINO_POT = A2;

const unsigned long INTERVALO_PISCA = 300;
const unsigned long INTERVALO_LCD = 250;
const unsigned long TEMPO_DEBOUNCE = 200;
const unsigned long TEMPO_MENSAGEM = 1500;
const unsigned long TEMPO_SAIDA = 5000;
const unsigned long DURACAO_BIP = 80;
const int FREQUENCIA_BUZZER = 2000;
const int FREQUENCIA_BIP = 1000;

const int DESARMADO = 0;
const int ARMANDO = 1;
const int ARMADO = 2;
const int ALARME = 3;

const int TELA_ESTADO = 0;
const int TELA_SENSORES = 1;
const int TELA_HISTORICO = 2;
const int TELA_AJUDA = 3;
const int NUM_TELAS = 4;

// Textos do tutorial (maximo 16 caracteres por linha)
const int NUM_PASSOS = 8;
const char* tutorialL1[NUM_PASSOS] = {
  "== TUTORIAL ==", "Botao MODO:", "Botao TELA:", "Ao armar voce",
  "Se houver mov.:", "Serial: a=armar", "Potenciometro:", "Tudo pronto!"
};
const char* tutorialL2[NUM_PASSOS] = {
  "TELA=prox MODO=x", "arma / desarma", "troca de tela", "tem 5s p/ sair",
  "ALARME dispara", "d=desarm h=ajuda", "brilho do alerta", "TELA p/ comecar"
};

Adafruit_LiquidCrystal lcd(0);

volatile bool movimentoDetectado = false;

int estado = DESARMADO;
unsigned long inicioEstado = 0;
int tela = TELA_ESTADO;
int luz = 0;
int brilhoAlerta = 0;
bool piscaLigado = false;
int ultimoSegundo = -1;

bool emTutorial = false;
int passoTutorial = 0;

// Aviso temporario no display
const char* msgL1 = "";
const char* msgL2 = "";
unsigned long inicioMsg = 0;
bool msgAtiva = false;
bool redesenhar = true;

int totalDisparos = 0;
unsigned long ultimoDisparo = 0;

// Coluna atual do LCD, para completar a linha com espacos
int coluna = 0;

bool modoAnterior = false, telaAnterior = false;
unsigned long ultimoModo = 0, ultimoTela = 0, ultimoPisca = 0, ultimoLcd = 0;

// Interrupcao do PIR
void detectarMovimento() {
  movimentoDetectado = true;
}

const char* nomeEstado() {
  if (estado == ARMANDO) return "ARMANDO";
  if (estado == ARMADO) return "ARMADO";
  if (estado == ALARME) return "ALARME";
  return "DESARMADO";
}

int porcentagemAlerta() {
  return brilhoAlerta * 100 / 255;
}

// ===== Escrita no LCD sem usar String (economiza memoria de programa) =====
void inicioLinha(int linha) {
  lcd.setCursor(0, linha);
  coluna = 0;
}

void texto(const char* t) {
  lcd.print(t);
  coluna = coluna + strlen(t);
}

void numero(long n) {
  lcd.print(n);
  if (n < 0) { coluna = coluna + 1; n = -n; }
  do { coluna = coluna + 1; n = n / 10; } while (n > 0);
}

void fimLinha() {
  while (coluna < 16) {
    lcd.print(" ");
    coluna = coluna + 1;
  }
}

void escreverLinha(int linha, const char* t) {
  inicioLinha(linha);
  texto(t);
  fimLinha();
}

void escrever2(const char* l1, const char* l2) {
  escreverLinha(0, l1);
  escreverLinha(1, l2);
}

void mostrarMensagem(const char* l1, const char* l2) {
  msgL1 = l1;
  msgL2 = l2;
  inicioMsg = millis();
  msgAtiva = true;
  redesenhar = true;
}

void mudarEstado(int novo) {
  estado = novo;
  inicioEstado = millis();
  movimentoDetectado = false;
  piscaLigado = false;
  ultimoSegundo = -1;
  digitalWrite(PINO_LED_ARMADO, estado == ARMADO);
  noTone(PINO_BUZZER);
  analogWrite(PINO_LED_ALERTA, 0);
  lcd.setBacklight(HIGH);
  redesenhar = true;
  Serial.print("Estado: ");
  Serial.println(nomeEstado());
}

void armar(const char* origem) {
  if (estado == ALARME) {
    mostrarMensagem(origem, "Desarme antes");
  } else if (estado != DESARMADO) {
    mostrarMensagem(origem, "Ja esta armado");
  } else {
    mudarEstado(ARMANDO);
    mostrarMensagem(origem, "Armando...");
  }
}

void desarmar(const char* origem) {
  if (estado == DESARMADO) {
    mostrarMensagem(origem, "Ja desarmado");
  } else {
    mudarEstado(DESARMADO);
    mostrarMensagem(origem, "DESARMADO");
  }
}

void iniciarTutorial() {
  emTutorial = true;
  passoTutorial = 0;
  msgAtiva = false;
  redesenhar = true;
}

void encerrarTutorial() {
  emTutorial = false;
  tela = TELA_ESTADO;
  mostrarMensagem("Fim do tutorial", "t = rever");
}

// Botoes ligados no GND, usando pull-up interno
bool botaoApertado(int pino, bool &anterior, unsigned long &ultimoTempo) {
  bool apertado = digitalRead(pino) == LOW;
  bool novoAperto = apertado && !anterior && millis() - ultimoTempo > TEMPO_DEBOUNCE;
  if (novoAperto) ultimoTempo = millis();
  anterior = apertado;
  return novoAperto;
}

void lerBotoes() {
  bool apertouModo = botaoApertado(PINO_BOTAO_MODO, modoAnterior, ultimoModo);
  bool apertouTela = botaoApertado(PINO_BOTAO_TELA, telaAnterior, ultimoTela);

  if (emTutorial) {
    if (apertouModo) {
      encerrarTutorial();
    } else if (apertouTela) {
      passoTutorial = passoTutorial + 1;
      if (passoTutorial >= NUM_PASSOS) encerrarTutorial();
      else redesenhar = true;
    }
    return;
  }

  if (apertouModo) {
    if (estado == DESARMADO) armar("Botao MODO");
    else desarmar("Botao MODO");
  }
  if (apertouTela) {
    tela = (tela + 1) % NUM_TELAS;
    msgAtiva = false;
    redesenhar = true;
  }
}

void lerSensores() {
  luz = map(analogRead(PINO_LDR), 0, 1023, 0, 100);
  brilhoAlerta = map(analogRead(PINO_POT), 0, 1023, 0, 255);
}

void mostrarAjudaSerial() {
  Serial.println("a=armar d=desarmar s=status t=tutorial");
}

// a = armar, d = desarmar, s = status, t = tutorial, h = ajuda
void lerSerial() {
  if (!Serial.available()) return;
  char comando = Serial.read();

  // ignora Enter e espaco
  if (comando == 10 || comando == 13 || comando == 32) return;

  // maiuscula vira minuscula
  if (comando >= 65 && comando <= 90) comando = comando + 32;

  if (comando == 'a') {
    emTutorial = false;
    armar("Comando Serial");
  }
  else if (comando == 'd') {
    emTutorial = false;
    desarmar("Comando Serial");
  }
  else if (comando == 's') {
    emTutorial = false;
    Serial.println(nomeEstado());
    Serial.print("Luz %: ");
    Serial.println(luz);
    Serial.print("Disparos: ");
    Serial.println(totalDisparos);
    tela = TELA_SENSORES;
    mostrarMensagem("Comando Serial", "Status enviado");
  }
  else if (comando == 't') {
    if (estado == DESARMADO) iniciarTutorial();
    else mostrarMensagem("Comando Serial", "Desarme antes");
  }
  else if (comando == 'h') {
    emTutorial = false;
    mostrarAjudaSerial();
    tela = TELA_AJUDA;
    msgAtiva = false;
    redesenhar = true;
  }
  else {
    emTutorial = false;
    Serial.println("Invalido. h = ajuda");
    mostrarMensagem("Cmd invalido", "h = ajuda");
  }
}

void verificarMovimento() {
  if (estado == ARMADO && movimentoDetectado) {
    totalDisparos = totalDisparos + 1;
    ultimoDisparo = millis();
    Serial.println("ALERTA: movimento!");
    mudarEstado(ALARME);
  }
}

// Contagem antes de armar: bip a cada segundo e LED verde piscando
void atualizarArmando() {
  if (estado != ARMANDO) return;
  unsigned long decorrido = millis() - inicioEstado;

  if (decorrido >= TEMPO_SAIDA) {
    mudarEstado(ARMADO);
    mostrarMensagem("Sistema ARMADO", "Vigiando...");
    return;
  }

  int segundo = decorrido / 1000;
  if (segundo != ultimoSegundo) {
    ultimoSegundo = segundo;
    tone(PINO_BUZZER, FREQUENCIA_BIP, DURACAO_BIP);
    redesenhar = true;
  }

  int fase = (decorrido / 250) % 2;
  digitalWrite(PINO_LED_ARMADO, fase == 1 ? HIGH : LOW);
}

// Alarme: buzzer, LED vermelho e luz do display piscam juntos
void atualizarAlarme() {
  if (estado != ALARME || millis() - ultimoPisca < INTERVALO_PISCA) return;
  ultimoPisca = millis();
  piscaLigado = !piscaLigado;

  if (piscaLigado) {
    tone(PINO_BUZZER, FREQUENCIA_BUZZER);
    analogWrite(PINO_LED_ALERTA, brilhoAlerta);
    lcd.setBacklight(HIGH);
  } else {
    noTone(PINO_BUZZER);
    analogWrite(PINO_LED_ALERTA, 0);
    lcd.setBacklight(LOW);
  }
  redesenhar = true;
}

// Quanto mais escuro, mais forte o LED amarelo
void atualizarLedLuz() {
  analogWrite(PINO_LED_LUZ, map(luz, 0, 100, 255, 0));
}

// Ex.: Luz:45% =====
void linhaComBarra(int linha, const char* rotulo, int pct) {
  inicioLinha(linha);
  texto(rotulo);
  numero(pct);
  texto("% ");
  int blocos = map(pct, 0, 100, 0, 7);
  for (int i = 0; i < blocos; i++) texto("=");
  fimLinha();
}

void desenharAlarme() {
  if (piscaLigado) {
    escrever2("!! MOVIMENTO !!", "MODO p/desarmar");
  } else {
    escrever2("*** ALARME ***", "ou digite d");
  }
}

void desenharContagem() {
  long restante = (TEMPO_SAIDA - (millis() - inicioEstado) + 999) / 1000;
  inicioLinha(0);
  texto("ARMANDO em ");
  numero(restante);
  texto("s");
  fimLinha();
  escreverLinha(1, "Saia do local!");
}

void desenharTela() {
  if (tela == TELA_ESTADO) {
    inicioLinha(0);
    texto("Status:");
    texto(nomeEstado());
    fimLinha();
    if (estado == DESARMADO) escreverLinha(1, "MODO p/ armar");
    else escreverLinha(1, "MODO p/desarmar");
  }
  else if (tela == TELA_SENSORES) {
    linhaComBarra(0, "Luz:", luz);
    linhaComBarra(1, "Alr:", porcentagemAlerta());
  }
  else if (tela == TELA_HISTORICO) {
    inicioLinha(0);
    texto("Disparos: ");
    numero(totalDisparos);
    fimLinha();
    if (totalDisparos == 0) {
      escreverLinha(1, "Nenhum ainda");
    } else {
      long s = (millis() - ultimoDisparo) / 1000;
      inicioLinha(1);
      texto("Ultimo ha ");
      numero(s);
      texto("s");
      fimLinha();
    }
  }
  else {
    escrever2("Cmd: a d s t h", "TELA = prox tela");
  }
}

// Ordem: tutorial, alarme, aviso, contagem, tela escolhida
void atualizarDisplay() {
  if (!redesenhar && millis() - ultimoLcd < INTERVALO_LCD) return;
  redesenhar = false;
  ultimoLcd = millis();

  if (emTutorial) {
    escrever2(tutorialL1[passoTutorial], tutorialL2[passoTutorial]);
  }
  else if (estado == ALARME) {
    desenharAlarme();
  }
  else if (msgAtiva && millis() - inicioMsg < TEMPO_MENSAGEM) {
    escrever2(msgL1, msgL2);
  }
  else {
    msgAtiva = false;
    if (estado == ARMANDO) desenharContagem();
    else desenharTela();
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(PINO_PIR, INPUT);
  pinMode(PINO_BOTAO_MODO, INPUT_PULLUP);
  pinMode(PINO_BOTAO_TELA, INPUT_PULLUP);
  pinMode(PINO_LED_ALERTA, OUTPUT);
  pinMode(PINO_LED_LUZ, OUTPUT);
  pinMode(PINO_LED_ARMADO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PINO_PIR), detectarMovimento, RISING);

  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);

  mudarEstado(DESARMADO);
  mostrarAjudaSerial();
  iniciarTutorial();
}

void loop() {
  lerBotoes();
  lerSensores();
  lerSerial();
  verificarMovimento();
  atualizarArmando();
  atualizarAlarme();
  atualizarLedLuz();
  atualizarDisplay();
}
