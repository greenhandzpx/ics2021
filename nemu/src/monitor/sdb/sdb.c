#include <isa.h>
#include <cpu/cpu.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

#include "memory/paddr.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args) {
  char *arg = strtok(args, " ");
  uint64_t steps;
  if (arg == NULL) {
    // we should execute for one step
    cpu_exec(1);
  } else {
    char *leftover;
    steps = strtoul(arg, &leftover, 10);
    cpu_exec(steps);
  }
  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(args, " ");
  if (arg && arg[0] == 'r') {
    isa_reg_display();
  } else if (arg && arg[0] == 'w') {

  } 
  return 0;
}
 
static int cmd_x(char *args) {
  char *arg = strtok(args, " ");
  if (arg == NULL) {
    return 0;
  }

  char *leftover;
  uint64_t cnt = strtoul(arg, &leftover, 10);
  // acquire the next arg(aka addr)
  char *addr_str = strtok(arg + strlen(arg) + 1, " ");
  // transform the addr_str to hex format
  paddr_t guest_addr = strtoul(addr_str + 2, &leftover, 16);
  // transform the guest_addr to host_addr
  uint8_t* host_addr = guest_to_host(guest_addr);

  for (int i = 0; i < cnt; ++i) {
    printf("%p\n", host_addr);
    host_addr += cnt;
  }
  return 0;
}

static int cmd_p(char *args) {
  return 0;
}

static int cmd_w(char *args) {
  return 0;
}

static int cmd_d(char *args) {
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Execute for N steps", cmd_si},
  { "info", "Print the registers or watchpoints", cmd_info},
  { "x", "Scan the memory", cmd_x},
  { "p", "Predicate", cmd_p},
  { "w", "Set a watchpoint", cmd_w},
  { "d", "Delete a watchpoint", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
