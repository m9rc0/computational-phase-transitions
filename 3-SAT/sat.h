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

typedef struct
{
  int *val; // val[n] keeps track of the values assigned to the variables (-1 unused, 0 false and 1 true)
  int *satTrue; // satTrue[m] tells  how many lits in the clause are positive (if > 0 then good)
  int *satFalse; // satFalse[m] tells how many lits are false (if == 3 then bad)
  int trailSize; // keeps track of how many variables are used
} BacktrackingState;

void rng_init(RNGState *rng, uint64_t initstate, uint64_t initseq);
uint32_t rng_next(RNGState *rng);
double rng_next_double(RNGState *rng);

Formula generateFormula(int n, int m);
void freeFormula(Formula formula);
void printFormula(Formula formula);

BacktrackingState *initiateBacktrackingSolver(int n, int m);
void freeBacktrackingState(BacktrackingState *state);
int backtrackingSolver(const Formula *formula, BacktrackingState *state);
int checkTrue(const int *satTrue, int m);
void updateState(ClauseList *clauseList, int *satTrue, int *satFalse, int literal);
int checkFalse(const int *satFalse, int m);
void undoState(ClauseList *clauseList, int *satTrue, int *satFalse, int literal);

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

  // Stack-allocate RNGState to eliminate memory leak
  RNGState rng;
  rng_init(&rng, time(NULL), (uint64_t)(uintptr_t)&formula);

  formula.m = m;
  formula.n = n;

  formula.clauses = malloc(m * sizeof(Clause));
  formula.clauselists = malloc(2 * n * sizeof(ClauseList));

  // Initialize occurrence count sizes to 0
  for (int i = 0; i < 2 * n; i++)
  {
    formula.clauselists[i].clauseNumber = 0;
  }

  // Generate clauses (sampling directly from [0, 2n - 1], allowing duplicate literals)
  for (int i = 0; i < m; i++)
  {
    int random_literal0 = (int)(((uint64_t)rng_next(&rng) * (uint64_t)(2 * n)) >> 32);
    int random_literal1 = (int)(((uint64_t)rng_next(&rng) * (uint64_t)(2 * n)) >> 32);
    int random_literal2 = (int)(((uint64_t)rng_next(&rng) * (uint64_t)(2 * n)) >> 32);

    formula.clauses[i].literals[0] = random_literal0;
    formula.clauses[i].literals[1] = random_literal1;
    formula.clauses[i].literals[2] = random_literal2;

    formula.clauselists[random_literal0].clauseNumber++;
    formula.clauselists[random_literal1].clauseNumber++;
    formula.clauselists[random_literal2].clauseNumber++;
  }

  // Allocate occurrence list arrays
  for (int i = 0; i < 2 * n; i++)
  {
    formula.clauselists[i].clauseList = malloc(formula.clauselists[i].clauseNumber * sizeof(int));
    formula.clauselists[i].clauseNumber = 0; // Reset counter for population phase
  }

  // Populate occurrence lists
  for (int i = 0; i < m; i++)
  {
    Clause *clause = &formula.clauses[i];

    ClauseList *clauseList0 = &formula.clauselists[clause->literals[0]];
    clauseList0->clauseList[clauseList0->clauseNumber++] = i;

    ClauseList *clauseList1 = &formula.clauselists[clause->literals[1]];
    clauseList1->clauseList[clauseList1->clauseNumber++] = i;

    ClauseList *clauseList2 = &formula.clauselists[clause->literals[2]];
    clauseList2->clauseList[clauseList2->clauseNumber++] = i;
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
        Clause *clause = &formula.clauses[i];
        for (int j = 0; j < 3; j++)
        {
            int lit = clause->literals[j];
            int var = lit / 2;
            int is_negated = lit & 1; // Check odd/even

            if (is_negated)
                printf("!%d ", var);
            else
                printf("%d ", var);
        }
        printf("\n");
    }
}


BacktrackingState *initiateBacktrackingSolver(int n, int m)
{
  BacktrackingState *state;
  state = malloc(sizeof(BacktrackingState));

  state->val = malloc(n*sizeof(int));
  for (int i = 0; i < n; i++)
    {
      state->val[i] = -1;
    }
  state->satTrue = calloc(m,sizeof(int));
  state->satFalse = calloc(m,sizeof(int));
  state->trailSize = 0;
  return state;
}

void freeBacktrackingState(BacktrackingState *state)
{
  free(state->satFalse);
  free(state->satTrue);
  free(state->val);
  free(state);
}

int backtrackingSolver(const Formula *formula, BacktrackingState *state)
{
  int literal;
  if (checkTrue(state->satTrue, formula->m) == 1)
    {
      return 1;
    }
  if (state->trailSize == formula->n)
    {
      return 0;
    }
  literal = 2*state->trailSize;
  state->trailSize++;
  // try to assign to True
  state->val[literal>>1] = 1;
  updateState(formula->clauselists, state->satTrue, state->satFalse, literal);
  if (checkFalse(state->satFalse, formula->m) == 1)
    {
      if (backtrackingSolver(formula, state) == 1) return 1;
    }
  undoState(formula->clauselists, state->satTrue, state->satFalse, literal);

  // try assign to False
  state->val[literal>>1] = 0;
  literal = literal ^ 1;
  updateState(formula->clauselists, state->satTrue, state->satFalse, literal);
  if (checkFalse(state->satFalse, formula->m) == 1)
    {
      if (backtrackingSolver(formula, state) == 1) return 1;
    }
  undoState(formula->clauselists, state->satTrue, state->satFalse, literal);
  state->val[literal>>1] = -1;
  state->trailSize--;
  // if nothing works backtrack 
  return 0;
}

int checkTrue(const int *satTrue, int m)
{
  // checks if all clauses are satisfied
  int count = 0;

  for (int i = 0; i < m; i++)
    {
      if (satTrue[i] > 0)
	{
	  count++;
	}
    }
  
  if (count == m)
    {
      return 1;
    }

  return 0;
}

void updateState(ClauseList *clauseList, int *satTrue, int *satFalse, int literal)
{
  // given the assigned literal checks the list of clauses relative to the literal and updates the relative counts;
  // update the true
  int i;
  ClauseList *list = clauseList+literal;
  for (i = 0; i < list->clauseNumber; i++)
    {
      satTrue[list->clauseList[i]]++;
    }
  
  // update the false
  literal = literal ^ 1; // this reverses the literal
  list = clauseList+literal;
  for (i = 0; i < list->clauseNumber; i++)
    {
      satFalse[list->clauseList[i]]++;
    }
  
}

int checkFalse(const int *satFalse, int m)
{
  // if at least one clause has 3 literals false returns 0, else returns 1
  int i;

  for (i = 0; i < m; i++)
    {
      if (satFalse[i] == 3)
	{
	  return 0;
	}
    }
  return 1;
}

void undoState(ClauseList *clauseList, int *satTrue, int *satFalse, int literal)
{
  // given the assigned literal checks the list of clauses relative to the literal and updates the relative counts reducig by 1 all counts;
  // update the true
  int i;
  ClauseList *list = clauseList+literal;
  for (i = 0; i < list->clauseNumber; i++)
    {
      satTrue[list->clauseList[i]]--;
    }
  
  // update the false
  literal = literal ^ 1; // this reverses the literal
  list = clauseList+literal;
  for (i = 0; i < list->clauseNumber; i++)
    {
      satFalse[list->clauseList[i]]--;
    }
  
}
