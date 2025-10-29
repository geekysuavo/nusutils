
/* nusutils: generalized deterministic nonuniform sampling utilities.
 * Copyright (C) 2015, 2025 Bradley Worley <geekysuavo@gmail.com>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 *
 *   Free Software Foundation, Inc.
 *   51 Franklin Street, Fifth Floor
 *   Boston, MA  02110-1301, USA.
 */

/* include the evaluation header. */
#include "eval.h"

/* * * * evaluation format strings * * * */

/* FMT_GAP: format string for all gap equation assignments. */
#define FMT_GAP \
  "g(x::Float64, d::Int32, O::Array, N::Array, L::Float64) = %s + 1.0;"

/* FMT_PDF: format string for all density function assignments. */
#define FMT_PDF \
  "f(x::Array, N::Array) = %s;"

/* * * * preprogrammed gap equation expression strings * * * */

/* EXPR_POISRND: expression for quasirandom poisson-distributed terms. */
#define EXPR_POISRND \
  "poisrnd(x) = -x - 2.0"

/* EXPR_PG: expression for the poisson-gap preprogrammed method. */
#define EXPR_PG \
  "poissongap(x, d, O, N, L) = \
   poisrnd(L * sin((pi / 2) * (x + sum(O)) / sum(N)))"

/* EXPR_SG: expression for the sine-gap preprogrammed method. */
#define EXPR_SG \
  "sinegap(x, d, O, N, L) = \
   L * sin((pi / 2) * (x + sum(O)) / sum(N))"

/* EXPR_SB: expression for the sine-burst preprogrammed method. */
#define EXPR_SB \
  "sineburst(x, d, O, N, L) = \
   L * sin((pi / 2) * (x + sum(O)) / sum(N)) \
     * sin((pi / 4) * N[d] * (x + sum(O)) / sum(N))^2"

/* * * * global variables * * * */

/* evaltyp: type of evaluation engine currently in use. */
evaltype_t evaltyp;

/* evalfn: julia function handle that holds the compiled equation. */
jl_function_t *evalfn;

/* evalrng: quasirandom number generator for poisson-distributed terms. */
qrng_t evalrng;

/* * * * function definitions * * * */

/* evalinit_gap(): gap-specific initialization function.
 * see evalinit() for more details.
 */
int evalinit_gap (const char *fstr) {
  /* declare required variables:
   *  @stmt: gap equation assignment statement string.
   *  @nstmt: number of characters in the statement string.
   */
  char *stmt;
  int nstmt;

  /* allocate a function string to evaluate. */
  nstmt = strlen(fstr) + strlen(FMT_GAP) + 32;
  stmt = (char*) malloc(nstmt * sizeof(char));

  /* check that the string was evaluated. */
  if (!stmt)
    return EVAL_ERR;

  /* build the function assignment string. */
  snprintf(stmt, nstmt, FMT_GAP, fstr);

  /* evaluate preprogrammed function definitions. */
  (void) jl_eval_string(EXPR_POISRND);
  (void) jl_eval_string(EXPR_PG);
  (void) jl_eval_string(EXPR_SG);
  (void) jl_eval_string(EXPR_SB);

  /* evaluate the function assignment. */
  (void) jl_eval_string(stmt);
  free(stmt);

  /* check that the evaluation succeeded. */
  if (jl_exception_occurred())
    return EVAL_ERR;

  /* get the compiled function handle. */
  evalfn = jl_get_function(jl_main_module, "g");

  /* return success. */
  return EVAL_OK;
}

/* evalinit_pdf(): density-specific initialization function.
 * see evalinit() for more details.
 */
int evalinit_pdf (const char *fstr) {
  /* declare required variables:
   *  @stmt: gap equation assignment statement string.
   *  @nstmt: number of characters in the statement string.
   */
  char *stmt;
  int nstmt;

  /* allocate a function string to evaluate. */
  nstmt = strlen(fstr) + strlen(FMT_PDF) + 32;
  stmt = (char*) malloc(nstmt * sizeof(char));

  /* check that the string was evaluated. */
  if (!stmt)
    return EVAL_ERR;

  /* build the function assignment string. */
  snprintf(stmt, nstmt, FMT_PDF, fstr);

  /* evaluate the function assignment. */
  (void) jl_eval_string(stmt);
  free(stmt);

  /* check that the evaluation succeeded. */
  if (jl_exception_occurred())
    return EVAL_ERR;

  /* get the compiled function handle. */
  evalfn = jl_get_function(jl_main_module, "f");

  /* return success. */
  return EVAL_OK;
}

/* evalinit(): initialize the equation evaluation engine.
 *
 * arguments:
 *  @fstr: julia function string to compile and call.
 *  @ftype: evaluation engine type: gap or pdf.
 *
 * returns:
 *  integer indicating whether initialization succeeded.
 */
int evalinit (const char *fstr, evaltype_t ftype) {
  /* allocate the quasirandom number generator. */
  if (!qrngalloc(&evalrng, 1))
    return EVAL_ERR;

  /* iterate the qrng just once to avoid returning zero. */
  qrngeval(&evalrng);

  /* determine the type of evaluation engine to initialize. */
  switch (ftype) {
    /* gap equation. */
    case EVAL_GAP:
      return evalinit_gap(fstr);

    /* density function. */
    case EVAL_PDF:
      return evalinit_pdf(fstr);

    /* otherwise. */
    default:
      return EVAL_ERR;
  }

  /* return failure: unknown engine type and failed switch? */
  return EVAL_ERR;
}

/* evalfree(): called to destroy the equation evaluation engine. this
 * allows the julia environment to free its internals and frees any
 * extra allocated memory.
 */
void evalfree (void) {
  /* free the quasirandom number generator. */
  qrngfree(&evalrng);

  /* clean up the julia internals. */
  jl_atexit_hook(0);
}

/* evalpois(): return a quasirandomly poisson-distributed value,
 * given a rate parameter.
 *
 * arguments:
 *  @lambda: negative of the input rate parameter.
 *
 * returns:
 *  floating point value of the result.
 */
double evalpois (double lambda) {
  /* declare required variables:
   *  @L: exponentiated negated rate.
   *  @k: final poisson variate.
   */
  double L, p, k;

  /* initialize the intermediate values. */
  L = exp(lambda);
  k = 0.0;
  p = 1.0;

  /* loop until the final iterate is reached. */
  do {
    /* compute a new iterate within the qrng. */
    qrngeval(&evalrng);

    /* update the intermediate values. */
    p *= evalrng.x[0];
    k += 1.0;
  }
  while (p >= L);

  /* return the final value. */
  return k;
}

/* evalgap(): compute the next term in the deterministic gap sequence
 * given the current value and the sequence parameters.
 *
 * arguments:
 *  @x: pointer to the current and next sequence term.
 *  @d: current dimension of the Nyquist grid.
 *  @O: current offset position in the Nyquist grid.
 *  @N: total size of the Nyquist grid.
 *  @L: scaling factor for sequence terms.
 *
 * returns:
 *  integer indicating whether the sequence is well-behaved (1) (i.e. whether
 *  the scaling factor is in bounds) or not (0).
 */
int evalgap (double *x, int d, tuple_t *O, tuple_t *N, double L) {
  /* declare required variables:
   *  @i: general array index and loop counter.
   *  @ret: return status value for this function.
   *  @theta: sequence term angular value.
   *  @gx: unboxed gap equation result.
   */
  int i, ret = EVAL_OK;
  double theta, gx;

  /* declare required julia variables:
   *  @gval: boxed return value of the gap method call.
   *  @args: array of value pointers passed to the gap method.
   *  @arrtype: data type of the arrays passed to the gap method.
   *  @arro, @arrn: arrays passed to the gap method.
   *  @dato, @datn: array data pointers.
   */
  jl_value_t *xval, *dval, *Lval, *gval;
  jl_value_t **args;
  jl_value_t *arrtype;
  jl_array_t *arro, *arrn;
  double *dato, *datn;

  /* compute the angular term value. */
  theta = (*x + tupsum(O)) / tupsum(N);

  /* determine whether the angular term is in bounds.
   *
   * this check is to ensure that the value of the provided scaling
   * factor generates a well-behaved sequence. poorly behaved
   * sequences having large scaling factors must be identified
   * in order to assign large errors to their parameters and
   * thus ensure simplex optimization succeeds.
   */
  if (theta > 1.0)
    ret = EVAL_INVALID;

  /* box up the scalar arguments. */
  xval = jl_box_float64(*x);
  dval = jl_box_int32(d + 1);
  Lval = jl_box_float64(L);

  /* initialize the array data type. */
  arrtype = jl_apply_array_type((jl_value_t*) jl_float64_type, 1);

  /* allocate the origin and size arrays. */
  arro = jl_alloc_array_1d(arrtype, tupsize(O));
  arrn = jl_alloc_array_1d(arrtype, tupsize(N));

  /* access the origin and size array data pointers. */
  dato = (double*) jl_array_data(arro, double);
  datn = (double*) jl_array_data(arrn, double);

  /* fill the origin and size arrays. */
  for (i = 0; i < tupsize(O); i++) {
    dato[i] = (double) tupget(O, i);
    datn[i] = (double) tupget(N, i);
  }

  /* initialize the argument array. */
  JL_GC_PUSHARGS(args, 5);

  /* construct an argument array for the gap method call. */
  args[0] = xval;
  args[1] = dval;
  args[2] = (jl_value_t*) arro;
  args[3] = (jl_value_t*) arrn;
  args[4] = Lval;

  /* call the gap equation with the current arguments. */
  gval = jl_call(evalfn, args, 5);

  /* check if an exception occurred. */
  if (jl_exception_occurred()) {
    /* output an error. */
    fprintf(stderr, "error: g(%.3lf, %d, [%u.0",
      *x, d, tupget(O, 0));
    for (i = 1; i < tupsize(O); i++)
      fprintf(stderr, ", %u.0", tupget(O, i));
    fprintf(stderr, "], [%u.0", tupget(N, 0));
    for (i = 1; i < tupsize(N); i++)
      fprintf(stderr, ", %u.0", tupget(N, i));
    fprintf(stderr, "], %.3lf) ==> %s\n", L,
      jl_typeof_str(jl_exception_occurred()));

    /* force the error to be printed. */
    fflush(stderr);

    /* return an exception status. */
    ret = EVAL_EXCEPTION;
  }
  else {
    /* unbox the computed result. */
    gx = jl_unbox_float64(gval);

    /* check the sign of the result. */
    if (gx >= 0.0) {
      /* perform a deterministic update. */
      *x += gx;
    }
    else {
      /* perform a quasi-random update. */
      *x += evalpois(gx + 1.0);
    }
  }

  /* release the references to the function arguments. */
  JL_GC_POP();

  /* return the well-behaved flag. */
  return ret;
}

/* evalpdf(): compute the density function at a given grid point.
 *
 * arguments:
 *  @fx: pointer to the output density value.
 *  @x: pointer to the current grid index.
 *  @N: total size of the Nyquist grid.
 *
 * returns:
 *  integer indicating whether evaluation succeeded (1) or not (0).
 */
int evalpdf (double *fx, tuple_t *x, tuple_t *N) {
  /* declare required variables:
   *  @i: general array index and loop counter.
   *  @ret: return status value for this function.
   */
  int i, ret = EVAL_OK;

  /* declare required julia variables:
   *  @boxfval: boxed return value of the method call.
   *  @args: array of value pointers passed to the method.
   *  @arrtype: data type of the arrays passed to the method.
   *  @arrx, @arrn: arrays passed to the method.
   *  @datx, @datn: array data pointers.
   */
  jl_value_t *boxfval;
  jl_value_t **args;
  jl_value_t *arrtype;
  jl_array_t *arrx, *arrn;
  double *datx, *datn;

  /* initialize the array data type. */
  arrtype = jl_apply_array_type((jl_value_t*) jl_float64_type, 1);

  /* allocate the origin and size arrays. */
  arrx = jl_alloc_array_1d(arrtype, tupsize(x));
  arrn = jl_alloc_array_1d(arrtype, tupsize(N));

  /* access the origin and size array data pointers. */
  datx = (double*) jl_array_data(arrx, double);
  datn = (double*) jl_array_data(arrn, double);

  /* fill the origin and size arrays. */
  for (i = 0; i < tupsize(x); i++) {
    datx[i] = (double) tupget(x, i);
    datn[i] = (double) tupget(N, i);
  }

  /* initialize the argument array. */
  JL_GC_PUSHARGS(args, 2);

  /* construct an argument array for the method call. */
  args[0] = (jl_value_t*) arrx;
  args[1] = (jl_value_t*) arrn;

  /* call the gap equation with the current arguments. */
  boxfval = jl_call(evalfn, args, 2);

  /* check if an exception occurred. */
  if (jl_exception_occurred()) {
    /* output an error. */
    fprintf(stderr, "error: f([%u.0",
      tupget(x, 0));
    for (i = 1; i < tupsize(x); i++)
      fprintf(stderr, ", %u.0", tupget(x, i));
    fprintf(stderr, "], [%u.0", tupget(N, 0));
    for (i = 1; i < tupsize(N); i++)
      fprintf(stderr, ", %u.0", tupget(N, i));
    fprintf(stderr, "]) ==> %s\n",
      jl_typeof_str(jl_exception_occurred()));

    /* force the error to be printed. */
    fflush(stderr);

    /* zero the computed result. */
    *fx = 0.0;

    /* return an exception status. */
    ret = EVAL_EXCEPTION;
  }
  else {
    /* unbox the computed result. */
    *fx = jl_unbox_float64(boxfval);
  }

  /* release the references to the function arguments. */
  JL_GC_POP();

  /* return the evaluation status. */
  return ret;
}
