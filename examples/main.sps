&insert x = 5;

function main(void)
{
  system.output(x);
  if (x > 3) {
    system.warning("x is large");
  } else {
    system.error("x too small");
  }
}
