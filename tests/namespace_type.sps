namespace Math { function show(void) { system.output(7); } }

function main(void) {
  system.output(system.type(Math.show));
  system.output(system.type(Math.base));
  Math.show();
}
