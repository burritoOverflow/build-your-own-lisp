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

typedef struct {
  int type;  // which fields are meaningful
  long num;  // the underlying value (when LISPVAL_NUM)
  int err;   // which error (when error has occured, i.e type == LISPVAL_ERR)
} lispvalue;

// possible lispvalue types
enum { LISPVAL_NUM, LISPVAL_ERR };

// possible error types
enum { LISPVALERR_DIV_ZERO, LISPVALERR_BAD_OP, LISPVALERR_BAD_NUM };

// creation functions
lispvalue lispvalue_num(const long x) {
  lispvalue v;
  v.type = LISPVAL_NUM;
  v.num = x;
  return v;
}

lispvalue lispvalue_err(const int x) {
  lispvalue v;
  v.type = LISPVAL_ERR;
  v.err = x;
  return v;
}

void lispvalue_show_value(const lispvalue lispval) {
  switch (lispval.type) {
    case LISPVAL_NUM:
      printf("%li\n", lispval.num);
      break;

    case LISPVAL_ERR: {
      if (lispval.err == LISPVALERR_DIV_ZERO) {
        fprintf(stderr, SETRED "Error: Attempting to divide by 0\n" RESETCOLOR);
      }

      if (lispval.err == LISPVALERR_BAD_OP) {
        fprintf(stderr, SETRED "Error: Invalid Operator\n" RESETCOLOR);
      }

      if (lispval.err == LISPVALERR_BAD_NUM) {
        fprintf(stderr, SETRED "Error: Invalid Number\n" RESETCOLOR);
      }
    }
  }
}

lispvalue eval_op(const lispvalue x, const char* op, const lispvalue y) {
  // if either operand is an error, return that lispvalue
  if (x.type == LISPVAL_ERR) {
    return x;
  }

  if (y.type == LISPVAL_ERR) {
    return y;
  }

  if (strcmp(op, "+") == 0) {
    return lispvalue_num(x.num + y.num);
  }

  if (strcmp(op, "-") == 0) {
    return lispvalue_num(x.num - y.num);
  }

  if (strcmp(op, "*") == 0) {
    return lispvalue_num(x.num * y.num);
  }

  if (strcmp(op, "/") == 0) {
    // divide by zero error
    if (y.num == 0) {
      return lispvalue_err(LISPVALERR_DIV_ZERO);
    }
    return lispvalue_num(x.num / y.num);
  }

  return lispvalue_err(LISPVALERR_BAD_OP);
}

lispvalue eval(mpc_ast_t* t) {
  // Return the number when encountered
  if (strstr(t->tag, "number")) {
    errno = 0;
    const long x = strtol(t->contents, NULL, 10);
    // check for errno ==  not a representable value
    return errno != ERANGE ? lispvalue_num(x)
                           : lispvalue_err(LISPVALERR_BAD_NUM);
  }

  // Operator is always second child
  const char* op = t->children[1]->contents;

  // store the third child
  lispvalue x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    ++i;
  }

  return x;
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  printf("DIY Lispy\n");
  printf("Press C-c to Exit.\n\n");

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // clang-format off
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);
  // clang-format on

  while (true) {
    char* user_input = readline("DIYLispy > ");
    // handle EOF (C-d)
    if (user_input == NULL) {
      continue;
    }
    add_history(user_input);

    // attempt to parse user's input
    mpc_result_t result;

    if (mpc_parse("<stdin>", user_input, Lispy, &result)) {
      const lispvalue eval_result = eval(result.output);
      lispvalue_show_value(eval_result);
      mpc_ast_delete(result.output);
    } else {
      // otherwise, show error
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free(user_input);
  }  // end REPL

  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  exit(EXIT_SUCCESS);
}