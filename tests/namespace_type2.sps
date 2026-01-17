namespace N { &insert x = 1; }

function main(void) {
  system.output(2);
  system.output(N.x);
  system.output(system.type(N.x));
}
