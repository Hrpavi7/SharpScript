function main(void)
{
  system.history.add(3);
  system.history.add(4);
  system.output(system.len(system.history.get()));
  system.history.clear();
  system.output(system.len(system.history.get()));
}
