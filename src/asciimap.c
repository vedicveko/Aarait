/********************************************************************\
*   History: Developed by: mlkesl@stthomas.edu			     *
*                     and: mlk                                       *
*   MapArea: when given a room, ch, x, and y                         *
*            this function will fill in values of map as it should   *
*   ShowMap: will simply spit out the contents of map array          *
*	    Would look much nicer if you built your own areas        *
*	    without all of the overlapping stock Rom has             *
*   do_map: core function, takes map size as argument                *
*   update: map is now a 2 dimensional array of integers             *
*	    uses SECT_MAX for null                                   *
*	    uses SECT_MAX+1 for mazes or one ways to SECT_ENTER	     *
*	    use the SECTOR numbers to represent sector types :)	     *
*                                                                    *
\********************************************************************/
 
/**********************************************************\
* WELCOME TO THE WORLD OF CIRCLEMUD ASCII MAP!!!           *
*  This was originall for Rom(who would use that?) and is  *
*  now available for CircleMUD. This was done on bpl17 but *
*  the only major conversions were room structures so it   *
*  should work on any Circle with fairly up to date rooms. *
*                                                          *
*  If you use this drop me a line maybe, and if your       *
*  generous put me in a line in your credits. Doesn't      *
*  matter though, ENJOY! Feel free to fix any bugs here    *
*  make sure you mail the CircleMUD discussion list to let *
*  everyone know!                                          *
*                                                          *
*  Edward Felch, efelch@hotmail.com  4/25/2001             *
\**********************************************************/

/************************************************************************************\ 
  Notes:
  - You will need more cool sector types!
  - In act.informative.c put in checks for if a room is flagged
    wilderness, if it is: do_map(ch, " "); instead of the normal
    room name and description sending.
  - #define SECT_MAX 22 should be the max number of sector types + 1
    when I tried to include oasis.h for this it gave me errors so I
    just used the number 22.
  - In utils.h I added this in:
    #define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
  - Edit XXX.zon so its top room is real high or something and use
     that as a masssively large world map type file
  - We have a ROOM_NOVIEW flag as well as a ROOM_NOENTER flag (used in act.movement.c)
\************************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"

#define MAX_MAP 80 
// 72 
#define MAX_MAP_DIR 4
#define SECT_MAX 22

extern struct room_data *world;

int map[MAX_MAP][MAX_MAP];
int offsets[4][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0, -1} };

/* Heavily modified - Edward */
void MapArea(room_rnum room, struct char_data *ch, int x, int y, int min, int max)
{
  room_rnum prospect_room;
  struct room_direction_data *pexit;
  int door;

  /* marks the room as visited */
  map[x][y] = world[room].sector_type;

  /* Otherwise we get a nasty crash */
  if (IS_SET(world[ch->in_room].room_flags, ROOM_NOVIEW))
    return;

  for ( door = 0; door < MAX_MAP_DIR; door++ ) {
      if ( (pexit = world[room].dir_option[door]) > 0  &&   
           (pexit->to_room > 0 ) &&
	   (!IS_SET(pexit->exit_info, EX_CLOSED)))
        {
          if ( (x < min) || ( y < min) || ( x > max ) || ( y >max) ) return;
          prospect_room = pexit->to_room;
         
          /* one way into area OR maze */	
          /* if not two way */
          if ( world[prospect_room].dir_option[rev_dir[door]] &&
	     world[prospect_room].dir_option[rev_dir[door]]->to_room != room)
          { 
	    map[x][y] = SECT_MAX + 1;
	    return;
          }
          /* end two way */
          /* players cant see past these */
          if ( (world[prospect_room].sector_type == SECT_ROCK_MOUNTAIN) 
	        ||(world[prospect_room].sector_type == SECT_SNOW_MOUNTAIN) 
//	        ||(world[prospect_room].sector_type == SECT_CITY) 
//	        ||(world[prospect_room].sector_type == SECT_INSIDE)
                ||(IS_NIGHT && GET_LEVEL(ch) < LVL_IMMORT)
                ||(IS_SET(world[prospect_room].room_flags, ROOM_DARK) 
                && GET_LEVEL(ch) < LVL_IMMORT)
                || IS_SET(world[prospect_room].room_flags, ROOM_NOVIEW))
	  { 
	    map[x+offsets[door][0]][y+offsets[door][1]] = world[prospect_room].sector_type;
            /* ^--two way into area */		
	  }

         if ( map[x+offsets[door][0]][y+offsets[door][1]] == SECT_MAX ) {
           MapArea(pexit->to_room,ch,x + offsets[door][0], y + offsets[door][1], min, max);
         }
      } /* end if exit there */
  }
  return;
}

/* mlk :: shows a map, specified by size */
void ShowMap( struct char_data* ch, int min, int max)
{
  int x, y;

  /* every row */
  for (x = min; x < max; ++x) { 
    /* every column */
    for (y = min; y < max; ++y) { 
        if ( (y==min) || (map[x][y-1]!=map[x][y]) ) {
          switch(map[x][y]) {
            case SECT_MAX:	  send_to_char(" ",ch);		break;
            case SECT_FOREST:	  send_to_char("/cg@",ch);	break;
            case SECT_FIELD:	  send_to_char("/cG\"",ch);	break;
            case SECT_HILLS:	  send_to_char("/cG^",ch);	break;
            case SECT_UNDERWATER: send_to_char("/cc:",ch);break;

            case SECT_ROAD:	  send_to_char("/cyx",ch);	break;
            case SECT_MOUNTAIN:	  send_to_char("/cy^",ch);	break;
            case SECT_WATER_SWIM: send_to_char("/cB~",ch);	break;
            case SECT_WATER_NOSWIM:send_to_char("/cb~",ch);	break;
            case SECT_BLANK:	  send_to_char(" ",ch);		break;
            case SECT_INSIDE:	  send_to_char("/cW%",ch);	break;
            case SECT_CITY:	  send_to_char("/cW#",ch);	break;
            case SECT_ROCK_MOUNTAIN:send_to_char("/cR^",ch);	break;
            case SECT_SNOW_MOUNTAIN:send_to_char("/cW^",ch);	break;
            case SECT_RUINS:	  send_to_char("/cg#",ch);	break;
            case SECT_JUNGLE:	  send_to_char("/cg&",ch);	break;
            case SECT_SWAMP:	  send_to_char("/cG%",ch);	break;
            case SECT_LAVA:	  send_to_char("/cR\"",ch);	break;
            case SECT_BRIDGE:	  send_to_char("/cW:",ch);	break;
            case SECT_FARM:	  send_to_char("/cy=",ch);	break;
            case SECT_ACORN:	  send_to_char("/cg@",ch);	break;
            case SECT_EMPTY:	  send_to_char(" ",ch);		break;
            case (SECT_MAX+1):	  send_to_char("/cr?",ch);	break;
            default: 		  send_to_char("/cR*",ch);	break;
          }
        }
        else {
          switch(map[x][y]) {
	    case SECT_MAX:	send_to_char(" ",ch);		break;
	    case SECT_FOREST:	send_to_char("@",ch);		break;
	    case SECT_FIELD:	send_to_char("\"",ch);		break;
	    case SECT_HILLS:	send_to_char("^",ch);		break;
               case SECT_UNDERWATER:    send_to_char(":",ch);break;

	    case SECT_ROAD:	send_to_char("x",ch);		break;
	    case SECT_MOUNTAIN:	send_to_char("^",ch);		break;
	    case SECT_WATER_SWIM:send_to_char("~",ch);		break;
	    case SECT_WATER_NOSWIM:send_to_char("~",ch);	break;
	    case SECT_BLANK:	send_to_char(" ",ch);		break;
	    case SECT_INSIDE:	send_to_char("%",ch);		break;
	    case SECT_CITY:	send_to_char("#",ch);		break;
	    case SECT_ROCK_MOUNTAIN:send_to_char("^",ch);	break;
	    case SECT_SNOW_MOUNTAIN:send_to_char("^",ch);	break;
	    case SECT_RUINS:	send_to_char("#",ch);		break;
	    case SECT_JUNGLE:	send_to_char("&",ch);		break;
	    case SECT_SWAMP:	send_to_char("%",ch);		break;
	    case SECT_LAVA:	send_to_char("\"",ch);		break;
	    case SECT_BRIDGE:	send_to_char(":",ch);		break;
	    case SECT_FARM:	send_to_char("=",ch);		break;
	    case SECT_ACORN:	send_to_char("@",ch);		break;
	    case SECT_EMPTY:	send_to_char(" ",ch);		break;
	    case (SECT_MAX+1):	send_to_char("?",ch);		break;
	    default: 		send_to_char("*",ch);
          } 
        }
    }
    send_to_char("\n\r",ch); 
  }
  return;
}


/* This is for simplicities sake, if the room's name if
** like An unfinished room, it will replace it with a fake
** room name here - Edward 
*/
void get_room_name(struct char_data *ch, int argument)
{
   switch(argument) {
       case SECT_MAX:
           sprintf(buf,"The Wilderness");break;
       case SECT_FOREST:
           sprintf(buf,"With in a Forest");break;
       case SECT_FIELD:
           sprintf(buf,"On an Open Field");break;
       case SECT_HILLS:
           sprintf(buf,"On a Hill");break;
       case SECT_ROAD:
           sprintf(buf,"On a Road");break;
       case SECT_MOUNTAIN:
           sprintf(buf,"Climbing a Mountain");break;
       case SECT_WATER_SWIM:
           sprintf(buf,"In Shallow Water");break;
       case SECT_WATER_NOSWIM:
           sprintf(buf,"Adrift In Deep Water");break;
       case SECT_BLANK:
           sprintf(buf,"The Wilderness");break;
       case SECT_INSIDE:
           sprintf(buf,"The Wilderness");break;
       case SECT_CITY:
           sprintf(buf,"With in a City");break;
       case SECT_ROCK_MOUNTAIN:
           sprintf(buf,"On a Rocky Mountain");break;
       case SECT_SNOW_MOUNTAIN:
           sprintf(buf,"On a Snowy Mountain");break;
       case SECT_RUINS:
           sprintf(buf,"At Some Ancient Ruins");break;
       case SECT_JUNGLE:
           sprintf(buf,"Deep with in a Jungle");break;
       case SECT_SWAMP:
           sprintf(buf,"With in a Murky Swamp");break;
       case SECT_LAVA:
           sprintf(buf,"In a Lava Flow");break;
       case SECT_BRIDGE:
           sprintf(buf,"A Bridge");break;
       case SECT_FARM:
           sprintf(buf,"Farmlands");break;
       case SECT_ACORN:
           sprintf(buf,"In An Acorn Forest");break;
       case SECT_EMPTY:
           sprintf(buf,"The Wilderness");break;
       case (SECT_MAX+1):
           sprintf(buf,"The Wilderness");break;
       default:
           sprintf(buf,"The Wilderness");break;
  }
  send_to_char(buf,ch);
}

/* will put a small map with current room desc and title */
/* this is the main function to show the map, its do_map with " " */

/* Heavily modified - Edward */
void ShowRoom(struct char_data *ch, int min, int max)
{
  int x, y; //, str_pos = 0, desc_pos = 0, start;
//  char buf[500];
  char desc[500];
//  char line[100];

  strcpy(desc, world[ch->in_room].description);
  /* mlk :: rounds edges */
//  map[min][min]=SECT_MAX;map[max-1][max-1]=SECT_MAX;
//  map[min][max-1]=SECT_MAX;map[max-1][min]=SECT_MAX;

  /* every row */
  for (x = min; x < max; ++x) { 
    /* every column */
    for (y = min; y < max; ++y) { 
        if ( (y==min) || (map[x][y-1]!=map[x][y]) )
           switch(map[x][y]) {
	     case SECT_MAX:		send_to_char(" ",ch);	break;
	     case SECT_FOREST:	send_to_char("/cg@",ch);		break;
	     case SECT_FIELD:	send_to_char("/cG\"",ch);	break;
	     case SECT_HILLS:	send_to_char("/cG^",ch);		break;
             case SECT_UNDERWATER:    send_to_char("/cc:",ch);break;
	     case SECT_ROAD:		send_to_char("/cyx",ch);	break;
	     case SECT_MOUNTAIN:	send_to_char("/cy^",ch);	break;
	     case SECT_WATER_SWIM:	send_to_char("/cB~",ch);	break;
	     case SECT_WATER_NOSWIM:	send_to_char("/cb~",ch);	break;
	     case SECT_BLANK:	send_to_char(" ",ch);		break;
	     case SECT_INSIDE:	send_to_char("/cW%",ch);		break;
	     case SECT_CITY:	send_to_char("/cW#",ch);	break;
	     case SECT_ROCK_MOUNTAIN:send_to_char("/cR^",ch);	break;
	     case SECT_SNOW_MOUNTAIN:send_to_char("/cW^",ch);	break;
	     case SECT_RUINS:	send_to_char("/cg#",ch);		break;
	     case SECT_JUNGLE:	send_to_char("/cg&",ch);	break;
	     case SECT_SWAMP:	send_to_char("/cG%",ch);		break;
	     case SECT_LAVA:	send_to_char("/cr\"",ch);	break;
	     case SECT_BRIDGE:	send_to_char("/cW:",ch);		break;
	     case SECT_FARM:	send_to_char("/cy=",ch);		break;
	     case SECT_ACORN:	send_to_char("/cg@",ch);		break;
	     case SECT_EMPTY:	send_to_char(" ",ch);		break;
	     case (SECT_MAX+1):	send_to_char("/cD?",ch);		break;
	     default: 		send_to_char("/cR*",ch);		break;
           }
           else 
             switch(map[x][y]) {
	       case SECT_MAX:	send_to_char(" ",ch);		break;
	       case SECT_FOREST:send_to_char("@",ch);		break;
	       case SECT_FIELD:	send_to_char("\"",ch);		break;
	       case SECT_HILLS:	send_to_char("^",ch);		break;
               case SECT_UNDERWATER:    send_to_char(":",ch);break;

	       case SECT_ROAD:	send_to_char("x",ch);		break;
	       case SECT_MOUNTAIN:	send_to_char("^",ch);	break;
	       case SECT_WATER_SWIM:	send_to_char("~",ch);	break;
	       case SECT_WATER_NOSWIM:	send_to_char("~",ch);	break;
	       case SECT_BLANK:		send_to_char(" ",ch);	break;
	       case SECT_INSIDE:	send_to_char("%",ch);	break;
	       case SECT_CITY:		send_to_char("#",ch);	break;
	       case SECT_ROCK_MOUNTAIN:send_to_char("^",ch);	break;
	       case SECT_SNOW_MOUNTAIN:send_to_char("^",ch);	break;
	       case SECT_RUINS:		send_to_char("#",ch);	break;
	       case SECT_JUNGLE:	send_to_char("&",ch);	break;
	       case SECT_SWAMP:		send_to_char("%",ch);	break;
	       case SECT_LAVA:		send_to_char("\"",ch);	break;
	       case SECT_BRIDGE:	send_to_char(":",ch);	break;
	       case SECT_FARM:		send_to_char("=",ch);	break;
	       case SECT_ACORN:		send_to_char("@",ch);	break;
	       case SECT_EMPTY:		send_to_char(" ",ch);	break;
	       case (SECT_MAX+1):	send_to_char("?",ch);	break;
	       default: 		send_to_char("*",ch);	break;
 	     }
    }
/*
    if (x == min) {
      if (!strcmp(world[ch->in_room].name, "The Wilderness") ||
          !strcmp(world[ch->in_room].name, "Room") || 
          !strcmp(world[ch->in_room].name, "An unfinished room"))
         get_room_name(ch, world[ch->in_room].sector_type);
      else {
         sprintf(buf,"/c0   %s",world[ch->in_room].name);
         send_to_char(buf,ch);
      }
      
    }*/
/* I removed the room descriptions from the ascii map, the map is descriptive enough
** I think - Edward. Just remove the comments if you want them back. 

    else {
        start = str_pos;
        for (desc_pos = desc_pos ; desc[desc_pos]!='\0' ; desc_pos++) { 
          if (desc[desc_pos]=='\n') {
            line[str_pos-start]='\0';
            str_pos += 3;
            desc_pos += 2;
	    break;
	  }
          else if (desc[desc_pos]=='\r') {
	    line[str_pos-start]='\0';
            str_pos += 2;
	    break;
	  }
          else {
	    line[str_pos-start]=desc[desc_pos];
	    str_pos += 1;
	  }
        }
        line[str_pos-start]='\0';
        if (x == min + 1) send_to_char("  ", ch);
        send_to_char("   /c0", ch);
        send_to_char(line, ch);
        send_to_char("/c0",ch);
    }
*/
    send_to_char("\n\r",ch); 
  } 
  send_to_char("\n\r",ch);  /* puts a line between contents/people */
  return;
}

/* This is the main map function, do_map(ch, " ") is good to use */
/* do_map(ch "number") is for immortals to see the world map     */

/* Edward: If you play with some of the values here you can make the normal
** map people see larger or smaller. size = URANGE(9, size, MAX_MAP), the 9
** is the map size shown by default. Also look for: ShowMap (ch, min, max+1);
** and change the size of min and max and see what you like.
*/
void make_big_map( struct char_data *ch, char *argument )
{
  int size, center, x, y, min, max;
  char arg1[10];
  one_argument(argument, arg1);
  size = atoi(arg1);
  size = URANGE(12, size, MAX_MAP);

  center = MAX_MAP/2;

  min = MAX_MAP/2 - size/2;
  max = MAX_MAP/2 + size/2;
// MAX_MAP
  for (x = 0; x < MAX_MAP; ++x)
      for (y = 0; y < MAX_MAP; ++y)
           map[x][y]=SECT_MAX;

  /* starts the mapping with the center room */
  MapArea(ch->in_room, ch, center, center, min-1, max-1); 
  /* marks the center, where ch is */
  map[center][center] = SECT_MAX+2;  

     /* can be any number above SECT_MAX+1 	*/
    /* switch default will print out the *	*/

/*
  if ( (GET_LEVEL(ch) < LVL_IMMORT)||(IS_NPC(ch)) ) {
     if (IS_SET(world[ch->in_room].room_flags, ROOM_NOVIEW) ) {
       send_to_char("You can not do that here.\n\r",ch);
       return;
     }
       if (IS_DARK(ch->in_room) ) {
          send_to_char( "The wilderness is pitch black at night... \n\r", ch );
          return;
       }
       else {
           ShowMap(ch, min, max+1);
         ShowRoom(ch,MAX_MAP/2-3,MAX_MAP/2+3);
         return;
       }
    } */
    /* mortals not in city, enter or inside will always get a ShowRoom */
    ShowMap(ch, min, max+1);
   return;
}

ACMD(do_bigmap) {

 make_big_map(ch, "100");

}

