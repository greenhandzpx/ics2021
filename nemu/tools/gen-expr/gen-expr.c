#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int cnt = 0; 

int choose(int bound) {
  return rand() % bound;
}

static void gen_num() {
  // uint32_t *dummy = &buf[cnt];
  // *dummy = rand() % 10000;
  sprintf(&buf[cnt], "%u", rand() % 1000 + 1);
  // printf("gen a num:%s\n", &buf[cnt]);
  while (buf[cnt] != '\0') {
    ++cnt;
  }
}

static void gen(char sym) {
  sprintf(&buf[cnt], "%c", sym);
  ++cnt;
}

static void gen_rand_op() {
  switch (choose(4)) {
    case 0: gen('+'); break;
    case 1: gen('-'); break;
    case 2: gen('*'); break;
    default: gen('/'); break;
  }
}

static void gen_rand_expr() {
  // printf("cnt:%d\n", cnt);
  // if (cnt > 60) {
  //   buf[cnt] = '\0';
  //   return;
  // }
  if (cnt > 60) {
    gen_num();
    return;
  }

  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_rand_expr(); gen(')'); break;
    default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    memset(buf, '\0', 65536);
    cnt = 0;
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
