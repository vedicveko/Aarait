/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "coins.h"
#include "quest.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern int pk_allowed(struct char_data *ch, struct char_data *vict);	/* see config.c */
extern int auto_save;		/* see config.c -- not used in this file */
extern int max_exp_gain;	/* see config.c */
extern int max_exp_loss;	/* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;
ACMD(do_get);
extern struct job_data *job_table;
extern struct job_data *job;
int get_gold_weight(int amount, int type);


/* External procedures */
char *fread_action(FILE * fl, int nr);
ACMD(do_flee);
int backstab_mult(int level);
int thaco(int ch_class, int level);
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
extern struct aq_data *aquest_table;
void clearMemory(struct char_data *ch);


/* local functions */
void perform_group_gain(struct char_data *ch, int base,
			struct char_data *victim);
void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		 int w_type);
void appear(struct char_data *ch);
void load_messages(void);
void check_killer(struct char_data *ch, struct char_data *vict);
void make_corpse(struct char_data *ch);
void change_alignment(struct char_data *ch, struct char_data *victim);
void death_cry(struct char_data *ch);
void raw_kill(struct char_data *ch, struct char_data *killer);
void hunt_victim(struct char_data *ch);
void die(struct char_data *ch, struct char_data *killer);
void group_gain(struct char_data *ch, struct char_data *victim);
void solo_gain(struct char_data *ch, struct char_data *victim);
char *replace_string(const char *str, const char *weapon_singular,
		     const char *weapon_plural);
void perform_violence(void);
int advanced_combat(struct char_data *ch, struct char_data *vict);
void diag_char_to_char(struct char_data *i, struct char_data *ch);
void make_bar(char *b, int now, int max, int len);
void perform_auto_skills(struct char_data *ch, struct char_data *vict);
int fighting_injuries(struct char_data *ch);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] = {
    {"hit", "hits"},		/* 0 */
    {"sting", "stings"},
    {"whip", "whips"},
    {"slash", "slashes"},
    {"bite", "bites"},
    {"bludgeon", "bludgeons"},	/* 5 */
    {"crush", "crushes"},
    {"pound", "pounds"},
    {"claw", "claws"},
    {"maul", "mauls"},
    {"thrash", "thrashes"},	/* 10 */
    {"pierce", "pierces"},
    {"blast", "blasts"},
    {"punch", "punches"},
    {"stab", "stabs"},
    {"cleave", "cleaves"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))


#define TOROOM(x, y)    (world[(x)].dir_option[(y)]->to_room)

int react_first_step(struct char_data *ch, struct char_data *victim)
{
    int dir = 0;
    room_rnum x = victim->in_room;

    for (dir = 0; dir < NUM_OF_DIRS; dir++) {
	if (world[x].dir_option[dir]) {
	    if (TOROOM(x, dir) == NOWHERE || !CAN_GO(victim, dir))
		continue;

	    if (EXIT(victim, dir)->to_room == IN_ROOM(ch)) {
		return (dir);
	    }
	} else
	    continue;
    }
    return (-1);
}

void summon_ranged_helpers(struct char_data *ch)
{
    struct follow_type *f, *next_fol;

    if (!HUNTING(ch) || HUNTING(ch) == NULL) {
	return;
    }
    for (f = ch->followers; f; f = next_fol) {
	next_fol = f->next;
	HUNTING(f->follower) = HUNTING(ch);

    }

}
void mob_reaction(struct char_data *ch, struct char_data *victim)
{
    int dir;

    if (AFF_FLAGGED(victim, AFF_BLIND) || AFF_FLAGGED(victim, AFF_CHARM))
	return;
    if (GET_POS(victim) <= POS_STUNNED)
	return;
    if (FIGHTING(victim) && FIGHTING(victim) != ch)
	return;

    if (IS_NPC(victim) && (FIGHTING(victim) == ch)) {

	if (MOB_FLAGGED(victim, MOB_MEMORY))
	    remember(victim, ch);

	sprintf(buf, "$n whines in pain!");
	act(buf, FALSE, victim, 0, 0, TO_ROOM);
	GET_POS(victim) = POS_STANDING;

	if (MOB_FLAGGED(victim, MOB_RUN_AWAY)) {
	    do_flee(ch, 0, 0, 0);
	    return;
	}
	if ((dir = react_first_step(ch, victim)) < 0) {
	    sprintf(buf, "$n franticly looks around, confused!");
	    act(buf, FALSE, victim, 0, 0, TO_ROOM);
	    return;
	} else {
	    if (!do_simple_move(victim, dir, 1))
		return;
	}

	if (victim->in_room == ch->in_room) {
	    hit(victim, ch, TYPE_UNDEFINED);
	    HUNTING(victim) = ch;
	    summon_ranged_helpers(victim);
	} else {
	    if (!MOB_FLAGGED(victim, MOB_MEMORY)) {
		SET_BIT(MOB_FLAGS(victim), MOB_MEMORY);
		remember(victim, ch);
	    }
	    HUNTING(victim) = ch;
	    hunt_victim(victim);
	}
    }

}

/* The Fight related routines */

void appear(struct char_data *ch)
{

    if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE)); {

	REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

	if (GET_LEVEL(ch) < LVL_IMMORT)
	    act("$n slowly fades into existence.", FALSE, ch, 0, 0,
		TO_ROOM);
	else
	    act
		("You feel a strange presence as $n appears, seemingly from nowhere.",
		 FALSE, ch, 0, 0, TO_ROOM);
    }
}


void load_messages(void)
{
    FILE *fl;
    int i, type;
    struct message_type *messages;
    char chk[128];

    if (!(fl = fopen(MESS_FILE, "r"))) {
	log("SYSERR: Error reading combat message file %s: %s", MESS_FILE,
	    strerror(errno));
	exit(1);
    }
    for (i = 0; i < MAX_MESSAGES; i++) {
	fight_messages[i].a_type = 0;
	fight_messages[i].number_of_attacks = 0;
	fight_messages[i].msg = 0;
    }


    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
	fgets(chk, 128, fl);

    while (*chk == 'M') {
	fgets(chk, 128, fl);
	sscanf(chk, " %d\n", &type);
	for (i = 0;
	     (i < MAX_MESSAGES) && (fight_messages[i].a_type != type)
	     && (fight_messages[i].a_type); i++);
	if (i >= MAX_MESSAGES) {
	    log
		("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
	    exit(1);
	}
	CREATE(messages, struct message_type, 1);
	fight_messages[i].number_of_attacks++;
	fight_messages[i].a_type = type;
	messages->next = fight_messages[i].msg;
	fight_messages[i].msg = messages;

	messages->die_msg.attacker_msg = fread_action(fl, i);
	messages->die_msg.victim_msg = fread_action(fl, i);
	messages->die_msg.room_msg = fread_action(fl, i);
	messages->miss_msg.attacker_msg = fread_action(fl, i);
	messages->miss_msg.victim_msg = fread_action(fl, i);
	messages->miss_msg.room_msg = fread_action(fl, i);
	messages->hit_msg.attacker_msg = fread_action(fl, i);
	messages->hit_msg.victim_msg = fread_action(fl, i);
	messages->hit_msg.room_msg = fread_action(fl, i);
	messages->god_msg.attacker_msg = fread_action(fl, i);
	messages->god_msg.victim_msg = fread_action(fl, i);
	messages->god_msg.room_msg = fread_action(fl, i);
	fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*'))
	    fgets(chk, 128, fl);
    }

    fclose(fl);
}


void update_pos(struct char_data *victim)
{
    if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
	return;
    else if (GET_HIT(victim) > 0)
	GET_POS(victim) = POS_STANDING;
    else if (GET_HIT(victim) <= -11)
	GET_POS(victim) = POS_DEAD;
    else if (GET_HIT(victim) <= -6)
	GET_POS(victim) = POS_MORTALLYW;
    else if (GET_HIT(victim) <= -3)
	GET_POS(victim) = POS_INCAP;
    else
	GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data *ch, struct char_data *vict)
{
    char buf[256];

    if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
	return;
    if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict)
	|| ch == vict)
	return;

    SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
    sprintf(buf,
	    "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
    mudlog(buf, BRF, LVL_IMMORT, TRUE);
    send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
    if (ch == vict)
	return;

    if (FIGHTING(ch)) {
	core_dump();
	return;
    }

    ch->next_fighting = combat_list;
    combat_list = ch;

    if (AFF_FLAGGED(ch, AFF_SLEEP))
	REMOVE_BIT(AFF_FLAGS(ch), AFF_SLEEP);

    if (MOB_FLAGGED(vict, MOB_HUNTER)) {
	HUNTING(vict) = ch;
    }

    FIGHTING(ch) = vict;
    GET_POS(ch) = POS_FIGHTING;

    if (!pk_allowed(ch, vict))
	check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
    struct char_data *temp;

    if (ch == next_combat_list)
	next_combat_list = ch->next_fighting;

    REMOVE_FROM_LIST(ch, combat_list, next_fighting);
    ch->next_fighting = NULL;
    FIGHTING(ch) = NULL;
    GET_POS(ch) = POS_STANDING;
    update_pos(ch);
}



void make_corpse(struct char_data *ch)
{
    struct obj_data *corpse, *o;
    struct obj_data *money;
    int i;

    corpse = create_obj();

    corpse->item_number = NOTHING;
    corpse->in_room = NOWHERE;
    corpse->name = str_dup("corpse");

    sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
    corpse->description = str_dup(buf2);

    sprintf(buf2, "the corpse of %s", GET_NAME(ch));
    corpse->short_description = str_dup(buf2);

    GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
    GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
    GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
    GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
    GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
    GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
    GET_OBJ_RENT(corpse) = 100000;
    if (IS_NPC(ch))
	GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
    else
	GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

    /* transfer character's inventory to the corpse */
    corpse->contains = ch->carrying;
    for (o = corpse->contains; o != NULL; o = o->next_content)
	o->in_obj = corpse;
    object_list_new_owner(corpse, NULL);

    /* transfer character's equipment to the corpse */
    for (i = 0; i < NUM_WEARS; i++)
	if (GET_EQ(ch, i))
	    obj_to_obj(unequip_char(ch, i), corpse);

    /* transfer gold */
    if (convert_all_to_copper(ch) > 0) {
	/* following 'if' clause added to fix gold duplication loophole */
	if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
	    money =
		create_money(GET_PLATINUM(ch), GET_GOLD(ch),
			     GET_SILVER(ch), GET_COPPER(ch), MIXED_COINS);
	    obj_to_obj(money, corpse);

	}
	GET_MONEY(ch, GOLD_COINS) = 0;
	GET_MONEY(ch, COPPER_COINS) = 0;
	GET_MONEY(ch, SILVER_COINS) = 0;
	GET_MONEY(ch, PLAT_COINS) = 0;
    }
    ch->carrying = NULL;
    IS_CARRYING_N(ch) = 0;
    IS_CARRYING_W(ch) = 0;

    obj_to_room(corpse, ch->in_room);
}


/* When ch kills victim */
void change_alignment(struct char_data *ch, struct char_data *victim)
{
    /*
     * new alignment change algorithm: if you kill a monster with alignment A,
     * you move 1/16th of the way to having alignment -A.  Simple and fast.
     */
    GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}



void death_cry(struct char_data *ch)
{
    int door;

    act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0,
	TO_ROOM);

    for (door = 0; door < NUM_OF_DIRS; door++)
	if (CAN_GO(ch, door))
	    send_to_room
		("Your blood freezes as you hear someone's death cry.\r\n",
		 world[ch->in_room].dir_option[door]->to_room);
}



void raw_kill(struct char_data *ch, struct char_data *killer)
{
    if (FIGHTING(ch))
	stop_fighting(ch);

    while (ch->affected)
	affect_remove(ch, ch->affected);

    death_cry(ch);

    make_corpse(ch);

    if (killer)
	autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);

    extract_char(ch);

    if (killer)			/* move to end to extract ch first */
	autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);

}



void die(struct char_data *ch, struct char_data *killer)
{
    if (!IS_NPC(ch))
	REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);

    MEMORY(ch) = NULL;
    clearMemory(ch);
    forget(ch, killer);
    forget(killer, ch);
    MEMORY(killer) = NULL;
    clearMemory(killer);

    HUNTING(ch) = NULL;
    LOOKING_FOR(ch) = NULL;
    HUNTING(killer) = NULL;
    LOOKING_FOR(ch) = NULL;

    raw_kill(ch, killer);
}



void perform_group_gain(struct char_data *ch, int base,
			struct char_data *victim)
{
    int share;

    share = MIN(max_exp_gain, MAX(1, base));

    if (share > 1) {
	sprintf(buf2,
		"You receive your share of experience -- %d points.\r\n",
		share);
	send_to_char(buf2, ch);
    } else
	send_to_char
	    ("You receive your share of experience -- one measly little point!\r\n",
	     ch);

    gain_exp(ch, share);
    change_alignment(ch, victim);
}


void group_gain(struct char_data *ch, struct char_data *victim)
{
    int tot_members, base, tot_gain;
    struct char_data *k;
    struct follow_type *f;

    if (!(k = ch->master))
	k = ch;

    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
	tot_members = 1;
    else
	tot_members = 0;

    for (f = k->followers; f; f = f->next)
	if (AFF_FLAGGED(f->follower, AFF_GROUP)
	    && f->follower->in_room == ch->in_room)
	    tot_members++;

    /* round up to the next highest tot_members */
    tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

    /* prevent illegal xp creation when killing players */
    if (!IS_NPC(victim))
	tot_gain = MIN(max_exp_loss * 2 / 3, tot_gain);

    if (tot_members >= 1)
	base = MAX(1, tot_gain / tot_members);
    else
	base = 0;

    if (AFF_FLAGGED(k, AFF_GROUP) && k->in_room == ch->in_room)
	perform_group_gain(k, base, victim);

    for (f = k->followers; f; f = f->next)
	if (AFF_FLAGGED(f->follower, AFF_GROUP)
	    && f->follower->in_room == ch->in_room)
	    perform_group_gain(f->follower, base, victim);
}


void solo_gain(struct char_data *ch, struct char_data *victim)
{
    int exp;

    exp = MIN(max_exp_gain, GET_EXP(victim) / 3);

    /* Calculate level-difference bonus */
    if (IS_NPC(ch))
	exp +=
	    MAX(0,
		(exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
    else
	exp +=
	    MAX(0,
		(exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);

    exp = MAX(exp, 1);

    if (exp > 1) {
	sprintf(buf2, "You receive %d experience points.\r\n", exp);
	send_to_char(buf2, ch);
    } else
	send_to_char("You receive one lousy experience point.\r\n", ch);

    gain_exp(ch, exp);
    change_alignment(ch, victim);
}


char *replace_string(const char *str, const char *weapon_singular,
		     const char *weapon_plural)
{
    static char buf[256];
    char *cp = buf;

    for (; *str; str++) {
	if (*str == '#') {
	    switch (*(++str)) {
	    case 'W':
		for (; *weapon_plural; *(cp++) = *(weapon_plural++));
		break;
	    case 'w':
		for (; *weapon_singular; *(cp++) = *(weapon_singular++));
		break;
	    default:
		*(cp++) = '#';
		break;
	    }
	} else
	    *(cp++) = *str;

	*cp = 0;
    }				/* For */

    return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		 int w_type)
{
    char *buf;
    int msgnum;

    static struct dam_weapon_type {
	const char *to_room;
	const char *to_char;
	const char *to_victim;
    } dam_weapons[] = {

	/* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

	{
	    "$n tries to #w $N, but misses.",	/* 0: 0     */
		"You try to #w $N, but miss.",
		"$n tries to #w you, but misses."}, {
	    "$n tickles $N as $e #W $M.",	/* 1: 1..2  */
		"You tickle $N as you #w $M.",
		"$n tickles you as $e #W you."}, {
	    "$n barely #W $N.",	/* 2: 3..4  */
	"You barely #w $N.", "$n barely #W you."}, {
	    "$n #W $N.",	/* 3: 5..6  */
	"You #w $N.", "$n #W you."}, {
	    "$n #W $N hard.",	/* 4: 7..10  */
	"You #w $N hard.", "$n #W you hard."}, {
	    "$n #W $N very hard.",	/* 5: 11..14  */
	"You #w $N very hard.", "$n #W you very hard."}, {
	    "$n #W $N extremely hard.",	/* 6: 15..19  */
	"You #w $N extremely hard.", "$n #W you extremely hard."}, {
	    "$n massacres $N to small fragments with $s #w.",	/* 7: 19..23 */
		"You massacre $N to small fragments with your #w.",
		"$n massacres you to small fragments with $s #w."}, {
	    "$n OBLITERATES $N with $s deadly #w!!",	/* 8: > 23   */
		"You OBLITERATE $N with your deadly #w!!",
		"$n OBLITERATES you with $s deadly #w!!"}
    };


    w_type -= TYPE_HIT;		/* Change to base of table with text */

    if (dam == 0)
	msgnum = 0;
    else if (dam <= 2)
	msgnum = 1;
    else if (dam <= 4)
	msgnum = 2;
    else if (dam <= 6)
	msgnum = 3;
    else if (dam <= 10)
	msgnum = 4;
    else if (dam <= 14)
	msgnum = 5;
    else if (dam <= 19)
	msgnum = 6;
    else if (dam <= 23)
	msgnum = 7;
    else
	msgnum = 8;

    /* damage message to onlookers */
    buf = replace_string(dam_weapons[msgnum].to_room,
			 attack_hit_text[w_type].singular,
			 attack_hit_text[w_type].plural);
    act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

    /* damage message to damager */
    send_to_char(CCYEL(ch, C_CMP), ch);
    buf = replace_string(dam_weapons[msgnum].to_char,
			 attack_hit_text[w_type].singular,
			 attack_hit_text[w_type].plural);
    act(buf, FALSE, ch, NULL, victim, TO_CHAR);
    send_to_char(CCNRM(ch, C_CMP), ch);

    /* damage message to damagee */
    send_to_char(CCRED(victim, C_CMP), victim);
    buf = replace_string(dam_weapons[msgnum].to_victim,
			 attack_hit_text[w_type].singular,
			 attack_hit_text[w_type].plural);
    act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
    send_to_char(CCNRM(victim, C_CMP), victim);

// BEEP
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		  int attacktype)
{
    int i, j, nr;
    struct message_type *msg;

    struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

    for (i = 0; i < MAX_MESSAGES; i++) {
	if (fight_messages[i].a_type == attacktype) {
	    nr = dice(1, fight_messages[i].number_of_attacks);
	    for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
		msg = msg->next;

	    if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
		act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict,
		    TO_CHAR);
		act(msg->god_msg.victim_msg, FALSE, ch, weap, vict,
		    TO_VICT);
		act(msg->god_msg.room_msg, FALSE, ch, weap, vict,
		    TO_NOTVICT);
	    } else if (dam != 0) {
		if (GET_POS(vict) == POS_DEAD) {
		    send_to_char(CCYEL(ch, C_CMP), ch);
		    act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict,
			TO_CHAR);
		    send_to_char(CCNRM(ch, C_CMP), ch);

		    send_to_char(CCRED(vict, C_CMP), vict);
		    act(msg->die_msg.victim_msg, FALSE, ch, weap, vict,
			TO_VICT | TO_SLEEP);
		    send_to_char(CCNRM(vict, C_CMP), vict);

		    act(msg->die_msg.room_msg, FALSE, ch, weap, vict,
			TO_NOTVICT);
		} else {
		    send_to_char(CCYEL(ch, C_CMP), ch);
		    act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict,
			TO_CHAR);
		    send_to_char(CCNRM(ch, C_CMP), ch);

		    send_to_char(CCRED(vict, C_CMP), vict);
		    act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict,
			TO_VICT | TO_SLEEP);
		    send_to_char(CCNRM(vict, C_CMP), vict);

		    act(msg->hit_msg.room_msg, FALSE, ch, weap, vict,
			TO_NOTVICT);
		}
	    } else if (ch != vict) {	/* Dam == 0 */
		send_to_char(CCYEL(ch, C_CMP), ch);
		act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict,
		    TO_CHAR);
		send_to_char(CCNRM(ch, C_CMP), ch);

		send_to_char(CCRED(vict, C_CMP), vict);
		act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict,
		    TO_VICT | TO_SLEEP);
		send_to_char(CCNRM(vict, C_CMP), vict);

		act(msg->miss_msg.room_msg, FALSE, ch, weap, vict,
		    TO_NOTVICT);
	    }
	    return (1);
	}
    }
    return (0);
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(struct char_data *ch, struct char_data *victim, int dam,
	   int attacktype)
{
    if (GET_POS(victim) <= POS_DEAD) {
	log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
	    GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)),
	    GET_NAME(ch));
	die(victim, ch);
	return (0);		/* -je, 7/7/92 */
    }

    /* peaceful rooms */
    if (ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
	send_to_char
	    ("This room just has such a peaceful, easy feeling...\r\n",
	     ch);
	return (0);
    }

    /* shopkeeper protection */
    if (!ok_damage_shopkeeper(ch, victim))
	return (0);

    /* You can't damage an immortal! */
    if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT))
	dam = 0;

    if (victim != ch) {
	/* Start the attacker fighting the victim */
	if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
	    set_fighting(ch, victim);

	/* Start the victim fighting the attacker */
	if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
	    set_fighting(victim, ch);
	    if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
		remember(victim, ch);
	}
    }

    /* If you attack a pet, it hates your guts */
    if (victim->master == ch)
	stop_follower(victim);

    /* If the attacker is invisible, he becomes visible */
    if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
	appear(ch);

    /* Cut damage in half if victim has sanct, to a minimum 1 */
    if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
	dam /= 2;

    /* Check for PK if this is not a PK MUD */
    if (!pk_allowed(ch, victim)) {
	check_killer(ch, victim);
	if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
	    dam = 0;
    }
    /* Set the maximum damage per round and subtract the hit points */
    dam = MAX(MIN(dam, 100), 0);
    GET_HIT(victim) -= dam;

    /* Gain exp for the hit */
/*  if (ch != victim)
    gain_exp(ch, GET_LEVEL(victim) * dam);
*/
    update_pos(victim);

    /*
     * skill_message sends a message from the messages file in lib/misc.
     * dam_message just sends a generic "You hit $n extremely hard.".
     * skill_message is preferable to dam_message because it is more
     * descriptive.
     * 
     * If we are _not_ attacking with a weapon (i.e. a spell), always use
     * skill_message. If we are attacking with a weapon: If this is a miss or a
     * death blow, send a skill_message if one exists; if not, default to a
     * dam_message. Otherwise, always send a dam_message.
     */
    if (!IS_WEAPON(attacktype))
	skill_message(dam, ch, victim, attacktype);
    else {
	if (GET_POS(victim) == POS_DEAD || dam == 0) {
	    if (!skill_message(dam, ch, victim, attacktype))
		dam_message(dam, ch, victim, attacktype);
	} else {
	    dam_message(dam, ch, victim, attacktype);
	}
    }

    /* Use send_to_char -- act() doesn't send message if you are DEAD. */
    switch (GET_POS(victim)) {
    case POS_MORTALLYW:
	act("$n is mortally wounded, and will die soon, if not aided.",
	    TRUE, victim, 0, 0, TO_ROOM);
	send_to_char
	    ("You are mortally wounded, and will die soon, if not aided.\r\n",
	     victim);
	break;
    case POS_INCAP:
	act("$n is incapacitated and will slowly die, if not aided.", TRUE,
	    victim, 0, 0, TO_ROOM);
	send_to_char
	    ("You are incapacitated an will slowly die, if not aided.\r\n",
	     victim);
	break;
    case POS_STUNNED:
	act("$n is stunned, but will probably regain consciousness again.",
	    TRUE, victim, 0, 0, TO_ROOM);
	send_to_char
	    ("You're stunned, but will probably regain consciousness again.\r\n",
	     victim);
	break;
    case POS_DEAD:
	act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
	send_to_char("You are dead!  Sorry...\r\n", victim);
	break;

    default:			/* >= POSITION SLEEPING */
	if (dam > (GET_MAX_HIT(victim) / 4))
	    send_to_char("That really did HURT!\r\n", victim);

	if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
	    sprintf(buf2,
		    "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
		    CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
	    send_to_char(buf2, victim);
	    if ((ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
		|| (MOB_FLAGGED(victim, MOB_RUN_AWAY))) {
		if (!number(0, 1)) {
		    do_flee(victim, NULL, 0, 0);
		}
	    }
	}
	if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	    GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
	    send_to_char("You wimp out, and attempt to flee!\r\n", victim);
	    do_flee(victim, NULL, 0, 0);
	}
	break;
    }

    /* Help out poor linkless people who are attacked */
    if (!IS_NPC(victim) && !(victim->desc)
	&& GET_POS(victim) > POS_STUNNED) {
	do_flee(victim, NULL, 0, 0);
	if (!FIGHTING(victim)) {
	    act("$n is rescued by divine forces.", FALSE, victim, 0, 0,
		TO_ROOM);
	    GET_WAS_IN(victim) = victim->in_room;
	    char_from_room(victim);
	    char_to_room(victim, 0);
	}
    }

    /* stop someone from fighting if they're stunned or worse */
    if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
	stop_fighting(victim);

    /* Uh oh.  Victim died. */
    if (GET_POS(victim) == POS_DEAD) {
	if (IS_GOBLIN_SLAYER(ch) && IS_NPC_GOBLIN(victim)) {
	    //  improve_stat(ch, APPLY_STR);
	    //  improve_stat(ch, APPLY_DEX);
	    improve_hp(ch);
	    improve_skill(ch, SKILL_GOBLIN_SLAYING, 5);
	    GET_GOBLINS_SLAYED(ch)++;
	    improve_points(ch, number(1, 5));
	}
	if (IS_GOBLIN_SLAYER(ch)) {
	    improve_hp(ch);
	}
/*
    if ((ch != victim) && (IS_NPC(victim) || victim->desc)) {
      if (AFF_FLAGGED(ch, AFF_GROUP))
	group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    } */

	if (!IS_NPC(victim)) {
	    sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim),
		    GET_NAME(ch), world[victim->in_room].name);
	    mudlog(buf2, BRF, LVL_IMMORT, TRUE);

	    if (MOB_FLAGGED(ch, MOB_MEMORY))
		forget(ch, victim);
	} else {

	    GET_MOB_HUNGER(ch) += 10;
	}
// BEEP

	die(victim, ch);
	if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
	    do_get(ch, "all corpse", 0, 0);
	}
	return (-1);
    }
    return (dam);
}



void hit(struct char_data *ch, struct char_data *victim, int type)
{
    struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
    int w_type = 0, victim_ac = 0, attack_bonus = 0, dam = 0, diceroll = 0;
    int skill = 0;

    /* Do some sanity checking, in case someone flees, etc. */
    if (ch->in_room != victim->in_room) {
	if (FIGHTING(ch) && FIGHTING(ch) == victim)
	    stop_fighting(ch);
	return;
    }

    if (!IS_NPC(ch) && !pk_allowed(ch, victim)) {
	stop_fighting(ch);
	send_to_char("But, you can't kill them!\r\n", ch);
	return;
    }

    /* Find the weapon type (for display purposes only) */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON
	&& type != TYPE_PUNCH) w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
    else {
	if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
	    w_type = ch->mob_specials.attack_type + TYPE_HIT;
	else
	    w_type = TYPE_HIT;
    }

    // Calc the attack bonus 

    attack_bonus += ability_modifer[(int) GET_STR(ch)];

    if (IS_NPC(ch)) {
	attack_bonus += GET_MOB_SIZE(ch);
    }

    attack_bonus += GET_LEVEL(ch);	// This is base attack b.

    if (GET_POS(victim) < POS_FIGHTING)
	attack_bonus += 20;

    /* Calculate the raw armor including magic armor.  Higher AC is better. */
    victim_ac = GET_AC(victim);
    if (IS_NPC(victim)) {
	victim_ac += GET_MOB_SIZE(victim);
    }
    // NOTE SHIELD BONUS IS ALREADY ADDED TO AC!

    victim_ac += ability_modifer[(int) GET_DEX(victim)];
    if (!IS_NPC(ch)) {
	if (PRF_FLAGGED(ch, PRF_DEBUG)) {
	    sprintf(buf, "%s: [AC: %d AM: %d]\r\n", GET_NAME(victim),
		    GET_AC(victim),
		    ability_modifer[(int) GET_DEX(victim)]);
	    send_to_room(buf, ch->in_room);
	}
    }
    /* roll the die and take your chances... */
    if (IS_NPC(ch)) {
	diceroll = number(1, (victim_ac + 10));
    } else {
	diceroll = number(1, 20);
    }


    if (!IS_NPC(ch)) {
	if (PRF_FLAGGED(ch, PRF_DEBUG)) {
	    sprintf(buf, "%s: [D %d VICT: %d ATT: %d]\r\n", GET_NAME(ch),
		    diceroll, victim_ac, attack_bonus);
	    send_to_room(buf, ch->in_room);
	}
    }
    /* decide whether this is a hit or a miss */
    if ((diceroll + attack_bonus) <= victim_ac) {
	/* the attacker missed the victim */
	if (type == SKILL_BACKSTAB)
	    damage(ch, victim, 0, SKILL_BACKSTAB);
	else
	    damage(ch, victim, 0, w_type);
    } else {
	/* okay, we know the guy has been hit.  now calculate damage. */

	/* Start with the damage bonuses: the damroll and strength apply */
	if (wielded
	    && IS_OBJ_STAT(GET_EQ(ch, WEAR_WIELD), ITEM_TWO_HANDED)) {
	    dam = (ability_modifer[(int) GET_STR(ch)] * 1.5);
	} else if (wielded) {
	    dam = ability_modifer[(int) GET_STR(ch)];
	}
	dam += GET_DAMROLL(ch);
	if (!IS_NPC(ch)) {
	    dam += (GET_SKILL(ch, type - 150) / 10);
	}
	/* Maybe holding arrow? */
	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
	    /* Add weapon-based damage if a weapon is being wielded */
	    dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
	} else {
	    /* If no weapon, add bare hand damage instead */
	    if (IS_NPC(ch)) {
		dam +=
		    dice(ch->mob_specials.damnodice,
			 ch->mob_specials.damsizedice);
	    } else {
		dam += number(0, 2);	/* Max 2 bare hand damage for players */
	    }
	}

	/*
	 * Include a damage multiplier if victim isn't ready to fight:
	 *
	 * Position sitting  1.33 x normal
	 * Position resting  1.66 x normal
	 * Position sleeping 2.00 x normal
	 * Position stunned  2.33 x normal
	 * Position incap    2.66 x normal
	 * Position mortally 3.00 x normal
	 *
	 * Note, this is a hack because it depends on the particular
	 * values of the POSITION_XXX constants.
	 */
	if (GET_POS(victim) < POS_FIGHTING)
	    dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

	/* at least 1 hp damage min per hit */
	if (IS_NPC_GOBLIN(ch)) {
	    if (IS_NIGHT && dam) {
		dam += 5;
	    } else if (IS_DAY && dam) {
		dam -= 5;
	    }
	}

	dam = MAX(1, dam);
	if (diceroll == 20) {
	    dam = dam * 2;
	    send_to_char("You score a critical hit!\r\n", ch);
	    act("$n scores a critical hit!\r\n", FALSE, ch, 0, 0, TO_ROOM);
	}

	if (w_type == TYPE_PUNCH || TYPE_UNDEFINED) {
	    skill = SKILL_PUNCH;
	} else {
	    skill = (w_type - 150);
	}
	if (diceroll == 20) {
	    improve_skill(ch, skill, 5);
	} else {
	    improve_skill(ch, skill, 0);
	}

	damage(ch, victim, dam, w_type);
    }
}


void ranged_hit(struct char_data *ch, struct char_data *victim, int type,
		struct obj_data *obj)
{
    struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
    int victim_ac = 0, attack_bonus = 0, dam, diceroll;

    /* Do some sanity checking, in case someone flees, etc. */
    if (ch->in_room != victim->in_room) {
	if (FIGHTING(ch) && FIGHTING(ch) == victim)
	    stop_fighting(ch);
	return;
    }
    // Calc the attack bonus 

    attack_bonus += (GET_SKILL(ch, type - 150) / 10);

    attack_bonus += ability_modifer[(int) GET_DEX(ch)];

    if (IS_NPC(ch)) {
	attack_bonus += GET_MOB_SIZE(ch);
    }

    attack_bonus += GET_LEVEL(ch);	// This is base attack b.

    if (GET_POS(victim) < POS_FIGHTING)
	attack_bonus *= 2;

    /* Calculate the raw armor including magic armor.  Higher AC is better. */
    victim_ac = GET_AC(victim);
    if (IS_NPC(victim)) {
	victim_ac += GET_MOB_SIZE(victim);
    }
    // NOTE SHIELD BONUS IS ALREADY ADDED TO AC!

    victim_ac += ability_modifer[(int) GET_DEX(ch)];



    /* roll the die and take your chances... */
    diceroll = number(1, 20);

    /* decide whether this is a hit or a miss */
    if ((diceroll + attack_bonus) <= victim_ac) {
	/* the attacker missed the victim */
	damage(ch, victim, 0, type);
    } else {
	/* okay, we know the guy has been hit.  now calculate damage. */

	/* Start with the damage bonuses: the damroll and strength apply */
	dam = ability_modifer[(int) GET_STR(ch)];
	dam += GET_DAMROLL(ch);
	if (!IS_NPC(ch)) {
	    dam += (GET_SKILL(ch, type - 150) / 10);
	}
	/* Maybe holding arrow? */
	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
	    /* Add weapon-based damage if a weapon is being wielded */
	    dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
	}

	/*
	 * Include a damage multiplier if victim isn't ready to fight:
	 *
	 * Position sitting  1.33 x normal
	 * Position resting  1.66 x normal
	 * Position sleeping 2.00 x normal
	 * Position stunned  2.33 x normal
	 * Position incap    2.66 x normal
	 * Position mortally 3.00 x normal
	 *
	 * Note, this is a hack because it depends on the particular
	 * values of the POSITION_XXX constants.
	 */
	if (GET_POS(victim) < POS_FIGHTING)
	    dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

	/* at least 1 hp damage min per hit */
	dam = MAX(1, dam);

	dam *= 3;

	if (diceroll == 20) {
	    dam = dam * 2;
	    send_to_char("You score a critical hit!\r\n", ch);
	    act("$n scores a critical hit!\r\n", FALSE, ch, 0, 0, TO_ROOM);
	}
	damage_objs(victim);
	damage(ch, victim, dam, type);
    }
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
    struct char_data *ch;
    int block_skill = 0, parry_skill = 0;

    for (ch = combat_list; ch; ch = next_combat_list) {
	next_combat_list = ch->next_fighting;

	if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
	    stop_fighting(ch);
	    continue;
	}

/*    if(GET_WAIT_STATE(ch)) { 
       GET_WAIT_STATE(ch) -= PULSE_VIOLENCE; 
    }
  */
//
// Wait States
//
	if (IS_NPC(ch)) {
	    if (GET_SKILL_WAIT(ch)) {
		GET_SKILL_WAIT(ch)--;
	    }
	    if (GET_MOB_WAIT(ch) > 0) {
		GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
		continue;
	    }
	    GET_MOB_WAIT(ch) = 0;
//
// Stand up Mobiles
//
	    if ((GET_POS(ch) < POS_FIGHTING)) {
		if (GET_HIT(ch) > (GET_MAX_HIT(ch) / 2))
		    act("$n quickly stands up.", 1, ch, 0, 0, TO_ROOM);
		else if (GET_HIT(ch) > (GET_MAX_HIT(ch) / 6))
		    act("$n slowly stands up.", 1, ch, 0, 0, TO_ROOM);
		else
		    act("$n gets to $s feet very slowly.", 1, ch, 0, 0,
			TO_ROOM);
		GET_POS(ch) = POS_FIGHTING;
	    }
	}
//
// Fighting Injuries
//
//	fighting_injuries(ch);

	if (GET_POS(ch) < POS_FIGHTING) {
	    send_to_char("You can't fight while sitting!!\r\n", ch);
	    continue;
	}
//
// Block/Parry
//
	if (!IS_NPC(FIGHTING(ch))) {
	    block_skill = GET_SKILL(FIGHTING(ch), SKILL_BLOCK);
	    parry_skill = GET_SKILL(FIGHTING(ch), SKILL_PARRY);
	} else {
	    block_skill = number(1, 100);
	    parry_skill = number(1, 100);

	}
	if (GET_EQ(FIGHTING(ch), WEAR_SHIELD)) {
	    if (number(0, 300) <
		block_skill + (2 * (GET_DEX(FIGHTING(ch)) - GET_DEX(ch)))) {
		act("You block $N's vicious attack with your shield!",
		    FALSE, FIGHTING(ch), 0, ch, TO_CHAR);
		act("$n blocks $N's vicious attack with $s shield!",
		    FALSE, FIGHTING(ch), 0, ch, TO_NOTVICT);
		damage_objs_exact(FIGHTING(ch), WEAR_SHIELD);
		improve_skill(FIGHTING(ch), SKILL_BLOCK, 3);
		continue;
	    }
	}
	if (GET_EQ(ch, WEAR_WIELD) && GET_EQ(FIGHTING(ch), WEAR_WIELD)) {
	    if (number(0, 300) <
		parry_skill + (2 * (GET_DEX(FIGHTING(ch)) - GET_DEX(ch)))) {
		act("You parry $N's vicious attack with your $o!",
		    FALSE, FIGHTING(ch), GET_EQ(FIGHTING(ch),
						WEAR_WIELD), ch, TO_CHAR);
		act("$n parries $N's vicious attack with $s $o!",
		    FALSE, FIGHTING(ch), GET_EQ(FIGHTING(ch),
						WEAR_WIELD), ch,
		    TO_NOTVICT);
		damage_objs_exact(FIGHTING(ch), WEAR_WIELD);
		improve_skill(FIGHTING(ch), SKILL_PARRY, 3);
		continue;
	    }
	}

	if (!number(0, 50) && GET_EQ(FIGHTING(ch), WEAR_WIELD)) {
	    improve_skill(FIGHTING(ch), SKILL_BLOCK, 0);
	}
	if (!number(0, 50) && GET_EQ(ch, WEAR_WIELD)
	    && GET_EQ(FIGHTING(ch), WEAR_WIELD)) {
	    improve_skill(FIGHTING(ch), SKILL_PARRY, 0);

	}
//
// Use combat skills or a standard hit.
//
	    hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
//
// Run Away Mobs Run Away
//
	if (!IS_NPC(ch)) {
	    if (MOB_FLAGGED(ch, MOB_RUN_AWAY)) {
		if (!number(0, 1)) {
		    do_flee(ch, NULL, 0, 0);
		}
	    }

	    /* XXX: Need to see if they can handle "" instead of NULL. */
	    if (MOB_FLAGGED(ch, MOB_SPEC)
		&& mob_index[GET_MOB_RNUM(ch)].func != NULL)
		(mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");


	}
    }
}

void do_generic_knockdown(struct char_data *ch, struct char_data *vict,
			  int skill)
{

    int hit_roll = 0, to_hit = 0;

    if (GET_POS(vict) < POS_FIGHTING)
	send_to_char("How can you bash someone who's already down?\r\n",
		     ch);
    else {
	hit_roll = number(1, 80) + GET_STR(ch);
	to_hit = (100 - (int) (100 * GET_LEVEL(ch) / 250));

	if (GET_LEVEL(vict) >= LVL_IMMORT)
	    hit_roll = 0;

	if (hit_roll < to_hit) {
	    GET_POS(ch) = POS_SITTING;
	    damage(ch, vict, 0, skill);
	    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
	} else {
	    GET_POS(vict) = POS_SITTING;

	    damage(ch, vict, GET_LEVEL(ch) / 2, skill);
	    WAIT_STATE(vict, PULSE_VIOLENCE * 3);
	}
    }

    if (!IS_NPC(ch)) {
	improve_skill(ch, skill, 0);
	GET_SKILL_WAIT(ch) = PULSE_VIOLENCE * 3;
    }
}

/* This is for straight damage skills like kick and such. */

