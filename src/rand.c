// Thank you, @Roddy: http://stackoverflow.com/questions/7602919/how-do-i-generate-random-numbers-without-rand-function
unsigned short lfsr = 0xACE1u;
unsigned bit;

unsigned myrand()
{
  bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
  return lfsr =  (lfsr >> 1) | (bit << 15);
}
