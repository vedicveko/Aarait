// Misc commands and functions

#define __ACT_OTHER_C__
    
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
#include "coins.h"
    
/* extern variables */ 
extern struct room_data *world;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern char *class_abbrevs[];
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern int auto_save;
extern int track_through_doors;
extern struct char_data *character_list;
extern char *fish_names[];

/* extern procedures */ 
void list_skills(struct char_data *ch);
void appear(struct char_data *ch);
void write_aliases(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
void die(struct char_data *ch);
void Crash_rentsave(struct char_data *ch, int cost);
void add_follower(struct char_data *ch, struct char_data *leader);
void hunt_victim(struct char_data *ch);
void mobaction(struct char_data *ch, char *string, char *action);

/* local functions */ 
void livestock_eat_from_room(struct char_data *ch);
ACMD(do_flee);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_sneak);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
int perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_quit) 
{
    struct descriptor_data *d, *next_d;
    if (IS_NPC(ch) || !ch->desc)
	return;
    if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
	send_to_char("You have to type quit--no less, to quit!\r\n", ch);
    
    else if (GET_POS(ch) == POS_FIGHTING)
	send_to_char("No way!  You're fighting for your life!\r\n", ch);
    
    else if (GET_POS(ch) < POS_STUNNED) {
	send_to_char("You die before your time...\r\n", ch);
	die(ch);
    } else {
	int loadroom = ch->in_room;
        GET_LOADROOM(ch) = GET_ROOM_VNUM(ch->in_room);
	if (!GET_INVIS_LEV(ch))
	    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
	sprintf(buf, "%s has quit the game.", GET_NAME(ch));
	mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
	send_to_char("Goodbye, friend.. Come back soon!\r\n", ch);
	
	    /*
	     * kill off all sockets connected to the same player as the one who is
	     * trying to quit.  Helps to maintain sanity as well as prevent duping.
	     */ 
	    for (d = descriptor_list; d; d = next_d) {
	    next_d = d->next;
	    if (d == ch->desc)
		continue;
	    if (d->character
		 && (GET_IDNUM(d->character) == GET_IDNUM(ch))) STATE(d) =
		    CON_DISCONNECT; }
	if (free_rent)
	    Crash_rentsave(ch, 0);
	extract_char(ch);	/* Char is saved in extract char */
	
	    /* If someone is quitting in their house, let them load back here */ 
	    if (ROOM_FLAGGED(loadroom, ROOM_HOUSE))
	    save_char(ch, loadroom);
    }
}
ACMD(do_save) 
{
    if (IS_NPC(ch) || !ch->desc)
	return;
    
	/* Only tell the char we're saving if they actually typed "save" */ 
	if (cmd) {
	
	    /*
	     * This prevents item duplication by two PC's using coordinated saves
	     * (or one PC with a house) and system crashes. Note that houses are
	     * still automatically saved without this enabled. This code assumes
	     * that guest immortals aren't trustworthy. If you've disabled guest
	     * immortal advances from mortality, you may want < instead of <=.
	     */ 
	    if (auto_save && GET_LEVEL(ch) <= LVL_IMMORT) {
	    send_to_char("Saving aliases.\r\n", ch);
	    write_aliases(ch);
	    return;
	}
	sprintf(buf, "Saving %s and aliases.\r\n", GET_NAME(ch));
	send_to_char(buf, ch);
    }
    write_aliases(ch);
    save_char(ch, NOWHERE);
    Crash_crashsave(ch);
}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */ 
    ACMD(do_not_here) 
{
    send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}
ACMD(do_sneak) 
{
    struct affected_type af;
    byte percent;
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK)) {
	send_to_char("You have no idea how to do that.\r\n", ch);
	return;
    }
    send_to_char("Okay, you'll try to move silently for a while.\r\n",
		  ch); if (AFF_FLAGGED(ch, AFF_SNEAK))
	affect_from_char(ch, SKILL_SNEAK);
    percent = number(1, 101);	/* 101% is a complete failure */
    if (percent >
	  GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
	return;
    af.type = SKILL_SNEAK;
    af.duration = GET_LEVEL(ch);
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_SNEAK;
    affect_to_char(ch, &af);
}
ACMD(do_hide) 
{
    byte percent;
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE)) {
	send_to_char("You have no idea how to do that.\r\n", ch);
	return;
    }
    send_to_char("You attempt to hide yourself.\r\n", ch);
    if (AFF_FLAGGED(ch, AFF_HIDE))
	REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
    percent = number(1, 101);	/* 101% is a complete failure */
    if (percent >
	  GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide)
	return;
    SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}
ACMD(do_steal) 
{
    struct char_data *vict;
    struct obj_data *obj;
    char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
    int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;
    if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL)) {
	send_to_char("You have no idea how to do that.\r\n", ch);
	return;
    }
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
	send_to_char 
	    ("This room just has such a peaceful, easy feeling...\r\n",
	     ch); return;
    }
    two_arguments(argument, obj_name, vict_name);
    if (!(vict = get_char_vis(ch, vict_name, FIND_CHAR_ROOM))) {
	send_to_char("Steal what from who?\r\n", ch);
	return;
    } else if (vict == ch) {
	send_to_char("Come on now, that's rather stupid!\r\n", ch);
	return;
    }
    
	/* 101% is a complete failure */ 
	percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;
    if (GET_POS(vict) < POS_SLEEPING)
	percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */
    if (!pt_allowed && !IS_NPC(vict))
	pcsteal = 1;
    if (!AWAKE(vict))		/* Easier to steal from sleeping people. */
	percent -= 50;
    
	/* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */ 
	if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal
	    || GET_MOB_SPEC(vict) == shop_keeper)
	percent = 101;		/* Failure */
    if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {
	if (!(obj = get_obj_in_list_vis(ch, obj_name, vict->carrying))) {
	    for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
		if (GET_EQ(vict, eq_pos)
		     && (isname(obj_name, GET_EQ(vict, eq_pos)->name))
		     && CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
		    obj = GET_EQ(vict, eq_pos);
		    break;
		}
	    if (!obj) {
		act("$E hasn't got that item.", FALSE, ch, 0, vict,
		     TO_CHAR); return;
	    } else {		/* It is equipment */
		if ((GET_POS(vict) > POS_STUNNED)) {
		    send_to_char 
			("Steal the equipment now?  Impossible!\r\n", ch);
		    return;
		} else {
		    act("You unequip $p and steal it.", FALSE, ch, obj, 0,
			 TO_CHAR);
			act("$n steals $p from $N.", FALSE, ch, obj, vict,
			     TO_NOTVICT);
			obj_to_char(unequip_char(vict, eq_pos), ch);
		}
	    }
	} else {		/* obj found in inventory */
	    percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */
	    if (percent > GET_SKILL(ch, SKILL_STEAL)) {
		ohoh = TRUE;
		send_to_char("Oops..\r\n", ch);
		act("$n tried to steal something from you!", FALSE, ch, 0,
		     vict, TO_VICT);
		act("$n tries to steal something from $N.", TRUE, ch, 0,
		     vict, TO_NOTVICT);
	    } else {		/* Steal the item */
		if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
		    if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <
			 CAN_CARRY_W(ch)) {
			obj_from_char(obj);
			obj_to_char(obj, ch);
			send_to_char("Got it!\r\n", ch);
		    }
		} else
		    send_to_char("You cannot carry that much.\r\n", ch);
	    }
	}
    } else {			/* Steal some coins */
	if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
	    ohoh = TRUE;
	    send_to_char("Oops..\r\n", ch);
	    act("You discover that $n has $s hands in your wallet.",
		 FALSE, ch, 0, vict, TO_VICT);
	    act("$n tries to steal gold from $N.", TRUE, ch, 0, vict,
		 TO_NOTVICT); } else {
	    
		/* Steal some gold coins */ 
		gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
	    gold = MIN(1782, gold);
	    if (gold > 0) {
		GET_GOLD(ch) += gold;
		GET_GOLD(vict) -= gold;
		if (gold > 1) {
		    sprintf(buf, "Bingo!  You got %d gold coins.\r\n",
			     gold); send_to_char(buf, ch);
		} else {
		    send_to_char 
			("You manage to swipe a solitary gold coin.\r\n",
			 ch); }
	    } else {
		send_to_char("You couldn't get any gold...\r\n", ch);
	    }
	}
    }
    if (ohoh && IS_NPC(vict) && AWAKE(vict))
	hit(vict, ch, TYPE_UNDEFINED);
}
ACMD(do_visible) 
{
    if (GET_LEVEL(ch) >= LVL_IMMORT) {
	perform_immort_vis(ch);
	return;
    }
    if AFF_FLAGGED
	(ch, AFF_INVISIBLE) {
	appear(ch);
	send_to_char("You break the spell of invisibility.\r\n", ch);
    } else
	send_to_char("You are already visible.\r\n", ch);
}
int perform_group(struct char_data *ch, struct char_data *vict) 
{
    if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
	return (0);
    SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
    if (ch != vict)
	act("$N is now a member of your group.", FALSE, ch, 0, vict,
	     TO_CHAR);
	act("You are now a member of $n's group.", FALSE, ch, 0, vict,
	     TO_VICT);
	act("$N is now a member of $n's group.", FALSE, ch, 0, vict,
	     TO_NOTVICT); return (1);
}
void print_group(struct char_data *ch) 
{
    struct char_data *k;
    struct follow_type *f;
    if (!AFF_FLAGGED(ch, AFF_GROUP))
	send_to_char("But you are not the member of a group!\r\n", ch);
    
    else {
	send_to_char("Your group consists of:\r\n", ch);
	k = (ch->master ? ch->master : ch);
	if (AFF_FLAGGED(k, AFF_GROUP)) {
	    sprintf(buf,
		     "     [%3dH %3dM %3dV] [%2d %s] $N (Head of group)",
		     GET_HIT(k), GET_MANA(k), GET_MOVE(k), GET_LEVEL(k),
		     CLASS_ABBR(k));
	    act(buf, FALSE, ch, 0, k, TO_CHAR);
	}
	for (f = k->followers; f; f = f->next) {
	    if (!AFF_FLAGGED(f->follower, AFF_GROUP))
		continue;
	    sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N",
		      GET_HIT(f->follower), GET_MANA(f->follower),
		      GET_MOVE(f->follower), GET_LEVEL(f->follower),
		      CLASS_ABBR(f->follower));
	    act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
	}
    }
}
ACMD(do_group) 
{
    struct char_data *vict;
    struct follow_type *f;
    int found;
    one_argument(argument, buf);
    if (!*buf) {
	print_group(ch);
	return;
    }
    if (ch->master) {
	act 
	    ("You can not enroll group members without being head of a group.",
	     FALSE, ch, 0, 0, TO_CHAR);
	return;
    }
    if (!str_cmp(buf, "all")) {
	perform_group(ch, ch);
	for (found = 0, f = ch->followers; f; f = f->next)
	    found += perform_group(ch, f->follower);
	if (!found)
	    send_to_char 
		("Everyone following you is already in your group.\r\n",
		 ch); return;
    }
    if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
	send_to_char(NOPERSON, ch);
    
    else if ((vict->master != ch) && (vict != ch))
	act("$N must follow you to enter your group.", FALSE, ch, 0, vict,
	     TO_CHAR); 
    else {
	if (!AFF_FLAGGED(vict, AFF_GROUP))
	    perform_group(ch, vict);
	
	else {
	    if (ch != vict)
		act("$N is no longer a member of your group.", FALSE, ch,
		     0, vict, TO_CHAR);
	    act("You have been kicked out of $n's group!", FALSE, ch, 0,
		 vict, TO_VICT);
	    act("$N has been kicked out of $n's group!", FALSE, ch, 0,
		 vict, TO_NOTVICT);
	    REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
	}
    }
}
ACMD(do_ungroup) 
{
    struct follow_type *f, *next_fol;
    struct char_data *tch;
    one_argument(argument, buf);
    if (!*buf) {
	if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
	    send_to_char("But you lead no group!\r\n", ch);
	    return;
	}
	sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
	for (f = ch->followers; f; f = next_fol) {
	    next_fol = f->next;
	    if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
		REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
		send_to_char(buf2, f->follower);
		if (!AFF_FLAGGED(f->follower, AFF_CHARM))
		    stop_follower(f->follower);
	    }
	}
	REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
	send_to_char("You disband the group.\r\n", ch);
	return;
    }
    if (!(tch = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
	send_to_char("There is no such person!\r\n", ch);
	return;
    }
    if (tch->master != ch) {
	send_to_char("That person is not following you!\r\n", ch);
	return;
    }
    if (!AFF_FLAGGED(tch, AFF_GROUP)) {
	send_to_char("That person isn't in your group.\r\n", ch);
	return;
    }
    REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);
    act("$N is no longer a member of your group.", FALSE, ch, 0, tch,
	  TO_CHAR);
	act("You have been kicked out of $n's group!", FALSE, ch, 0, tch,
	     TO_VICT);
	act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch,
	     TO_NOTVICT); if (!AFF_FLAGGED(tch, AFF_CHARM))
	stop_follower(tch);
}

/* This patch will make it so that reporting no longer shows your
 * actual numbers, instead it gives a short statement based upon
 * the percentage of your normal max amount.
 * An example would be:
 * Kylen reports, "I seem to be bleeding all over the place, I have
 *   quite a lot of mana, and I couldn't move another step if my life
 *   depended on it."
 * Of course, you'll want to alter the messages to be appropriate to your
 * theme, maybe even make a set of different mana messages for non-casters,
 * mages, and priests, etc.
 *
 * I put everything in act.other.c before ACMD(do_report), but to really
 * do it right, these chars should go in constants.c
 */ 
    
/* Add these three sets before the do_report command */ 
const char *hit_rept_msgs[] = { "I think I'm going to pass out... ", /* 0 */ 
	"I have many grevious wounds! ", /* 10% */ 
	"I seem to be bleeding all over the place, ", /* 20% */ 
	"I've lost track of all the places I've been hit, ", /* 30% */ 
	"Some of these wounds look pretty bad, ", /* 40% */ 
	"I could use a healer, ", /* 50% */ 
	"Am I supposed to be bleeding this much? ", /* 60% */ 
	"My body aches all over, ", /* 70% */ 
	"I seem to have a few scratches, ", /* 80% */ 
	"I am feeling quite well, ", /* 90% */ 
	"I am in excellent health, " /* 100% */  
};
const char *mana_rept_msgs[] =
    { "I have no mystical energy to speak of, ", /* 0 */ 
	"my mystical reserves are almost depleted, ", /* 10% */ 
	"my mystical energies feel extremely weak, ", /* 20% */ 
	"I need to channel my reserves, ", /* 30% */ 
	"I have less mystical energy left than I thought, ", /* 40% */ 
	"I could use a chance to restore my mana, ", /* 50% */ 
	"I'm fine so quit asking, ", /* 60% */ 
	"I'm feeling the strain on my powers a bit, ", /* 70% */ 
	"I have a good deal of mana, ", /* 80% */ 
	"I have a quite a lot of mana, ", /* 90% */ 
	"my reserves of mana are full, " /* 100% */  
};
const char *move_rept_msgs[] =
    { "and I have almost no energy left.'\r\n", /* 0 */ 
	"I really could use a rest.'\r\n", /* 10% */ 
	"I could stumble a short way.'\r\n", /* 20% */ 
	"I think I could hike a short distance.'\r\n", /* 30% */ 
	"I'm feeling quite winded.'\r\n", /* 40% */ 
	"I could walk for a while a way.'\r\n", /* 50% */ 
	"My feet are a bit to weary.'\r\n", /* 60% */ 
	"I could walk for quite a while.'\r\n", /* 70% */ 
	"I am good to go.'\r\n", /* 80% */ 
	"I have a lot of energy.'\r\n", /* 90% */ 
	"I could walk forever.'\r\n" /* 100% */  
};


/* This is the altered do_report command. Insert the lines marked with the + */ 
    ACMD(do_report) 
{
    struct char_data *k;
    struct follow_type *f;
    int percent;
    if (!AFF_FLAGGED(ch, AFF_GROUP)) {
	send_to_char("But you are not a member of any group!\r\n", ch);
	return;
    }
    sprintf(buf, "%s reports, '", GET_NAME(ch));
    
	/* Health */ 
	if (GET_HIT(ch) <= 0)
	sprintf(buf + strlen(buf), "I should already be dead! ");
    
    else if (GET_HIT(ch) < 3)
	sprintf(buf + strlen(buf), "I am barely holding on to life, ");
    
    else if (GET_HIT(ch) >= GET_MAX_HIT(ch))
	sprintf(buf + strlen(buf), "I am in perfect health, ");
    
    else {
	percent = MIN(10, 
			((float)
			 ((float) GET_HIT(ch) / (float) GET_MAX_HIT(ch)) *
			 10));
	    sprintf(buf + strlen(buf), "%s", hit_rept_msgs[percent]);
    } 
	/* Move */ 
	if (GET_MOVE(ch) <= 0)
	sprintf(buf + strlen(buf),
		 "and I couldn't walk anywhere if my life depended on it.'\r\n");
    
    else if (GET_MOVE(ch) < 3)
	sprintf(buf + strlen(buf),
		 "and I think I could stumble another step.'\r\n");
    
    else if (GET_MOVE(ch) >= GET_MAX_MOVE(ch))
	sprintf(buf + strlen(buf), "and I am fully rested.'\r\n");
    
    else {
	percent = MIN(10, 
			((float)
			 ((float) GET_MOVE(ch) /
			  (float) GET_MAX_MOVE(ch))  *10));
	    sprintf(buf + strlen(buf), "%s", move_rept_msgs[percent]);
    } CAP(buf);
    k = (ch->master ? ch->master : ch);
    for (f = k->followers; f; f = f->next)
	if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
	    send_to_char(buf, f->follower);
    if (k != ch)
	send_to_char(buf, k);
    send_to_char("You report to the group.\r\n", ch);
}
ACMD(do_split) 
{
    int amount, num, share, rest;
    struct char_data *k;
    struct follow_type *f;
    char cointype[10];
    int type;
    if (IS_NPC(ch))
	return;
    two_arguments(argument, buf, cointype);
    if (is_number(buf)) {
	amount = atoi(buf);
	if (amount <= 0) {
	    send_to_char("Sorry, you can't do that.\r\n", ch);
	    return;
	}
	if (!strcmp(cointype, "platinum") 
	      ||is_abbrev(cointype, "platinum"))
	    type = PLAT_COINS;
	
	else if (!strcmp(cointype, "gold") || is_abbrev(cointype, "gold"))
	    type = GOLD_COINS;
	
	else if (!strcmp(cointype, "silver") 
		 ||is_abbrev(cointype, "silver"))
	    type = SILVER_COINS;
	
	else if (!strcmp(cointype, "copper") 
		 ||is_abbrev(cointype, "copper") 
		 ||!strcmp(cointype, "coins") 
		 ||is_abbrev(cointype, "coins"))
	    type = COPPER_COINS;
	
	else {
	    send_to_char("Split how many of what?\r\n", ch);
	    return;
	    if (amount > GET_MONEY(ch, type)) {
		send_to_char 
		    ("You don't seem to have that much gold to split.\r\n",
		     ch); return;
	    }
	    k = (ch->master ? ch->master : ch);
	    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
		num = 1;
	    
	    else
		num = 0;
	    for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && 
		     (!IS_NPC(f->follower)) && 
		     (f->follower->in_room == ch->in_room)) num++;
	    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
		share = amount / num;
		rest = amount % num;
	    } else {
		send_to_char 
		    ("With whom do you wish to share your gold?\r\n", ch);
		return;
	    }
	    remove_money_from_char(ch, (share * (num - 1)), type);
	    sprintf(buf, "%s splits %d coins; you receive %d.\r\n",
		      GET_NAME(ch), amount, share);
	    if (rest) {
		sprintf(buf + strlen(buf),
			 "%d coin%s %s not splitable, so %s " 
			 "keeps the money.\r\n", rest,
			 (rest == 1) ? "" : "s",
			 (rest == 1) ? "was" : "were", GET_NAME(ch));
	    }
	    if (AFF_FLAGGED(k, AFF_GROUP)
		 && (k->in_room == ch->in_room)  &&!(IS_NPC(k))
		 && k != ch) {add_money_to_char(ch, share, type);
		send_to_char(buf, k);
	    }
	    for (f = k->followers; f; f = f->next) {
		if (AFF_FLAGGED(f->follower, AFF_GROUP)
		     && (!IS_NPC(f->follower))
		     && (f->follower->in_room == ch->in_room)
		     && f->follower != ch) {
		    add_money_to_char(f->follower, share, type);
		    send_to_char(buf, f->follower);
		}
	    }
	    add_money_to_char(k, share, type);
	}
	send_to_char(buf, ch);
    } else {
	send_to_char 
	    ("How many coins do you wish to split with your group?\r\n",
	     ch); return;
    }
}
ACMD(do_wimpy) 
{
    int wimp_lev;
    
	/* 'wimp_level' is a player_special. -gg 2/25/98 */ 
	if (IS_NPC(ch))
	return;
    one_argument(argument, arg);
    if (!*arg) {
	if (GET_WIMP_LEV(ch)) {
	    sprintf(buf, "Your current wimp level is %d hit points.\r\n",
		     GET_WIMP_LEV(ch));
	    send_to_char(buf, ch);
	    return;
	} else {
	    send_to_char 
		("At the moment, you're not a wimp.  (sure, sure...)\r\n",
		 ch); return;
	}
    }
    if (isdigit(*arg)) {
	if ((wimp_lev = atoi(arg)) != 0) {
	    if (wimp_lev < 0)
		send_to_char 
		    ("Heh, heh, heh.. we are jolly funny today, eh?\r\n",
		     ch); 
	    else if (wimp_lev > GET_MAX_HIT(ch))
		send_to_char 
		    ("That doesn't make much sense, now does it?\r\n", ch);
	    
	    else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
		send_to_char 
		    ("You can't set your wimp level above half your hit points.\r\n",
		     ch); 
	    else {
		sprintf(buf,
			 "Okay, you'll wimp out if you drop below %d hit points.\r\n",
			 wimp_lev); send_to_char(buf, ch);
		GET_WIMP_LEV(ch) = wimp_lev;
	    }
	} else {
	    send_to_char 
		("Okay, you'll now tough out fights to the bitter end.\r\n",
		 ch); GET_WIMP_LEV(ch) = 0;
	}
    } else
	send_to_char 
	    ("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n",
	     ch); }
ACMD(do_gen_write) 
{
    FILE * fl;
    char *tmp, buf[MAX_STRING_LENGTH];
    const char *filename;
    struct stat fbuf;
    time_t ct;
    switch (subcmd) {
    case SCMD_BUG:
	filename = BUG_FILE;
	break;
    case SCMD_TYPO:
	filename = TYPO_FILE;
	break;
    case SCMD_IDEA:
	filename = IDEA_FILE;
	break;
    default:
	return;
    }
    ct = time(0);
    tmp = asctime(localtime(&ct));
    if (IS_NPC(ch)) {
	send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
	return;
    }
    skip_spaces(&argument);
    delete_doubledollar(argument);
    if (!*argument) {
	send_to_char("That must be a mistake...\r\n", ch);
	return;
    }
    sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
    mudlog(buf, CMP, LVL_IMMORT, TRUE);
    if (stat(filename, &fbuf) < 0) {
	perror("SYSERR: Can't stat() file");
	return;
    }
    if (fbuf.st_size >= max_filesize) {
	send_to_char 
	    ("Sorry, the file is full right now.. try again later.\r\n",
	     ch); return;
    }
    if (!(fl = fopen(filename, "a"))) {
	perror("SYSERR: do_gen_write");
	send_to_char("Could not open the file.  Sorry.\r\n", ch);
	return;
    }
    fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	     GET_ROOM_VNUM(IN_ROOM(ch)), argument);
    fclose(fl);
    send_to_char("Okay.  Thanks!\r\n", ch);
}

#define TOG_OFF 0
#define TOG_ON  1
    
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
    ACMD(do_gen_tog) 
{
    long result;
    const char *tog_messages[][2] = { 
	    {"Nohassle disabled.\r\n", "Nohassle enabled.\r\n"}, 
	{"Brief mode off.\r\n", "Brief mode on.\r\n"}, 
	{"Compact mode off.\r\n", "Compact mode on.\r\n"}, 
	{"You can now hear tells.\r\n", "You are now deaf to tells.\r\n"},
	{"You can now hear auctions.\r\n",
	  "You are now deaf to auctions.\r\n"},
	{"You can now hear shouts.\r\n",
	  "You are now deaf to shouts.\r\n"},
	{"You can now hear gossip.\r\n",
	  "You are now deaf to gossip.\r\n"},
	{"You can now hear the congratulation messages.\r\n",
	  "You are now deaf to the congratulation messages.\r\n"},
	{"You can now hear the Wiz-channel.\r\n",
	  "You are now deaf to the Wiz-channel.\r\n"},
	{"You will no longer see the room flags.\r\n",
	  "You will now see the room flags.\r\n"},
	{"You will now have your communication repeated.\r\n",
	  "You will no longer have your communication repeated.\r\n"},
	{"HolyLight mode off.\r\n", "HolyLight mode on.\r\n"},
	
	{"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
	 "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
	{"Autoexits disabled.\r\n", "Autoexits enabled.\r\n"},
	{"You Will no longer track through doors.\r\n",
	  "You Will now track through doors.\r\n"},
	{"You will no longer automatically loot corpses.\r\n",
	  "You will now automatically loot corpses.\r\n"},
	{"You will no longer see tips.\r\n",
	  "You will now see tips.\r\n"},
	{"You will no longer auto bait your pole.\r\n",
	  "You will now auto bait your pole.\r\n"} 
    };
    if (IS_NPC(ch))
	return;
    switch (subcmd) {
    case SCMD_NOHASSLE:
	result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
	break;
    case SCMD_BRIEF:
	result = PRF_TOG_CHK(ch, PRF_BRIEF);
	break;
    case SCMD_COMPACT:
	result = PRF_TOG_CHK(ch, PRF_COMPACT);
	break;
    case SCMD_NOTELL:
	result = PRF_TOG_CHK(ch, PRF_NOTELL);
	break;
    case SCMD_NOAUCTION:
	result = PRF_TOG_CHK(ch, PRF_NOAUCT);
	break;
    case SCMD_DEAF:
	result = PRF_TOG_CHK(ch, PRF_DEAF);
	break;
    case SCMD_NOGOSSIP:
	result = PRF_TOG_CHK(ch, PRF_NOGOSS);
	break;
    case SCMD_NOGRATZ:
	result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
	break;
    case SCMD_NOWIZ:
	result = PRF_TOG_CHK(ch, PRF_NOWIZ);
	break;
    case SCMD_ROOMFLAGS:
	result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
	break;
    case SCMD_NOREPEAT:
	result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
	break;
    case SCMD_HOLYLIGHT:
	result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
	break;
    case SCMD_SLOWNS:
	result = (nameserver_is_slow = !nameserver_is_slow);
	break;
    case SCMD_AUTOEXIT:
	result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
	break;
    case SCMD_TRACK:
	result = (track_through_doors = !track_through_doors);
	break;
    case SCMD_AUTOLOOT:
	result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
	break;
    case SCMD_TIPS:
	result = PRF_TOG_CHK(ch, PRF_TIPS);
	break;
    case SCMD_AUTOBAIT:
	result = PRF_TOG_CHK(ch, PRF_AUTOBAIT);
	if (GET_CURR_TIP(ch) == 8) {
	    GET_CURR_TIP(ch)++;
	}
	break;
    default:
	log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
	return;
    }
    if (result)
	send_to_char(tog_messages[subcmd][TOG_ON], ch);
    
    else
	send_to_char(tog_messages[subcmd][TOG_OFF], ch);
    return;
}
void send_to_zone_outdoor(int zone_rnum, char *messg) 
{
    struct descriptor_data *i;
    if (!messg || !*messg)
	return;
    for (i = descriptor_list; i; i = i->next)
	if (!i->connected && i->character && AWAKE(i->character) && 
	     (IN_ROOM(i->character) != NOWHERE)
	     && !IS_SET(ROOM_FLAGS(i->character->in_room), ROOM_INDOORS)
	     && (world[IN_ROOM(i->character)].zone == zone_rnum)) {
	    if (!IS_NPC(i->character) && AWAKE(i->character)) {
		send_to_char(messg, i->character);
	    }
	}
}
void send_to_zone_indoor(int zone_rnum, char *messg) 
{
    struct descriptor_data *i;
    if (!messg || !*messg)
	return;
    for (i = descriptor_list; i; i = i->next)
	if (!i->connected && i->character && AWAKE(i->character) && 
	      (IN_ROOM(i->character) != NOWHERE)
	      && IS_SET(ROOM_FLAGS(i->character->in_room), ROOM_INDOORS)
	      && (world[IN_ROOM(i->character)].zone == zone_rnum)) {
	    if (!IS_NPC(i->character) && AWAKE(i->character)) {
		send_to_char(messg, i->character);
	    }
	}
}
ACMD(do_souwee) 
{
    struct follow_type *f, *next_fol;
    struct obj_data *i;
    struct char_data *tracker, *pig = NULL;
    struct char_data *next_tracker;
    int door = 0;
    bool hunting = 0, found = 0;
    if (!IS_PIG_FARMER(ch)) {
	send_to_char("Why do you care about feeding pigs anyways?", ch);
	return;
    }
    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    act("You shout, 'SOOOOUUUWWWEEEEE!'", FALSE, ch, 0, 0, TO_CHAR);
    act("$n shouts, 'SOOOOUUUWWWEEEEE!'", FALSE, ch, 0, 0, TO_ROOM);
    for (door = 0; door < NUM_OF_DIRS; door++)
	if (CAN_GO(ch, door))
	    send_to_room 
		("You hear the shout of, 'SOOOUUUWEEEE!', in the distance.\r\n",
		 world[ch->in_room].dir_option[door]->to_room);
    for (f = ch->followers; f; f = next_fol) {
	next_fol = f->next;
	
	    // Are we being followed by a pig?
	    if (IS_NPC_PIG(f->follower)) {
	    pig = f->follower;
	}
    }
    if (!pig || pig == NULL) {
	return;
    }
    if(ch->in_room != pig->in_room) { return; }
   
    if (GET_MOB_WEIGHT(pig) > number(100, 125)) {
	act 
	    ("$n struggles, but he is to fat to reach the ground with his mouth.\n",
	     FALSE, pig, 0, 0, TO_ROOM);
	return;
    }
    for (tracker = character_list; tracker; tracker = next_tracker) {
	next_tracker = tracker->next;
	if (IS_NPC_CRAB(tracker) || IS_NPC_SKELETON(tracker)) {
	    if (number(0, 40)) {
		continue;
	    }
	    if (world[tracker->in_room].zone != world[pig->in_room].zone) {
		continue;
	    }
	    HUNTING(tracker) = pig;
	    hunt_victim(tracker);
	    hunting = 1;
	    
//          sprintf(buf, "%s: I am hunting %s.", GET_NAME(tracker),
//                  GET_NAME(HUNTING(tracker)));
//          mudlog(buf, CMP, LVL_GOD, TRUE);
		break;
	}
    }
    for (i = world[ch->in_room].contents; i; i = i->next_content) {
	if (GET_OBJ_VNUM(i) == 200 || GET_OBJ_VNUM(i) == 599) {
	    found = 1;
	    if (GET_MOB_HUNGER(pig) < 15) {
		
		    // eat the acorn
		    if (GET_OBJ_VNUM(i) == 200) {
		    GET_MOB_WEIGHT(pig) += number(3, 6);
		}
		if (GET_OBJ_VNUM(i) == 599) {
		    GET_MOB_WEIGHT(pig) += number(6, 9);
		}
		GET_MOB_HUNGER(pig) += 3;
		act 
		    ("$n sniffs around before pouncing on $p with piggy glee.",
		     FALSE, pig, i, 0, TO_ROOM);
		act("$n eats $p.\n", FALSE, pig, i, 0, TO_ROOM);
		obj_from_room(i);
		extract_obj(i);
		found = 1;
		for (f = ch->followers; f; f = next_fol) {
		    next_fol = f->next;
		    if (IS_NPC_LIVESTOCK(f->follower)
			  && !IS_NPC_PIG(f->
					  follower))
		    {livestock_eat_from_room(f->follower); }
		}
		if (IS_PIG_FARMER(ch)) {
		    improve_skill(ch, SKILL_PIG_FARMING, number(1, 5));
		    improve_points(ch, number(1, 5));
		}
		break;
	    } else {
		act("$n looks to stuffed to move.", FALSE, pig, 0, 0,
		     TO_ROOM); }
	}
    }
    if (!found) {
	act("$n sniffs around but doesn't find anything to eat!\n",
	      FALSE, pig, 0, 0, TO_ROOM);
	return;		// Pig found but no food
    }
    
	// Pick a random Crab in the zone to begin tracking the Pig
	for (tracker = character_list; tracker; tracker = next_tracker) {
	next_tracker = tracker->next;
	if (!IS_NPC(tracker)) {
	    continue;
	}
	if (IS_NPC_CRAB(tracker)) {
	    if (number(0, 60)) {
		continue;
	    }
	    HUNTING(tracker) = pig;
	    hunt_victim(tracker);
	    
//          sprintf(buf, "%s: I am hunting %s.", GET_NAME(tracker),
//                  GET_NAME(HUNTING(tracker)));
//          mudlog(buf, CMP, LVL_GOD, TRUE);
		hunting = 1;
	    break;
	}
    }
    if (hunting) {
	send_to_char("You feel the hair on your neck stand up.\r\n", ch);
	mobaction(pig, " ", "shiver");
    }
}

#define SAUSAGE 101
    ACMD(do_feed) 
{
    struct char_data *vict;
    struct obj_data *obj;
    char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
    two_arguments(argument, obj_name, vict_name);
    if (!(vict = get_char_vis(ch, vict_name, FIND_CHAR_ROOM))) {
	send_to_char("Feed who?\r\n", ch);
	return;
    } else if (vict == ch) {
	send_to_char("Come on now, that's rather stupid!\r\n", ch);
	return;
    }
    if (!IS_NPC(vict)) {
	send_to_char("Just ask them to eat something.\r\n", ch);
	return;
    }
    if (!(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying))) {
	send_to_char("Feed them what?\r\n", ch);
	return;
    }
    if (GET_OBJ_VNUM(obj) != SAUSAGE && IS_NPC_WOLF(vict)) {
	send_to_char("Wolves only eat sausages!\r\n", ch);
	return;
    }
    if (IS_NPC_WOLF(vict)) {
	if (GET_MOB_HUNGER(vict) < 10) {
	    GET_MOB_HUNGER(vict) += 20;
	    obj_from_char(obj);
	    extract_obj(obj);
	    if (IS_PIG_FARMER(ch)) {
		improve_skill(ch, SKILL_PIG_FARMING, 0);
		improve_points(ch, 0);
	    }
	    act("$n feeds $p to $N.\n", FALSE, ch, 0, vict, TO_ROOM);
	    act("You feed $p to $N.\n", FALSE, ch, obj, vict, TO_CHAR);
	    if (vict->master != ch) {
		add_follower(vict, ch);
	    }
	}
	
	    // He's full
	    else {
	    act("$n tries to feed $p to $N.\n", FALSE, ch, 0, vict,
		 TO_ROOM);
		act("You try to feed $p to $N.\n", FALSE, ch, obj, vict,
		     TO_CHAR);
		act("$N looks full and refuses $p.\n", FALSE, ch, 0, vict,
		     TO_ROOM); }
    }
    return;
}
void perform_fishing(void) 
{
    struct descriptor_data *i;
    struct char_data *fish = NULL;
    struct obj_data *pole;
    int bonus = 0, bait = 0;
    for (i = descriptor_list; i; i = i->next) {
	if (!i->connected) {
	    
		// They have a fish on the line.
		if (GET_REELIN(i->character)  &&GET_FISHON(i->character)) {
		fish =
		    get_char_num(real_mobile(GET_FISHON(i->character)));
if(!fish) {
log("No fish in perform fishing()!");
return;
}

		    if (GET_MOB_WEIGHT(fish) >
			 number(0, GET_STR(i->character))  &&!number(0,
								      3))
		{sprintf(buf,
			  "%s drags your line out further into the water.\r\n",
			  GET_NAME(fish));
		    CAP(buf);
		    send_to_char(buf, i->character);
		    sprintf(buf,
			     "%s drags %s's line out further into the water.\r\n",
			     GET_NAME(fish), GET_NAME(i->character));
                    CAP(buf);
		    act(buf, FALSE, i->character, 0, 0, TO_ROOM);
		    GET_REELIN(i->character) += 4;
		}
		if (GET_REELIN(i->character) > number(10, 900)) {
		    GET_REELIN(i->character) = 0;
		    GET_FISHON(i->character) = 0;
		    sprintf(buf, "%s breaks your line.", GET_NAME(fish));
		    act(buf, FALSE, i->character, 0, fish, TO_CHAR);
		    sprintf(buf, "%s breaks %s's line.", GET_NAME(fish),
			     GET_NAME(i->character));
		    act(buf, FALSE, i->character, 0, fish, TO_ROOM);
		}
		if (!number(0, 2)) {
		    sprintf(buf,
			     "%s leaps from the water in an attempt to free itself.",
			     GET_NAME(fish));
		    act(buf, FALSE, fish, 0, 0, TO_ROOM);
		}
		if (!number(0, 3)) {
		    sprintf(buf, "%s swims in a fury to free itself.",
			     GET_NAME(fish));
		    act(buf, FALSE, fish, 0, 0, TO_ROOM);
		}
		if (!number(0, 200)) {
		    send_to_char("You line suddenly goes slack.\r\n",
				  i->character);
			send_to_char("You must have lost your bait!\r\n",
				      i->character);
			act("$n's line suddenly goes slack.", FALSE,
			     i->character, 0, 0, TO_ROOM);
		    GET_FISHON(i->character) = 0;
		    GET_REELIN(i->character) = 0;
		}
	    }
	    if (IS_AFFECTED(i->character, AFF_FISHING)) {
		
		    // Fuck with the fisherman, break lines, steal bait ect....
		    if (!number(0, 300)) {
		    switch (number(0, 3)) {
		    case 0:
			send_to_char 
			    ("You feel a slight tug on your line.\r\n",
			     i->character); break;
		    case 1:
			send_to_char 
			    ("You feel a mighty yank on your line.\r\n",
			     i->character); break;
		    case 2:
			send_to_char 
			    ("A fly lands on the tip of your fishing pole.\r\n",
			     i->character);
			    act
			    ("A fly lands on the tip of $n's fishing pole.",
			     FALSE, i->character, 0, 0, TO_ROOM);
			break;
		    case 3:
			send_to_char 
			    ("The end of your fishing pole twitches.\r\n",
			     i->character);
			    act("The end of $n's fishing pole twitches.",
				 FALSE, i->character, 0, 0, TO_ROOM);
			break;
		    }
		    GET_OBJ_VAL(GET_EQ(i->character, WEAR_WIELD), 0) = 0;
		    GET_OBJ_VAL(GET_EQ(i->character, WEAR_WIELD), 1) = 1;
		    continue;
		}
		if (!number(0, 15000)) {
		    send_to_char 
			("You feel a mighty yank on your line and watch in horror as your fishing pole flies from your hands and into the murky water.\r\n",
			 i->character);
			act 
			("$n gets an odd look on his face as $s pole flies from $s hands and into the murky water.",
			 FALSE, i->character, 0, 0, TO_ROOM);
		    pole = GET_EQ(i->character, WEAR_WIELD);
		    obj_to_char(unequip_char(i->character, WEAR_WIELD),
				 i->character); obj_from_char(pole);
		    extract_obj(pole);
		    REMOVE_BIT(AFF_FLAGS(i->character), AFF_FISHING);
		    continue;
		}
		
		    /* This is evil and probally needs to be rewritten
		       So now we have a random guy who is both a fisherman and fishing.
		       Let's now search the room for NPC_FISH they can catch
		     */ 
		    for (fish = world[i->character->in_room].people; fish;
			 fish = fish->next_in_room) {
		    if (!IS_NPC_FISH(fish)) {
			continue;
		    }
		    if (number(0, 1)) {
			continue;
		    }
		    
// This is the code that determines if they catch the fish or not would go. 
			bait =
			GET_OBJ_VAL(GET_EQ(i->character, WEAR_WIELD), 1);
		    if (bait == GET_DAMROLL(fish)) {
			bonus += 10;
		    } else {
			bonus -= 10;
		    }
		    if (bait >= 15) {
			bonus += bait;
		    }		// Super bait
		    bonus += number(0, GET_OBJ_WEIGHT(GET_EQ 
							 (i->character,
							  WEAR_WIELD)));
			if (weather_info.sunlight == GET_HITROLL(fish)) {
			bonus += 5;
		    }
		    bonus += RM_CHUM(i->character->in_room);
		    if (!bait) {
			bonus -= 10;
		    }
		    if (GET_MOB_WEIGHT(fish) > 10) {
			bonus -= (GET_MOB_WEIGHT(fish) / 2);
		    }
		    if (GET_SKILL(i->character, SKILL_FISHING) >
			  10  &&IS_FISHERMAN(i->character)) {
			bonus +=
			    (GET_SKILL(i->character, SKILL_FISHING) / 10);
		    }
		    if (GET_OBJ_CSLOTS(GET_EQ(i->character, WEAR_WIELD)) > 10) {
			bonus 
                         += 
                        GET_OBJ_CSLOTS(GET_EQ(i->character, WEAR_WIELD)) / 10;
		    }
		    if (PRF_FLAGGED(i->character, PRF_DEBUG)) {
			sprintf(buf, "%s: Bonus: %d\r\n", GET_NAME(fish),
				 bonus);
			    send_to_room(buf, i->character->in_room); }
		    if (bonus > number(1, (GET_MOB_WEIGHT(fish) + 1))) {
			
			    // They caught the fish. Tell them to reelin and remove bait
			    GET_OBJ_VAL(GET_EQ(i->character, WEAR_WIELD),
					0) = 0;
			GET_OBJ_VAL(GET_EQ(i->character, WEAR_WIELD), 1) =
			    0;
			    REMOVE_BIT(AFF_FLAGS(i->character),
					AFF_FISHING);
			    GET_FISHON(i->character) = GET_MOB_VNUM(fish);
			if (GET_CURR_TIP(i->character) == 4) {
			    GET_CURR_TIP(i->character)++;
			}
			if (GET_MOB_WEIGHT(fish) > 10) {
			    GET_REELIN(i->character) +=
				(GET_MOB_WEIGHT(fish) / 10);
			}
			send_to_char 
			    ("You feel something take hold of your line!\r\nREEL IT IN!\r\n",
			     i->character);
			    act("$n's pole bends down sharply!", FALSE,
				 i->character, 0, fish, TO_ROOM);
			if (IS_FISHERMAN(i->character)) {
			    improve_skill(i->character, SKILL_FISHING, 0);
			    improve_points(i->character, 0);
			}
			break;
		    } else if (!number(0, 3)) {
			switch (number(0, 2)) {
			case 0:
			    sprintf(buf,
				     "Your pole jiggles and you notice %s swimming by.\r\n",
				     GET_NAME(fish));
			    send_to_char(buf, i->character);
			    sprintf(buf,
				     "%s's pole jiggles and you notice %s swimming by.",
				     GET_NAME(i->character),
				     GET_NAME(fish));
				act(buf, FALSE, i->character, 0, 0,
				     TO_ROOM); break;
			case 1:
			    send_to_char("Your pole jiggles.\r\n",
					  i->character);
				act("$n's pole jiggles.", FALSE,
				     i->character, 0, 0, TO_ROOM);
			    break;
			case 2:
			    sprintf(buf,
				     "You notice %s swimming by.\r\n",
				     GET_NAME(fish));
			    send_to_char(buf, i->character);
			    sprintf(buf, "You notice %s swimming by.",
				     GET_NAME(fish));
			    act(buf, FALSE, i->character, 0, 0, TO_ROOM);
			    break;
			default:
			    sprintf(buf,
				     "You notice %s swimming by.\r\n",
				     GET_NAME(fish));
			    send_to_char(buf, i->character);
			    sprintf(buf, "You notice %s swimming by.",
				     GET_NAME(fish));
			    act(buf, FALSE, i->character, 0, 0, TO_ROOM);
			    break;
			}
		    }
		}
	    }
	}
    }
    return;
}
void add_to_webpage(struct char_data *ch, int mode) 
{
    FILE * fl;
    char *tmp, buf[MAX_STRING_LENGTH];
    const char *filename;
    struct stat fbuf;
    time_t ct;
    int points = GET_POINTS(ch);
    ct = time(0);
    tmp = asctime(localtime(&ct));
    switch (mode) {
    case MODE_JOB_DONE:
	sprintf(buf,
		 "<strong>%s of %s with %d points and %d coins<br> has completed their job as a %s!</strong><p>\n",
		 GET_NAME(ch), noble_houses[(int) GET_HOUSE(ch)], points,
		 (convert_all_to_copper(ch) + GET_BANK_GOLD(ch)),
		 pc_class_types[(int) GET_CLASS(ch)]);
	filename = "/home/aarait/public_html/job_done.txt";
	if (stat(filename, &fbuf) < 0) {
	    perror("SYSERR: Can't stat() file");
	    return;
	}
	if (fbuf.st_size >= max_filesize) {
	    log 
		("Sorry, the file is full right now. try again later. job done");
	    return;
	}
	if (!(fl = fopen(filename, "a"))) {
	    perror("SYSERR: add_to_webpage, JOB_DONE");
	    log("Could not open the file.  Sorry.");
	    return;
	}
	fprintf(fl, buf);
	fclose(fl);
	break;
	
	    /* End of MODE DONE */ 
    default:
	log("INVALID MODE PASSED TO add_to_webpage()");
	mudlog("INVALID MODE PASSED TO add_to_webpage()", BRF, 31, TRUE);
	break;
    }
}
ACMD(do_has) 
{
    int percent;
    sprintf(buf, "%s reports, '", GET_NAME(ch));
    
	/* Health */ 
	if (GET_HIT(ch) <= 0)
	sprintf(buf + strlen(buf), "I should already be dead! ");
    
    else if (GET_HIT(ch) < 3)
	sprintf(buf + strlen(buf), "I am barely holding on to life, ");
    
    else if (GET_HIT(ch) >= GET_MAX_HIT(ch))
	sprintf(buf + strlen(buf), "I am in perfect health, ");
    
    else {
	percent = MIN(10, 
			((float)
			 ((float) GET_HIT(ch) / (float) GET_MAX_HIT(ch)) *
			 10));
	    sprintf(buf + strlen(buf), "%s", hit_rept_msgs[percent]);
    } 
	/* Move */ 
	if (GET_MOVE(ch) <= 0)
	sprintf(buf + strlen(buf),
		 "and I couldn't walk anywhere if my life depended on it.'\r\n");
    
    else if (GET_MOVE(ch) < 3)
	sprintf(buf + strlen(buf),
		 "and I think I could stumble another step.'\r\n");
    
    else if (GET_MOVE(ch) >= GET_MAX_MOVE(ch))
	sprintf(buf + strlen(buf), "and I am fully rested.'\r\n");
    
    else {
	percent = MIN(10, 
			((float)
			 ((float) GET_MOVE(ch) /
			  (float) GET_MAX_MOVE(ch))  *10));
	    sprintf(buf + strlen(buf), "%s", move_rept_msgs[percent]);
    } act(buf, FALSE, ch, 0, 0, TO_NOTVICT);
    send_to_char("You report to the room.\r\n", ch);
} void update_chum(void) 
{
    int i;
    for (i = 0; i < top_of_world; i++)
	if (RM_CHUM(i) > 0)
	    RM_CHUM(i) -= 1;
}
void increase_chum(int rm, int ammount) 
{
    RM_CHUM(rm) += ammount;
} 

#define MAX_RECORDS 5
int fish_records[MAX_FISH + 1];
int goblin_records[MAX_RECORDS];
int pig_records[MAX_RECORDS];
int wealth_records[MAX_RECORDS];
void init_world_records(void) 
{
    int i, j, wealth = 0, leader = 0, old_leader = 0, pounds = 1, n = 0;
    int one = 0, two = 0, three = 0, four = 0, five = 0;
    int poundstwo = 1, poundsthree = 1, poundsfour = 1, poundsfive = 1;
    extern int top_of_p_table;
    extern struct player_index_element *player_table;
    struct char_file_u chdata;
    FILE * opf;
    log("Updating World Records.");
    
// Loop through the fish and find the guy with the record for that fish.
	for (i = 0; i < MAX_FISH; i++) {
	
// Loop through the pfile for victims
	    pounds = 0;
	leader = 0;
	old_leader = 0;
	for (j = 0; j <= top_of_p_table; j++) {
	    load_char((player_table + j)->name, &chdata);
	    
// Does this guy have the heaviest fish?
//            sprintf(buf, "POUNDS %d NAME: %s BEAT %d", pounds, chdata.name, chdata.player_specials_saved.types_of_fish[i][1]);
//            log(buf);
		if (chdata.player_specials_saved.types_of_fish[i][1] >
		     pounds) {pounds =
		    chdata.player_specials_saved.types_of_fish[i][1];
		old_leader = leader;
		leader = j;
	    }
	}
	
// Incaseshit.
	    if (!leader || !pounds) {
	    continue;
	}
	
// Then, let's make the highest poundage the leader.
	    fish_records[i] = leader;
    }
    pounds = 0;
    
// Now, let us check for Goblin Slayers and Pig Farmers.
	for (j = 0; j <= top_of_p_table; j++) {
	load_char((player_table + j)->name, &chdata);
	if (chdata.player_specials_saved.goblins_slayed > pounds) {
	    five = four;
	    four = three;
	    three = two;
	    two = one;
	    one = j;
	    pounds = chdata.player_specials_saved.goblins_slayed;
	} else if (chdata.player_specials_saved.goblins_slayed >
		    poundstwo) {five = four;
	    four = three;
	    three = two;
	    two = j;
	    poundstwo = chdata.player_specials_saved.goblins_slayed;
	}
	
	else if (chdata.player_specials_saved.goblins_slayed >
		 poundsthree) {five = four;
	    four = three;
	    three = j;
	    poundsthree = chdata.player_specials_saved.goblins_slayed;
	}
	
	else if (chdata.player_specials_saved.goblins_slayed > poundsfour) {
	    five = four;
	    four = j;
	    poundsfour = chdata.player_specials_saved.goblins_slayed;
	}
	
	else if (chdata.player_specials_saved.goblins_slayed > poundsfive) {
	    five = j;
	    poundsfive = chdata.player_specials_saved.goblins_slayed;
	}
    }
    goblin_records[0] = one;
    goblin_records[1] = two;
    goblin_records[2] = three;
    goblin_records[3] = four;
    goblin_records[4] = five;
    one = 0;
    two = 0;
    three = 0;
    four = 0;
    five = 0;
    pounds = 1;
    poundstwo = 1;
    poundsthree = 1;
    poundsfour = 1;
    poundsfive = 1;
    for (j = 0; j <= top_of_p_table; j++) {
	load_char((player_table + j)->name, &chdata);
	if (chdata.player_specials_saved.pounds_of_pig > pounds) {
	    five = four;
	    four = three;
	    three = two;
	    two = one;
	    one = j;
	    pounds = chdata.player_specials_saved.pounds_of_pig;
	} else if (chdata.player_specials_saved.pounds_of_pig > poundstwo) {
	    five = four;
	    four = three;
	    three = two;
	    two = j;
	    poundstwo = chdata.player_specials_saved.pounds_of_pig;
	}
	
	else if (chdata.player_specials_saved.pounds_of_pig > poundsthree) {
	    five = four;
	    four = three;
	    three = j;
	    poundsthree = chdata.player_specials_saved.pounds_of_pig;
	} else if (chdata.player_specials_saved.pounds_of_pig >
		    poundsfour) {five = four;
	    four = j;
	    poundsfour = chdata.player_specials_saved.pounds_of_pig;
	} else if (chdata.player_specials_saved.pounds_of_pig >
		    poundsfive) {five = j;
	    poundsfive = chdata.player_specials_saved.pounds_of_pig;
	}
    }
    pig_records[0] = one;
    pig_records[1] = two;
    pig_records[2] = three;
    pig_records[3] = four;
    pig_records[4] = five;
    
	// Update the Webpage
	*buf = '\0';
    if (
	 (opf =
	  fopen("//home//aarait//public_html//fish-records.txt",
		"w"))  == 0) return;	/* or log it ? *shrug* */
    for (n = 0; n < MAX_FISH; n++) {
	if (fish_records[n]) {
	    load_char((player_table + fish_records[n])->name, &chdata);
	    sprintf(buf,
		     "%s<strong>%s</strong> %d Pounds (Caught by %s)\n",
		     buf, fish_names[n],
		     chdata.player_specials_saved.types_of_fish[n][1],
		     chdata.name); }
    }
    fprintf(opf, buf);
    fclose(opf);
    *buf = '\0';
    if (
	 (opf =
	  fopen("//home//aarait//public_html//goblin-records.txt",
		 "w")) == 0)
	return;			/* or log it ? *shrug* */
    for (i = 0; i <= MAX_RECORDS; i++) {
	if (goblin_records[i]) {
	    load_char((player_table + goblin_records[i])->name, &chdata);
	    sprintf(buf, "%s%d. %s:  [%d Goblins Slayed]\n", buf, (i + 1),
		     chdata.name,
		     chdata.player_specials_saved.goblins_slayed); }
    }
    fprintf(opf, buf);
    fclose(opf);
    *buf = '\0';
    if ((opf = fopen("//home//aarait//public_html//pig-records.txt", "w"))
	  == 0)
	return;		/* or log it ? *shrug* */
    for (i = 0; i <= MAX_RECORDS; i++) {
	if (pig_records[i]) {
	    load_char((player_table + pig_records[i])->name, &chdata);
	    sprintf(buf, "%s%d. %s:  [%d Pounds of Pig Raised]\n", buf,
		     (i + 1), chdata.name,
		     chdata.player_specials_saved.pounds_of_pig); }
    }
    fprintf(opf, buf);
    fclose(opf);
    
// Wealth Records
	one = 0;
    two = 0;
    three = 0;
    four = 0;
    five = 0;
    pounds = 1;
    poundstwo = 1;
    poundsthree = 1;
    poundsfour = 1;
    poundsfive = 1;
    for (j = 0; j <= top_of_p_table; j++) {
	load_char((player_table + j)->name, &chdata);
	if (chdata.level > 30) {
	    continue;
	}
	wealth =
	    (chdata.points.money[PLAT_COINS] +
	     chdata.points.money[GOLD_COINS] +
	     chdata.points.money[SILVER_COINS] +
	     chdata.points.money[COPPER_COINS] +
	     chdata.points.bank_gold); if (wealth > pounds) {
	    five = four;
	    four = three;
	    three = two;
	    two = one;
	    one = j;
	    pounds = wealth;
	} else if (wealth > poundstwo) {
	    five = four;
	    four = three;
	    three = two;
	    two = j;
	    poundstwo = wealth;
	}
	
	else if (wealth > poundsthree) {
	    five = four;
	    four = three;
	    three = j;
	    poundsthree = wealth;
	}
	
	else if (wealth > poundsfour) {
	    five = four;
	    four = j;
	    poundsfour = wealth;
	}
	
	else if (wealth > poundsfive) {
	    five = j;
	    poundsfive = wealth;
	}
    }
    wealth_records[0] = one;
    wealth_records[1] = two;
    wealth_records[2] = three;
    wealth_records[3] = four;
    wealth_records[4] = five;
    *buf = '\0';
    if (
	 (opf =
	  fopen("//home//aarait//public_html//wealth-records.txt",
		"w"))  == 0)
	return;
    for (i = 0; i <= MAX_RECORDS; i++) {
	if (wealth_records[i]) {
	    load_char((player_table + wealth_records[i])->name, &chdata);
	    sprintf(buf, "%s%d. %s:  [%d Coins]\n", buf, (i + 1),
		     chdata.name,
		     (chdata.points.money[PLAT_COINS] +
		       chdata.points.money[GOLD_COINS] +
		       chdata.points.money[SILVER_COINS] +
		       chdata.points.money[COPPER_COINS] +
		       chdata.points.bank_gold)); }
    }
    fprintf(opf, buf);
    fclose(opf);
    one = 0;
    two = 0;
    three = 0;
    four = 0;
    five = 0;
}
ACMD(do_records) 
{
    int i = 0;
    extern struct player_index_element *player_table;
    struct char_file_u chdata;
    sprintf(buf, "Current Fishing Records:\r\n\r\n");
    for (i = 0; i < MAX_FISH; i++) {
	if (fish_records[i]) {
	    load_char((player_table + fish_records[i])->name, &chdata);
	    sprintf(buf, "%s%-30s: [%-3d Pounds] Caught by %s\r\n", buf,
		     fish_names[i],
		     chdata.player_specials_saved.types_of_fish[i][1],
		     chdata.name); }
    }
    sprintf(buf, "%s\r\nTop Five Goblin Slayers:\r\n\r\n", buf);
    for (i = 0; i < MAX_RECORDS; i++) {
	if (goblin_records[i]) {
	    load_char((player_table + goblin_records[i])->name, &chdata);
	    sprintf(buf, "%s%-2d. %-26s:[%-4d Goblins Slayed]\r\n", buf,
		     (i + 1), chdata.name,
		     chdata.player_specials_saved.goblins_slayed); }
    }
    sprintf(buf, "%s\r\nTop Five Pig Farmers:\r\n\r\n", buf);
    for (i = 0; i < MAX_RECORDS; i++) {
	if (pig_records[i]) {
	    load_char((player_table + pig_records[i])->name, &chdata);
	    sprintf(buf, "%s%-2d. %-26s:[%-4d Pounds of Pig Raised]\r\n",
		     buf, (i + 1), chdata.name,
		     chdata.player_specials_saved.pounds_of_pig); }
    }
    
/*
    sprintf(buf, "%s\r\nTop Five Wealthiest People:\r\n\r\n", buf);

    for (i = 0; i < MAX_RECORDS; i++) {
	if (wealth_records[i]) {
	    load_char((player_table + wealth_records[i])->name, &chdata);
	    sprintf(buf, "%s%d. %-26s:  [%-4d Coins]\n", buf,
		    (i + 1), chdata.name,
		    (chdata.points.money[PLAT_COINS] +
                     chdata.points.money[GOLD_COINS] +
                     chdata.points.money[SILVER_COINS] +
                     chdata.points.money[COPPER_COINS] +
                     chdata.points.bank_gold));
	}
    }

*/ 
	sprintf(buf, "%s\r\n", buf);
    page_string(ch->desc, buf, TRUE);
    if (GET_CURR_TIP(ch) == 11) {
	GET_CURR_TIP(ch) = 0;
	send_to_char 
	    ("/cGTIP:: Visit our webpage at http:////thevedic.net//~aarait/c0\r\n",
	     ch);
	    send_to_char 
	    ("/cGTIP:: Use the TIPS command to stop viewing these messages./c0\r\n",
	     ch); }
}
ACMD(do_tame) 
{
    struct char_data *vict = NULL, *tracker = NULL, *next_tracker = NULL;
    int percent, prob, found = 0, hunting = 0;
    struct follow_type *f, *next_fol;
    one_argument(argument, arg);
    if (!*arg) {
	send_to_char("Befriend what?\r\n", ch);
	return;
    }
    if (!(vict = get_char_room_vis(ch, arg))) {
	send_to_char("They're not here...\r\n", ch);
	return;
    }
    if (vict == ch) {
	send_to_char("How sad that you have no other friends...\r\n", ch);
	return;
    }
    if (!IS_NPC(vict)) {
	send_to_char("I don't think so...\r\n", ch);
	return;
    }
    if (!IS_NPC_PIG(vict)) {
	send_to_char("That is not pig!\r\n", ch);
	return;
    }
    percent = dice(1, 100);
    prob = GET_SKILL(ch, SKILL_TAME) + GET_CHA(ch);
    if (percent > prob) {
	act("$n tries to tame $N, but it doesn't work.", FALSE, ch, 0,
	     vict, TO_ROOM);
	act("You try to tame $N and fail!", FALSE, ch, 0, vict, TO_CHAR);

        // Pick a random Crab in the zone to begin tracking the Pig
        for (tracker = character_list; tracker; tracker = next_tracker) {
        next_tracker = tracker->next;
        if (!IS_NPC(tracker)) {
            continue;
        }
        if (IS_NPC_CRAB(tracker) || IS_NPC_SKELETON(tracker) || IS_NPC_WOLF(tracker)) {
            if (!number(0, 3)) {
                continue;
            }
            HUNTING(tracker) = vict;
            hunt_victim(tracker);

//          sprintf(buf, "%s: I am hunting %s.", GET_NAME(tracker),
//                  GET_NAME(HUNTING(tracker)));
//          mudlog(buf, CMP, LVL_GOD, TRUE);
                hunting = 1;
            break;
        }
    }

    if (hunting) {
        send_to_char("You feel the hair on your neck stand up.\r\n", ch);
        mobaction(vict, " ", "shiver");
    }
	do_flee(vict, NULL, 0, 0);
	improve_skill(ch, SKILL_TAME, 0);
	return;
    }
    if (vict->master) {
	send_to_char("That pig has already been tamed.\r\n", ch);
	return;
    }
    for (f = ch->followers; f; f = next_fol) {
	next_fol = f->next;
	if (IS_NPC(f->follower) && IS_NPC_PIG(f->follower)) {
	    found = 1;
	}
    }
    if (found) {
	send_to_char("But, you've already got a pig.\r\n", ch);
	return;
    }
//    improve_stat(ch, APPLY_CHA);
    improve_skill(ch, SKILL_TAME, 0);
    act("You tame $N.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tames $N.", FALSE, ch, 0, vict, TO_ROOM);
    sprintf(buf, "follow %s", GET_NAME(ch));
    command_interpreter(vict, buf);
    if (IS_NPC(vict)) {
	REMOVE_BIT(MOB_FLAGS(vict), MOB_AGGRESSIVE);
	REMOVE_BIT(MOB_FLAGS(vict), MOB_SPEC);
	REMOVE_BIT(MOB_FLAGS(vict), MOB_ACORN_EAT);
    }
}
void livestock_eat_from_room(struct char_data *ch)
{
    int weight = 0, food = 0;;
    if (!ch->master) {
	return;
    }
    switch (GET_RACE(ch)) {
    case RACE_NPC_COW:
	weight = 5;
	if (GET_MOB_WEIGHT(ch) > number(600, 800)) {
	    return;
	}
	break;
    case RACE_NPC_GOAT:
    case RACE_NPC_SHEEP:
	weight = 3;
	if (GET_MOB_WEIGHT(ch) > number(100, 120)) {
	    return;
	}
	break;
    case RACE_NPC_CHICKEN:
	weight = 1;
	if (GET_MOB_WEIGHT(ch) > number(6, 10)) {
	    return;
	}
	break;
    default:
	return;
    }
    if (SECT(ch->in_room) == SECT_FARM) {
	weight *= 2;
    }
    food = number(0, MAX_ITEMS_TO_EAT);
    sprintf(buf, "%s eats %s.", GET_NAME(ch), items_to_eat[food]);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    GET_MOB_WEIGHT(ch) += weight;
}

ACMD(do_flee)
{
  int i, attempt, loss;
  struct char_data *was_fighting;

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("You are in pretty bad shape, unable to flee!\r\n", ch);
    return;
  }

  for (i = 0; i < 6; i++) {
    attempt = number(0, NUM_OF_DIRS - 1);       /* Select a random direction */
    if (CAN_GO(ch, attempt) &&
        !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      was_fighting = FIGHTING(ch);
      if (do_simple_move(ch, attempt, TRUE)) {
        send_to_char("You flee head over heels.\r\n", ch);
        if (was_fighting && !IS_NPC(ch)) {
          loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
          loss *= GET_LEVEL(was_fighting);
          gain_exp(ch, -loss);
        }

      } else {
        act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}
