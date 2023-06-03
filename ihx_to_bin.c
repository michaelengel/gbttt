#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 0x10000
#define WR_SIZE 0x8000

char buf[BUF_SIZE];
unsigned buf_pos = 0, max_buf_pos = 0;

unsigned read_hex_digit(FILE *in) {
  char c = fgetc(in);
  if (c <= '9' && c >= '0') return c - '0';
  if (c <= 'f' && c >= 'a') return c - 'a' + 10;
  if (c <= 'F' && c >= 'A') return c - 'A' + 10;

  printf("'%c' not a hex digit.\n", c);
  exit(1);
}

unsigned read_hex_byte(FILE *in) {
  unsigned hi = read_hex_digit(in),
           lo = read_hex_digit(in);

  return (hi << 4) | lo;
}

unsigned read_hex_word(FILE *in) {
  unsigned hi = read_hex_byte(in),
           lo = read_hex_byte(in);

  return (hi << 8) | lo;
}

unsigned read_ihx_line(FILE* in) {
  char newline, colon = fgetc(in);
  if (colon != ':') { puts("Expected ':'"); exit(1); }

  unsigned size = read_hex_byte(in);
  buf_pos = read_hex_word(in);

  if (size == 0) return 0;

  unsigned rtype = read_hex_byte(in);

  unsigned i;
  for (i = 0; i < size; ++i) {
    buf[buf_pos++] = read_hex_byte(in);
    if (buf_pos > max_buf_pos) max_buf_pos = buf_pos;
  }

  unsigned checksum = read_hex_byte(in);

  newline = fgetc(in);
  if (newline != '\n') { puts("Expected newline."); exit(1); }

  return size;
}

void mk_gb_checksums(void) {
  unsigned i, y = 0;
  unsigned char x = 0;
  for (i = 0x134; i < 0x14d; i++) x = x - buf[i] - 1;
  buf[0x14d] = x;

  for (i = 0; i < WR_SIZE; i++) if (i != 0x14e && i != 0x14f) y += buf[i];
  // buf[0x14e] = (y >> 8) & 0xff;
  // buf[0x14f] = y & 0xff;
}

int main(int argc, char** argv) {
  unsigned i;

  memset(buf, 0, BUF_SIZE);
  while (read_ihx_line(stdin));
  mk_gb_checksums();
  fwrite(buf, sizeof(buf[0]), WR_SIZE, stdout);
  
  return 0;
}
