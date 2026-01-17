namespace Math {
  &insert base = 10;
  function show(void) {
    system.output(base);
  }
}

enum Color { RED = 1, GREEN, BLUE = 4 }

function main(void)
{
  system.output(Math.base);
  Math.show();
  system.output(Color.RED);
  system.output(Color.GREEN);
  system.output(Color.BLUE);
}
