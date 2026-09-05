#include <stdio.h>
#include <stdlib.h>

#include "./sat.h"


int main()
{
  Formula formula = generateFormula(5, 7);
  printFormula(formula);
  return 9;
}

