# SharpScript math library
# Usage:
#   #include "lib/math.h"
#   system.output(clamp(5, 1, 3));
#   system.output(sum([1,2,3]));
#   system.output(avg([2,4]));

function clamp(x, lo, hi)
{
  if (x < lo) {
    return lo;
  } else if (x > hi) {
    return hi;
  }
  return x;
}

function sum(arr)
{
  &insert i = 0;
  &insert s = 0;
  while (i < system.len(arr)) {
    s = s + arr[i];
    i = i + 1;
  }
  return s;
}

function avg(arr)
{
  &insert n = system.len(arr);
  if (n == 0) return 0;
  return sum(arr) / n;
}

function pow2(x)
{
  return system.pow(x, 2);
}
