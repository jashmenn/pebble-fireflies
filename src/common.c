// Thank you, @Roddy: http://stackoverflow.com/questions/7602919/how-do-i-generate-random-numbers-without-rand-function
unsigned short lfsr = 0xACE1u;
unsigned bit;

// returns a number between 0 - 65535 (?)
unsigned lfsr_rand()
{
  bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
  return lfsr =  (lfsr >> 1) | (bit << 15);
}

long linearmap(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// random number between low inclusive, high exclusive
int rand_between(int low, int high) {
  return linearmap(lfsr_rand(), 0, 65535, low, high);
}

