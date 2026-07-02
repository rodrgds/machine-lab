#include <lcom/lcom.h>

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;
  lcom_printf("hello from Machine Lab\n");
  lcom_exit();
  return 0;
}
