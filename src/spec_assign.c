/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern int dts_are_dumps;
extern int mini_mud;
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;

SPECIAL(dump);
SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(guild_guard);
SPECIAL(guild);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(gen_board);
SPECIAL(butcher);
SPECIAL(pig_trader);
SPECIAL(bennyguard);
SPECIAL(employment_master);
SPECIAL(blacksmith);
SPECIAL(bank);
SPECIAL(questmaster);
SPECIAL(dynamite);
SPECIAL(ticket_clerk);
SPECIAL(livestock_butcher);

/* local functions */
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
  mob_rnum rnum;

  if ((rnum = real_mobile(mob)) >= 0)
    mob_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) >= 0)
    obj_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
  room_rnum rnum;

  if ((rnum = real_room(room)) >= 0)
    world[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
	ASSIGNMOB(1202, janitor);
        ASSIGNMOB(1200, receptionist);
	ASSIGNMOB(1201, postmaster);

//        ASSIGNMOB(111, receptionist); 
        

        ASSIGNMOB(194, cityguard);
        ASSIGNMOB(195, cityguard);
        ASSIGNMOB(196, cityguard);

        ASSIGNMOB(197, janitor);
        ASSIGNMOB(198, janitor);
        ASSIGNMOB(199, janitor); 

        ASSIGNMOB(1002, ticket_clerk); 

  /*      

        ASSIGNMOB(113, postmaster);
        ASSIGNMOB(115, butcher);      
        ASSIGNMOB(116, pig_trader);
        ASSIGNMOB(117, cityguard); 
        ASSIGNMOB(118, cityguard);
        ASSIGNMOB(125, employment_master);
        ASSIGNMOB(133, blacksmith); 
        ASSIGNMOB(114, bank);
        ASSIGNMOB(136, livestock_butcher);
        ASSIGNMOB(451, bennyguard);
*/
        // Job Givers 
        ASSIGNMOB(127, questmaster);
}



/* assign special procedures to objects */
void assign_objects(void)
{
	ASSIGNOBJ(110, gen_board);
        ASSIGNOBJ(1390, dynamite);
	return;
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;


  if (dts_are_dumps)
    for (i = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	world[i].func = dump;
}
