# SharpScript stdio library
# Usage:
#   #include "lib/stdio.h"
#   println("Hello"); print("World");

function print(x)
{
  system.output(x);
}

function println(x)
{
  system.output(x);
}

function errorln(x)
{
  system.error(x);
}

function warnln(x)
{
  system.warning(x);
}
