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

long eval_op(const long x, const char* op, const long y) {
  if (strcmp(op, "+") == 0) {
    return x + y;
  }

  if (strcmp(op, "-") == 0) {
    return x - y;
  }

  if (strcmp(op, "*") == 0) {
    return x * y;
  }

  if (strcmp(op, "/") == 0) {
    return x / y;
  }

  return 0;
}

long eval(mpc_ast_t* t) {
  // Return the number when encountered
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  // Operator is always second child
  const char* op = t->children[1]->contents;

  // store the third child
  long x = eval(t->children[2]);

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
      const long eval_result = eval(result.output);
      printf("%li\n", eval_result);
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