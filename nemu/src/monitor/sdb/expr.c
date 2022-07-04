#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <string.h>
#include <stdlib.h>

#include "memory/paddr.h"

enum {
  TK_NOTYPE = 256, TK_EQ,
  /* TODO: Add more token types */
  TK_HEX,  // hexadecimal
  TK_AND,  // &&
  TK_DEREF, // dereference
  TK_REG    // reg_name
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {"0x[0-9]+", TK_HEX}, // hexadecimal number
  {"\\(", '('},
  {"\\)", ')'},
  {"[0-9]+", 'd'},      // decimal number(greedy match)
  {"\\/", '/'},         // divide
  {"\\*", '*'},         // multiply or dereference
  {"\\-", '-'},         // subtract
  {"\\+", '+'},         // plus
  {"&&", TK_AND},       // &&
  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
  {"\\$..|\\$s10|\\$s11", TK_REG}
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[96] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case '+':
            tokens[nr_token].type = '+';
            ++nr_token;
            break;
          case '-':
            tokens[nr_token].type = '-';
            ++nr_token;
            break;
          case '*':
            tokens[nr_token].type = '*';
            ++nr_token;
            break;
          case '/':
            tokens[nr_token].type = '/';
            ++nr_token;
            break;
          case '(':
            tokens[nr_token].type = '(';
            ++nr_token;
            break;
          case ')':
            tokens[nr_token].type = ')';
            ++nr_token;
            break;
          case 'd':
            tokens[nr_token].type = 'd';
            memset(tokens[nr_token].str, '\0', sizeof(tokens[nr_token].str));
            if (substr_len > 32) {
              strncpy(tokens[nr_token].str, substr_start, 32);
            } else {
              strncpy(tokens[nr_token].str, substr_start, substr_len);
            }
            ++nr_token;
            break;
          case TK_HEX:
            tokens[nr_token].type = TK_HEX;
            memset(tokens[nr_token].str, '\0', sizeof(tokens[nr_token].str));
            // we can just trim the '0x'
            if (substr_len > 34) {
              strncpy(tokens[nr_token].str, substr_start+2, 32);
            } else {
              strncpy(tokens[nr_token].str, substr_start+2, substr_len-2);
            }
            ++nr_token;
            break;
          case TK_AND:
            tokens[nr_token].type = TK_AND;
            ++nr_token;
            break;
          case TK_EQ:
            tokens[nr_token].type = TK_EQ;
            ++nr_token;
            break;
          case TK_REG:
            tokens[nr_token].type = TK_REG;
            memset(tokens[nr_token].str, '\0', sizeof(tokens[nr_token].str));
            if (substr_len > 32) {
              strncpy(tokens[nr_token].str, substr_start, 32);
            } else {
              strncpy(tokens[nr_token].str, substr_start, substr_len);
            }
            ++nr_token;
          case TK_NOTYPE:
            break;
          default: TODO();
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


/**
 * @return 0 --- success, 1 --- fail but legal, 2 --- illegal
 */
int check_parantheses(int left, int right) {
  if (tokens[left].type != '(' || tokens[right].type != ')') {
    Log("no paran 1");
    return 1;
  }
  int paren_cnt = 0;
  int stk[10000];
  for (int i = left; i <= right; ++i) {
    if (tokens[i].type == '(') {
      stk[paren_cnt++] = i;
    } else if (tokens[i].type == ')') {
      if (paren_cnt <= 0) {
        return 2;
      }
      --paren_cnt; 
    }
  }
  if (paren_cnt != 0) {
    return 2;
  }
  if (stk[0] != left) {
    Log("no paran 2, stk[0]:%d", stk[0]);
    return 1;
  }
  return 0;
}

uint32_t eval(int left, int right) {
  if (left > right) {
    return 0;
  }


  if (left == right) {
    assert(tokens[left].type == 'd' || tokens[left].type == TK_HEX ||
            tokens[left].type == TK_REG);
    if (tokens[left].type == 'd') {
      Log("num: %s", tokens[left].str);
      return strtoul(tokens[left].str, NULL, 10);

    } else if (tokens[left].type == TK_HEX) {
      return strtoul(tokens[left].str, NULL, 16);

    } else {
      bool success;
      // fetch the val in the reg
      // remove the '$' 
      uint32_t res = isa_reg_str2val(tokens[left].str+1, &success);
      if (!success) {
        return 0;
      }
      return res;
    }
  }


  int res = check_parantheses(left, right);
  if (res == 0) {
    // the whole predicate is wrapped in a pair of brackets
    Log("find a pair of wrapped brackets");
    return eval(left+1, right-1);
  } else if (res == 2) {
    // illegal parantheses 
    return 0;
  }

  // traverse the tokens and find the main operator
  /**
   * level 1: ()
   * level 2: *(dereference)
   * level 3: * /
   * level 4: + -
   */
  
  int level = 1, loc = -1, op_type;
  for (int i = left; i < right; ++i) {
    if (tokens[i].type == '(') {
      // we should pass all the tokens wrapped in a pair of brackets
      int k = i;
      int cnt = 0;
      for (; k < right; ++k) {
        if (tokens[k].type == '(') {
          ++cnt;
        } else if (tokens[k].type == ')') {
          --cnt;
          if (cnt == 0) {
            break;
          }
        }
      }
      i = k;
      continue;
    }
    if (tokens[i].type == '+' || tokens[i].type == '-') {
      // we should set the last lowest level operator as main operator
      op_type = tokens[i].type;
      level = 4;
      loc = i;
    } else if (level < 4 && (tokens[i].type == '*' || tokens[i].type == '/')) {
      op_type = tokens[i].type;
      level = 3;
      loc = i; 
    } else if (level < 3 && tokens[i].type == TK_DEREF) {
      op_type = tokens[i].type;
      level = 2;
      loc = i; 
    }
  } 
  if (loc == -1) {
    return 0;
  }
  Log("main op idx: %d", loc);
  uint32_t val1 = eval(left, loc - 1);
  Log("left val:%u", val1);
  uint32_t val2 = eval(loc + 1, right);
  Log("right val:%u", val2);
  switch (op_type) {
    case '+':
      return val1 + val2; 
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      return val1 / val2; 
    case TK_DEREF:
      // transform the guest_addr to host_addr
      uint32_t* host_addr = (uint32_t*)guest_to_host(val2);
      Log("deref: addr:%u, val:%u", val2, *host_addr);
      return *host_addr;
    default:
      return 0;
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  *success = true;
  /* TODO: Insert codes to evaluate the expression. */

  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && 
      (i == 0 || (tokens[i - 1].type != ')' && tokens[i-1].type != 'd' &&
      tokens[i-1].type != TK_HEX) ) ) {
      tokens[i].type = TK_DEREF;
    }
  }

  return eval(0, nr_token - 1);
}
