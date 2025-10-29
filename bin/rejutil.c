
/* rejutil: quasirandom rejection sampling schedule generation utility.
 * Copyright (C) 2015 Bradley Worley <geekysuavo@gmail.com>.
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

/* include the main header. */
#include "rejutil.h"

/* main(): application entry point.
 *
 * arguments:
 *  @argc: number of command line arguments.
 *  @argv: command line argument string array.
 *
 * returns:
 *  integer representing whether execution terminated without error (0)
 *  or not (1).
 */
int main (int argc, char **argv) {
  /* declare variables to hold schedule parameters:
   *  @D: total number of Nyquist grid dimensions.
   *  @N: tuple holding the Nyquist grid sizes.
   *  @d: effective sampling density, in (0,1).
   */
  unsigned int D;
  tuple_t N;
  double d;

  /* declare variables to hold schedule values:
   *  @xlst: tuple of linear indices in the schedule.
   *  @xt: tuple to hold unpacked linear indices.
   */
  tuple_t xlst, xt;

  /* declare a general-purpose loop index variable:
   *  @i: loop counter and iteration index.
   */
  unsigned int i;

  /* determine the number of grid dimensions. */
  D = argc - 3;

  /* check that a supported number of dimensions was requested.
   *
   * the REJUTIL_DIMS_MIN and REJUTIL_DIMS_MAX preprocessor definitions,
   * located in rejutil.h, are used to soft-limit the number of dimensions
   * that the program will build sampling schedules upon.
   */
  if (D < REJUTIL_DIMS_MIN || D > REJUTIL_DIMS_MAX) {
    /* output a usage statement and return failure. */
    fprintf(stderr, REJUTIL_USAGE, argv[0]);
    return 1;
  }

  /* read in the sampling density. */
  d = atof(argv[1]);

  /* validate the sampling density. */
  if (d <= 0.0 || d >= 1.0) {
    /* output an error message and return failure. */
    fprintf(stderr, "error: sampling density must lie in (0,1)\n");
    return 1;
  }

  /* allocate the grid size tuple. */
  if (!tupalloc(&N, D) || !tupalloc(&xt, D)) {
    /* output an error message and return failure. */
    fprintf(stderr, "error: failed to allocate grid size tuple\n");
    return 1;
  }

  /* read in the grid sizes. */
  for (i = 0; i < tupsize(&N); i++) {
    /* read the currently indexed argument. */
    tupset(&N, i, atoi(argv[i + 2]));

    /* validate the grid size. */
    if (tupget(&N, i) == 0) {
      /* output an error message and return failure. */
      fprintf(stderr, "error: invalid N%u grid size\n", i + 1);
      return 1;
    }
  }

  /* initialize the julia interpreter. */
  jl_init();

  /* build the final schedule array. */
  if (!rej(argv[argc - 1], &N, d, &xlst)) {
    /* output an error message and return failure. */
    fprintf(stderr, "error: failed to compute output schedule\n");
    return 1;
  }

  /* print the final schedule values. */
  for (i = 0; i < tupsize(&xlst); i++) {
    /* unpack and print the current schedule value. */
    tupunpack(tupget(&xlst, i), &N, &xt);
    tupprint(&xt, stdout);
  }

  /* free the allocated tuples. */
  tupfree(&xlst);
  tupfree(&xt);
  tupfree(&N);

  /* return successfully. */
  evalfree();
  return 0;
}

