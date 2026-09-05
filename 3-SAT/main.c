#include <stdio.h>
#include <stdlib.h>

#include "./sat.h"


int main()
{
  int n = 100, m = 200;
  Formula formula = generateFormula(n, m);
  printf("Formula generated!\n");
  fflush(stdout);
  
  BacktrackingState *state = initiateBacktrackingSolver(n, m);
  int result = backtrackingSolver(&formula, state);
  printf("Done!\n");
  //  printFormula(formula);
  printf("Result: %i\n", result);
  /* for (int i = 0; i < n; i++) */
  /*   { */
  /*     printf("%i ", state->val[i]); */
  /*   } */
  printf("\n");
  
  return 9;
}

