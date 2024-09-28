#include <errno.h>
#include <readline/chardefs.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// keep deps ordered
// clang-format off
#include <readline/history.h>
#include <readline/readline.h>

#include "mpc.h"
// clang-format on

#define SETRED "\033[31m"
#define RESETCOLOR "\033[0m"

typedef struct lispvalue {
  int type;  // which fields are meaningful
  long num;  // the underlying value (when LISPVAL_NUM)
  char* err;
  char* symbol;
  int count;  // number of cells
  struct lispvalue** cell;
} lispvalue;

// possible lispvalue types
enum { LISPVAL_NUM, LISPVAL_ERR, LISPVALUE_SYMBOL, LISPVALUE_SEXPR };

// possible error types
enum { LISPVALERR_DIV_ZERO, LISPVALERR_BAD_OP, LISPVALERR_BAD_NUM };

// creation functions
static lispvalue* lispvalue_num(const long x) {
  lispvalue* v = malloc(sizeof(lispvalue));
  v->type = LISPVAL_NUM;
  v->num = x;
  return v;
}

static lispvalue* lispvalue_err(const char* m) {
  lispvalue* v = malloc(sizeof(lispvalue));
  v->type = LISPVAL_ERR;
  v->err = malloc(sizeof(m));
  strcpy(v->err, m);
  return v;
}

static lispvalue* lispvalue_sym(char* s) {
  lispvalue* v = malloc(sizeof(lispvalue));
  v->type = LISPVALUE_SYMBOL;
  v->symbol = malloc(strlen(s) + 1);
  strcpy(v->symbol, s);
  return v;
}

static lispvalue* lispvalue_sexpr(void) {
  lispvalue* v = malloc(sizeof(lispvalue));
  v->type = LISPVALUE_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

static void lispvalue_print(lispvalue* v);

static void lispvalue_del(lispvalue* v) {
  switch (v->type) {
    case LISPVAL_NUM:
      break;

    case LISPVAL_ERR:
      free(v->err);
      break;

    case LISPVALUE_SYMBOL:
      free(v->symbol);
      break;

    case LISPVALUE_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lispvalue_del(v->cell[i]);
      }

      free(v->cell);
      break;
  }

  free(v);
}

static lispvalue* lispvalue_read_num(mpc_ast_t* t) {
  errno = 0;
  const long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lispvalue_num(x) : lispvalue_err("Invalid Number.");
}

// add x to v's cells.
static lispvalue* lispvalue_add(lispvalue* v, lispvalue* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lispvalue*) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

static lispvalue* lispvalue_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return lispvalue_read_num(t);
  }

  if (strstr(t->tag, "symbol")) {
    return lispvalue_sym(t->contents);
  }

  lispvalue* x = NULL;

  // create empty list
  if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
    x = lispvalue_sexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0 ||
        strcmp(t->children[i]->contents, ")") == 0 ||
        strcmp(t->children[i]->tag, "regex") == 0) {
      continue;
    }

    x = lispvalue_add(x, lispvalue_read(t->children[i]));
  }

  return x;
}

static void lispvalue_expr_print(lispvalue* v, char start_char, char end_char) {
  putchar((int)start_char);

  for (int i = 0; i < v->count; i++) {
    lispvalue_print(v->cell[i]);

    if (i != (v->count - 1)) {
      putchar((int)' ');
    }
  }

  putchar((int)end_char);
}

static void lispvalue_print(lispvalue* v) {
  switch (v->type) {
    case LISPVAL_NUM:
      printf("%li", v->num);
      break;

    case LISPVAL_ERR:
      printf(SETRED "Error: %s" RESETCOLOR, v->err);
      break;

    case LISPVALUE_SYMBOL:
      printf("%s", v->symbol);
      break;

    case LISPVALUE_SEXPR:
      lispvalue_expr_print(v, '(', ')');
      break;
  }
}

static void lispvalue_println(lispvalue* v) {
  lispvalue_print(v);
  putchar('\n');
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  printf("DIY Lispy\n");
  printf("Press C-c to Exit.\n\n");

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // clang-format off
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                          \
      number : /-?[0-9]+/ ;                    \
      symbol : '+' | '-' | '*' | '/' ;         \
      sexpr  : '(' <expr>* ')' ;               \
      expr   : <number> | <symbol> | <sexpr> ; \
      lispy  : /^/ <expr>* /$/ ;               \
    ",
    Number, Symbol, Sexpr, Expr, Lispy);
  // clang-format on

  while (true) {
    char* user_input = readline("DIYLispy > ");
    // exit on EOF (C-d)
    if (user_input == NULL) {
      break;
    }
    add_history(user_input);

    // attempt to parse user's input
    mpc_result_t result;
    if (mpc_parse("<stdin>", user_input, Lispy, &result)) {
      lispvalue* x = lispvalue_read(result.output);
      lispvalue_println(x);
      lispvalue_del(x);
    } else {
      // otherwise, show error
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free(user_input);
  }  // end REPL

  mpc_cleanup(4, Number, Symbol, Sexpr, Expr, Lispy);
  exit(EXIT_SUCCESS);
}