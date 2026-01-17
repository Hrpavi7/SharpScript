function main(void)
{
  system.store("a", 42);
  system.output(system.recall("a"));
  system.memclear();
  system.output(system.recall("a"));
}
