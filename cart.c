// Tic-Tac-Toe fuer Gameboy
// mit dem sdcc-Compiler

typedef unsigned char tile_t[16];
typedef unsigned char map_row_t[32];

// Struktur fuer Sprite-Daten (4 Byte)
typedef struct {
  unsigned char y, x, tile, flags;
} sprite_t;

// Konstanten fuer die GB-Hardware

// Paletten
#define PAL_LIGHT 0x50
#define PAL_NORMAL 0xe4
#define PAL_DARK 0xfa
#define PAL_BLACK 0xff

// Register fuer Background- und Window-Position
#define BGPOSY ((unsigned char*)0xff42)
#define BGPOSX ((unsigned char*)0xff43)
#define WNDPOSY ((unsigned char*)0xff4a)
#define WNDPOSX ((unsigned char*)0xff4b)

// Register fuer buttons
#define BUTTONS ((unsigned char*)0xff00)

// Paletten, LCD-Controller, Maps, Tiles, Sprites...
// (siehe http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf)
#define BGPAL ((unsigned char*)0xff47)
#define SPRITEPAL0 ((unsigned char*)0xff48)
#define SPRITEPAL1 ((unsigned char*)0xff49)
#define LCDCONT ((volatile unsigned char*)0xff40)
#define LCDSTAT ((volatile unsigned char*)0xff41)
#define LO_MAP ((map_row_t*)0x9800)
#define HI_MAP ((map_row_t*)0x9c00)
#define LO_TILES ((tile_t*)0x8000)
#define HI_TILES ((tile_t*)0x8c00)
#define SPRITES ((sprite_t *)0xfe00)

// Interrupt-Enable-Register
#define IRQEN ((unsigned char *)0xffff)

// Bits im LCD-Controller-Register
enum LCDCONT_BIT {
  LCD_ENABLE = 0x80,
  LCD_WSEL = 0x40,
  LCD_WINDOW = 0x20,
  LCD_BGTILES = 0x10,
  LCD_BGSEL = 0x08,
  LCD_TRWIN = 0x02,
  LCD_BG = 0x01
};

// Hilfsfunktionen fuer das Display
void disable_lcd(void) { *LCDCONT &= ~LCD_ENABLE; }
void enable_lcd(void) { *LCDCONT |= LCD_ENABLE; }

void disable_bg(void) { *LCDCONT &= ~LCD_BG; }
void enable_bg(void) { *LCDCONT |= LCD_BG; }

void disable_window(void) { *LCDCONT &= ~LCD_WINDOW; }
void enable_window(void) { *LCDCONT |= LCD_WINDOW; }

void disable_window_transparency(void) { *LCDCONT |= LCD_TRWIN; }
void enable_window_transparency(void) { *LCDCONT &= ~LCD_TRWIN; }

void set_window_map_lo(void) { *LCDCONT &= ~LCD_WSEL; }
void set_window_map_hi(void) { *LCDCONT |= LCD_WSEL; }

void set_bg_map_lo(void) { *LCDCONT &= ~LCD_BGSEL; }
void set_bg_map_hi(void) { *LCDCONT |= LCD_BGSEL; }

void set_tiles_lo(void) { *LCDCONT |= LCD_BGTILES; }
void set_tiles_hi(void) { *LCDCONT &= ~LCD_BGTILES; }

void set_window_map(int i) {
  if (i) set_window_map_hi(); else set_window_map_lo();
}

void set_bg_map(int i) { if (i) set_bg_map_hi(); else set_bg_map_lo(); }

void set_tiles(int i) { if (i) set_tiles_hi(); else set_tiles_lo(); }

void set_bgpal(unsigned char c) { *BGPAL = c; }
void set_spritepal0(unsigned char c) { *SPRITEPAL0 = c; }
void set_spritepal1(unsigned char c) { *SPRITEPAL1 = c; }

void set_window_pos(int x, int y) {
 *WNDPOSX = x;
 *WNDPOSY = y;
}

void set_bg_pos(int x, int y) {
  *BGPOSX = x;
  *BGPOSY = y;
}

void wait_for_display() { while ((*LCDSTAT & 2) != 0); }
void wait_for_vblank() { while ((*LCDSTAT & 3) != 1); }
void wait_for_hblank() { while ((*LCDSTAT & 3) != 0); }
void wait_for_vblank_end() { while ((*LCDSTAT & 3) == 1); }
void wait_for_hblank_end() { while ((*LCDSTAT & 3) == 0); }

void set_tile_on_vblank(int x, int y, unsigned char t) {
  wait_for_vblank();
  LO_MAP[y][x] = t;
}

// Hilfsfunktionen fuer log2, Modulo, Integer-Division und -Multiplikation
int log2(int s) {
  int i = 0;

  while (s) {
    s = s >> 1;
    if (s) i = i + 1;
  }

  return i;
}

int _div(int a, int b, int *r) {
  int i, q;

  int la = log2(a), lb = log2(b), ldiff = la - lb;

  for (i = 16 - ldiff, q = 0; i <= 16; i++) {
    int x = b << (16 - i), qx = 1 << (16 - i);

    if (a >= x) {
      q = q + qx;
      a = a - x;
    }
  }

  if (r != (int *)0) *r = a; // Rest

  return q; // Quotient
}

int mod(int a, int b) {
  int r;
  div(a, b);
  return r;
}

int div(int a, int b) {
  int r;
  return _div(a, b, &r);
}

int mul(int a, int b) {
  int p = 0;

  while (a) {
    if (a & 1) p = p + b;
    a = a >> 1;
    b = b << 1;
  }

  return p;
}

// Aus tile.til generiertes Array (in tiles.inc) importieren
#include "tiles.inc"

// Tile mit Nummer id an Pos x/y setzen
void set_tile(int id, int x, int y) {
  set_tile_on_vblank(x, y, id);
}

// Scrolling wird hier nicht genutzt
int scroll_x, scroll_y;
void set_scroll(int x, int y) {
  scroll_x = x;
  scroll_y = y;
  wait_for_vblank();
  set_bg_pos(x << 3, y << 3);
}

// Hintergrund loeschen
void clear(void) {
  int i, j;
  for (i = 0; i < 32; i++)
    for (j = 0; j < 32; j++)
      set_tile(' ', i, j);
}

int char_pos_x, char_pos_y, scrolling;

// Tile an aktuelle Position (in char_pos_x/char_pos_y) ausgeben
void gbputc(char c) {
  int y_scroll = 0;
  if (c == '\n') { char_pos_x=0; char_pos_y++; y_scroll = 1;}
  else {
    if (char_pos_y == 32) char_pos_y = 0;
    set_tile(c, char_pos_x, char_pos_y);
    if (!y_scroll) char_pos_x++;
    if (char_pos_x == 20) { char_pos_y++; char_pos_x = 0; y_scroll = 1; }
    if (char_pos_y == 18) scrolling = 1;
  }

  if (scrolling && y_scroll) {
    int i;
    set_scroll(0, (scroll_y + 1)&0x1f);
    for (i = 0; i < 32; i++)
      set_tile(' ', i, char_pos_y);
  }
}

// Zeichen (Tile mit Nummer c) an Pos. x/y ausgeben
void gbputcxy(int x, int y, char c) {
  int tmpx = char_pos_x;
  int tmpy = char_pos_y;

  char_pos_x = x;
  char_pos_y = y;
  gbputc(c);

  char_pos_x = tmpx;
  char_pos_y = tmpy;
}

// Eine einfache "putstring"-Funktion
void gbputs(const char *s) {
  while (*s) gbputc(*(s++));
}

void main(void);

// Grundlegende Initialisierung des Gameboy
void init(void) {
  int i, j;

  // LCD deaktivieren (dann Zugriff auf Sprites ohne Warten auf VBlank moeglich)
  *LCDCONT = 0;
  disable_lcd();

  // Wir nutzen kein Window
  disable_window();

  // Der Hintergrund ist ab Position (0,0) gemappt
  set_bg_pos(0, 0);

  // Interrupts abschalten
  *IRQEN = 0;

  // Alle Sprites auf Position (0,0) setzen -> links oben ausserhalb des Bildschirms
  for (i = 0; i < 40; i++)
    (SPRITES + i)->x = (SPRITES + i)->y = 0;
  
  // Tile-Daten aus "tiles"-Array (aus tiles.til generiert) in Tile-Speicher kopieren
  for (i = 0; i < 256; i++)
    for (j = 0; j < 16; j++)
      LO_TILES[i][j] = tiles[i][j];

  // Background-Map und Tile-Map 0 nutzen
  set_bg_map(0);
  set_tiles(0);

  // Normale Background-Palette
  set_bgpal(PAL_NORMAL);

  // Background aktivieren, dann LCD anschalten
  enable_bg();
  enable_lcd();

  // ...los geht's!
  main();

  for (;;);
}
 
// Das Spielfeld fuer tic-tac-toe ist einfach eine
// 3x3-Matrix mit Inhalt:
// "0" (kein Stein), 
// "1" (Stein von Spieler 1, "X"),
// "2" (Stein von Spieler 2, "O")
int field[3][3];
int xp, yp;

// Feststellen, ob einer der Spieler gewonnen hat
// Rueckgabe: 0 (kein Gewinner), 1 oder 2 (Spieler 1 oder 2 gewonnen)
int check_win(void) {
  int w = 0;
  int i;
  // Pruefen, ob Zeile mit drei identischen Steinen ("X" oder "O") existiert
  for (i=0; i<3; i++) {
    if ((field[0][i] == field[1][i]) && (field[1][i] == field[2][i])) {
      w = field[0][i];
      // Wenn w = 0, dann haben wir eine leere Zeile -> kein Gewinner
      if (w>0) return w;
    }
  }

  // Pruefen, ob Spalte mit drei identischen Steinen ("X" oder "O") existiert
  for (i=0; i<3; i++) {
    if ((field[i][0] == field[i][1]) && (field[i][1] == field[i][2])) {
      w = field[i][0];
      // Wenn w = 0, dann haben wir eine leere Zeile -> kein Gewinner
      if (w>0) return w;
    }
  }

  // Diagonalen ((0,0),(1,1),(2,2)) und ((2,0),(1,1),(0,2)) pruefen
  if ((field[0][0] == field[1][1]) && (field[1][1] == field[2][2])) {
    w = field[0][0];
    if (w>0) return w;
  }
  if ((field[0][2] == field[1][1]) && (field[1][1] == field[2][0])) {
    w = field[0][2];
    if (w>0) return w;
  }

  return w;
}

// Feststellen, ob Spielfeld voll ist
// Ueberprueft, ob alle Felder (x=0..2, y=0..2) einen Wert != 0 haben,
// also ein Stein von Spieler 1 oder 2 gesetzt ist
int full(void) {
  int i,j;
  int f = 1;
  for (i=0; i<3; i++) {
    for (j=0; j<3; j++) {
      if (field[i][j] == 0) f = 0;
    } 
  } 
  return f;
}

// Funktionen, um "X", "O" oder " " an Zeichenposition (x,y)
// auf dem Bildschirm zu setzen
void setx(int x, int y) {
  gbputcxy(6 + x * 4, 4 + y * 4, 'X');
}

void seto(int x, int y) {
  gbputcxy(6 + x * 4, 4 + y * 4, 'O');
}

void clearxy(int x, int y) {
  gbputcxy(6 + x * 4, 4 + y * 4, ' ');
}

void main(void) {
  clear();
  set_scroll(0, 0);
  int x = 0, y = 0;
  int end;

  // Das Spiel endet nie...
  while (1) {
    end = 0;
    char_pos_x = char_pos_y = scrolling = 0;
  
    int player = 1;
    int i, j;

    // Interne Darstellung des Spielfelds initialisieren
    // (3x3)-Matrix: 0 = unbelegt, 1 = Spieler 1 "X", 2 = Spieler 2 "O"
    for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
        field[i][j] = 0;
      }
    }
  
    // Ausgabe ab Zeichenposition Spalte 0, Zeile 3
    // Eine Zeichenposition ist 8x8 Pixel gross
    char_pos_x = 0; char_pos_y = 3;

    // Hier wird ein geschicktes Mapping verwendet:
    // Der ASCII-Code der ausgegebenen Zeichen wird in die 
    // Hintergrundbildschirm Tilemap geschrieben.
    // Entsprechend wird an der jeweiligen Stelle die Tile
    // mit dem ASCII-Code dargestellt.
    // Die Tiles sind in der Datei "tiles.til" definiert,
    // diese wird beim Bauen (mit make) automatisch in die
    // notwendigen Hexdaten fuer den C-Compiler uebersetzt.

    // Leerzeichen " " ist Tile 0x20, das keine Pixel gesetzt hat
    // Die Tiles "|" (0x7c), "-" (0x2d) und "+" (0x2b) sind so
    // gestaltet, dass die Darstellung auf dem Bildschirm dem
    // Aussehen des jeweiligen Zeichens entspricht.

    // Mit "\n" wird die Ausgabeposition auf Spalte 0, naechste Zeile
    // gesetzt.

    gbputs("        |   |   \n");
    gbputs("        |   |   \n");
    gbputs("        |   |   \n");
    gbputs("     ---+---+---\n");
    gbputs("        |   |   \n");
    gbputs("        |   |   \n");
    gbputs("        |   |   \n");
    gbputs("     ---+---+---\n");
    gbputs("        |   |   \n");
    gbputs("        |   |   \n");
    gbputs("        |   |   \n");
  
    // Der Sprite-Speicher ist von der CPU aus nur zuverlaessig in der
    // vertikalen Austastluecke des Videosignals (VBlank) beschreibbar.
    // In der restlichen Zeit hat der LCD-Controller Zugriff auf den
    // Sprite-Speicher, um die jeweiligen Displaydaten darzustellen.

    // Daher warten wir hier erst darauf, dass eine Darstellung stattfindet
    wait_for_display();
    // ...und dann der Uebergang zu VBlank. Damit stellen wir sicher, 
    // dass der folgende Code direkt _zu Beginn_ der VBlank-Periode
    // ausgefuehrt wird. Damit wird sichergestellt, dass genug Zeit 
    // fuer den Zugriff auf den Sprite-Speicher zur Verfuegung steht.
    wait_for_vblank();
  
    // Bit 2 von LCDCONT aktiviert die Sprite-Anzeige
    *LCDCONT = *LCDCONT | 0x2;
    // Dies setzt die Farbpalette fuer die Sprites
    *SPRITEPAL0 = 0xE2; // palette
  
    // Initialposition fuer das Sprite
    y = 70; x = 80;
  
    // Wir setzen hier fuer den Cursor (Sprite #0) die vier benoetigten Parameter:
    (SPRITES+0)->y = y;        // y-Position (in Pixeln, 8 Pixel vert Offset)
    (SPRITES+0)->x = x;        // x-Position (in Pixeln, 8 Pixel horiz Offset)
    (SPRITES+0)->tile = 'Q';   // Tile des Cursor-Sprite (0x51) - Bitmap fuer den "X"-Cursor
    (SPRITES+0)->flags = 0x00; // Parameter fuer das Sprite
  
    // Schleife, bis wir feststellen, dass das Spiel beendet ist
    while (end==0) {

      // Sprite-Speicher beschreiben, also warten...
      wait_for_display();
      wait_for_vblank();

      // Cursorform (Sprite #0) auf die Tile fuer den aktuellen Spieler setzen
      // Spieler 1 hat "X" (Tile 0x51 = ASCII-Code von 'Q'), 
      // Spieler 2 hat "O" (Tile 0x52 = ASCII-Code von 'R')
      (SPRITES+0)->tile = player+'P';  // Tricky: 'P'+1 = 'Q', 'P'+2 = 'R'
      gbputcxy(10, 0, 0x30+player);
  
      // Abfrage der Buttons des GB
      // Wir wollen hier den Cursor mit dem direction pad (hoch, runter, links, rechts)
      // bewegen.

      // Die Buttons im GB sind nicht alle direkt abfragbar, vielmehr muss vorher 
      // konfiguriert werden, welche Gruppe Buttons (direction oder action) abgefragt
      // werden soll. Achtung: Abfrage ist "active low", also ein 0-Bit="gedrueckt"

      // Konfiguration der Button-Matrix, Bits im Register "BUTTONS" (0xff00):

      //       Bit5        Bit6 (Bits 5 und 6 sind Ausgabe)
      //        |           |
      // Bit0 --O-rechts----O- A
      //        |           |
      // Bit1 --O-links-----O- B
      //        |           |
      // Bit2 --O-hoch------O- Select
      //        |           |
      // Bit3 --O-runter----O- Start
      //
      // (Bits 0-3 sind Eingabe)


      // Direction buttons waehlen, indem Bit 5 auf "0" gesetzt wird
      *BUTTONS = ~0x10; // Bit 5 auf 1 setzen und invertieren!

      // jetzt die Buttons einlesen, invertieren ("~") und nur untere 4 ("& 0xf") betrachten
      switch (~*BUTTONS & 0xf) {
        case 1<<3: // runter ist bit 3...
          y++; if (y>144+8) y = 0; // dann y-Pos. des Cursor-Sprites erhoehen, wrap wenn unten
          break;
        case 1<<2: // hoch (bit 2)
          y--; if (y<0) y = 144+8; // y-Pos. des Cursor-Sprites erniedrigen, wrap wenn oben
          break;
        case 1<<1: // links (bit 1)
          x--; if (x<0) x = 172+8; // ...entsprechend fuer die x-Position
          break;
        case 1<<0: // rechts (bit 0)
          x++; if (x>172+8) x = 0;
          break;
      }

      // Position des Cursor-Sprites aktualisieren
      wait_for_display();
      wait_for_vblank();
      (SPRITES+0)->y = y;
      (SPRITES+0)->x = x;

      // Action buttons selektieren (bit 6 auf "0")
      *BUTTONS = ~0x20;

      // Hier fragen wir nur den "A"-Button ab, andere werden ignoriert
      if ((~*BUTTONS & 0xf) == 0x01) {
          xp = -1; yp = -1;

#if 0
          // Diese Funktion prueft die x/y-Koordinaten explizit
          if ((x >  45) && (x <  72)) xp = 0;
          if ((x >  74) && (x < 103)) xp = 1;
          if ((x > 105) && (x < 132)) xp = 2;
          if ((y >  37) && (y <  63)) yp = 0;
          if ((y >  65) && (y <  95)) yp = 1;
          if ((y >  97) && (y < 123)) yp = 2;
#endif

          // Hier wird Division verwendet, um die Pixelkoordinaten des Cursor-Sprites
          // in Feldkoordinaten (x und y von 0-2) umzurechnen
          // Die GB-CPU kann nicht dividieren, daher Hilfsfunktion div verwenden
          xp = _div(x-45, 28, 0);
          yp = _div(y-37, 28, 0);

          // Wenn der Cursor in einem Feld stand und Button "A" gedrueckt...
          if ((xp >= 0) && (xp < 3) && (yp >= 0) && (yp < 3)) {
            // und das entsprechende Feld noch unbesetzt ist...
            if (field[xp][yp] == 0) {
              // Dann besetzen ("X" fuer Spieler 1, "O" fuer Spieler 2)
              // setx setzt die Tile "X" oder "O" ins jeweilige Feld auf den Bildschirm
              // In field wird der interne Zustand des Spielfelds aktualisiert
              // Danach ist der jeweils andere Spieler dran
              if (player == 1) { setx(xp, yp); field[xp][yp] = 1; player = 2; } 
              else if (player == 2) { seto(xp, yp); field[xp][yp] = 2; player = 1; }
            }
          }
      }
  
      // Pruefe nach jedem Zug, ob Spieler 1 oder 2 gewonnen hat
      // und setze entsprechenden Text links oben auf den Bildschirm
      // Setze dann Flag zum Beenden der Schleife
      if (check_win() == 1) {
        gbputcxy(0, 0, '@');
        gbputcxy(1, 0, '1');
        end = 1;
      }
      else if (check_win() == 2) {
        gbputcxy(0, 0, '@');
        gbputcxy(1, 0, '2');
        end = 1;
      }
      // Wenn bisher kein Spieler gewonnen hat, kann das Spiel auch
      // unentschieden ausgegangen sein - das Spielfeld ist voll
      // (kein Zug mehr moeglich), aber keine Dreierreihe (horizontal,
      // vertikal oder diagonal)
      else if (full()) {
        gbputcxy(0, 0, '@');
        gbputcxy(1, 0, '0');
        end = 1;
      }
    }

    // Wir kommen hier an, wenn ein Spieler gewonnen hat oder das Spiel
    // unentschieden ausgegangen ist

    // Sprites deaktivieren, kein Stein mehr setzbar
    *LCDCONT &= ~0x2; 

    // Anzeige fuer den aktuellen Spieler ausblenden
    gbputcxy(10, 0, 0x00);

    wait_for_display();
    wait_for_vblank();
  
    // Action-Buttons abfragen
    *BUTTONS = 0xdf;

    // Warte auf START-Button, bevor neues Spiel gestartet wird
    while ((~*BUTTONS & 0xf) != 0x08);
    gbputcxy(0, 0, ' ');
    gbputcxy(1, 0, ' ');
  }
}
