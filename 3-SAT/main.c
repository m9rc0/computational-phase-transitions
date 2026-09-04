#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int result;
  int *solution;
  int n;
} formula_output;

int formula_example1(int *x);
int formula_example2(int *x);
int formula_example3(int *x);

int main()
{
  // n is the number of variables, m = alpha * n is the number of clauses
  int x[4] = {1,0,1,1};
  printf("%i\n", formula_example2(x));
  
  return 9;
}

int formula_example1(int *x)
{
  // simple case with 4 variables, single clause
  return x[0]||x[1]||x[2];
}


int formula_example2(int *x)
{
  // simple case with 4 variables, single clause
  return (x[0]||x[1]||(!x[2]))&&((!x[0])||x[2]||x[3]);
}

int formula_example3(int *x)
{
  // simple case with 4 variables, single clause
  return (x[0]||x[0]||x[0])&&((!x[0])||(!x[0])||(!x[0]))&&(x[1]||x[2]||x[3]);
}

formula_output backtracking_SAT(int *x, int n)
{
  formula_output out;
}
