# nusutils

A set of command-line utilities for deterministically (_i.e._ without
pseudorandom numbers) constructing nonuniform sampling schedules on
multidimensional Nyquist grids. The **gaputil** is a utility to construct
schedules using the generalized gap sampling framework published in:

> Worley, B., Powers, R., _Deterministic Multidimensional Nonuniform
> Gap Sampling_, Journal of Magnetic Resonance, 2015, 261: 19-26.

The **rejutil** and **jitutil** are utilities to construct schedules from
density functions using a quasirandom accept-reject sampling method
published in:

> Worley, B., _Subrandom Methods for Multidimensional Nonuniform
> Sampling_, Journal of Magnetic Resonance, 2016, 269: 128-137.

## Introduction

Gap sampling came into vogue in the field of Nuclear Magnetic Resonance (NMR)
when Hyberts and Wagner introduced their Poisson-gap (PG) sampler for building
Nonuniform Sampling (NUS) schedules. This project is the software realization
of a full generalization of their PG method to admit _any_ gap equation.

Initial versions of **gaputil** contained hard-coded Poisson-gap, sine-gap,
and sine-burst (_cf._ Worley and Powers, above) gap equations. The generality
of the proposed framework was a tempting opportunity for me to embed the
[Julia programming language](http://julialang.org/) into one of my software
projects. By embedding Julia into my existing gap sampling algorithm, this
utility enables the construction of NUS schedules from _completely arbitrary_
gap equations.

### Examples using **gaputil**

The **gaputil** program reads a gap equation from the command line in string
form, passes it to Julia for just-in-time compilation, and then evaluates it
as needed until an optimal schedule is arrived at. The function prototype
used by **gaputil** looks like this in Julia:

```julia
g(x::Float64, d::Int32,
  O::Array{Float64,1},
  N::Array{Float64,1},
  L::Float64)
```

where **x** holds the current gap sequence term, **d** is the currently
sampled grid dimension, **O** is the current origin in the grid, **N**
is the size of the grid, and **L** is a scaling factor that **gaputil**
will optimize in its attempts to create a schedule of the correct
global sampling density.

The simplest possible gap equation merely uses the scaling factor,
resulting in a uniform lattice:

```julia
g(x, d, O, N, L) = L
```

The sine-gap equation is fairly simple as well:

```julia
g(x, d, O, N, L) =
  L * sin((pi/2) * (x + sum(O)) / sum(N))
```

The sine-burst equation gets a bit more complicated:

```julia
g(x, d, O, N, L) =
  L * sin((pi/2) * (x + sum(O)) / sum(N)) *
  sin((pi/4) * N[d] * (x + sum(O)) / sum(N))^2
```

In short, **gaputil** accepts almost any inline function having
Julia syntax.

### Examples using **rejutil** and **jitutil**

The **rejutil** and **jitutil** programs also use Julia to interpret their
density function, but this function is of a simpler form:

```julia
f(x::Array{Float64,1}, N::Array{Float64,1})
```

where **x** now holds the _current_ grid index, and **N** still holds the
total grid size. No optimization is performed: **rejutil** simply draws a
set number of points (grid indices) from the distribution specified in the
density function. On the other hand, **jitutil** draws the same number of
points using a jittered rejection sampling algorithm.

The simplest possible density function is a uniform grid:

```julia
f(x, N) = 1.0
```

An exponentially weighted function would look like this:

```julia
f(x, N) = exp(-sum(x ./ N))
```

Like **gaputil**, **rejutil** and **jitutil** will accept any inline
function having Julia syntax and conforming to the interface specified
above.

### Installing

You will need to have Julia compiled and installed in order to build
**nusutils**. It is recommended that you download the latest version
from your distribution's package manager. In my hands, v1.12.1 from
Homebrew works great.

Once Julia is installed into the path, you can compile and install
**gaputil**, **rejutil** and **jitutil** as follows:

```bash
git clone git://github.com/geekysuavo/nusutils.git
cd nusutils
make
sudo make install
```

## Licensing

This project is released under the [GNU GPL 2.0](LICENSE).
