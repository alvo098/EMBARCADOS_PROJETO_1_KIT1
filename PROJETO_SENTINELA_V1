#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== Pinos =====
const int PINO_BOTAO_MODO = 4;
const int PINO_LED_ALERTA = 5;
const int PINO_LED_LUZ = 6;
const int PINO_BOTAO_TELA = 7;
const int PINO_BUZZER = 8;
const int PINO_TRIG = 9;      // sensor ultrassonico HC-SR04
const int PINO_ECHO = 10;     // sensor ultrassonico HC-SR04
const int PINO_LED_ARMADO = 13;
const int PINO_LDR = A0;
const int PINO_POT = A2;

// ===== Tempos =====
const unsigned long INTERVALO_PISCA = 300;
const unsigned long INTERVALO_LCD = 250;
const unsigned long TEMPO_DEBOUNCE = 200;
const unsigned long TEMPO_MENSAGEM = 1500;
const unsigned long TEMPO_SAIDA = 5000;
const unsigned long DURACAO_BIP = 80;
const int FREQUENCIA_BUZZER = 2000;
const int FREQUENCIA_BIP = 1000;

// ===== Ultrassonico =====
const unsigned long INTERVALO_MEDIDA = 100;  // mede 10 vezes por segundo
const unsigned long TEMPO_CALIBRACAO = 1000; // ao armar, mede o ambiente por 1s
const int DIST_MAX = 400;                    // cm (sem eco = longe)
const int LIMIAR = 20;                       // dispara se ficar 20cm mais perto que o normal
const int LEITURAS_CONFIRMA = 3;             // precisa de 3 leituras seguidas (0,3s)

// ===== Estados =====
const int DESARMADO = 0;
const int ARMANDO = 1;
const int ARMADO = 2;
const int ALARME = 3;

// ===== Telas =====
const int TELA_ESTADO = 0;
const int TELA_SENSORES = 1;
const int TELA_DISTANCIA = 2;
const int TELA_HISTORICO = 3;
const int TELA_AJUDA = 4;
const int NUM_TELAS = 5;

// Textos do tutorial (maximo 16 caracteres por linha)
const int NUM_PASSOS = 8;
const char* tutorialL1[NUM_PASSOS] = {
  "== TUTORIAL ==", "Botao MODO:", "Botao TELA:", "Ao armar voce",
  "Se algo passar", "Serial: a=armar", "Potenciometro:", "Tudo pronto!"
};
const char* tutorialL2[NUM_PASSOS] = {
  "TELA=prox MODO=x", "arma / desarma", "troca de tela", "tem 5s p/ sair",
  "ALARME dispara", "d=desarm h=ajuda", "brilho do alerta", "TELA p/ comecar"
};

LiquidCrystal_I2C lcd(0x27, 16, 2);

int estado = DESARMADO;
unsigned long inicioEstado = 0;
int tela = TELA_ESTADO;
int luz = 0;
int brilhoAlerta = 0;
bool piscaLigado = false;
int ultimoSegundo = -1;

// Ultrassonico
int distancia = DIST_MAX;
int referencia = 0;
bool calibrando = false;
long somaCalibracao = 0;
int amostrasCalibracao = 0;
int leiturasPerto = 0;
unsigned long ultimaMedida = 0;

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
  piscaLigado = false;
  ultimoSegundo = -1;

  // Ao armar, comeca a medir a distancia "normal" do ambiente
  if (estado == ARMADO) {
    calibrando = true;
    somaCalibracao = 0;
    amostrasCalibracao = 0;
    leiturasPerto = 0;
  }

  digitalWrite(PINO_LED_ARMADO, estado == ARMADO);
  noTone(PINO_BUZZER);
  analogWrite(PINO_LED_ALERTA, 0);
  lcd.backlight();
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

// ===== Ultrassonico =====
// Retorna a distancia em cm. Sem eco (nada na frente) conta como DIST_MAX.
int medirDistancia() {
  digitalWrite(PINO_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);

  unsigned long duracao = pulseIn(PINO_ECHO, HIGH, 25000UL); // espera ate 25 ms
  if (duracao == 0) return DIST_MAX;

  int cm = duracao / 58;
  if (cm > DIST_MAX) cm = DIST_MAX;
  return cm;
}

void disparar() {
  totalDisparos = totalDisparos + 1;
  ultimoDisparo = millis();
  Serial.print("ALERTA: algo a ");
  Serial.print(distancia);
  Serial.println(" cm!");
  mudarEstado(ALARME);
}

// Mede a distancia, calibra ao armar e dispara se algo se aproximar
void verificarDistancia() {
  if (millis() - ultimaMedida < INTERVALO_MEDIDA) return;
  ultimaMedida = millis();

  distancia = medirDistancia();
  if (tela == TELA_DISTANCIA) redesenhar = true;

  if (estado != ARMADO) return;

  // 1o segundo armado: aprende a distancia normal do ambiente
  if (calibrando) {
    somaCalibracao = somaCalibracao + distancia;
    amostrasCalibracao = amostrasCalibracao + 1;

    if (millis() - inicioEstado >= TEMPO_CALIBRACAO) {
      referencia = somaCalibracao / amostrasCalibracao;
      calibrando = false;
      Serial.print("Referencia: ");
      Serial.print(referencia);
      Serial.println(" cm. Vigiando...");
      if (referencia <= LIMIAR + 5) {
        Serial.println("Aviso: objeto muito perto do sensor!");
      }
    }
    return;
  }

  // Algo ficou bem mais perto que o normal?
  if (distancia < referencia - LIMIAR) {
    leiturasPerto = leiturasPerto + 1;
  } else {
    leiturasPerto = 0;
  }

  if (leiturasPerto >= LEITURAS_CONFIRMA) {
    disparar();
  }
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
    Serial.print("Alerta %: ");
    Serial.println(porcentagemAlerta());
    Serial.print("Distancia cm: ");
    Serial.println(distancia);
    if (estado == ARMADO && !calibrando) {
      Serial.print("Referencia cm: ");
      Serial.println(referencia);
    }
    Serial.print("Disparos: ");
    Serial.println(totalDisparos);
    tela = TELA_DISTANCIA;
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

// Contagem antes de armar: bip a cada segundo e LED verde piscando
void atualizarArmando() {
  if (estado != ARMANDO) return;
  unsigned long decorrido = millis() - inicioEstado;

  if (decorrido >= TEMPO_SAIDA) {
    mudarEstado(ARMADO);
    mostrarMensagem("Sistema ARMADO", "Medindo local..");
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
    lcd.backlight();
  } else {
    noTone(PINO_BUZZER);
    analogWrite(PINO_LED_ALERTA, 0);
    lcd.noBacklight();
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
    escrever2("!! INTRUSO !!", "MODO p/desarmar");
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
  else if (tela == TELA_DISTANCIA) {
    inicioLinha(0);
    texto("Dist: ");
    if (distancia >= DIST_MAX) texto("livre");
    else { numero(distancia); texto("cm"); }
    fimLinha();

    inicioLinha(1);
    texto("Ref: ");
    if (estado == ARMADO && !calibrando) {
      numero(referencia);
      texto("cm");
    } else if (estado == ARMADO) {
      texto("medindo");
    } else {
      texto("--");
    }
    fimLinha();
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

  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  pinMode(PINO_BOTAO_MODO, INPUT_PULLUP);
  pinMode(PINO_BOTAO_TELA, INPUT_PULLUP);
  pinMode(PINO_LED_ALERTA, OUTPUT);
  pinMode(PINO_LED_LUZ, OUTPUT);
  pinMode(PINO_LED_ARMADO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);
  digitalWrite(PINO_TRIG, LOW);

  lcd.init();
  lcd.backlight();

  mudarEstado(DESARMADO);
  mostrarAjudaSerial();
  iniciarTutorial();
}

void loop() {
  lerBotoes();
  lerSensores();
  lerSerial();
  verificarDistancia();
  atualizarArmando();
  atualizarAlarme();
  atualizarLedLuz();
  atualizarDisplay();
}
