/**************************************************************************
 * File: ferry.c                                                          *
 * Usage: This is code for ferries.                                       *
 * Created: 7/27/99                                                       *
 **************************************************************************/


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "coins.h"

/* external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern int rev_dir[];
extern char *dirs[];

extern struct room_data *world;
static int ferrypath = 0;

void enter_stat(int station, char wherefrom[100]);
void leave_stat(int station, char whereto[100]);

void train_upd(void)
{
    int station[2];
    station[0] = real_room(1558);
    station[1] = real_room(1845);

    switch (ferrypath) {
    case 0:
	leave_stat(station[0], "Doole");
	break;
    case 1:
	enter_stat(station[1], "Doole");
	break;
    case 2:
	leave_stat(station[1], "Allyton");
	break;
    case 3:
	enter_stat(station[0], "Allyton");
	break;
    default:
	log("SYSERR: Big problem with the ferry!");
	break;
    }
    if (ferrypath == 3) {
	ferrypath = 0;
    }
    else {
        ferrypath++;
    }
    return;
}

void enter_stat(int station, char whereat[100])
{
    int transroom = 0, edoor = 1;
    transroom = real_room(5);

    world[transroom].dir_option[edoor]->to_room = station;
    TOGGLE_BIT(world[station].room_flags, ROOM_FERRY);

//    sprintf(buf, "E t: %d s: %d w: %s", transroom, station, whereat);
//    log(buf);

    send_to_room
	("The ferry pulls into the docks and ties on.\r\nDropping a gank plank opening up a path to the east.\n\r",
	 transroom);
    sprintf(buf, "The ferry master announces, 'Welcome to %s'\n\r",
	    whereat);
    send_to_room(buf, transroom);

    send_to_room
	("With a slight bump, the ferry pulls into the station.\n\r",
	 station);
    send_to_room("The ticket clerk says, 'Now Boarding!'\r\n", station);

    return;
}

void leave_stat(int station, char whereto[100])
{
    int transroom = 0, edoor = 1;
    transroom = real_room(5);

    world[transroom].dir_option[edoor]->to_room = NOWHERE;
    TOGGLE_BIT(world[station].room_flags, ROOM_FERRY);

//    sprintf(buf, "L t: %d s: %d w: %s", transroom, station, whereto);
//    log(buf);


    sprintf(buf, "The ferry master announces, 'Next stop: %s'\n\r",
	    whereto);
    send_to_room(buf, transroom);
    send_to_room(buf, station);
    send_to_room
	("The ferry pulls away from the docks, and starts it's journey\n\r",
	 transroom);
    send_to_room
	("The ferry pulls away from the docks, and it sails away.\n\r",
	 station);
    return;
}

#define FERRY_PRICE 100

SPECIAL(ticket_clerk)
{
    struct char_data *clerk = me;
    struct follow_type *f, *next_fol;
    if (CMD_IS("buy")) {

	if (!IS_SET(world[ch->in_room].room_flags, ROOM_FERRY)) {
	    sprintf(buf, "%s, the ferry has not docked yet.",
		    GET_NAME(ch));
	    mobsay(clerk, buf);
	    mobsay(clerk,
		   "Please wait until the ferry arrives, then you can buy passage.");
	    return TRUE;

	}
	if (convert_all_to_copper(ch) < FERRY_PRICE) {
	    sprintf(buf, "%s, You do not have enough money.",
		    GET_NAME(ch));
	    mobsay(clerk, buf);
	    return TRUE;
	}

	make_change(ch, FERRY_PRICE, BUYING);

	send_to_char
	    ("The ticket clerk takes your money and you board the ferry.\r\n",
	     ch);
	act("The ticket clerk takes $n's money and $e boards the ferry.",
	    FALSE, ch, 0, 0, TO_ROOM);

	char_from_room(ch);
	char_to_room(ch, real_room(5));
	look_at_room(ch, ch->in_room);
	act("$n boards the ferry.", FALSE, ch, 0, 0, TO_ROOM);
	for (f = ch->followers; f; f = next_fol) {
	    next_fol = f->next;

	    // Are we being followed?
	    if (IS_NPC_LIVESTOCK(f->follower)) {
		char_from_room(f->follower);
		char_to_room(f->follower, real_room(5));
		act("$n boards the ferry.", FALSE, f->follower, 0, 0,
		    TO_ROOM);
	    }
	}
	return TRUE;

    }

    return FALSE;
}
