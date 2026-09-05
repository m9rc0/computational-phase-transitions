#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  int literals[3]; //list with 3 literals for each clause
} Clause;

typedef struct {
  int *clauseList; // list with indices of clauses containing the literal
  int clauseNumber; // number of clauses that contain the literal
} ClauseList; // this should serve as an helper to store all the clauses for each literal

typedef struct {
  int m; // number of clauses
  int n; // number of variables
  Clause *clauses; // this is the array of m clauses that make the formula
  ClauseList *clauselists; // array of 2*n occurences lists for each literal
} Formula;

// Thread-Safe Random Number Generator
typedef struct {
    uint64_t state;
    uint64_t inc;
} RNGState;       

void rng_init(RNGState *rng, uint64_t initstate, uint64_t initseq);
uint32_t rng_next(RNGState *rng);
double rng_next_double(RNGState *rng);
Formula generateFormula(int n, int m);
void freeFormula(Formula formula);
void printFormula(Formula formula);

void rng_init(RNGState *rng, uint64_t initstate, uint64_t initseq) 
{
    rng->state = 0U;
    // The increment must be odd. We shift and bitwise OR to ensure this.
    rng->inc = (initseq << 1u) | 1u;
    
    // Step the generator twice to mix the seed and sequence
    rng_next(rng);
    rng->state += initstate;
    rng_next(rng);
}

// Generates a uniformly distributed 32-bit random integer
uint32_t rng_next(RNGState *rng) 
{
    uint64_t oldstate = rng->state;
    
    // Advance internal state (LCG step)
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    
    // Output function (XSH RR: xorshift and rotate)
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

// Generates a uniformly distributed double in the range [0, 1)
double rng_next_double(RNGState *rng) 
{
    // Divide by 2^32 to map [0, 4294967295] to [0.0, 1.0)
    // 4294967296.0 is 2^32 represented as a double constant
    return (double)rng_next(rng) / 4294967296.0;
}


Formula generateFormula(int n, int m)
{
  Formula formula;

  // initialize the generator
  RNGState *rng;
  rng = malloc(sizeof(RNGState));
  rng_init(rng, time(NULL), (uint64_t)(uintptr_t)rng);

  formula.m = m;
  formula.n = n;

  formula.clauses = malloc(m*sizeof(Clause));
  formula.clauselists = malloc(2*n*sizeof(ClauseList));

  //set the initial sizes to 0;
  for (int i = 0; i < 2*n; i++)
    {
      formula.clauselists->clauseNumber = 0;
    }

  for (int i = 0; i < m; i++)
    {
      int random_literal0 = (int)(((uint64_t)rng_next(rng) * (uint64_t)(2 * n)) >> 32); // this is called Lemire's method and is very efficient and truly maps into the wanted range uniformly
      int random_literal1 = (int)(((uint64_t)rng_next(rng) * (uint64_t)(2 * n)) >> 32); 
      int random_literal2 = (int)(((uint64_t)rng_next(rng) * (uint64_t)(2 * n)) >> 32);       

      // generate the clause
      (formula.clauses+i)->literals[0] = random_literal0;
      (formula.clauses+i)->literals[1] = random_literal1;
      (formula.clauses+i)->literals[2] = random_literal2;
      //update the size of the occurency list
      (formula.clauselists+random_literal0)->clauseNumber++;
      (formula.clauselists+random_literal1)->clauseNumber++;
      (formula.clauselists+random_literal2)->clauseNumber++;
    }

  // initialize properly occurency lists
  for (int i = 0; i < 2*n; i++)
    {
      (formula.clauselists+i)->clauseList = malloc((formula.clauselists+i)->clauseNumber*sizeof(int));
      (formula.clauselists+i)->clauseNumber = 0; // set again to 0 will be needed in next loop
    }

  for (int i = 0; i < m; i++)
    {
      Clause *clause = (formula.clauses+i);

      ClauseList *clauseList0 = formula.clauselists+(clause->literals[0]);
      clauseList0->clauseList[clauseList0->clauseNumber] = i;
      clauseList0->clauseNumber++;

      ClauseList *clauseList1 = formula.clauselists+(clause->literals[1]);
      clauseList1->clauseList[clauseList1->clauseNumber] = i;
      clauseList1->clauseNumber++;

      ClauseList *clauseList2 = formula.clauselists+(clause->literals[2]);
      clauseList2->clauseList[clauseList2->clauseNumber] = i;
      clauseList2->clauseNumber++;
    }
  
  return formula;
}

void freeFormula(Formula formula)
{
  // free the clauses 
  free(formula.clauses);

  // free the occurencylists
  for (int i = 0; i < 2*formula.n; i++)
    {
      free((formula.clauselists+i)->clauseList);
    }
  
  // free the list of lists
  free(formula.clauselists);
}

void printFormula(Formula formula)
{
  for (int i = 0; i < formula.m; i++)
    {
      Clause *clause = formula.clauses+i;
      for (int j = 0; j < 3; j++)
	{
	  if (clause->literals[j] < formula.n)
	    {
	      printf("%i ", clause->literals[j]/2);
	    }
	  else
	    {
	      printf("!%i ", clause->literals[j]/2);
	    }
	}
      printf("\n");
    }
}
