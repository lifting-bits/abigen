#include <stdio.h>
#include <zlib.h>

int main() {
  printf("Before crc32()\n");
  crc32(1, NULL, 0);
  printf("After crc32()\n");

  return 0;
}
