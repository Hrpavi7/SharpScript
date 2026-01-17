namespace A {
  function f(void) { system.output(99); }
}

function main(void) {
  system.output(system.type(A.f));
  A.f();
}
