#include <Wire.h>

#define OLED_ADDR   0x3C
#define DEBUG       1          // 1 = imprime boton y joystick en el Monitor Serie (115200)
#define TEST_MODE   0         // 1 = pantalla de diagnostico (valores de A0..A7), 0 = juego
#define JOY_X_CH    7          // canal ADC del eje X: 7 = A0, 6 = A1, 5 = A2, 4 = A3 ...
#define JOY_Y_CH    6          // canal ADC del eje Y: 6 = A1, 5 = A2, ...

#define SHIP_W      15
#define SHIP_H      9
#define ROCK_W      9
#define ROCK_H      9
#define NUM_ROCKS   3
#define PLAY_MIN_Y  10         // debajo del marcador

extern "C" {
  void system_init_assembly(void);
  void read_inputs_assembly(void);
  void update_physics_assembly(void);
  void render_buffer_assembly(void);

  extern uint8_t buffer_pantalla[];
  extern uint32_t estado_boton;
  extern uint32_t eje_x;
  extern uint32_t eje_y;
  extern uint32_t lectura_adc[8];
  extern int nave_x;
  extern int nave_y;
  extern int disparo_activo;
  extern int disparo_x;
  extern int disparo_y;
}

// ---------- Sprites (X = pixel encendido) ----------
const char* const sprite_nave[SHIP_H] = {
  "...XX..........",
  "...XXXX........",
  "X..XXXXXXX.....",
  "XXXXXXXXXXXXX..",
  "XXXXXX..XXXXXXX",
  "XXXXXXXXXXXXX..",
  "X..XXXXXXX.....",
  "...XXXX........",
  "...XX.........."
};

const char* const sprite_roca[ROCK_H] = {
  "XXXXXXXXX",
  "X.......X",
  "X.......X",
  "X..XXX..X",
  "X..XXX..X",
  "X..XXX..X",
  "X.......X",
  "X.......X",
  "XXXXXXXXX"
};

// ---------- Fuente 3x5 (cada fila son 3 bits, bit 2 = izquierda) ----------
const uint8_t fuente[][5] = {
  {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},   // 0-4
  {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1}, {7,5,7,5,7}, {7,5,7,1,7},   // 5-9
  {7,4,7,1,7},   // S (10)
  {7,4,4,4,7},   // C (11)
  {7,5,5,5,7},   // O (12)
  {6,5,6,5,5},   // R (13)
  {7,4,7,4,7},   // E (14)
  {4,4,4,4,7},   // L (15)
  {7,2,2,2,7},   // I (16)
  {5,5,5,5,2},   // V (17)
  {0,2,0,2,0},   // :  (18)
  {7,4,5,5,7},   // G (19)
  {7,5,7,5,5},   // A (20)
  {5,7,7,5,5}    // M (21)
};

int glyphIndex(char caracter) {
  if (caracter >= '0' && caracter <= '9') return caracter - '0';
  switch (caracter) {
    case 'S': return 10;
    case 'C': return 11;
    case 'O': return 12;
    case 'R': return 13;
    case 'E': return 14;
    case 'L': return 15;
    case 'I': return 16;
    case 'V': return 17;
    case ':': return 18;
    case 'G': return 19;
    case 'A': return 20;
    case 'M': return 21;
  }
  return -1;
}

// ---------- Estado del juego ----------
int roca_x[NUM_ROCKS];
int roca_y[NUM_ROCKS];
int puntuacion = 0;
int vidas = 3;
unsigned long fotograma = 0;

void resetRock(int indice_roca) {
  roca_x[indice_roca] = 128 + random(0, 60);
  roca_y[indice_roca] = random(PLAY_MIN_Y, 64 - ROCK_H + 1);
}

void setup() {
#if DEBUG
  Serial.begin(115200);
#endif
  Wire1.begin();
  Wire1.setClock(400000);
  delay(100);
  system_init_assembly();
  randomSeed(micros());

  sendCmd(0xAE);
  sendCmd(0xD5); sendCmd(0x80);
  sendCmd(0xA8); sendCmd(0x3F);
  sendCmd(0xD3); sendCmd(0x00);
  sendCmd(0x40);
  sendCmd(0x8D); sendCmd(0x14);
  sendCmd(0x20); sendCmd(0x00);   // direccionamiento horizontal
  sendCmd(0xA1);
  sendCmd(0xC8);
  sendCmd(0xDA); sendCmd(0x12);
  sendCmd(0x81); sendCmd(0xCF);
  sendCmd(0xD9); sendCmd(0xF1);
  sendCmd(0xDB); sendCmd(0x40);
  sendCmd(0xA4);
  sendCmd(0xA6);
  sendCmd(0xAF);

  for (int indice_roca = 0; indice_roca < NUM_ROCKS; indice_roca++) {
    resetRock(indice_roca);
    roca_x[indice_roca] = 128 + indice_roca * 45;
  }
}

void testScreen() {
  render_buffer_assembly();
  char buffer_texto[16];
  for (int indice_entrada = 0; indice_entrada < 8; indice_entrada++) { // A0 = canal 7 ... A7 = canal 0
    snprintf(buffer_texto, sizeof(buffer_texto), "A%d:%04d", indice_entrada, (int)lectura_adc[7 - indice_entrada]);
    drawText(0, 1 + indice_entrada * 7, buffer_texto);
  }
  if (estado_boton) {                           // cuadro lleno = boton presionado
    for (int coordenada_x = 90; coordenada_x < 106; coordenada_x++)
      for (int coordenada_y = 20; coordenada_y < 36; coordenada_y++) drawPixel(coordenada_x, coordenada_y, 1);
  } else {
    for (int coordenada_x = 90; coordenada_x < 106; coordenada_x++) { drawPixel(coordenada_x, 20, 1); drawPixel(coordenada_x, 35, 1); }
    for (int coordenada_y = 20; coordenada_y < 36; coordenada_y++)  { drawPixel(90, coordenada_y, 1); drawPixel(105, coordenada_y, 1); }
  }
  enviar_framebuffer_a_oled();
}

void loop() {
  read_inputs_assembly();
#if TEST_MODE
  testScreen();
  delay(50);
  return;
#endif
  eje_x = lectura_adc[JOY_X_CH];
  eje_y = lectura_adc[JOY_Y_CH];
  update_physics_assembly();

  int velocidad = 2 + puntuacion / 25;
  if (velocidad > 4) velocidad = 4;

  for (int indice_roca = 0; indice_roca < NUM_ROCKS; indice_roca++) {
    roca_x[indice_roca] -= velocidad;

    // El obstaculo salio por la izquierda
    if (roca_x[indice_roca] < -ROCK_W) {
      resetRock(indice_roca);
      puntuacion++;
    }

    // Disparo destruye obstaculo
    if (disparo_activo &&
        disparo_x + 3 >= roca_x[indice_roca] && disparo_x <= roca_x[indice_roca] + ROCK_W - 1 &&
        disparo_y >= roca_y[indice_roca] && disparo_y <= roca_y[indice_roca] + ROCK_H - 1) {
      disparo_activo = 0;
      resetRock(indice_roca);
      puntuacion += 5;
      continue;
    }

    // Obstaculo choca con la nave
    if (nave_x < roca_x[indice_roca] + ROCK_W && nave_x + SHIP_W > roca_x[indice_roca] &&
        nave_y < roca_y[indice_roca] + ROCK_H && nave_y + SHIP_H > roca_y[indice_roca]) {
      vidas--;
      resetRock(indice_roca);
      flashHit();                       // parpadeo de pantalla = golpe recibido
      if (vidas <= 0) {
        gameOver();
        puntuacion = 0;
        vidas = 3;
      }
    }
  }
  if (puntuacion > 9999) puntuacion = 9999;

  // ---------- Dibujo ----------
  render_buffer_assembly();                       // limpia el buffer (ensamblador)

  drawHUD();
  drawSprite(nave_x, nave_y, sprite_nave, SHIP_W, SHIP_H);

  if (disparo_activo) {
    for (int pixel_disparo = 0; pixel_disparo < 4; pixel_disparo++) drawPixel(disparo_x + pixel_disparo, disparo_y, 1);
  }

  for (int indice_roca = 0; indice_roca < NUM_ROCKS; indice_roca++) {
    drawSprite(roca_x[indice_roca], roca_y[indice_roca], sprite_roca, ROCK_W, ROCK_H);
  }

  enviar_framebuffer_a_oled();

#if DEBUG
  if (++fotograma % 5 == 0) {
    Serial.print("btn=");  Serial.print(estado_boton);
    Serial.print(" joyX="); Serial.print(eje_x);
    Serial.print(" joyY="); Serial.print(eje_y);
    Serial.print(" nave=("); Serial.print(nave_x);
    Serial.print(",");       Serial.print(nave_y);
    Serial.print(") vidas="); Serial.println(vidas);
  }
#endif
  delay(10);
}

// ---------- Funciones de dibujo ----------
void drawPixel(int coordenada_x, int coordenada_y, uint8_t color_pixel) {
  if (coordenada_x < 0 || coordenada_x >= 128 || coordenada_y < 0 || coordenada_y >= 64) return;
  uint16_t indice_buffer = coordenada_x + (coordenada_y / 8) * 128;
  if (color_pixel) buffer_pantalla[indice_buffer] |= (1 << (coordenada_y % 8));
  else             buffer_pantalla[indice_buffer] &= ~(1 << (coordenada_y % 8));
}

void drawSprite(int coordenada_x, int coordenada_y, const char* const* filas_sprite, int ancho, int alto) {
  for (int fila = 0; fila < alto; fila++) {
    for (int columna = 0; columna < ancho; columna++) {
      if (filas_sprite[fila][columna] == 'X') drawPixel(coordenada_x + columna, coordenada_y + fila, 1);
    }
  }
}

void drawChar(int coordenada_x, int coordenada_y, char caracter) {
  int indice_glyph = glyphIndex(caracter);
  if (indice_glyph < 0) return;
  for (int fila = 0; fila < 5; fila++) {
    for (int columna = 0; columna < 3; columna++) {
      if (fuente[indice_glyph][fila] & (4 >> columna)) drawPixel(coordenada_x + columna, coordenada_y + fila, 1);
    }
  }
}

void drawText(int coordenada_x, int coordenada_y, const char* texto) {
  while (*texto) {
    drawChar(coordenada_x, coordenada_y, *texto++);
    coordenada_x += 4;
  }
}

void drawHUD() {
  char buffer_texto[16];
  snprintf(buffer_texto, sizeof(buffer_texto), "SCORE:%04d", puntuacion);
  drawText(0, 1, buffer_texto);

  char texto_vidas[12] = "LIVES:";
  for (int indice_vida = 0; indice_vida < vidas && indice_vida < 3; indice_vida++) texto_vidas[6 + indice_vida] = 'I';
  texto_vidas[6 + (vidas < 3 ? vidas : 3)] = 0;
  drawText(90, 1, texto_vidas);

  for (int coordenada_x = 0; coordenada_x < 128; coordenada_x++) drawPixel(coordenada_x, 8, 1);   // linea separadora
}

// ---------- Efectos ----------
void flashHit() {
  sendCmd(0xA7);          // invertir pantalla
  delay(120);
  sendCmd(0xA6);          // normal
}

void gameOver() {
  render_buffer_assembly();
  drawText(46, 22, "GAME OVER");
  char buffer_texto[16];
  snprintf(buffer_texto, sizeof(buffer_texto), "SCORE:%04d", puntuacion);
  drawText(44, 34, buffer_texto);
  enviar_framebuffer_a_oled();
  delay(2000);
  disparo_activo = 0;
  for (int indice_roca = 0; indice_roca < NUM_ROCKS; indice_roca++) {
    resetRock(indice_roca);
    roca_x[indice_roca] = 128 + indice_roca * 45;
  }
}

// ---------- OLED ----------
void sendCmd(byte comando) {
  Wire1.beginTransmission(OLED_ADDR);
  Wire1.write(0x00);
  Wire1.write(comando);
  Wire1.endTransmission();
}

void enviar_framebuffer_a_oled() {
  sendCmd(0x21); sendCmd(0); sendCmd(127);   // columnas 0-127
  sendCmd(0x22); sendCmd(0); sendCmd(7);     // paginas 0-7

  for (uint16_t desplazamiento = 0; desplazamiento < 1024; desplazamiento += 16) {
    Wire1.beginTransmission(OLED_ADDR);
    Wire1.write(0x40);
    Wire1.write(&buffer_pantalla[desplazamiento], 16);
    Wire1.endTransmission();
  }
}
