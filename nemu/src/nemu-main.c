#include <common.h>
#include <stdio.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

word_t expr(char *e, bool *success);


int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  // // read the input file to test whether the expression evaluation is correct
  // FILE *input = fopen("tools/gen-expr/input", "r");
  // if (input == NULL) {
  //   printf("no input file\n");
  //   return 0;
  // }
  // char buf[65535];
  // char std_res_buf[65535];
  // uint32_t test_res, std_res;
  // while (fgets(buf, sizeof(buf), input) != NULL) {
  //   int i = 0;
  //   for (i = 0; buf[i] != '\0'; ++i) {
  //     if (buf[i] == ' ') {
  //       break;
  //     }
  //   }
  //   buf[i] = '\0';
  //   strcpy(std_res_buf, buf);
  //   std_res = strtoul(std_res_buf, NULL, 10);
  //   printf("expr: %s\n", buf+i+1);
  //   bool success;
  //   test_res = expr(buf+i+1, &success);
  //   if (std_res != test_res) {
  //     printf("not equal, std:%u, test:%u\n", std_res, test_res);
  //   }
  //   memset(buf, '\0', sizeof(buf));
  //   memset(std_res_buf, '\0', sizeof(std_res_buf));
  // }
  bool success;
  uint32_t test_res = expr("", &success);
  printf("res: %u\n", test_res);
  engine_start();

  return is_exit_status_bad();
}
