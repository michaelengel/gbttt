#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char tiles[256][16];

void read_tiles(FILE *in) {
  /* We can have multiple tiles side-by-side on a line. This is the set we are
   * currently reading. */
  unsigned char reading[256];
  int n_reading, col, row, line = 1;

  enum {
    S_NORMAL, S_COMMENT, S_NAMES, S_NAME_CHAR, S_NAME_AFTERCHAR, S_NAME_NUM,
    S_TILES, S_TILE_IGNORE, S_TILE_PIX, S_TILE_HEADLINE, S_ERROR
  } state = S_NORMAL;

  int comment = 0;
  char c;

  // Clear the tiles.
  memset(tiles, 0, sizeof(tiles));

  while ((c = fgetc(in)) != EOF) {
    if (c == '\n') ++line;

    if (state == S_NORMAL) {
      if (c == '>') {
        n_reading = 0;
        state = S_NAMES;
      } else if (c == '#') {
        state = S_COMMENT;
      } else if (c == ' ' || c == '\n') {
        /* Ignore whitespace. */
      } else {
        state = S_ERROR;
      }
    } else if (state == S_COMMENT) {
      if (c == '\n') state = S_NORMAL;
    } else if (state == S_NAMES) {
      if (c == ' ') {
      } else if (c == '\'') {
        state = S_NAME_CHAR;
      } else if (c >= '0' && c <= '9') {
        state = S_NAME_NUM;
        ungetc(c, in);
      } else if (c == '\n') {
        state = S_TILE_HEADLINE;
      } else {
        state = S_ERROR;
      }
    } else if (state == S_NAME_NUM) {
      int i;
      fscanf(in, "%i", &i);
      reading[n_reading++] = i;
    } else if (state == S_NAME_CHAR) {
      reading[n_reading++] = c;
      state = S_NAME_AFTERCHAR;
    } else if (state == S_NAME_AFTERCHAR) {
      if (c == '\'') state = S_NAMES;
      else state = S_ERROR;
    } else if (state == S_TILE_HEADLINE) {
      row = -1;
      if (c == '\n') state = S_TILES;
    } else if (state == S_TILES) {
      row++;
      if (c >= '0' && c <= '7') {
        col = 0;
        state = S_TILE_IGNORE;
      } else {
        state = S_ERROR;
      }
    } else if (state == S_TILE_IGNORE) {
      if (c == '\n') {
        if (row == 7) state = S_NORMAL;
        else state = S_TILES;
      } else {
        state = S_TILE_PIX;
      }
    } else if (state == S_TILE_PIX) {
      if (c == ' ' || c == '0') {
        /* Do nothing; already cleared. */
      } else if (c == '-' || c == '1') {
        tiles[reading[col/8]][row*2 + 0] |= (0x80>>(col%8));
      } else if (c == '+' || c == '2') {
        tiles[reading[col/8]][row*2 + 1] |= (0x80>>(col%8));
      } else if (c == 'X' || c == '*' || c == '3') {
        tiles[reading[col/8]][row*2 + 0] |= (0x80>>(col%8));
        tiles[reading[col/8]][row*2 + 1] |= (0x80>>(col%8));
      } else if (c == '\n') {
        if (row == 7) state = S_NORMAL;
        else state = S_TILES;
      } else {
        state = S_ERROR;
      }
      col++;
      state = S_TILE_IGNORE;
    } else if (state == S_ERROR) {
      fprintf(stderr, "Parsing error on line %d.\n", line);
      exit(1);
    }
  }
}

void write_tiles(FILE *out, const char *name) {
  int i, j;

  fprintf(out, "const unsigned char %s[256][16] = {\n", name);

  for (i = 0; i < 256; ++i) {
    fputs("  {", out);
    for (j = 0; j < 16; ++j) {
      fprintf(out, "0x%02x", tiles[i][j]);
      if (j != 15) {
        fputs(", ", out);
      } else {
        fputc('}', out);
        if (i == 255) fputc(' ', out); else fputc(',', out);
      }
    }
    fprintf(out, " /* 0x%02x */\n", i);
  }

  fputs("};\n", out);
}

int main(int argc, char **argv) {
  FILE *f_in = argc > 1 ? fopen(argv[1], "r") : stdin;

  if (!f_in) {
    fputs("Could not open input file.\n", stderr);
    return 1;
  }
  
  read_tiles(f_in);
  fclose(f_in);

  FILE *f_out = argc > 2 ? fopen(argv[2], "w") : stdout;

  if (!f_out) {
    fputs("Could not open input file.\n", stderr);
    return 1;
  }
  
  write_tiles(f_out, argc > 3 ? argv[3] : "tiles");
  fclose(f_out);

  return 0;
}
