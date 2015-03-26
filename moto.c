#include <stdio.h>
#include <time.h>
#include <wiringPi.h>

int port[2][4] = {{9, 7, 15, 16}, {1, 2, 3, 4}};

void setValue(int v, int id, int r)
{
  int i;
  unsigned char f_rotation[] = {1, 3, 2, 6, 4, 12, 8, 9};

  if (r == 0)
    v = f_rotation[v % 8];      // clockwise
  else
    v = f_rotation[7 - (v % 8)];  // anticlockwise

  for (i = 0; i < 4; i++)
  {
    digitalWrite (port[id][i], (v >> i) & 1);
  }
}

int main (int argc, char* argv[])
{
  int i;
  int round = 360;
  int port_id = 0;
  time_t rawtime;

  time (&rawtime);

  if (argc > 1) {
    port_id = atoi(argv[1]);
    round = atoi(argv[2]) * 360;
  }

  printf ("%s : Moto will rotate %d with %d degree\n", ctime (&rawtime), port_id, round) ;

  wiringPiSetup () ;
  for (i = 0; i < 4; i++)
  { 
    pinMode (port[port_id][i], OUTPUT) ;
  }

  for (i = 0; i < (round*64/5.625); i++)
  {
    setValue(i, port_id, 1);
    delay (1) ;               // mS
  }
  return 0 ;
}

