/* setgalaxy.c -- new tool, supercedes resetplanets

   usage:
   setgalaxy l              restore planet locations
   setgalaxy r              standard reset of galaxy
   setgalaxy t              tourney reset of galaxy - equal agris
   setgalaxy f              flatten all planets to 1 army
   setgalaxy F              top out all planets at START_ARMIES
   setgalaxy n <num>:<str>  rename planet <num> to <str>
   setgalaxy C              cool server idea from felix@coop.com 25 Mar 1994
   setgalaxy Z              close up shop for maintenance
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/file.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <pwd.h>
#include <ctype.h>
#include "defs.h"
#include INC_STRINGS
#include "struct.h"
#include "planets.h"
#include "data.h"

extern int openmem(int);

static void usage(void);
static void CoolServerIdea(void);
static void CloseUpShop(void);
static void doINLResources(void);
static void doResources(void);

static int agricount[4] = { 0, 0, 0, 0};
#define AGRI_LIMIT	3

int main(int argc, char **argv)
{
    int i;
    int top_armies = START_ARMIES;

    srandom(getpid());
    openmem(0);
    if (argc == 1) usage();

    if (argc == 3) {
      if ((*argv[1] == 'F') || (*argv[1] == 'r') || (*argv[1] == 't')) 
      top_armies = atoi(argv[2]);
      else {

	int num;
	char name[NAME_LEN];
	if (sscanf(argv[2], "%d:%[^\n]", &num, name)==2) {
	    if ((num >= 0) && (num <= 39)) {
		printf("Renaming planet #%d to %s.\n", num, name);
		strcpy(planets[num].pl_name, name);
		planets[num].pl_namelen = strlen(name);
		planets[num].pl_flags |= PLREDRAW;
	    }
	    else
		printf("Planet number must be in range (0-39).\n");
	    exit(0);
	}
	else usage();
      }
    }
 
    if (*argv[1] == 'C') {
        CoolServerIdea();
        exit(0);
    }

    if (*argv[1] == 'Z') {
        CloseUpShop();
        exit(0);
    }

    if (*argv[1] == 'l') {
	for (i = 0; i < MAXPLANETS; i++) {
	    planets[i].pl_x = pdata[i].pl_x;
	    planets[i].pl_y = pdata[i].pl_y;
	}
	printf("Restored locations.\n");
	exit(0);
    }

    if (*argv[1] == 'f') { /* flatten planets */
	for (i = 0; i < 40; i++) {
	    planets[i].pl_armies = 1;
	}
	printf("All planets set to 1 army.\n");
    }
    else if (*argv[1] == 'F') { /* top out planets */
	for (i = 0; i < 40; i++) {
	    planets[i].pl_armies = top_armies;
	}
	printf("All planets set to %d armies.\n",top_armies);
    }
    else if (*argv[1] == 't') { /* tourney reset resources, owners */
	MCOPY(pdata, planets, sizeof(pdata));
	for (i = 0; i < 40; i++) {
	    planets[i].pl_armies = top_armies;
	}
	doINLResources();
    /* reset the SB construction and surrender countdown immediately */
	for (i = 0; i <= MAXTEAM; i++) {
	  teams[i].s_turns = 0;
	  teams[i].s_surrender = 0;
	}
    }
    else if (*argv[1] == 'r') { /* reset resources, owners */
	MCOPY(pdata, planets, sizeof(pdata));
	for (i = 0; i < 40; i++) {
	    planets[i].pl_armies = top_armies;
	}
	doResources();
	printf("Agri counts: %d/%d/%d/%d.\n", agricount[0],
	       agricount[1], agricount[2], agricount[3]);
    /* reset the SB construction and surrender countdown immediately */
	for (i = 0; i <= MAXTEAM; i++) {
	  teams[i].s_turns = 0;
	  teams[i].s_surrender = 0;
	}
    }
    else usage();
    return 1;		/* satisfy lint */
}

/* the four close planets to the home planet */
static int core_planets[4][4] =
{{ 7, 9, 5, 8,},
 { 12, 19, 15, 16,},
 { 24, 29, 25, 26,},
 { 34, 39, 38, 37,},
};
/* the outside edge, going around in order */
static int front_planets[4][5] =
{{ 1, 2, 4, 6, 3,},
 { 14, 18, 13, 17, 11,},
 { 22, 28, 23, 21, 27,},
 { 31, 32, 33, 35, 36,},
};

static void doINLResources(void)
{
  int i, j, k, which;
  for (i = 0; i < 4; i++){
    /* one core AGRI */
    planets[core_planets[i][random() % 4]].pl_flags |= PLAGRI;

    /* one front AGRI */
    which = random() % 2;
    if (which){
      planets[front_planets[i][random() % 2]].pl_flags |= PLAGRI;

      /* place one repair on the other front */
      planets[front_planets[i][(random() % 3) + 2]].pl_flags |= PLREPAIR;

      /* place 2 FUEL on the other front */
      for (j = 0; j < 2; j++){
      do {
        k = random() % 3;
      } while (planets[front_planets[i][k + 2]].pl_flags & PLFUEL) ;
      planets[front_planets[i][k + 2]].pl_flags |= PLFUEL;
      }
    } else {
      planets[front_planets[i][(random() % 2) + 3]].pl_flags |= PLAGRI;

      /* place one repair on the other front */
      planets[front_planets[i][random() % 3]].pl_flags |= PLREPAIR;

      /* place 2 FUEL on the other front */
      for (j = 0; j < 2; j++){
      do {
        k = random() % 3;
      } while (planets[front_planets[i][k]].pl_flags & PLFUEL);
      planets[front_planets[i][k]].pl_flags |= PLFUEL;
      }
    }

    /* drop one more repair in the core (home + 1 front + 1 core = 3 Repair)*/
    planets[core_planets[i][random() % 4]].pl_flags |= PLREPAIR;

    /* now we need to put down 2 fuel (home + 2 front + 2 = 5 fuel) */
    for (j = 0; j < 2; j++){
      do {
      k = random() % 4;
      } while (planets[core_planets[i][k]].pl_flags & PLFUEL);
      planets[core_planets[i][k]].pl_flags |= PLFUEL;
    }
  }
}


static void doResources(void)
{
    int i;

    do {
        agricount[0] = 0;
        agricount[1] = 0;
        agricount[2] = 0;
        agricount[3] = 0;

      for (i = 0; i < 40; i++) {
          /*  if (planets[i].pl_flags & PLHOME)
              planets[i].pl_flags |= (PLREPAIR|PLFUEL|PLAGRI);*/
          if (random() % 4 == 0)
              planets[i].pl_flags |= PLREPAIR;
          if (random() % 2 == 0)
              planets[i].pl_flags |= PLFUEL;
          if (random() % 8 == 0) {
              planets[i].pl_flags |= PLAGRI; agricount[i/10]++;
          }
        }
    } while ((agricount[0] > AGRI_LIMIT) || /* bug: used && 1/23/92 TC */
           (agricount[1] > AGRI_LIMIT) ||
           (agricount[2] > AGRI_LIMIT) ||
           (agricount[3] > AGRI_LIMIT));
}


static void usage(void)
{
    printf("   usage:\n");
    printf("   setgalaxy l              restore planet locations\n");
    printf("   setgalaxy r (num)        standard reset of galaxy\n");
    printf("   setgalaxy t (num)        tourney reset of galaxy - equal agris\n");
    printf("   setgalaxy f              flatten all planets to 1 army\n");
    printf("   setgalaxy F (num)        top out all planets at (num) armies\n");
    printf("   setgalaxy n <num>:<str>  rename planet <num> to <str>\n");
    printf("   setgalaxy C              triple planet mayhem\n");
    exit(0);
}

static void CoolServerIdea(void)
{
    int i;

    for (i=0; i<MAXPLANETS; i++)
    {
      planets[i].pl_flags = 0;
      planets[i].pl_owner = 0;
      planets[i].pl_x = -10000;
      planets[i].pl_y = -10000;
      planets[i].pl_info = 0;
      planets[i].pl_armies = 0;
      strcpy ( planets[i].pl_name, "" );
    }

    planets[20].pl_couptime = 999999; /* no Klingons */
    planets[30].pl_couptime = 999999; /* no Orions */

    /* earth */
    i = 0;
    planets[i].pl_flags |= FED | PLHOME | PLCORE | PLAGRI | PLFUEL | PLREPAIR;
    planets[i].pl_x = 40000;
    planets[i].pl_y = 65000;
    planets[i].pl_armies = 40;
    planets[i].pl_info = FED;
    planets[i].pl_owner = FED;
    strcpy ( planets[i].pl_name, "Earth" );

    /* romulus */
    i = 10;
    planets[i].pl_flags |= ROM | PLHOME | PLCORE | PLAGRI | PLFUEL | PLREPAIR;
    planets[i].pl_x = 40000;
    planets[i].pl_y = 35000;
    planets[i].pl_armies = 40;
    planets[i].pl_info = ROM;
    planets[i].pl_owner = ROM;
    strcpy ( planets[i].pl_name, "Romulus" );

    /* indi */
    i = 18;
    planets[i].pl_flags |= PLFUEL | PLREPAIR;
    planets[i].pl_flags &= ~PLAGRI;
    planets[i].pl_x = 15980;
    planets[i].pl_y = 50000;
    planets[i].pl_armies = 4;
    planets[i].pl_info &= ~ALLTEAM;
    strcpy ( planets[i].pl_name, "Indi" );

    for (i=0; i<MAXPLANETS; i++)
    {
        planets[i].pl_namelen = strlen(planets[i].pl_name);
    }
}

static void CloseUp(int i) {
  int m, dx, dy, t = 200;
  dx = (planets[i].pl_x - 50000)/t;
  dy = (planets[i].pl_y - 50000)/t;
  for(m=0; m<t; m++) {
    planets[i].pl_x -= dx;
    planets[i].pl_y -= dy;
    usleep(100000);
  }
  planets[i].pl_x = -10000;
  planets[i].pl_y = -10000;
  planets[i].pl_flags = 0;
  planets[i].pl_owner = 0;
  planets[i].pl_info = 0;
  planets[i].pl_armies = 0;
  strcpy ( planets[i].pl_name, "" );
  planets[i].pl_namelen = strlen(planets[i].pl_name);
  
}

static void CloseUpShop2(void)
{
  int x, y;

  for (y=0; y<5; y++) {
    for (x=0; x<4; x++) {
      CloseUp(front_planets[x][y]);
    }
    sleep(2);
  }
  for (y=0; y<4; y++) {
    for (x=0; x<4; x++) {
      CloseUp(core_planets[x][y]);
    }
    sleep(2);
  }
  CloseUp(0);
  CloseUp(10);
  CloseUp(20);
  CloseUp(30);
}

static void CloseUpShop() {
  int i, m, dx[MAXPLANETS], dy[MAXPLANETS], t = 600;

  for(i=0; i<MAXPLANETS; i++) {
    dx[i] = (planets[i].pl_x - 50000)/t;
    dy[i] = (planets[i].pl_y - 50000)/t;
  }

  for(m=0; m<t; m++) {
    for(i=0; i<MAXPLANETS; i++) {
      planets[i].pl_x -= dx[i];
      planets[i].pl_y -= dy[i];
    }
    if (m == (t-100)) {
      for(i=0; i<MAXPLANETS; i++) {
	planets[i].pl_flags = 0;
	planets[i].pl_owner = 0;
	planets[i].pl_info = ALLTEAM;
	planets[i].pl_armies = 0;
	strcpy ( planets[i].pl_name, "   " );
	planets[i].pl_namelen = strlen(planets[i].pl_name);
      }
    }
    usleep(200000);
  }

  for(i=0; i<MAXPLANETS; i++) {
    planets[i].pl_x = -20000;
    planets[i].pl_y = -20000;
  }

  sleep(2);

  for(i=0; i<MAXPLAYER; i++) {
    players[i].p_team = 0;
    players[i].p_hostile = ALLTEAM;
    players[i].p_swar = ALLTEAM;
    players[i].p_war = ALLTEAM;
    sprintf(players[i].p_mapchars, "%c%c", 
	    teamlet[players[i].p_team], shipnos[i]);
    sprintf(players[i].p_longname, "%s (%s)", 
	    players[i].p_name, players[i].p_mapchars);
  }

  /* don't let them come back? */
  planets[0].pl_couptime = 999999;
  planets[10].pl_couptime = 999999;
  planets[20].pl_couptime = 999999;
  planets[30].pl_couptime = 999999;
}

