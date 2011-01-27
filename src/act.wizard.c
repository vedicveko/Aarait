/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "assemblies.h"
#include "coins.h"

/*   external vars  */
extern FILE *player_fl;
extern struct room_data *world;
extern struct job_data *job;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern struct job_data *job_table;
extern struct attack_hit_type attack_hit_text[];
extern char *class_abbrevs[];
extern time_t boot_time;
extern zone_rnum top_of_zone_table;
extern job_rnum top_of_jobs;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern mob_rnum top_of_mobt;
extern obj_rnum top_of_objt;
extern int top_of_p_table;
extern struct char_data *mob_proto;

/* for chars */
extern const char *pc_class_types[];

/* extern functions */
int level_exp(int chclass, int level);
void show_shops(struct char_data *ch, char *value);
void hcontrol_list_houses(struct char_data *ch);
void do_start(struct char_data *ch);
void appear(struct char_data *ch);
void reset_zone(zone_rnum zone);
void roll_real_abils(struct char_data *ch);
int parse_class(char arg);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);


/* local functions */
int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg);
void perform_immort_invis(struct char_data *ch, int level);
ACMD(do_echo);
ACMD(do_send);
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
void do_stat_room(struct char_data *ch);
void do_stat_object(struct char_data *ch, struct obj_data *j);
void do_stat_character(struct char_data *ch, struct char_data *k);
ACMD(do_stat);
ACMD(do_shutdown);
void stop_snooping(struct char_data *ch);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
void perform_immort_vis(struct char_data *ch);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
void print_zone_to_buf(char *bufptr, zone_rnum zone);
ACMD(do_show);
ACMD(do_set);
int add_to_save_list(zone_vnum zone, int type);
void show_people_on_jobs(struct char_data *ch);

ACMD(do_echo)
{
    skip_spaces(&argument);

    if (!*argument)
	send_to_char("Yes.. but what?\r\n", ch);
    else {
	if (subcmd == SCMD_EMOTE)
	    sprintf(buf, "$n %s", argument);
	else
	    strcpy(buf, argument);
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	    send_to_char(OK, ch);
	else
	    act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
}


ACMD(do_send)
{
    struct char_data *vict;

    half_chop(argument, arg, buf);

    if (!*arg) {
	send_to_char("Send what to who?\r\n", ch);
	return;
    }
    if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) {
	send_to_char(NOPERSON, ch);
	return;
    }
    send_to_char(buf, vict);
    send_to_char("\r\n", vict);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char("Sent.\r\n", ch);
    else {
	sprintf(buf2, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
	send_to_char(buf2, ch);
    }
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data *ch, char *rawroomstr)
{
    room_vnum tmp;
    room_rnum location;
    struct char_data *target_mob;
    struct obj_data *target_obj;
    char roomstr[MAX_INPUT_LENGTH];

    one_argument(rawroomstr, roomstr);

    if (!*roomstr) {
	send_to_char("You must supply a room number or name.\r\n", ch);
	return (NOWHERE);
    }
    if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
	tmp = atoi(roomstr);
	if ((location = real_room(tmp)) < 0) {
	    send_to_char("No room exists with that number.\r\n", ch);
	    return (NOWHERE);
	}
    } else if ((target_mob = get_char_vis(ch, roomstr, FIND_CHAR_WORLD)) !=
	       NULL) location = target_mob->in_room;
    else if ((target_obj = get_obj_vis(ch, roomstr)) != NULL) {
	if (target_obj->in_room != NOWHERE)
	    location = target_obj->in_room;
	else {
	    send_to_char("That object is not available.\r\n", ch);
	    return (NOWHERE);
	}
    } else {
	send_to_char("No such creature or object around.\r\n", ch);
	return (NOWHERE);
    }

    /* a location has been found -- if you're < GRGOD, check restrictions. */
    if (GET_LEVEL(ch) < LVL_GRGOD) {
	if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
	    send_to_char("You are not godly enough to use that room!\r\n",
			 ch);
	    return (NOWHERE);
	}
	if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
	    world[location].people && world[location].people->next_in_room) {
	    send_to_char
		("There's a private conversation going on in that room.\r\n",
		 ch);
	    return (NOWHERE);
	}
    }
    return (location);
}



ACMD(do_at)
{
    char command[MAX_INPUT_LENGTH];
    room_rnum location, original_loc;

    half_chop(argument, buf, command);
    if (!*buf) {
	send_to_char("You must supply a room number or a name.\r\n", ch);
	return;
    }

    if (!*command) {
	send_to_char("What do you want to do there?\r\n", ch);
	return;
    }

    if ((location = find_target_room(ch, buf)) < 0)
	return;

    /* a location has been found. */
    original_loc = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, location);
    command_interpreter(ch, command);

    /* check if the char is still there */
    if (ch->in_room == location) {
	char_from_room(ch);
	char_to_room(ch, original_loc);
    }
}


ACMD(do_goto)
{
    room_rnum location;

    if ((location = find_target_room(ch, argument)) < 0)
	return;

    if (POOFOUT(ch))
	sprintf(buf, "$n %s", POOFOUT(ch));
    else
	strcpy(buf, "$n disappears in a puff of smoke.");

    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, location);

    if (POOFIN(ch))
	sprintf(buf, "$n %s", POOFIN(ch));
    else
	strcpy(buf, "$n appears with an ear-splitting bang.");

    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
}



ACMD(do_trans)
{
    struct descriptor_data *i;
    struct char_data *victim;

    one_argument(argument, buf);
    if (!*buf)
	send_to_char("Whom do you wish to transfer?\r\n", ch);
    else if (str_cmp("all", buf)) {
	if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
	    send_to_char(NOPERSON, ch);
	else if (victim == ch)
	    send_to_char("That doesn't make much sense, does it?\r\n", ch);
	else {
	    if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
		send_to_char("Go transfer someone your own size.\r\n", ch);
		return;
	    }
	    act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0,
		TO_ROOM);
	    char_from_room(victim);
	    char_to_room(victim, ch->in_room);
	    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0,
		TO_ROOM);
	    act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
	    look_at_room(victim, 0);
	}
    } else {			/* Trans All */
	if (GET_LEVEL(ch) < LVL_GRGOD) {
	    send_to_char("I think not.\r\n", ch);
	    return;
	}

	for (i = descriptor_list; i; i = i->next)
	    if (STATE(i) == CON_PLAYING && i->character
		&& i->character != ch) {
		victim = i->character;
		if (GET_LEVEL(victim) >= GET_LEVEL(ch))
		    continue;
		act("$n disappears in a mushroom cloud.", FALSE, victim, 0,
		    0, TO_ROOM);
		char_from_room(victim);
		char_to_room(victim, ch->in_room);
		act("$n arrives from a puff of smoke.", FALSE, victim, 0,
		    0, TO_ROOM);
		act("$n has transferred you!", FALSE, ch, 0, victim,
		    TO_VICT);
		look_at_room(victim, 0);
	    }
	send_to_char(OK, ch);
    }
}



ACMD(do_teleport)
{
    struct char_data *victim;
    room_rnum target;

    two_arguments(argument, buf, buf2);

    if (!*buf)
	send_to_char("Whom do you wish to teleport?\r\n", ch);
    else if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
	send_to_char(NOPERSON, ch);
    else if (victim == ch)
	send_to_char("Use 'goto' to teleport yourself.\r\n", ch);
    else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	send_to_char("Maybe you shouldn't do that.\r\n", ch);
    else if (!*buf2)
	send_to_char("Where do you wish to send this person?\r\n", ch);
    else if ((target = find_target_room(ch, buf2)) >= 0) {
	send_to_char(OK, ch);
	act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0,
	    TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, target);
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0,
	    TO_ROOM);
	act("$n has teleported you!", FALSE, ch, 0, (char *) victim,
	    TO_VICT);
	look_at_room(victim, 0);
    }
}



ACMD(do_vnum)
{
    half_chop(argument, buf, buf2);

    if (!*buf || !*buf2
	|| (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj"))) {
	send_to_char("Usage: vnum { obj | mob } <name>\r\n", ch);
	return;
    }
    if (is_abbrev(buf, "mob"))
	if (!vnum_mobile(buf2, ch))
	    send_to_char("No mobiles by that name.\r\n", ch);

    if (is_abbrev(buf, "obj"))
	if (!vnum_object(buf2, ch))
	    send_to_char("No objects by that name.\r\n", ch);
}



void do_stat_room(struct char_data *ch)
{
    struct extra_descr_data *desc;
    struct room_data *rm = &world[ch->in_room];
    int i, found;
    struct obj_data *j;
    struct char_data *k;

    sprintf(buf, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name,
	    CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprinttype(rm->sector_type, sector_types, buf2);
    sprintf(buf, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d], Type: %s\r\n",
	    zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number,
	    CCNRM(ch, C_NRM), ch->in_room, buf2);
    send_to_char(buf, ch);

    sprintbit(rm->room_flags, room_bits, buf2);
    sprintf(buf, "SpecProc: %s, Flags: %s\r\n",
	    (rm->func == NULL) ? "None" : "Exists", buf2);
    send_to_char(buf, ch);

    send_to_char("Description:\r\n", ch);
    if (rm->description)
	send_to_char(rm->description, ch);
    else
	send_to_char("  None.\r\n", ch);

    if (rm->ex_description) {
	sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
	for (desc = rm->ex_description; desc; desc = desc->next) {
	    strcat(buf, " ");
	    strcat(buf, desc->keyword);
	}
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(strcat(buf, "\r\n"), ch);
    }
    sprintf(buf, "Chars present:%s", CCYEL(ch, C_NRM));
    for (found = 0, k = rm->people; k; k = k->next_in_room) {
	if (!CAN_SEE(ch, k))
	    continue;
	sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
		(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
	strcat(buf, buf2);
	if (strlen(buf) >= 62) {
	    if (k->next_in_room)
		send_to_char(strcat(buf, ",\r\n"), ch);
	    else
		send_to_char(strcat(buf, "\r\n"), ch);
	    *buf = found = 0;
	}
    }

    if (*buf)
	send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);

    if (rm->contents) {
	sprintf(buf, "Contents:%s", CCGRN(ch, C_NRM));
	for (found = 0, j = rm->contents; j; j = j->next_content) {
	    if (!CAN_SEE_OBJ(ch, j))
		continue;
	    sprintf(buf2, "%s %s", found++ ? "," : "",
		    j->short_description);
	    strcat(buf, buf2);
	    if (strlen(buf) >= 62) {
		if (j->next_content)
		    send_to_char(strcat(buf, ",\r\n"), ch);
		else
		    send_to_char(strcat(buf, "\r\n"), ch);
		*buf = found = 0;
	    }
	}

	if (*buf)
	    send_to_char(strcat(buf, "\r\n"), ch);
	send_to_char(CCNRM(ch, C_NRM), ch);
    }
    for (i = 0; i < NUM_OF_DIRS; i++) {
	if (rm->dir_option[i]) {
	    if (rm->dir_option[i]->to_room == NOWHERE)
		sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM),
			CCNRM(ch, C_NRM));
	    else
		sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
			GET_ROOM_VNUM(rm->dir_option[i]->to_room),
			CCNRM(ch, C_NRM));
	    sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
	    sprintf(buf,
		    "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n ",
		    CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1,
		    rm->dir_option[i]->key,
		    rm->dir_option[i]->keyword ? rm->dir_option[i]->
		    keyword : "None", buf2);
	    send_to_char(buf, ch);
	    if (rm->dir_option[i]->general_description)
		strcpy(buf, rm->dir_option[i]->general_description);
	    else
		strcpy(buf, "  No exit description.\r\n");
	    send_to_char(buf, ch);
	}
    }
}



void do_stat_object(struct char_data *ch, struct obj_data *j)
{
    int i, found;
    obj_vnum vnum;
    struct obj_data *j2;
    struct extra_descr_data *desc;

    vnum = GET_OBJ_VNUM(j);
    sprintf(buf, "Name: '%s%s%s', Aliases: %s\r\n", CCYEL(ch, C_NRM),
	    ((j->short_description) ? j->short_description : "<None>"),
	    CCNRM(ch, C_NRM), j->name);
    send_to_char(buf, ch);
    sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
    if (GET_OBJ_RNUM(j) >= 0)
	strcpy(buf2,
	       (obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None"));
    else
	strcpy(buf2, "None");
    sprintf(buf,
	    "VNum: [%s%5d%s], RNum: [%5d], Type: %s, SpecProc: %s\r\n",
	    CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j),
	    buf1, buf2);
    send_to_char(buf, ch);
    sprintf(buf, "L-Des: %s\r\n",
	    ((j->description) ? j->description : "None"));
    send_to_char(buf, ch);

    if (j->ex_description) {
	sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
	for (desc = j->ex_description; desc; desc = desc->next) {
	    strcat(buf, " ");
	    strcat(buf, desc->keyword);
	}
	strcat(buf, CCNRM(ch, C_NRM));
	send_to_char(strcat(buf, "\r\n"), ch);
    }
    send_to_char("Can be worn on: ", ch);
    sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    send_to_char("Set char bits : ", ch);
    sprintbit(j->obj_flags.bitvector, affected_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    send_to_char("Extra flags   : ", ch);
    sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    sprintf(buf,
	    "Weight: %d, Value: %d, Cost//day: %d, Timer: %d, Min Level: %d\r\n",
	    GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j),
	    GET_OBJ_TIMER(j), GET_OBJ_LEVEL(j));
    send_to_char(buf, ch);

    strcpy(buf, "In room: ");
    if (j->in_room == NOWHERE)
	strcat(buf, "Nowhere");
    else {
	sprintf(buf2, "%d", GET_ROOM_VNUM(IN_ROOM(j)));
	strcat(buf, buf2);
    }
    /*
     * NOTE: In order to make it this far, we must already be able to see the
     *       character holding the object. Therefore, we do not need CAN_SEE().
     */
    strcat(buf, ", In object: ");
    strcat(buf, j->in_obj ? j->in_obj->short_description : "None");
    strcat(buf, ", Carried by: ");
    strcat(buf, j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
    strcat(buf, ", Worn by: ");
    strcat(buf, j->worn_by ? GET_NAME(j->worn_by) : "Nobody");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    switch (GET_OBJ_TYPE(j)) {
    case ITEM_LIGHT:
	if (GET_OBJ_VAL(j, 2) == -1)
	    strcpy(buf, "Hours left: Infinite");
	else
	    sprintf(buf, "Hours left: [%d]", GET_OBJ_VAL(j, 2));
	break;
    case ITEM_WEAPON:
	sprintf(buf, "Todam: %dd%d, Message type: %d",
		GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
	break;
    case ITEM_ARMOR:
	sprintf(buf, "AC-apply: [%d]", GET_OBJ_VAL(j, 0));
	break;
    case ITEM_TRAP:
	sprintf(buf, "Spell: %d, - Hitpoints: %d",
		GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
	break;
    case ITEM_CONTAINER:
	sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
	sprintf(buf,
		"Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s",
		GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2),
		YESNO(GET_OBJ_VAL(j, 3)));
	break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
	sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
	sprintf(buf,
		"Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s",
		GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
		YESNO(GET_OBJ_VAL(j, 3)), buf2);
	break;
    case ITEM_NOTE:
	sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
	break;
    case ITEM_KEY:
	strcpy(buf, "");
	break;
    case ITEM_FOOD:
	sprintf(buf, "Makes full: %d, Poisoned: %s", GET_OBJ_VAL(j, 0),
		YESNO(GET_OBJ_VAL(j, 3)));
	break;
    case ITEM_MONEY:
	sprintf(buf, "Coins: %d", GET_OBJ_VAL(j, 0));
	break;
    default:
	sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
		GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
		GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
	break;
    }
    send_to_char(strcat(buf, "\r\n"), ch);

    /*
     * I deleted the "equipment status" code from here because it seemed
     * more or less useless and just takes up valuable screen space.
     */

    if (j->contains) {
	sprintf(buf, "\r\nContents:%s", CCGRN(ch, C_NRM));
	for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
	    sprintf(buf2, "%s %s", found++ ? "," : "",
		    j2->short_description);
	    strcat(buf, buf2);
	    if (strlen(buf) >= 62) {
		if (j2->next_content)
		    send_to_char(strcat(buf, ",\r\n"), ch);
		else
		    send_to_char(strcat(buf, "\r\n"), ch);
		*buf = found = 0;
	    }
	}

	if (*buf)
	    send_to_char(strcat(buf, "\r\n"), ch);
	send_to_char(CCNRM(ch, C_NRM), ch);
    }
    found = 0;
    send_to_char("Affections:", ch);
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
	if (j->affected[i].modifier) {
	    sprinttype(j->affected[i].location, apply_types, buf2);
	    sprintf(buf, "%s %+d to %s", found++ ? "," : "",
		    j->affected[i].modifier, buf2);
	    send_to_char(buf, ch);
	}
    if (!found)
	send_to_char(" None", ch);

    sprintf(buf, "\r\nCDP: %3d MDP: %3d\r\n", GET_OBJ_CSLOTS(j),
	    GET_OBJ_TSLOTS(j));
    send_to_char(buf, ch);
    send_to_char("\r\n", ch);

}


void do_stat_character(struct char_data *ch, struct char_data *k)
{
    int i, i2, found = 0;
    struct obj_data *j;
    struct follow_type *fol;
    struct affected_type *aff;

    sprinttype(GET_SEX(k), genders, buf);
    sprintf(buf2, " %s '%s'  IDNum: [%5ld], In room [%5d]\r\n",
	    (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
	    GET_NAME(k), GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)));
    send_to_char(strcat(buf, buf2), ch);
    if (IS_MOB(k)) {
	sprintf(buf, "Alias: %s, VNum: [%5d], RNum: [%5d]\r\n",
		k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
	send_to_char(buf, ch);
    }

    sprintf(buf, "Title: %s\r\n",
	    (k->player.title ? k->player.title : "<None>"));
    send_to_char(buf, ch);

    sprintf(buf, "L-Des: %s",
	    (k->player.long_descr ? k->player.long_descr : "<None>\r\n"));
    send_to_char(buf, ch);


    if (IS_NPC(k)) {
	sprintf(buf, "Monster Class: %-15s",
		npc_class_types[(int) GET_CLASS(k)]);
    } else {
	sprintf(buf, "Class: %-15s", pc_class_types[(int) GET_CLASS(k)]);
    }

    send_to_char(buf, ch);

    if (IS_NPC(k)) {
	sprintf(buf, "Monster Race: %-15s",
		npc_race_types[(int) GET_RACE(k)]);

    } else {
	sprintf(buf, "Race: %-15s", pc_race_types[(int) GET_RACE(k)]);
    }

    send_to_char(buf, ch);

    sprintf(buf, "\r\nLev: [%s%2d%s], XP: [%s%7d%s], Align: [%4d]\r\n",
	    CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
	    CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM),
	    GET_ALIGNMENT(k));
    send_to_char(buf, ch);

    if (!IS_NPC(k)) {
	strcpy(buf1, (char *) asctime(localtime(&(k->player.time.birth))));
	strcpy(buf2, (char *) asctime(localtime(&(k->player.time.logon))));
	buf1[10] = buf2[10] = '\0';

	sprintf(buf,
		"Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
		buf1, buf2, k->player.time.played / 3600,
		((k->player.time.played % 3600) / 60), age(k)->year);
	send_to_char(buf, ch);

	sprintf(buf,
		"Hometown: [%d], Speaks: [%d/%d/%d], (STL[%d]/per[%d]/NSTL[%d])",
		k->player.hometown, GET_TALK(k, 0), GET_TALK(k, 1),
		GET_TALK(k, 2), GET_PRACTICES(k),
		int_app[GET_INT(k)].learn, wis_app[GET_WIS(k)].bonus);
	/*. Display OLC zone for immorts . */
	if (GET_LEVEL(k) >= LVL_IMMORT)
	    sprintf(buf + strlen(buf), ", OLC[%d]", GET_OLC_ZONE(k));
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
    }
    sprintf(buf, "Str: [%s%d//%d%s]  Int: [%s%d%s]  Wis: [%s%d%s]  "
	    "Dex: [%s%d%s]  Con: [%s%d%s]  Cha: [%s%d%s]\r\n",
	    CCCYN(ch, C_NRM), GET_STR(k), GET_ADD(k), CCNRM(ch, C_NRM),
	    CCCYN(ch, C_NRM), GET_INT(k), CCNRM(ch, C_NRM),
	    CCCYN(ch, C_NRM), GET_WIS(k), CCNRM(ch, C_NRM),
	    CCCYN(ch, C_NRM), GET_DEX(k), CCNRM(ch, C_NRM),
	    CCCYN(ch, C_NRM), GET_CON(k), CCNRM(ch, C_NRM),
	    CCCYN(ch, C_NRM), GET_CHA(k), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprintf(buf,
	    "Hit p.:[%s%d//%d+%d%s]  Mana p.:[%s%d//%d+%d%s]  Move p.:[%s%d//%d+%d%s]\r\n",
	    CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
	    CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MANA(k),
	    GET_MAX_MANA(k), mana_gain(k), CCNRM(ch, C_NRM), CCGRN(ch,
								   C_NRM),
	    GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprintf(buf, "Coins: [%9d]P [%9d]G [%9d]S [%9d]C\r\n"
	    "Bank : [%d] (Total: %d)\r\n",
	    GET_PLATINUM(k), GET_GOLD(k), GET_SILVER(k), GET_COPPER(k),
	    GET_BANK_GOLD(k), convert_all_to_copper(k) + GET_BANK_GOLD(k));
    send_to_char(buf, ch);

    sprintf(buf,
	    "AC: [%d], Hitroll: [%2d], Damroll: [%2d], Saving throws: [%d/%d/%d/%d/%d]\r\n",
	    GET_AC(k), k->points.hitroll, k->points.damroll, GET_SAVE(k,
								      0),
	    GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3), GET_SAVE(k,
								     4));
    send_to_char(buf, ch);

    sprinttype(GET_POS(k), position_types, buf2);
    sprintf(buf, "Pos: %s, Fighting: %s", buf2,
	    (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));

    if (IS_NPC(k)) {
	strcat(buf, ", Attack type: ");
	strcat(buf, attack_hit_text[k->mob_specials.attack_type].singular);
    }
    if (k->desc) {
	sprinttype(STATE(k->desc), connected_types, buf2);
	strcat(buf, ", Connected: ");
	strcat(buf, buf2);
    }
    send_to_char(strcat(buf, "\r\n"), ch);

    strcpy(buf, "Default position: ");
    sprinttype((k->mob_specials.default_pos), position_types, buf2);
    strcat(buf, buf2);

    sprintf(buf2, ", Idle Timer (in tics) [%d]\r\n",
	    k->char_specials.timer);
    strcat(buf, buf2);
    send_to_char(buf, ch);

    if (IS_NPC(k)) {
	sprintbit(MOB_FLAGS(k), action_bits, buf2);
	sprintf(buf, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2,
		CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
    } else {
	sprintbit(PLR_FLAGS(k), player_bits, buf2);
	sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2,
		CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
	sprintbit(PRF_FLAGS(k), preference_bits, buf2);
	sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2,
		CCNRM(ch, C_NRM));
	send_to_char(buf, ch);
    }

    if (IS_MOB(k)) {
	sprintf(buf, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
		(mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
		k->mob_specials.damnodice, k->mob_specials.damsizedice);
	send_to_char(buf, ch);
    }
    sprintf(buf, "Carried: weight: %d, items: %d; ",
	    IS_CARRYING_W(k), IS_CARRYING_N(k));

    for (i = 0, j = k->carrying; j; j = j->next_content, i++);
    sprintf(buf + strlen(buf), "Items in: inventory: %d, ", i);

    for (i = 0, i2 = 0; i < NUM_WEARS; i++)
	if (GET_EQ(k, i))
	    i2++;
    sprintf(buf2, "eq: %d\r\n", i2);
    strcat(buf, buf2);
    send_to_char(buf, ch);

    sprintf(buf, "    WEIGHS: %d    REELIN: %d FISHON: %d\r\n",
	    GET_MOB_WEIGHT(k), GET_REELIN(k), GET_FISHON(k));
    send_to_char(buf, ch);

    if (!IS_NPC(k)) {
	sprintf(buf, "Hunger: %d, Thirst: %d, Drunk: %d\r\n",
		GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k,
								 DRUNK));
	send_to_char(buf, ch);
    } else {

	sprintf(buf, "Mob Hunger: %-2d  Size: %-8s \r\n",
		GET_MOB_HUNGER(k), mob_sizes[(int) GET_MOB_SIZE(k) + 8]);
	send_to_char(buf, ch);
    }

    sprintf(buf, "Master is: %s, Followers are:",
	    ((k->master) ? GET_NAME(k->master) : "<none>"));

    for (fol = k->followers; fol; fol = fol->next) {
	sprintf(buf2, "%s %s", found++ ? "," : "",
		PERS(fol->follower, ch));
	strcat(buf, buf2);
	if (strlen(buf) >= 62) {
	    if (fol->next)
		send_to_char(strcat(buf, ",\r\n"), ch);
	    else
		send_to_char(strcat(buf, "\r\n"), ch);
	    *buf = found = 0;
	}
    }

    if (*buf)
	send_to_char(strcat(buf, "\r\n"), ch);

    /* Showing the bitvector */
    sprintbit(AFF_FLAGS(k), affected_bits, buf2);
    sprintf(buf, "AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2,
	    CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    /* Routine to show what spells a char is affected by */
    if (k->affected) {
	for (aff = k->affected; aff; aff = aff->next) {
	    *buf2 = '\0';
	    sprintf(buf, "SPL: (%3dhr) %s%s ", aff->duration + 1,
		    CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
	    if (aff->modifier) {
		sprintf(buf2, "%+d to %s", aff->modifier,
			apply_types[(int) aff->location]);
		strcat(buf, buf2);
	    }
	    if (aff->bitvector) {
		if (*buf2)
		    strcat(buf, ", sets ");
		else
		    strcat(buf, "sets ");
		sprintbit(aff->bitvector, affected_bits, buf2);
		strcat(buf, buf2);
	    }
	    send_to_char(strcat(buf, "\r\n"), ch);
	}
    }
}


ACMD(do_stat)
{
    struct char_data *victim;
    struct obj_data *object;
    struct char_file_u tmp_store;
    int tmp;

    half_chop(argument, buf1, buf2);

    if (!*buf1) {
	send_to_char("Stats on who or what?\r\n", ch);
	return;
    } else if (is_abbrev(buf1, "room")) {
	do_stat_room(ch);
    } else if (is_abbrev(buf1, "mob")) {
	if (!*buf2)
	    send_to_char("Stats on which mobile?\r\n", ch);
	else {
	    if ((victim = get_char_vis(ch, buf2, FIND_CHAR_WORLD)) != NULL)
		do_stat_character(ch, victim);
	    else
		send_to_char("No such mobile around.\r\n", ch);
	}
    } else if (is_abbrev(buf1, "player")) {
	if (!*buf2) {
	    send_to_char("Stats on which player?\r\n", ch);
	} else {
	    if ((victim = get_player_vis(ch, buf2, FIND_CHAR_WORLD)) !=
		NULL) do_stat_character(ch, victim);
	    else
		send_to_char("No such player around.\r\n", ch);
	}
    } else if (is_abbrev(buf1, "file")) {
	if (!*buf2) {
	    send_to_char("Stats on which player?\r\n", ch);
	} else {
	    CREATE(victim, struct char_data, 1);
	    clear_char(victim);
	    if (load_char(buf2, &tmp_store) > -1) {
		store_to_char(&tmp_store, victim);
		victim->player.time.logon = tmp_store.last_logon;
		char_to_room(victim, 0);
		if (GET_LEVEL(victim) > GET_LEVEL(ch))
		    send_to_char("Sorry, you can't do that.\r\n", ch);
		else
		    do_stat_character(ch, victim);
		extract_char(victim);
	    } else {
		send_to_char("There is no such player.\r\n", ch);
		free(victim);
	    }
	}
    } else if (is_abbrev(buf1, "object")) {
	if (!*buf2)
	    send_to_char("Stats on which object?\r\n", ch);
	else {
	    if ((object = get_obj_vis(ch, buf2)) != NULL)
		do_stat_object(ch, object);
	    else
		send_to_char("No such object around.\r\n", ch);
	}
    } else {
	if (
	    (object =
	     get_object_in_equip_vis(ch, buf1, ch->equipment,
				     &tmp)) != NULL) do_stat_object(ch,
								    object);
	else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)) !=
		 NULL) do_stat_object(ch, object);
	else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_ROOM)) != NULL)
	    do_stat_character(ch, victim);
	else
	    if (
		(object =
		 get_obj_in_list_vis(ch, buf1,
				     world[ch->in_room].contents)) !=
		NULL) do_stat_object(ch, object);
	else if ((victim = get_char_vis(ch, buf1, FIND_CHAR_WORLD)) !=
		 NULL) do_stat_character(ch, victim);
	else if ((object = get_obj_vis(ch, buf1)) != NULL)
	    do_stat_object(ch, object);
	else
	    send_to_char("Nothing around by that name.\r\n", ch);
    }
}


ACMD(do_shutdown)
{
    if (subcmd != SCMD_SHUTDOWN) {
	send_to_char("If you want to shut something down, say so!\r\n",
		     ch);
	return;
    }
    one_argument(argument, arg);

    if (!*arg) {
	log("(GC) Shutdown by %s.", GET_NAME(ch));
	send_to_all("Shutting down.\r\n");
	circle_shutdown = 1;
    } else if (!str_cmp(arg, "now")) {
	log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
	send_to_all("Rebooting.. come back in a minute or two.\r\n");
	circle_shutdown = 1;
	circle_reboot = 2;
    } else if (!str_cmp(arg, "reboot")) {
	log("(GC) Reboot by %s.", GET_NAME(ch));
	send_to_all("Rebooting.. come back in a minute or two.\r\n");
	touch(FASTBOOT_FILE);
	circle_shutdown = circle_reboot = 1;
    } else if (!str_cmp(arg, "die")) {
	log("(GC) Shutdown by %s.", GET_NAME(ch));
	send_to_all("Shutting down for maintenance.\r\n");
	touch(KILLSCRIPT_FILE);
	circle_shutdown = 1;
    } else if (!str_cmp(arg, "pause")) {
	log("(GC) Shutdown by %s.", GET_NAME(ch));
	send_to_all("Shutting down for maintenance.\r\n");
	touch(PAUSE_FILE);
	circle_shutdown = 1;
    } else
	send_to_char("Unknown shutdown option.\r\n", ch);
}


void stop_snooping(struct char_data *ch)
{
    if (!ch->desc->snooping)
	send_to_char("You aren't snooping anyone.\r\n", ch);
    else {
	send_to_char("You stop snooping.\r\n", ch);
	ch->desc->snooping->snoop_by = NULL;
	ch->desc->snooping = NULL;
    }
}


ACMD(do_snoop)
{
    struct char_data *victim, *tch;

    if (!ch->desc)
	return;

    one_argument(argument, arg);

    if (!*arg)
	stop_snooping(ch);
    else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
	send_to_char("No such person around.\r\n", ch);
    else if (!victim->desc)
	send_to_char("There's no link.. nothing to snoop.\r\n", ch);
    else if (victim == ch)
	stop_snooping(ch);
    else if (victim->desc->snoop_by)
	send_to_char("Busy already. \r\n", ch);
    else if (victim->desc->snooping == ch->desc)
	send_to_char("Don't be stupid.\r\n", ch);
    else {
	if (victim->desc->original)
	    tch = victim->desc->original;
	else
	    tch = victim;

	if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
	    send_to_char("You can't.\r\n", ch);
	    return;
	}
	send_to_char(OK, ch);

	if (ch->desc->snooping)
	    ch->desc->snooping->snoop_by = NULL;

	ch->desc->snooping = victim->desc;
	victim->desc->snoop_by = ch->desc;
    }
}



ACMD(do_switch)
{
    struct char_data *victim;

    one_argument(argument, arg);

    if (ch->desc->original)
	send_to_char("You're already switched.\r\n", ch);
    else if (!*arg)
	send_to_char("Switch with who?\r\n", ch);
    else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
	send_to_char("No such character.\r\n", ch);
    else if (ch == victim)
	send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
    else if (victim->desc)
	send_to_char("You can't do that, the body is already in use!\r\n",
		     ch);
    else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
	send_to_char("You aren't holy enough to use a mortal's body.\r\n",
		     ch);
    else if (GET_LEVEL(ch) < LVL_GRGOD
	     && ROOM_FLAGGED(IN_ROOM(victim),
			     ROOM_GODROOM))
	    send_to_char("You are not godly enough to use that room!\r\n",
			 ch);
    else {
	send_to_char(OK, ch);

	ch->desc->character = victim;
	ch->desc->original = ch;

	victim->desc = ch->desc;
	ch->desc = NULL;
    }
}


ACMD(do_return)
{
    if (ch->desc && ch->desc->original) {
	send_to_char("You return to your original body.\r\n", ch);

	/*
	 * If someone switched into your original body, disconnect them.
	 *   - JE 2/22/95
	 *
	 * Zmey: here we put someone switched in our body to disconnect state
	 * but we must also NULL his pointer to our character, otherwise   
	 * close_socket() will damage our character's pointer to our descriptor
	 * (which is assigned below in this function). 12/17/99
	 */
	if (ch->desc->original->desc) {
	    ch->desc->original->desc->character = NULL;
	    STATE(ch->desc->original->desc) = CON_DISCONNECT;
	}

	/* Now our descriptor points to our original body. */
	ch->desc->character = ch->desc->original;
	ch->desc->original = NULL;

	/* And our body's pointer to descriptor now points to our descriptor. */
	ch->desc->character->desc = ch->desc;
	ch->desc = NULL;
    }
}



ACMD(do_load)
{
    struct char_data *mob;
    struct obj_data *obj;
    mob_vnum number;
    mob_rnum r_num;

    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || !isdigit(*buf2)) {
	send_to_char("Usage: load { obj | mob } <number>\r\n", ch);
	return;
    }
    if ((number = atoi(buf2)) < 0) {
	send_to_char("A NEGATIVE number??\r\n", ch);
	return;
    }
    if (is_abbrev(buf, "mob")) {
	if ((r_num = real_mobile(number)) < 0) {
	    send_to_char("There is no monster with that number.\r\n", ch);
	    return;
	}
	mob = read_mobile(r_num, REAL);
	char_to_room(mob, ch->in_room);

	act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	    0, 0, TO_ROOM);
	act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
	act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
    } else if (is_abbrev(buf, "obj")) {
	if ((r_num = real_object(number)) < 0) {
	    send_to_char("There is no object with that number.\r\n", ch);
	    return;
	}
	obj = read_object(r_num, REAL);
	if (load_into_inventory)
	    obj_to_char(obj, ch);
	else
	    obj_to_room(obj, ch->in_room);
	act("$n makes a strange magical gesture.", TRUE, ch, 0, 0,
	    TO_ROOM);
	act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
	act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
    } else
	send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}



ACMD(do_vstat)
{
    struct char_data *mob;
    struct obj_data *obj;
    mob_vnum number;		/* or obj_vnum ... */
    mob_rnum r_num;		/* or obj_rnum ... */

    two_arguments(argument, buf, buf2);

    if (!*buf || !*buf2 || !isdigit(*buf2)) {
	send_to_char("Usage: vstat { obj | mob } <number>\r\n", ch);
	return;
    }
    if ((number = atoi(buf2)) < 0) {
	send_to_char("A NEGATIVE number??\r\n", ch);
	return;
    }
    if (is_abbrev(buf, "mob")) {
	if ((r_num = real_mobile(number)) < 0) {
	    send_to_char("There is no monster with that number.\r\n", ch);
	    return;
	}
	mob = read_mobile(r_num, REAL);
	char_to_room(mob, 0);
	do_stat_character(ch, mob);
	extract_char(mob);
    } else if (is_abbrev(buf, "obj")) {
	if ((r_num = real_object(number)) < 0) {
	    send_to_char("There is no object with that number.\r\n", ch);
	    return;
	}
	obj = read_object(r_num, REAL);
	do_stat_object(ch, obj);
	extract_obj(obj);
    } else
	send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}




/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
    struct char_data *vict, *next_v;
    struct obj_data *obj, *next_o;

    one_argument(argument, buf);

    if (*buf) {			/* argument supplied. destroy single object
				 * or char */
	if ((vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)) != NULL) {
	    if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
		send_to_char("Fuuuuuuuuu!\r\n", ch);
		return;
	    }
	    act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

	    if (!IS_NPC(vict)) {
		sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch),
			GET_NAME(vict));
		mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
		if (vict->desc) {
		    STATE(vict->desc) = CON_CLOSE;
		    vict->desc->character = NULL;
		    vict->desc = NULL;
		}
	    }
	    extract_char(vict);
	} else
	    if (
		(obj =
		 get_obj_in_list_vis(ch, buf,
				     world[ch->in_room].contents)) !=
		NULL) {
	    act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
	    extract_obj(obj);
	} else {
	    send_to_char("Nothing here by that name.\r\n", ch);
	    return;
	}

	send_to_char(OK, ch);
    } else {			/* no argument. clean out the room */
	act("$n gestures... You are surrounded by scorching flames!",
	    FALSE, ch, 0, 0, TO_ROOM);
	send_to_room("The world seems a little cleaner.\r\n", ch->in_room);

	for (vict = world[ch->in_room].people; vict; vict = next_v) {
	    next_v = vict->next_in_room;
	    if (IS_NPC(vict))
		extract_char(vict);
	}

	for (obj = world[ch->in_room].contents; obj; obj = next_o) {
	    next_o = obj->next_content;
	    extract_obj(obj);
	}
    }
}



const char *logtypes[] = {
    "off", "brief", "normal", "complete", "\n"
};

ACMD(do_syslog)
{
    int tp;

    one_argument(argument, arg);

    if (!*arg) {
	tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
	      (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
	sprintf(buf, "Your syslog is currently %s.\r\n", logtypes[tp]);
	send_to_char(buf, ch);
	return;
    }
    if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
	send_to_char
	    ("Usage: syslog { Off | Brief | Normal | Complete }\r\n", ch);
	return;
    }
    REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
    SET_BIT(PRF_FLAGS(ch),
	    (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

    sprintf(buf, "Your syslog is now %s.\r\n", logtypes[tp]);
    send_to_char(buf, ch);
}



ACMD(do_advance)
{
    struct char_data *victim;
    char *name = arg, *level = buf2;
    int newlevel, oldlevel;

    two_arguments(argument, name, level);

    if (*name) {
	if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
	    send_to_char("That player is not here.\r\n", ch);
	    return;
	}
    } else {
	send_to_char("Advance who?\r\n", ch);
	return;
    }

    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
	send_to_char("Maybe that's not such a great idea.\r\n", ch);
	return;
    }
    if (IS_NPC(victim)) {
	send_to_char("NO!  Not on NPC's.\r\n", ch);
	return;
    }
    if (!*level || (newlevel = atoi(level)) <= 0) {
	send_to_char("That's not a level!\r\n", ch);
	return;
    }
    if (newlevel > LVL_IMPL) {
	sprintf(buf, "%d is the highest possible level.\r\n", LVL_IMPL);
	send_to_char(buf, ch);
	return;
    }
    if (newlevel > GET_LEVEL(ch)) {
	send_to_char("Yeah, right.\r\n", ch);
	return;
    }
    if (newlevel == GET_LEVEL(victim)) {
	send_to_char("They are already at that level.\r\n", ch);
	return;
    }
    oldlevel = GET_LEVEL(victim);
    if (newlevel < GET_LEVEL(victim)) {
	do_start(victim);
	GET_LEVEL(victim) = newlevel;
	send_to_char("You are momentarily enveloped by darkness!\r\n"
		     "You feel somewhat diminished.\r\n", victim);
    } else {
	act("$n makes some strange gestures.\r\n"
	    "A strange feeling comes upon you,\r\n"
	    "Like a giant hand, light comes down\r\n"
	    "from above, grabbing your body, that\r\n"
	    "begins to pulse with colored lights\r\n"
	    "from inside.\r\n\r\n"
	    "Your head seems to be filled with demons\r\n"
	    "from another plane as your body dissolves\r\n"
	    "to the elements of time and space itself.\r\n"
	    "Suddenly a silent explosion of light\r\n"
	    "snaps you back to reality.\r\n\r\n"
	    "You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
    }

    send_to_char(OK, ch);

    if (newlevel < oldlevel)
	log("(GC) %s demoted %s from level %d to %d.",
	    GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
    else
	log("(GC) %s has advanced %s to level %d (from %d)",
	    GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

    gain_exp_regardless(victim,
			level_exp(GET_CLASS(victim),
				  newlevel) - GET_EXP(victim));
    save_char(victim, NOWHERE);
}



ACMD(do_restore)
{
    struct char_data *vict;
    int i;

    one_argument(argument, buf);
    if (!*buf)
	send_to_char("Whom do you wish to restore?\r\n", ch);
    else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
	send_to_char(NOPERSON, ch);
    else {
	GET_HIT(vict) = GET_MAX_HIT(vict);
	GET_MANA(vict) = GET_MAX_MANA(vict);
	GET_MOVE(vict) = GET_MAX_MOVE(vict);

	if ((GET_LEVEL(ch) >= LVL_GRGOD)
	    && (GET_LEVEL(vict) >= LVL_IMMORT)) {
	    for (i = 1; i <= MAX_SKILLS; i++)
		SET_SKILL(vict, i, 100);

	    if (GET_LEVEL(vict) >= LVL_GRGOD) {
		vict->real_abils.str_add = 100;
		vict->real_abils.intel = 25;
		vict->real_abils.wis = 25;
		vict->real_abils.dex = 25;
		vict->real_abils.str = 25;
		vict->real_abils.con = 25;
		vict->real_abils.cha = 25;
		SET_BIT(PLR_FLAGS(vict), PLR_PIG_DONE);
		SET_BIT(PLR_FLAGS(vict), PLR_GOB_DONE);
		SET_BIT(PLR_FLAGS(vict), PLR_FISH_DONE);
	    }
	    vict->aff_abils = vict->real_abils;
	}
	update_pos(vict);
	send_to_char(OK, ch);
	act("You have been fully healed by $N!", FALSE, vict, 0, ch,
	    TO_CHAR);
    }
}


void perform_immort_vis(struct char_data *ch)
{
    if (GET_INVIS_LEV(ch) == 0
	&& !AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE)) {
	send_to_char("You are already fully visible.\r\n", ch);
	return;
    }

    GET_INVIS_LEV(ch) = 0;
    appear(ch);
    send_to_char("You are now fully visible.\r\n", ch);
}


void perform_immort_invis(struct char_data *ch, int level)
{
    struct char_data *tch;

    if (IS_NPC(ch))
	return;

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
	if (tch == ch)
	    continue;
	if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
	    act("You blink and suddenly realize that $n is gone.", FALSE,
		ch, 0, tch, TO_VICT);
	if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
	    act("You suddenly realize that $n is standing beside you.",
		FALSE, ch, 0, tch, TO_VICT);
    }

    GET_INVIS_LEV(ch) = level;
    sprintf(buf, "Your invisibility level is %d.\r\n", level);
    send_to_char(buf, ch);
}


ACMD(do_invis)
{
    int level;

    if (IS_NPC(ch)) {
	send_to_char("You can't do that!\r\n", ch);
	return;
    }

    one_argument(argument, arg);
    if (!*arg) {
	if (GET_INVIS_LEV(ch) > 0)
	    perform_immort_vis(ch);
	else
	    perform_immort_invis(ch, GET_LEVEL(ch));
    } else {
	level = atoi(arg);
	if (level > GET_LEVEL(ch))
	    send_to_char
		("You can't go invisible above your own level.\r\n", ch);
	else if (level < 1)
	    perform_immort_vis(ch);
	else
	    perform_immort_invis(ch, level);
    }
}


ACMD(do_gecho)
{
    struct descriptor_data *pt;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument)
	send_to_char("That must be a mistake...\r\n", ch);
    else {
	sprintf(buf, "%s\r\n", argument);
	for (pt = descriptor_list; pt; pt = pt->next)
	    if (STATE(pt) == CON_PLAYING && pt->character
		&& pt->character != ch)
		send_to_char(buf, pt->character);
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	    send_to_char(OK, ch);
	else
	    send_to_char(buf, ch);
    }
}


ACMD(do_poofset)
{
    char **msg;

    switch (subcmd) {
    case SCMD_POOFIN:
	msg = &(POOFIN(ch));
	break;
    case SCMD_POOFOUT:
	msg = &(POOFOUT(ch));
	break;
    default:
	return;
    }

    skip_spaces(&argument);

    if (*msg)
	free(*msg);

    if (!*argument)
	*msg = NULL;
    else
	*msg = str_dup(argument);

    send_to_char(OK, ch);
}



ACMD(do_dc)
{
    struct descriptor_data *d;
    int num_to_dc;

    one_argument(argument, arg);
    if (!(num_to_dc = atoi(arg))) {
	send_to_char("Usage: DC <user number> (type USERS for a list)\r\n",
		     ch);
	return;
    }
    for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

    if (!d) {
	send_to_char("No such connection.\r\n", ch);
	return;
    }
    if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
	if (!CAN_SEE(ch, d->character))
	    send_to_char("No such connection.\r\n", ch);
	else
	    send_to_char("Umm.. maybe that's not such a good idea...\r\n",
			 ch);
	return;
    }

    /* We used to just close the socket here using close_socket(), but
     * various people pointed out this could cause a crash if you're
     * closing the person below you on the descriptor list.  Just setting
     * to CON_CLOSE leaves things in a massively inconsistent state so I
     * had to add this new flag to the descriptor. -je
     *
     * It is a much more logical extension for a CON_DISCONNECT to be used
     * for in-game socket closes and CON_CLOSE for out of game closings.
     * This will retain the stability of the close_me hack while being
     * neater in appearance. -gg 12/1/97
     *
     * For those unlucky souls who actually manage to get disconnected
     * by two different immortals in the same 1/10th of a second, we have
     * the below 'if' check. -gg 12/17/99
     */
    if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
	send_to_char("They're already being disconnected.\r\n", ch);
    else {
	/*   
	 * Remember that we can disconnect people not in the game and
	 * that rather confuses the code when it expected there to be
	 * a character context.
	 */
	if (STATE(d) == CON_PLAYING)
	    STATE(d) = CON_DISCONNECT;
	else
	    STATE(d) = CON_CLOSE;

	sprintf(buf, "Connection #%d closed.\r\n", num_to_dc);
	send_to_char(buf, ch);
	log("(GC) Connection closed by %s.", GET_NAME(ch));
    }
}



ACMD(do_wizlock)
{
    int value;
    const char *when;

    one_argument(argument, arg);
    if (*arg) {
	value = atoi(arg);
	if (value < 0 || value > GET_LEVEL(ch)) {
	    send_to_char("Invalid wizlock value.\r\n", ch);
	    return;
	}
	circle_restrict = value;
	when = "now";
    } else
	when = "currently";

    switch (circle_restrict) {
    case 0:
	sprintf(buf, "The game is %s completely open.\r\n", when);
	break;
    case 1:
	sprintf(buf, "The game is %s closed to new players.\r\n", when);
	break;
    default:
	sprintf(buf, "Only level %d and above may enter the game %s.\r\n",
		circle_restrict, when);
	break;
    }
    send_to_char(buf, ch);
}


ACMD(do_date)
{
    char *tmstr;
    time_t mytime;
    int d, h, m;

    if (subcmd == SCMD_DATE)
	mytime = time(0);
    else
	mytime = boot_time;

    tmstr = (char *) asctime(localtime(&mytime));
    *(tmstr + strlen(tmstr) - 1) = '\0';

    if (subcmd == SCMD_DATE)
	sprintf(buf, "Current machine time: %s\r\n", tmstr);
    else {
	mytime = time(0) - boot_time;
	d = mytime / 86400;
	h = (mytime / 3600) % 24;
	m = (mytime / 60) % 60;

	sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
		((d == 1) ? "" : "s"), h, m);
    }

    send_to_char(buf, ch);
}



ACMD(do_last)
{
    struct char_file_u chdata;

    one_argument(argument, arg);
    if (!*arg) {
	send_to_char("For whom do you wish to search?\r\n", ch);
	return;
    }
    if (load_char(arg, &chdata) < 0) {
	send_to_char("There is no such player.\r\n", ch);
	return;
    }
    if ((chdata.level > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_IMPL)) {
	send_to_char("You are not sufficiently godly for that!\r\n", ch);
	return;
    }
    sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
	    chdata.char_specials_saved.idnum, (int) chdata.level,
	    class_abbrevs[(int) chdata.chclass], chdata.name, chdata.host,
	    ctime(&chdata.last_logon));
    send_to_char(buf, ch);
}


ACMD(do_force)
{
    struct descriptor_data *i, *next_desc;
    struct char_data *vict, *next_force;
    char to_force[MAX_INPUT_LENGTH + 2];

    half_chop(argument, arg, to_force);

    sprintf(buf1, "$n has forced you to '%s'.", to_force);

    if (!*arg || !*to_force)
	send_to_char("Whom do you wish to force do what?\r\n", ch);
    else if ((GET_LEVEL(ch) < LVL_GRGOD)
	     || (str_cmp("all", arg) && str_cmp("room", arg))) {
	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
	    send_to_char(NOPERSON, ch);
	else if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict))
	    send_to_char("No, no, no!\r\n", ch);
	else {
	    send_to_char(OK, ch);
	    act(buf1, TRUE, ch, NULL, vict, TO_VICT);
	    sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch),
		    GET_NAME(vict), to_force);
	    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	    command_interpreter(vict, to_force);
	}
    } else if (!str_cmp("room", arg)) {
	send_to_char(OK, ch);
	sprintf(buf, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);
	mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

	for (vict = world[ch->in_room].people; vict; vict = next_force) {
	    next_force = vict->next_in_room;
	    if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch))
		continue;
	    act(buf1, TRUE, ch, NULL, vict, TO_VICT);
	    command_interpreter(vict, to_force);
	}
    } else {			/* force all */
	send_to_char(OK, ch);
	sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
	mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

	for (i = descriptor_list; i; i = next_desc) {
	    next_desc = i->next;

	    if (STATE(i) != CON_PLAYING || !(vict = i->character)
		|| (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)))
		continue;
	    act(buf1, TRUE, ch, NULL, vict, TO_VICT);
	    command_interpreter(vict, to_force);
	}
    }
}



ACMD(do_wiznet)
{
    struct descriptor_data *d;
    char emote = FALSE;
    char any = FALSE;
    int level = LVL_IMMORT;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument) {
	send_to_char
	    ("Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n "
	     "       wiznet @<level> *<emotetext> | wiz @\r\n", ch);
	return;
    }
    switch (*argument) {
    case '*':
	emote = TRUE;
    case '#':
	one_argument(argument + 1, buf1);
	if (is_number(buf1)) {
	    half_chop(argument + 1, buf1, argument);
	    level = MAX(atoi(buf1), LVL_IMMORT);
	    if (level > GET_LEVEL(ch)) {
		send_to_char("You can't wizline above your own level.\r\n",
			     ch);
		return;
	    }
	} else if (emote)
	    argument++;
	break;
    case '@':
	for (d = descriptor_list; d; d = d->next) {
	    if (STATE(d) == CON_PLAYING
		&& GET_LEVEL(d->character) >= LVL_IMMORT
		&& !PRF_FLAGGED(d->character, PRF_NOWIZ)
		&& (CAN_SEE(ch, d->character)
		    || GET_LEVEL(ch) == LVL_IMPL)) {
		if (!any) {
		    strcpy(buf1, "Gods online:\r\n");
		    any = TRUE;
		}
		sprintf(buf1 + strlen(buf1), "  %s",
			GET_NAME(d->character));
		if (PLR_FLAGGED(d->character, PLR_WRITING))
		    strcat(buf1, " (Writing)\r\n");
		else if (PLR_FLAGGED(d->character, PLR_MAILING))
		    strcat(buf1, " (Writing mail)\r\n");
		else
		    strcat(buf1, "\r\n");

	    }
	}
	any = FALSE;
	for (d = descriptor_list; d; d = d->next) {
	    if (STATE(d) == CON_PLAYING
		&& GET_LEVEL(d->character) >= LVL_IMMORT
		&& PRF_FLAGGED(d->character, PRF_NOWIZ)
		&& CAN_SEE(ch, d->character)) {
		if (!any) {
		    strcat(buf1, "Gods offline:\r\n");
		    any = TRUE;
		}
		sprintf(buf1 + strlen(buf1), "  %s\r\n",
			GET_NAME(d->character));
	    }
	}
	send_to_char(buf1, ch);
	return;
    case '\\':
	++argument;
	break;
    default:
	break;
    }
    if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
	send_to_char("You are offline!\r\n", ch);
	return;
    }
    skip_spaces(&argument);

    if (!*argument) {
	send_to_char("Don't bother the gods like that!\r\n", ch);
	return;
    }
    if (level > LVL_IMMORT) {
	sprintf(buf1, "%s: <%d> %s%s\r\n", GET_NAME(ch), level,
		emote ? "<--- " : "", argument);
	sprintf(buf2, "Someone: <%d> %s%s\r\n", level,
		emote ? "<--- " : "", argument);
    } else {
	sprintf(buf1, "%s: %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
		argument);
	sprintf(buf2, "Someone: %s%s\r\n", emote ? "<--- " : "", argument);
    }

    for (d = descriptor_list; d; d = d->next) {
	if ((STATE(d) == CON_PLAYING) && (GET_LEVEL(d->character) >= level)
	    && (!PRF_FLAGGED(d->character, PRF_NOWIZ))
	    && (!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING))
	    && (d != ch->desc
		|| !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
	    send_to_char(CCCYN(d->character, C_NRM), d->character);
	    if (CAN_SEE(d->character, ch))
		send_to_char(buf1, d->character);
	    else
		send_to_char(buf2, d->character);
	    send_to_char(CCNRM(d->character, C_NRM), d->character);
	}
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(OK, ch);
}



ACMD(do_zreset)
{
    zone_rnum i;
    zone_vnum j;

    one_argument(argument, arg);
    if (!*arg) {
	send_to_char("You must specify a zone.\r\n", ch);
	return;
    }
    if (*arg == '*') {
	for (i = 0; i <= top_of_zone_table; i++)
	    reset_zone(i);
	send_to_char("Reset world.\r\n", ch);
	sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
	mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
	return;
    } else if (*arg == '.')
	i = world[ch->in_room].zone;
    else {
	j = atoi(arg);
	for (i = 0; i <= top_of_zone_table; i++)
	    if (zone_table[i].number == j)
		break;
    }
    if (i >= 0 && i <= top_of_zone_table) {
	reset_zone(i);
	sprintf(buf, "Reset zone %d (#%d): %s.\r\n", i,
		zone_table[i].number, zone_table[i].name);
	send_to_char(buf, ch);
	sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i,
		zone_table[i].name);
	mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
    } else
	send_to_char("Invalid zone number.\r\n", ch);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
    struct char_data *vict;
    long result;

    one_argument(argument, arg);

    if (!*arg)
	send_to_char("Yes, but for whom?!?\r\n", ch);
    else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
	send_to_char("There is no such player.\r\n", ch);
    else if (IS_NPC(vict))
	send_to_char("You can't do that to a mob!\r\n", ch);
    else if (GET_LEVEL(vict) > GET_LEVEL(ch))
	send_to_char("Hmmm...you'd better not.\r\n", ch);
    else {
	switch (subcmd) {
	case SCMD_REROLL:
	    send_to_char("Rerolled...\r\n", ch);
	    roll_real_abils(vict);
	    log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
	    sprintf(buf,
		    "New stats: Str %d//%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
		    GET_STR(vict), GET_ADD(vict), GET_INT(vict),
		    GET_WIS(vict), GET_DEX(vict), GET_CON(vict),
		    GET_CHA(vict));
	    send_to_char(buf, ch);
	    break;
	case SCMD_PARDON:
	    if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
		send_to_char("Your victim is not flagged.\r\n", ch);
		return;
	    }
	    REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
	    send_to_char("Pardoned.\r\n", ch);
	    send_to_char("You have been pardoned by the Gods!\r\n", vict);
	    sprintf(buf, "(GC) %s pardoned by %s", GET_NAME(vict),
		    GET_NAME(ch));
	    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	    break;
	case SCMD_NOTITLE:
	    result = PLR_TOG_CHK(vict, PLR_NOTITLE);
	    sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result),
		    GET_NAME(vict), GET_NAME(ch));
	    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	    strcat(buf, "\r\n");
	    send_to_char(buf, ch);
	    break;
	case SCMD_SQUELCH:
	    result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
	    sprintf(buf, "(GC) Squelch %s for %s by %s.", ONOFF(result),
		    GET_NAME(vict), GET_NAME(ch));
	    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	    strcat(buf, "\r\n");
	    send_to_char(buf, ch);
	    break;
	case SCMD_FREEZE:
	    if (ch == vict) {
		send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
		return;
	    }
	    if (PLR_FLAGGED(vict, PLR_FROZEN)) {
		send_to_char("Your victim is already pretty cold.\r\n",
			     ch);
		return;
	    }
	    SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
	    GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
	    send_to_char
		("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n",
		 vict);
	    send_to_char("Frozen.\r\n", ch);
	    act("A sudden cold wind conjured from nowhere freezes $n!",
		FALSE, vict, 0, 0, TO_ROOM);
	    sprintf(buf, "(GC) %s frozen by %s.", GET_NAME(vict),
		    GET_NAME(ch));
	    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	    break;
	case SCMD_THAW:
	    if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
		send_to_char
		    ("Sorry, your victim is not morbidly encased in ice at the moment.\r\n",
		     ch);
		return;
	    }
	    if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
		sprintf(buf,
			"Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
			GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
		send_to_char(buf, ch);
		return;
	    }
	    sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict),
		    GET_NAME(ch));
	    mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
	    REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
	    send_to_char
		("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n",
		 vict);
	    send_to_char("Thawed.\r\n", ch);
	    act("A sudden fireball conjured from nowhere thaws $n!", FALSE,
		vict, 0, 0, TO_ROOM);
	    break;
	case SCMD_UNAFFECT:
	    if (vict->affected) {
		while (vict->affected)
		    affect_remove(vict, vict->affected);
		send_to_char("There is a brief flash of light!\r\n"
			     "You feel slightly different.\r\n", vict);
		send_to_char("All spells removed.\r\n", ch);
	    } else {
		send_to_char
		    ("Your victim does not have any affections!\r\n", ch);
		return;
	    }
	    break;
	default:
	    log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)",
		subcmd, __FILE__);
	    break;
	}
	save_char(vict, NOWHERE);
    }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, zone_rnum zone)
{
    sprintf(bufptr,
	    "%s%3d %-30.30s Age: %3d; Reset: %3d (%1d); Top: %5d\r\n",
	    bufptr, zone_table[zone].number, zone_table[zone].name,
	    zone_table[zone].age, zone_table[zone].lifespan,
	    zone_table[zone].reset_mode, zone_table[zone].top);
}


ACMD(do_show)
{
    struct char_file_u vbuf;
    int i, j, k, l, con;	/* i, j, k to specifics? */
    zone_rnum zrn;
    zone_vnum zvn;
    char self = 0;
    struct char_data *vict;
    struct obj_data *obj;
    struct descriptor_data *d;
    char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], birth[80];

    struct show_struct {
	const char *cmd;
	const char level;
    } fields[] = {
	{
	"nothing", 0},		/* 0 */
	{
	"zones", LVL_IMMORT},	/* 1 */
	{
	"player", LVL_GOD}, {
	"rent", LVL_GOD}, {
	"stats", LVL_IMMORT}, {
	"errors", LVL_IMPL},	/* 5 */
	{
	"death", LVL_GOD}, {
	"godrooms", LVL_GOD}, {
	"shops", LVL_IMMORT}, {
	"houses", LVL_GOD}, {
	"snoop", LVL_GRGOD},	/* 10 */
	{
	"assemblies", LVL_GRGOD}, {
	"\n", 0}
    };

    skip_spaces(&argument);

    if (!*argument) {
	strcpy(buf, "Show options:\r\n");
	for (j = 0, i = 1; fields[i].level; i++)
	    if (fields[i].level <= GET_LEVEL(ch))
		sprintf(buf + strlen(buf), "%-15s%s", fields[i].cmd,
			(!(++j % 5) ? "\r\n" : ""));
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	return;
    }

    strcpy(arg, two_arguments(argument, field, value));

    for (l = 0; *(fields[l].cmd) != '\n'; l++)
	if (!strncmp(field, fields[l].cmd, strlen(field)))
	    break;

    if (GET_LEVEL(ch) < fields[l].level) {
	send_to_char("You are not godly enough for that!\r\n", ch);
	return;
    }
    if (!strcmp(value, "."))
	self = 1;
    buf[0] = '\0';
    switch (l) {
    case 1:			/* zone */
	/* tightened up by JE 4/6/93 */
	if (self)
	    print_zone_to_buf(buf, world[ch->in_room].zone);
	else if (*value && is_number(value)) {
	    for (zvn = atoi(value), zrn = 0;
		 zone_table[zrn].number != zvn && zrn <= top_of_zone_table;
		 zrn++);
	    if (zrn <= top_of_zone_table)
		print_zone_to_buf(buf, zrn);
	    else {
		send_to_char("That is not a valid zone.\r\n", ch);
		return;
	    }
	} else
	    for (zrn = 0; zrn <= top_of_zone_table; zrn++)
		print_zone_to_buf(buf, zrn);
	page_string(ch->desc, buf, TRUE);
	break;
    case 2:			/* player */
	if (!*value) {
	    send_to_char("A name would help.\r\n", ch);
	    return;
	}

	if (load_char(value, &vbuf) < 0) {
	    send_to_char("There is no such player.\r\n", ch);
	    return;
	}
	sprintf(buf, "Player: %-12s (%s) [%2d %s]\r\n", vbuf.name,
		genders[(int) vbuf.sex], vbuf.level,
		class_abbrevs[(int) vbuf.chclass]);
	sprintf(buf + strlen(buf),
		"Au: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n",
		vbuf.points.money[1], vbuf.points.bank_gold,
		vbuf.points.exp, vbuf.char_specials_saved.alignment,
		vbuf.player_specials_saved.spells_to_learn);
	strcpy(birth, ctime(&vbuf.birth));
	sprintf(buf + strlen(buf),
		"Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
		birth, ctime(&vbuf.last_logon), (int) (vbuf.played / 3600),
		(int) (vbuf.played / 60 % 60));
	send_to_char(buf, ch);
	break;
    case 3:
	if (!*value) {
	    send_to_char("A name would help.\r\n", ch);
	    return;
	}
	Crash_listrent(ch, value);
	break;
    case 4:
	i = 0;
	j = 0;
	k = 0;
	con = 0;
	for (vict = character_list; vict; vict = vict->next) {
	    if (IS_NPC(vict))
		j++;
	    else if (CAN_SEE(ch, vict)) {
		i++;
		if (vict->desc)
		    con++;
	    }
	}
	for (obj = object_list; obj; obj = obj->next)
	    k++;
	strcpy(buf, "Current stats:\r\n");
	sprintf(buf + strlen(buf),
		"  %5d players in game  %5d connected\r\n", i, con);
	sprintf(buf + strlen(buf), "  %5d registered\r\n",
		top_of_p_table + 1);
	sprintf(buf + strlen(buf),
		"  %5d mobiles          %5d prototypes\r\n", j,
		top_of_mobt + 1);
	sprintf(buf + strlen(buf),
		"  %5d objects          %5d prototypes\r\n", k,
		top_of_objt + 1);
	sprintf(buf + strlen(buf), "  %5d rooms            %5d zones\r\n",
		top_of_world + 1, top_of_zone_table + 1);
	sprintf(buf + strlen(buf), "  %5d large bufs\r\n", buf_largecount);
	sprintf(buf + strlen(buf),
		"  %5d buf switches     %5d overflows\r\n", buf_switches,
		buf_overflows);
	send_to_char(buf, ch);
	break;
    case 5:
	strcpy(buf, "Errant Rooms\r\n------------\r\n");
	for (i = 0, k = 0; i <= top_of_world; i++)
	    for (j = 0; j < NUM_OF_DIRS; j++)
		if (world[i].dir_option[j]
		    && world[i].dir_option[j]->to_room == 0)
		    sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k,
			    GET_ROOM_VNUM(i), world[i].name);
	page_string(ch->desc, buf, TRUE);
	break;
    case 6:
	strcpy(buf, "Death Traps\r\n-----------\r\n");
	for (i = 0, j = 0; i <= top_of_world; i++)
	    if (ROOM_FLAGGED(i, ROOM_DEATH))
		sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j,
			GET_ROOM_VNUM(i), world[i].name);
	page_string(ch->desc, buf, TRUE);
	break;
    case 7:
	strcpy(buf, "Godrooms\r\n--------------------------\r\n");
	for (i = 0, j = 0; i <= top_of_world; i++)
	    if (ROOM_FLAGGED(i, ROOM_GODROOM))
		sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n",
			++j, GET_ROOM_VNUM(i), world[i].name);
	page_string(ch->desc, buf, TRUE);
	break;
    case 8:
	show_shops(ch, value);
	break;
    case 9:
	send_to_char
	    ("This has been disabled I'm to lazy to change the cases\r\n",
	     ch);
	break;
    case 10:
	*buf = '\0';
	send_to_char("People currently snooping:\r\n", ch);
	send_to_char("--------------------------\r\n", ch);
	for (d = descriptor_list; d; d = d->next) {
	    if (d->snooping == NULL || d->character == NULL)
		continue;
	    if (STATE(d) != CON_PLAYING
		|| GET_LEVEL(ch) < GET_LEVEL(d->character)) continue;
	    if (!CAN_SEE(ch, d->character)
		|| IN_ROOM(d->character) == NOWHERE) continue;
	    sprintf(buf + strlen(buf), "%-10s - snooped by %s.\r\n",
		    GET_NAME(d->snooping->character),
		    GET_NAME(d->character));
	}
	send_to_char(*buf ? buf : "No one is currently snooping.\r\n", ch);
	break;			/* snoop */
    case 11:
	assemblyListToChar(ch);
	break;
    default:
	send_to_char("Sorry, I don't understand that.\r\n", ch);
	break;
    }
}


/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))


/* The set options available */
struct set_struct {
    const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
} set_fields[] = {

    {
    "brief", LVL_GOD, PC, BINARY},	/* 0 */
    {
    "invstart", LVL_GOD, PC, BINARY},	/* 1 */
    {
    "NOT USED", LVL_GOD, PC, MISC}, {
    "nosummon", LVL_GRGOD, PC, BINARY}, {
    "maxhit", LVL_GRGOD, BOTH, NUMBER}, {
    "maxmana", LVL_GRGOD, BOTH, NUMBER},	/* 5 */
    {
    "maxmove", LVL_GRGOD, BOTH, NUMBER}, {
    "hit", LVL_GRGOD, BOTH, NUMBER}, {
    "mana", LVL_GRGOD, BOTH, NUMBER}, {
    "move", LVL_GRGOD, BOTH, NUMBER}, {
    "align", LVL_GOD, BOTH, NUMBER},	/* 10 */
    {
    "str", LVL_GRGOD, BOTH, NUMBER}, {
    "stradd", LVL_GRGOD, BOTH, NUMBER}, {
    "int", LVL_GRGOD, BOTH, NUMBER}, {
    "wis", LVL_GRGOD, BOTH, NUMBER}, {
    "dex", LVL_GRGOD, BOTH, NUMBER},	/* 15 */
    {
    "con", LVL_GRGOD, BOTH, NUMBER}, {
    "cha", LVL_GRGOD, BOTH, NUMBER}, {
    "ac", LVL_GRGOD, BOTH, NUMBER}, {
    "gold", LVL_GOD, BOTH, NUMBER}, {
    "bank", LVL_GOD, PC, NUMBER},	/* 20 */
    {
    "exp", LVL_GRGOD, BOTH, NUMBER}, {
    "hitroll", LVL_GRGOD, BOTH, NUMBER}, {
    "damroll", LVL_GRGOD, BOTH, NUMBER}, {
    "invis", LVL_IMPL, PC, NUMBER}, {
    "nohassle", LVL_GRGOD, PC, BINARY},	/* 25 */
    {
    "frozen", LVL_FREEZE, PC, BINARY}, {
    "practices", LVL_GRGOD, PC, NUMBER}, {
    "lessons", LVL_GRGOD, PC, NUMBER}, {
    "drunk", LVL_GRGOD, BOTH, MISC}, {
    "hunger", LVL_GRGOD, BOTH, MISC},	/* 30 */
    {
    "thirst", LVL_GRGOD, BOTH, MISC}, {
    "killer", LVL_GOD, PC, BINARY}, {
    "thief", LVL_GOD, PC, BINARY}, {
    "level", LVL_IMPL, BOTH, NUMBER}, {
    "room", LVL_IMPL, BOTH, NUMBER},	/* 35 */
    {
    "roomflag", LVL_GRGOD, PC, BINARY}, {
    "siteok", LVL_GRGOD, PC, BINARY}, {
    "deleted", LVL_IMPL, PC, BINARY}, {
    "class", LVL_GRGOD, BOTH, MISC}, {
    "nowizlist", LVL_GOD, PC, BINARY},	/* 40 */
    {
    "quest", LVL_GOD, PC, BINARY}, {
    "loadroom", LVL_GRGOD, PC, MISC}, {
    "color", LVL_GOD, PC, BINARY}, {
    "idnum", LVL_IMPL, PC, NUMBER}, {
    "passwd", LVL_IMPL, PC, MISC},	/* 45 */
    {
    "nodelete", LVL_GOD, PC, BINARY}, {
    "sex", LVL_GRGOD, BOTH, MISC}, {
    "age", LVL_GRGOD, BOTH, NUMBER}, {
    "height", LVL_GOD, BOTH, NUMBER}, {
    "weight", LVL_GOD, BOTH, NUMBER},	/* 50 */
    {
    "olc", LVL_IMPL, PC, NUMBER}, {
    "house", LVL_GOD, PC, NUMBER}, {
    "rank", LVL_GOD, PC, NUMBER}, {
    "platinum", LVL_GRGOD, BOTH, NUMBER}, {
    "silver", LVL_GRGOD, BOTH, NUMBER}, {
    "copper", LVL_GRGOD, BOTH, NUMBER}, {
    "bot", LVL_GOD, PC, BINARY}, {
    "debug", LVL_GOD, PC, BINARY}, {
    "\n", 0, BOTH, MISC}
};


int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
    int i, on = 0, off = 0, value = 0;
    room_rnum rnum;
    room_vnum rvnum;
    char output[MAX_STRING_LENGTH];

    /* Check to make sure all the levels are correct */
    if (GET_LEVEL(ch) != LVL_IMPL) {
	if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict)
	    && vict != ch) {
	    send_to_char("Maybe that's not such a great idea...\r\n", ch);
	    return (0);
	}
    }
    if (GET_LEVEL(ch) < set_fields[mode].level) {
	send_to_char("You are not godly enough for that!\r\n", ch);
	return (0);
    }

    /* Make sure the PC/NPC is correct */
    if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
	send_to_char("You can't do that to a beast!\r\n", ch);
	return (0);
    } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
	send_to_char("That can only be done to a beast!\r\n", ch);
	return (0);
    }

    /* Find the value of the argument */
    if (set_fields[mode].type == BINARY) {
	if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
	    on = 1;
	else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
	    off = 1;
	if (!(on || off)) {
	    send_to_char("Value must be 'on' or 'off'.\r\n", ch);
	    return (0);
	}
	sprintf(output, "%s %s for %s.", set_fields[mode].cmd, ONOFF(on),
		GET_NAME(vict));
    } else if (set_fields[mode].type == NUMBER) {
	value = atoi(val_arg);
	sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
		set_fields[mode].cmd, value);
    } else {
	strcpy(output, "Okay.");	/* can't use OK macro here 'cause of \r\n */
    }

    switch (mode) {
    case 0:
	SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
	break;
    case 1:
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
	break;
    case 3:
	SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
	sprintf(output, "Nosummon %s for %s.\r\n", ONOFF(!on),
		GET_NAME(vict));
	break;
    case 4:
	vict->points.max_hit = RANGE(1, 5000);
	affect_total(vict);
	break;
    case 5:
	vict->points.max_mana = RANGE(1, 5000);
	affect_total(vict);
	break;
    case 6:
	vict->points.max_move = RANGE(1, 5000);
	affect_total(vict);
	break;
    case 7:
	vict->points.hit = RANGE(-9, vict->points.max_hit);
	affect_total(vict);
	break;
    case 8:
	vict->points.mana = RANGE(0, vict->points.max_mana);
	affect_total(vict);
	break;
    case 9:
	vict->points.move = RANGE(0, vict->points.max_move);
	affect_total(vict);
	break;
    case 10:
	GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
	affect_total(vict);
	break;
    case 11:
	if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
	    RANGE(3, 25);
	else
	    RANGE(3, 18);
	vict->real_abils.str = value;
	vict->real_abils.str_add = 0;
	affect_total(vict);
	break;
    case 12:
	vict->real_abils.str_add = RANGE(0, 100);
	if (value > 0)
	    vict->real_abils.str = 18;
	affect_total(vict);
	break;
    case 13:
	if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
	    RANGE(3, 25);
	else
	    RANGE(3, 18);
	vict->real_abils.intel = value;
	affect_total(vict);
	break;
    case 14:
	if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
	    RANGE(3, 25);
	else
	    RANGE(3, 18);
	vict->real_abils.wis = value;
	affect_total(vict);
	break;
    case 15:
	if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
	    RANGE(3, 25);
	else
	    RANGE(3, 18);
	vict->real_abils.dex = value;
	affect_total(vict);
	break;
    case 16:
	if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
	    RANGE(3, 25);
	else
	    RANGE(3, 18);
	vict->real_abils.con = value;
	affect_total(vict);
	break;
    case 17:
	if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
	    RANGE(3, 25);
	else
	    RANGE(3, 18);
	vict->real_abils.cha = value;
	affect_total(vict);
	break;
    case 18:
	vict->points.armor = RANGE(10, 200);
	affect_total(vict);
	break;
    case 19:
	remove_money_from_char(vict, GET_MONEY(vict, GOLD_COINS),
			       GOLD_COINS);
	add_money_to_char(vict, RANGE(0, 100000000), GOLD_COINS);
	break;

    case 20:
	GET_BANK_GOLD(vict) = RANGE(0, 100000000);
	break;
    case 21:
	vict->points.exp = RANGE(0, 50000000);
	break;
    case 22:
	vict->points.hitroll = RANGE(-20, 20);
	affect_total(vict);
	break;
    case 23:
	vict->points.damroll = RANGE(-20, 20);
	affect_total(vict);
	break;
    case 24:
	if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
	    send_to_char("You aren't godly enough for that!\r\n", ch);
	    return (0);
	}
	GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
	break;
    case 25:
	if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
	    send_to_char("You aren't godly enough for that!\r\n", ch);
	    return (0);
	}
	SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
	break;
    case 26:
	if (ch == vict && on) {
	    send_to_char("Better not -- could be a long winter!\r\n", ch);
	    return (0);
	}
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
	break;
    case 27:
    case 28:
	GET_PRACTICES(vict) = RANGE(0, 100);
	break;
    case 29:
    case 30:
    case 31:
	if (!str_cmp(val_arg, "off")) {
	    GET_COND(vict, (mode - 29)) = (char) -1;	/* warning: magic number here */
	    sprintf(output, "%s's %s now off.", GET_NAME(vict),
		    set_fields[mode].cmd);
	} else if (is_number(val_arg)) {
	    value = atoi(val_arg);
	    RANGE(0, 24);
	    GET_COND(vict, (mode - 29)) = (char) value;	/* and here too */
	    sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
		    set_fields[mode].cmd, value);
	} else {
	    send_to_char("Must be 'off' or a value from 0 to 24.\r\n", ch);
	    return (0);
	}
	break;
    case 32:
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
	break;
    case 33:
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
	break;
    case 34:
	if (value > GET_LEVEL(ch) || value > LVL_IMPL) {
	    send_to_char("You can't do that.\r\n", ch);
	    return (0);
	}
	RANGE(0, LVL_IMPL);
	vict->player.level = (byte) value;
	break;
    case 35:
	if ((rnum = real_room(value)) < 0) {
	    send_to_char("No room exists with that number.\r\n", ch);
	    return (0);
	}
	if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
	    char_from_room(vict);
	char_to_room(vict, rnum);
	break;
    case 36:
	SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
	break;
    case 37:
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
	break;
    case 38:
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
	break;
    case 39:
	if ((i = parse_class(*val_arg)) == CLASS_UNDEFINED) {
	    send_to_char("That is not a class.\r\n", ch);
	    return (0);
	}
	GET_CLASS(vict) = i;
	break;
    case 40:
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
	break;
    case 41:
	SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
	break;
    case 42:
	if (!str_cmp(val_arg, "off")) {
	    REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
	} else if (is_number(val_arg)) {
	    rvnum = atoi(val_arg);
	    if (real_room(rvnum) != NOWHERE) {
		SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
		GET_LOADROOM(vict) = rvnum;
		sprintf(output, "%s will enter at room #%d.",
			GET_NAME(vict), GET_LOADROOM(vict));
	    } else {
		send_to_char("That room does not exist!\r\n", ch);
		return (0);
	    }
	} else {
	    send_to_char("Must be 'off' or a room's virtual number.\r\n",
			 ch);
	    return (0);
	}
	break;
    case 43:
	SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
	break;
    case 44:
	if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
	    return (0);
	GET_IDNUM(vict) = value;
	break;
    case 45:
	if (GET_IDNUM(ch) > 1) {
	    send_to_char("Please don't use this command, yet.\r\n", ch);
	    return (0);
	}
	if (GET_LEVEL(vict) >= LVL_GRGOD) {
	    send_to_char("You cannot change that.\r\n", ch);
	    return (0);
	}
	strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)),
		MAX_PWD_LENGTH);
	*(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
	sprintf(output, "Password changed to '%s'.", val_arg);
	break;
    case 46:
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
	break;
    case 47:
	if ((i = search_block(val_arg, genders, FALSE)) < 0) {
	    send_to_char("Must be 'male', 'female', or 'neutral'.\r\n",
			 ch);
	    return (0);
	}
	GET_SEX(vict) = i;
	break;
    case 48:			/* set age */
	if (value < 2 || value > 200) {	/* Arbitrary limits. */
	    send_to_char("Ages 2 to 200 accepted.\r\n", ch);
	    return (0);
	}
	/*
	 * NOTE: May not display the exact age specified due to the integer
	 * division used elsewhere in the code.  Seems to only happen for
	 * some values below the starting age (17) anyway. -gg 5/27/98
	 */
	vict->player.time.birth =
	    time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
	break;

    case 49:			/* Blame/Thank Rick Glover. :) */
	GET_HEIGHT(vict) = value;
	affect_total(vict);
	break;

    case 50:
	GET_WEIGHT(vict) = value;
	affect_total(vict);
	break;

    case 51:
	GET_OLC_ZONE(vict) = value;
	break;

    case 52:
	GET_HOUSE(vict) = value;
	break;
    case 53:
	GET_RANK(vict) = value;
	break;
    case 54:			/* Substitute the proper case numbers for your set_struct */
	remove_money_from_char(vict, GET_MONEY(vict, PLAT_COINS),
			       PLAT_COINS);
	add_money_to_char(vict, RANGE(0, 100000000), PLAT_COINS);
	break;
    case 55:
	remove_money_from_char(vict, GET_MONEY(vict, SILVER_COINS),
			       SILVER_COINS);
	add_money_to_char(vict, RANGE(0, 100000000), SILVER_COINS);
	break;
    case 56:
	remove_money_from_char(vict, GET_MONEY(vict, COPPER_COINS),
			       COPPER_COINS);
	add_money_to_char(vict, RANGE(0, 100000000), COPPER_COINS);
	break;
    case 57:
	SET_OR_REMOVE(PLR_FLAGS(vict), PLR_BOT);
	break;
    case 58:
	SET_OR_REMOVE(PRF_FLAGS(vict), PRF_DEBUG);
	break;
    default:
	send_to_char("Can't set that!\r\n", ch);
	return (0);
    }

    strcat(output, "\r\n");
    send_to_char(CAP(output), ch);
    return (1);
}


ACMD(do_set)
{
    struct char_data *vict = NULL, *cbuf = NULL;
    struct char_file_u tmp_store;
    char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH],
	val_arg[MAX_INPUT_LENGTH];
    int mode, len, player_i = 0, retval;
    char is_file = 0, is_player = 0;

    half_chop(argument, name, buf);

    if (!strcmp(name, "file")) {
	is_file = 1;
	half_chop(buf, name, buf);
    } else if (!str_cmp(name, "player")) {
	is_player = 1;
	half_chop(buf, name, buf);
    } else if (!str_cmp(name, "mob"))
	half_chop(buf, name, buf);

    half_chop(buf, field, buf);
    strcpy(val_arg, buf);

    if (!*name || !*field) {
	send_to_char("Usage: set <victim> <field> <value>\r\n", ch);
	return;
    }

    /* find the target */
    if (!is_file) {
	if (is_player) {
	    if (!(vict = get_player_vis(ch, name, FIND_CHAR_WORLD))) {
		send_to_char("There is no such player.\r\n", ch);
		return;
	    }
	} else {		/* is_mob */
	    if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
		send_to_char("There is no such creature.\r\n", ch);
		return;
	    }
	}
    } else if (is_file) {
	/* try to load the player off disk */
	CREATE(cbuf, struct char_data, 1);
	clear_char(cbuf);
	if ((player_i = load_char(name, &tmp_store)) > -1) {
	    store_to_char(&tmp_store, cbuf);
	    if (GET_LEVEL(cbuf) >= GET_LEVEL(ch)) {
		free_char(cbuf);
		send_to_char("Sorry, you can't do that.\r\n", ch);
		return;
	    }
	    vict = cbuf;
	} else {
	    free(cbuf);
	    send_to_char("There is no such player.\r\n", ch);
	    return;
	}
    }

    /* find the command in the list */
    len = strlen(field);
    for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
	if (!strncmp(field, set_fields[mode].cmd, len))
	    break;

    /* perform the set */
    retval = perform_set(ch, vict, mode, val_arg);

    /* save the character if a change was made */
    if (retval) {
	if (!is_file && !IS_NPC(vict))
	    save_char(vict, NOWHERE);
	if (is_file) {
	    char_to_store(vict, &tmp_store);
	    fseek(player_fl, (player_i) * sizeof(struct char_file_u),
		  SEEK_SET);
	    fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
	    send_to_char("Saved in file.\r\n", ch);
	}
    }

    /* free the memory if we allocated it earlier */
    if (is_file)
	free_char(cbuf);
}

extern struct zone_data *zone_table;
extern int real_zone(int number);
ACMD(do_rlist)
{

    int i;
    int zone_num;
    int room_num;
    int room_count = 0;
    int curr_room;
    char tmp[128];

    one_argument(argument, arg);

    tmp[0] = '\0';

    if (!*arg)
	zone_num = (int) world[IN_ROOM(ch)].number / 100;
    else
	zone_num = atoi(arg);

    for (i = 0; i <= top_of_zone_table; i++)
	if (zone_table[i].number == zone_num) {

	    if (i >= 0 && i <= top_of_zone_table) {
		sprintf(buf,
			"\r\nRoom List For Zone # %d: %s.\r\n\r\nRoom #    Name\r\n---------------------------------------------------------------\r\n",
			zone_table[i].number, zone_table[i].name);
		send_to_char(buf, ch);
		*buf = '\0';
		for (curr_room = 0; curr_room <= top_of_world; curr_room++) {
		    room_num = world[curr_room].number;
		    if (room_num >= (zone_num * 100)
			&& room_num <= ((zone_num * 100) + 99)) {
			if (real_room(room_num) > NOWHERE) {
			    sprintf(tmp, " %-5d %s\r\n", room_num,
				    world[curr_room].name);
			    strcat(buf, tmp);
			    room_count++;
			}
		    }
		}
		sprintf(tmp, "\r\n%d rooms listed for zone %d\r\n\r\n",
			room_count, zone_num);
		strcat(buf, tmp);
		page_string(ch->desc, buf, TRUE);
	    } else
		send_to_char("Zone not found.\r\n", ch);
	}
}


extern struct obj_data *obj_proto;
ACMD(do_olist)
{

    int zone_num;
    int nr;
    int obj_count = 0;
    int i;
    char tmp[128];

    one_argument(argument, arg);

    tmp[0] = '\0';

    if (!*arg)
	zone_num = (int) world[IN_ROOM(ch)].number / 100;
    else
	zone_num = atoi(arg);

    for (i = 0; i <= top_of_zone_table; i++) {
	if (zone_table[i].number == zone_num) {
	    if (i >= 0 && i <= top_of_zone_table) {
		sprintf(buf, "\r\nObject List For Zone # %d: %s.\r\n\r\n"
			" Obj #   Level Name\r\n"
			"-----------------------------------------------------\r\n",
			zone_table[i].number, zone_table[i].name);
		send_to_char(buf, ch);
		*buf = '\0';
		for (nr = zone_num * 100; nr <= ((zone_num * 100) + 99);
		     nr++) {
		    if (real_object(nr) > 0) {
			sprintf(tmp, " %-5d %-30s (Cost: %-3d)\r\n", nr,
				obj_proto[real_object(nr)].
				short_description,
				obj_proto[real_object(nr)].obj_flags.cost);
			strcat(buf, tmp);
			obj_count++;
		    } else {
			/* sprintf(tmp, " /cB%-5d Unused./c0\r\n", nr); */
			/* strcat(buf, tmp); */
		    }
		}
	    }
	}
    }
    sprintf(tmp, "-----------\r\n  %d slots used.\r\n\r\n", obj_count);
    strcat(buf, tmp);
    page_string(ch->desc, buf, TRUE);
}

ACMD(do_mlist)
{

    int zone_num;
    int mob_count = 0;
    int i, nr;
    char tmp[128];

    one_argument(argument, arg);

    tmp[0] = '\0';
    if (!*arg)
	zone_num = (int) world[IN_ROOM(ch)].number / 100;
    else
	zone_num = atoi(arg);

    for (i = 0; i <= top_of_zone_table; i++) {
	if (zone_table[i].number == zone_num) {
	    if (i >= 0 && i <= top_of_zone_table) {
		sprintf(buf,
			"\r\nMobile List For Zone # %d: %s.\r\n\r\n MOB #  Level  Race      Class       Name\r\n---------------------------------------------------------------\r\n",
			zone_table[i].number, zone_table[i].name);
		send_to_char(buf, ch);
		*buf = '\0';
		for (nr = zone_num * 100; nr <= ((zone_num * 100) + 99);
		     nr++) {
		    if (real_mobile(nr) > 0) {
			sprintf(tmp, " %-5d %3d %10s %10s   %s \r\n", nr,
				mob_proto[real_mobile(nr)].player.level,
				npc_race_types[(int)
					       mob_proto[real_mobile(nr)].
					       player.race],
				npc_class_types[(int)
						mob_proto[real_mobile(nr)].
						player.chclass],
				mob_proto[real_mobile(nr)].player.
				short_descr);
			strcat(buf, tmp);
			mob_count++;
		    } else {
			/* sprintf(tmp, " /cB%-5d Unused./c0\r\n", nr); */
			/* strcat(buf, tmp); */
		    }
		}
	    }
	}
    }
    sprintf(tmp, "-----------\r\n  %d slots used.\r\n", mob_count);
    strcat(buf, tmp);
    page_string(ch->desc, buf, TRUE);
}

void do_newbie(struct char_data *vict)
{
    struct obj_data *obj;
    int i, w;
    int newbie_eq[] = { 1, 2, 3, 4, 5, 9, -1 };
    int fisherman_eq[] = { 6, 7, 7, 7, 7, 7, 7, 8, -1 };
    int pigf_eq[] = { 101, 14, -1 };
    int goblin_eq[] = { 11, 1372, 13, -1 };

    /*  Give some basic eq to this person (vict) */
    for (i = 0; newbie_eq[i] != -1; i++) {
	obj = read_object(newbie_eq[i], VIRTUAL);
	obj_to_char(obj, vict);
    }

    add_money_to_char(vict, number(1, 3), SILVER_COINS);
    add_money_to_char(vict, number(1, 3), COPPER_COINS);

    if (GET_CLASS(vict) == CLASS_FISHERMAN) {
	add_money_to_char(vict, number(1, 3), GOLD_COINS);
	for (i = 0; fisherman_eq[i] != -1; i++) {
	    obj = read_object(fisherman_eq[i], VIRTUAL);
	    obj_to_char(obj, vict);
	}
    }

    if (GET_CLASS(vict) == CLASS_PIG_FARMER) {
	add_money_to_char(vict, number(5, 10), GOLD_COINS);
	for (i = 0; pigf_eq[i] != -1; i++) {
	    obj = read_object(pigf_eq[i], VIRTUAL);
	    obj_to_char(obj, vict);
	}
    }

    if (GET_CLASS(vict) == CLASS_GOBLIN_SLAYER) {
	add_money_to_char(vict, number(1, 5), GOLD_COINS);
	for (i = 0; goblin_eq[i] != -1; i++) {
	    obj = read_object(goblin_eq[i], VIRTUAL);
	    obj_to_char(obj, vict);
	}
    }


    for (w = 0; w < NUM_WEARS; w++) {
	if (!GET_EQ(vict, w)) {
	    for (obj = vict->carrying; obj; obj = obj->next_content) {
		if (GET_OBJ_TYPE(obj) == ITEM_LIGHT
		    || GET_OBJ_TYPE(obj) == ITEM_ARMOR
		    || GET_OBJ_TYPE(obj) == ITEM_WORN) {
		    if (find_eq_pos(vict, obj, 0) == w) {
			obj_from_char(obj);
			equip_char(vict, obj, w);
		    }
		}
	    }
	}
    }

}

ACMD(do_file)
{
    FILE *req_file;
    int cur_line = 0, num_lines = 0, req_lines = 0, i, j;
    int l;
    char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH];

    struct file_struct {
	char *cmd;
	char level;
	char *file;
    } fields[] = {
	{
	"none", 31, "Does Nothing"}, {
	"bug", 31, "..//lib//misc//bugs"}, {
	"typo", 31, "..//lib//misc//typos"}, {
	"ideas", 31, "..//lib//misc//ideas"}, {
	"syslog", 31, "..//syslog"}, {
	"crash", 31, "..//syslog.CRASH"}, {
	"rip", 31, "..//log//rip"}, {
	"players", 31, "..//log//newplayers"}, {
	"rentgone", 31, "..//log//rentgone"}, {
	"godcmds", 31, "..//log//godcmds"}, {
	"\n", 0, "\n"}
    };

    skip_spaces(&argument);

    if (!*argument) {
	strcpy(buf,
	       "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
	for (j = 0, i = 1; fields[i].level; i++)

	    if (fields[i].level <= GET_LEVEL(ch))
		sprintf(buf, "%s%-15s%s\r\n", buf, fields[i].cmd,
			fields[i].file);
	// send_to_char(buf, ch);
	SEND_TO_Q(buf, ch->desc);
	return;
    }

    strcpy(arg, two_arguments(argument, field, value));

    for (l = 0; *(fields[l].cmd) != '\n'; l++)
	if (!strncmp(field, fields[l].cmd, strlen(field)))
	    break;

    if (*(fields[l].cmd) == '\n') {
	send_to_char("That is not a valid option!\r\n", ch);
	return;
    }

    if (GET_LEVEL(ch) < fields[l].level) {
	send_to_char("You are not godly enough to view that file!\r\n",
		     ch);
	return;

    }

    if (!*value)
	req_lines = 15;		/* default is the last 15 lines */
    else
	req_lines = atoi(value);

    req_lines = 100;

    /* open the requested file */
    if (!(req_file = fopen(fields[l].file, "r"))) {
	sprintf(buf2,
		"SYSERR: Error opening file %s using 'file' command.",
		fields[l].file);
	mudlog(buf2, BRF, LVL_IMPL, TRUE);
	return;
    }

    /* count lines in requested file */
    get_line(req_file, buf);
    while (!feof(req_file)) {
	num_lines++;

	get_line(req_file, buf);
    }
    fclose(req_file);


    /* Limit # of lines printed to # requested or # of lines in file or
       150 lines */
    if (req_lines > num_lines)
	req_lines = num_lines;
    if (req_lines > 150)
	req_lines = 150;


    /* close and re-open */
    if (!(req_file = fopen(fields[l].file, "r"))) {
	sprintf(buf2,
		"SYSERR: Error opening file %s using 'file' command.",
		fields[l].file);
	mudlog(buf2, BRF, LVL_IMPL, TRUE);
	return;
    }

    buf2[0] = '\0';

    /* and print the requested lines */

    get_line(req_file, buf);
    while (!feof(req_file)) {
	cur_line++;
	if (cur_line > (num_lines - req_lines)) {
	    sprintf(buf2, "%s%s\r\n", buf2, buf);
	}
	get_line(req_file, buf);
    }
    page_string(ch->desc, buf2, 1);


    fclose(req_file);
}

/* Add to bottom of act.wizard.c */

ACMD(do_copyto)
{

/* Only works if you have Oasis OLC */

    char buf2[10];
    char buf[80];
    int iroom = 0, rroom = 0;
    struct room_data *room;

    one_argument(argument, buf2);
    /* buf2 is room to copy to */


    CREATE(room, struct room_data, 1);



    iroom = atoi(buf2);

    rroom = real_room(atoi(buf2));

    *room = world[rroom];

    if (!*buf2) {
	send_to_char("Format: copyto <room number>\r\n", ch);
	return;
    }
    if (rroom <= 0) {

	sprintf(buf, "There is no room with the number %d.\r\n", iroom);

	send_to_char(buf, ch);

	return;
    }

/* Main stuff */

    if (world[ch->in_room].description) {
	world[rroom].description = str_dup(world[ch->in_room].description);


	sprintf(buf, "You copy the description to room %d.\r\n", iroom);
	send_to_char(buf, ch);
    }

    else

	send_to_char("This room has no description!\r\n", ch);
}

ACMD(do_dig)
{
/* Only works if you have Oasis OLC */

    char buf2[10];
    char buf3[10];
    char buf[80];
    int iroom = 0, rroom = 0;
    int dir = 0;
/*  struct room_data *room; */

    two_arguments(argument, buf2, buf3);
    /* buf2 is the direction, buf3 is the room */


    iroom = atoi(buf3);

    rroom = real_room(iroom);

    if (!*buf2) {
	send_to_char("Format: dig <dir> <room number>\r\n", ch);
	return;
    } else if (!*buf3) {
	send_to_char("Format: dig <dir> <room number>\r\n", ch);
	return;
    }
    if (rroom <= 0) {

	sprintf(buf, "There is no room with the number %d", iroom);

	send_to_char(buf, ch);

	return;
    }
/* Main stuff */
    switch (*buf2) {
    case 'n':
    case 'N':
	dir = NORTH;
	break;
    case 'e':
    case 'E':
	dir = EAST;
	break;
    case 's':
    case 'S':
	dir = SOUTH;
	break;
    case 'w':
    case 'W':
	dir = WEST;
	break;
    case 'u':
    case 'U':
	dir = UP;
	break;
    case 'd':
    case 'D':
	dir = DOWN;
	break;
    }

    CREATE(world[rroom].dir_option[rev_dir[dir]],
	   struct room_direction_data, 1);
    world[rroom].dir_option[rev_dir[dir]]->general_description = NULL;
    world[rroom].dir_option[rev_dir[dir]]->keyword = NULL;
    world[rroom].dir_option[rev_dir[dir]]->to_room = ch->in_room;

    CREATE(world[ch->in_room].dir_option[dir], struct room_direction_data,
	   1);
    world[ch->in_room].dir_option[dir]->general_description = NULL;
    world[ch->in_room].dir_option[dir]->keyword = NULL;
    world[ch->in_room].dir_option[dir]->to_room = rroom;


    sprintf(buf, "You make an exit %s to room %d.\r\n", buf2, iroom);
    send_to_char(buf, ch);

}

/* -CMS- */
ACMD(do_omni)
{

    struct char_data *tch;
    char tmp[256];
    int count = 0;

    send_to_char("Lvl  Class  Room   Name                "
		 "Health  Position\r\n", ch);
    for (tch = character_list; tch; tch = tch->next)
	if (!IS_NPC(tch) && GET_LEVEL(tch) < LVL_IMMORT) {
	    sprintf(tmp, "%2d  %4s    %-5d  %-20s  %3d%%  %s\r\n",
		    GET_LEVEL(tch), CLASS_ABBR(tch),
		    world[IN_ROOM(tch)].number, GET_NAME(tch),
		    (int) (GET_HIT(tch) * 100) / GET_MAX_HIT(tch),
		    position_types[(int) GET_POS(tch)]);
	    send_to_char(tmp, ch);
	    count++;
	}
    if (!count)
	send_to_char("  No mortals are on!\r\n", ch);

}

ACMD(do_peace)
{
    struct char_data *vict, *next_v;
    act("$n decides that everyone should just be friends.",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("Everything is quite peaceful now.\r\n", ch->in_room);
    for (vict = world[ch->in_room].people; vict; vict = next_v) {
	next_v = vict->next_in_room;
	if (IS_NPC(vict) && (FIGHTING(vict))) {
	    if (FIGHTING(FIGHTING(vict)) == vict)
		stop_fighting(FIGHTING(vict));
	    stop_fighting(vict);

	}
    }
}

extern struct player_index_element *player_table;
extern int top_of_p_table;

ACMD(do_players)
{

    FILE *pfile;
    int pnum = 0, minlvl = 0, cnt = 0;
    bool hostname = FALSE, level = FALSE;
    struct char_file_u vbuf;
    char buf[MAX_STRING_LENGTH * 4];

    if (!(pfile = fopen(PLAYER_FILE, "r+b"))) {
	return;
    }

    skip_spaces(&argument);
    if (*argument) {
	if (!(minlvl = atoi(argument)))
	    hostname = TRUE;
	else
	    level = TRUE;
    }

    sprintf(buf,
	    "/cbPlayers:\r\n------------------------------------/c0\r\n");
    fseek(pfile, (long) (pnum * sizeof(struct char_file_u)), SEEK_SET);
    fread(&vbuf, sizeof(struct char_file_u), 1, pfile);
    for (pnum = 1; !feof(pfile); pnum++) {

	if (level) {		/* If a level was specified *//* Storm 1/31/99 */
	    if (vbuf.level >= minlvl) {
		sprintf(buf,
			"%s/cg%-5ld:/cy[%3d/cy]/c0 %-13s/cw %-26s - %s\r",
			buf, vbuf.char_specials_saved.idnum, vbuf.level,
			vbuf.name, vbuf.host, ctime(&vbuf.last_logon));
		cnt++;
	    }
	} else if (hostname) {	/* If a string was specified *//* Storm 1/31/99 */
	    if (strstr(vbuf.host, argument)) {
		sprintf(buf,
			"%s/cg%-5ld:/cy[%3d/cy]/c0 %-13s/cw %-26s - %s\r",
			buf, vbuf.char_specials_saved.idnum, vbuf.level,
			vbuf.name, vbuf.host, ctime(&vbuf.last_logon));
		cnt++;
	    }
	} else {		/* if nothing was specified just list everyone.. *//* Storm 1/31/99
				 */
	    sprintf(buf, "%s/cg%-5ld:/cy[%3d/cy]/c0 %-13s/cw %-26s - %s\r",
		    buf, vbuf.char_specials_saved.idnum, vbuf.level,
		    vbuf.name, vbuf.host, ctime(&vbuf.last_logon));
	    cnt++;
	}
	fseek(pfile, (long) (pnum * sizeof(struct char_file_u)), SEEK_SET);
	fread(&vbuf, sizeof(struct char_file_u), 1, pfile);

    }

    sprintf(buf, "%s%d players in the database (%d listed).\r\n", buf,
	    pnum - 1, cnt);
    fclose(pfile);
    page_string(ch->desc, buf, TRUE);

}
