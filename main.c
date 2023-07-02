#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

char buffer[2];

int main() {
  buffer[0] = 27 << 1 | 1;
  return 0;
}
