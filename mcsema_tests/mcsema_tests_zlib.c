#include <stdio.h>
#include <zlib.h>

int main() {
  const char buffer[] = "Hello from Trail of Bits!";
  int crc_value = crc32(0, (const Bytef *) buffer, sizeof(buffer));

  printf("CRC value for '%s' is: %d\n", buffer, crc_value);
  return 0;
}
