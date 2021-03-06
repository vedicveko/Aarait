/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"

/* external structs */
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern int no_specials;

ACMD(do_get);
ACMD(do_eat);
ACMD(do_flee);
ACMD(do_follow);

/* local functions */
void mobile_activity(void);
void clearMemory(struct char_data *ch);
void add_follower(struct char_data *ch, struct char_data *leader);
extern struct time_info_data time_info;
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
void perform_remove(struct char_data *ch, int pos);
SPECIAL(shop_keeper);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void perform_get_from_container(struct char_data *ch, struct obj_data *obj,
				struct obj_data *cont, int mode);
void do_generic_skill(struct char_data *ch, struct char_data *vict,
		      int skill, int dam);
extern struct char_data *mob_proto;
void livestock_eat_from_room(struct char_data *ch);
void move_mob(struct char_data *ch);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)

const char *wake_msg[] = {
    "BUG PLEASE REPORT wake_msg",
    "",
    "$n flits down from their roosting spot and greets the new day.",
    "$n stands up and greets the new day.",
    "",
    "",
    "",
    "$n arrives with a smile and a refreshed look.",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

const char *sleep_msg[] = {
    "BUG PLEASE REPORT sleep_msg",
    "",
    "With a flap of wings, $n leaves in search of a place to roost.",
    "$n leaves to find a place to sleep.",
    "",
    "",
    "",
    "With a nod, $n leaves to go home to rest.",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

const char *hwake_msg[] = {
    "BUG PLEASE REPORT hwake_msg",
    "",
    "$n flits down from their hibernation spot and greets the new day.",
    "$n awakes up and greets the new season.",
    "",
    "",
    "",
    "$n arrives with a smile and a refreshed look.",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

const char *hsleep_msg[] = {
    "BUG PLEASE REPORT hsleep_msg",
    "",
    "With a flap of wings, $n leaves in search of a place to hibernate.",
    "$n leaves to find a place to hibernate for the winter.",
    "",
    "",
    "",
    "With a nod, $n leaves to go home for the winter.",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

const char *general_emote_msg[][4] = {

    {"You hear a quail call in the distance.",
     "You hear a turkey gobble.", "You hear the sound of running water.",
     "You hear a tree branch break."},
    {"$n clucks happily.", "$n flaps $s wings.", "$n peers around $mself.",
     "$n looks nervous."},
    {"$n ruffles their feathers.", "$n flexes their wings.",
     "A smal dust cloud arises as $n pounces on a bug.",
     "You hear a clucking sound coming from $n."},
    {"With a quick motion, $n scratches $mself.", "$n wrinkles $s nose.",
     "$n peers around $mself.", "$n looks nervous."},
    {"$n nibbles on a tin can.", "$n maaaahs.", "$n tries to butt you.",
     "$n tries to hide."},
    {"$n baaaaaass.", "A fly buzzes around $n.", "$n moves closer to you.",
     "$n flicks $s tongue."},
    {"$n makes a clacking sound.",
     "$n buzzes around madly for a bit, then stops suddenly.",
     "A chirp can be heard coming from $n.", "$n moves closer to you."},
    {"$n sighs.", "$n scratches $s chin.", "$n gazes around.",
     "$n wrinkles $s nose."},
    {"$n's bones rattle.", "$n looks lost.", "$n looks hungry.",
     "$n's bones rattle."},
    {"$n mooooooos.", "$n chews $s cud.", "$n poops.",
     "$n begins drooling."},
    {"$n picks $s nose.", "$n scratches $s ass.", "$n burps.",
     "$n eats a booger."},
    {"$n oinks.", "A fly buzzes around $n.",
     "A horrid smell wafts from $n.", "$n wiggles $s tail."},
    {"$n howls.", "$n growls.", "$n licks $s lips.",
     "$n yelps and begins scratching $mself."},
    {"$n waves $s claw about.", "$n's shell rattles.",
     "$n's shell makes a scraping sound.", "$n looks hungry."},
    {"The water makes a swirling motion.", "Bubbles rise from the water.",
     "You hear a splash.",
     "A fish jumps, glinting in the light and landing with a splash."},
};


void mobile_activity(void)
{
    register struct char_data *ch, *next_ch, *vict;
    struct obj_data *obj, *best_obj, *o;
    int found, max;
    memory_rec *names;
    struct obj_data *wield_obj;
    int i, a1 = 0, a2 = 0, w = 0, ac = 0, r_num;
    float max1, dam2;

    for (ch = character_list; ch; ch = next_ch) {
	next_ch = ch->next;

	if (!IS_MOB(ch) || !IS_NPC(ch))
	    continue;

	/* Examine call for special procedure */
	if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
	    if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
		log
		    ("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
		     GET_NAME(ch), GET_MOB_VNUM(ch));
		REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
	    } else {
		if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, ""))
		    continue;
	    }
	}

	/* If the mob has no specproc, do the default actions */
	if (FIGHTING(ch) || !AWAKE(ch))
	    continue;


	if (MOB_FLAGGED(ch, MOB_ACORN_EAT)) {
	    for (o = world[ch->in_room].contents; o; o = o->next_content) {
		if (GET_OBJ_VNUM(o) == 200 || GET_OBJ_VNUM(o) == 599) {
		    act("$n eats $p.", FALSE, ch, o, 0, TO_ROOM);
		    obj_from_room(o);
		    extract_obj(o);
		}
	    }
	}

	/* Scavenger (picking up objects) Goblins are scavengers too */

	if ((MOB_FLAGGED(ch, MOB_SCAVENGER) || IS_NPC_GOBLIN(ch)
	     || IS_NPC_HIGHHUMAN(ch)) && !FIGHTING(ch) && AWAKE(ch)) {
	    if (world[ch->in_room].contents && !number(0, 1)) {
		max = 1;
		best_obj = NULL;
		for (obj = world[ch->in_room].contents; obj;
		     obj = obj->next_content) if (CAN_GET_OBJ(ch, obj)) {
			if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER
			    && GET_OBJ_VAL(obj, 3) == 1) {
			    continue;
			}
			best_obj = obj;
		    }
		if (best_obj != NULL) {
		    obj_from_room(best_obj);
		    obj_to_char(best_obj, ch);
		    if (GET_OBJ_TYPE(best_obj) == ITEM_WEAPON) {
			(ch)->mob_specials.newitem = 1;
		    }
		    if (GET_OBJ_TYPE(best_obj) == ITEM_ARMOR
			|| GET_OBJ_TYPE(best_obj) == ITEM_WORN) {
			(ch)->mob_specials.newitemarmor = 1;
		    }
		    act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
		    if (GET_OBJ_TYPE(best_obj) == ITEM_FOOD) {
			do_eat(ch, best_obj->name, 0, 0);
		    }
		}
	    }
	}

	/* HERD MOBS  */
	if (MOB_FLAGGED(ch, MOB_HERD)) {
	    if (ch->master && ch->master->in_room != ch->in_room
		&& !number(0, 500)) {
		do_follow(ch, "me", 0, 0);
	    }
	    for (vict = world[ch->in_room].people; vict;
		 vict = vict->next_in_room) {
		if (ch == vict || !IS_MOB(vict)) {
		    continue;
		}
		if (MOB_FLAGGED(vict, MOB_LEADER)
		    && GET_RACE(ch) == GET_RACE(vict)) {
		    if (!number(0, 1)) {
			do_follow(ch, vict->player.name, 0, 0);
		    }
		}
	    }
	}

// Mob Movement
        move_mob(ch);


/* Hungry Wolves attack pigs && crabs */
	if (IS_NPC_WOLF(ch)) {
	    found = FALSE;
	    if (GET_MOB_HUNGER(ch) < 1) {
		for (vict = world[ch->in_room].people; vict && !found;
		     vict = vict->next_in_room) {
		    if (!IS_NPC(vict) || !CAN_SEE(ch, vict))
			continue;
		    if (GET_MOB_HUNGER(ch) < 6 && IS_NPC_LIVESTOCK(ch)) {
			hit(ch, vict, TYPE_UNDEFINED);
			found = TRUE;
		    }
		    if (GET_MOB_VNUM(vict) == 200) {
			hit(ch, vict, TYPE_UNDEFINED);
			found = TRUE;
		    }
		    if (IS_NPC_CRAB(ch) && number(0, 1)) {
			hit(ch, vict, TYPE_UNDEFINED);
			found = TRUE;
		    }
		}
	    }
	}

	/* Crabs attack piglets */
	if (IS_NPC_CRAB(ch)) {
	    found = FALSE;
	    if (GET_MOB_HUNGER(ch) < 10) {
		for (vict = world[ch->in_room].people; vict && !found;
		     vict = vict->next_in_room) {
		    if (!IS_NPC(vict) || !CAN_SEE(ch, vict))
			continue;
		    if (number(0, 1) && IS_NPC_LIVESTOCK(vict)
			&& vict->master) {
			hit(ch, vict, TYPE_UNDEFINED);
			found = TRUE;
		    }
		}
	    }
	}

	/* Skeletons attack pigs */
	if (IS_NPC_SKELETON(ch)) {
	    found = FALSE;
	    for (vict = world[ch->in_room].people; vict && !found;
		 vict = vict->next_in_room) {
		if (!IS_NPC(vict) || !CAN_SEE(ch, vict))
		    continue;
		if (number(0, 1) && IS_NPC_LIVESTOCK(vict) && vict->master) {
		    hit(ch, vict, TYPE_UNDEFINED);
		    found = TRUE;
		}
	    }
	}

	/* Aggressive Mobs */
	if (MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_TO_ALIGN)
	    || IS_NPC_GOBLIN(ch)) {
	    found = FALSE;
	    for (vict = world[ch->in_room].people; vict && !found;
		 vict = vict->next_in_room) {
		if (IS_NPC(vict) || !CAN_SEE(ch, vict)
		    || PRF_FLAGGED(vict, PRF_NOHASSLE))
		    continue;
		if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
		    continue;
		if (!MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN) ||
		    (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
		    (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict))
		    || (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict))) {
		    if (IS_NPC_GOBLIN(ch) && number(0, 1)) {
			ac = GET_AC(ch);
			if (IS_NPC(ch)) {
			    ac += GET_MOB_SIZE(ch);
			}

			ac += ability_modifer[(int) GET_DEX(ch)];
			hit(ch, vict, TYPE_UNDEFINED);
		    } else {
			hit(ch, vict, TYPE_UNDEFINED);
		    }
		    found = TRUE;
		}
	    }
	}

	/* Mob Memory */
	if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
	    found = FALSE;
	    for (vict = world[ch->in_room].people; vict && !found;
		 vict = vict->next_in_room) {
		if (IS_NPC(vict) || !CAN_SEE(ch, vict)
		    || PRF_FLAGGED(vict, PRF_NOHASSLE))
		    continue;
		for (names = MEMORY(ch); names && !found;
		     names = names->next) if (names->id == GET_IDNUM(vict)) {
			found = TRUE;
			act
			    ("'Hey!  You're the fiend that attacked me!!!', exclaims $n.",
			     FALSE, ch, 0, 0, TO_ROOM);
			hit(ch, vict, TYPE_UNDEFINED);
		    }
	    }
	}

	/* Helper Mobs */
	if (MOB_FLAGGED(ch, MOB_HELPER)
	    && !AFF_FLAGGED(ch, AFF_BLIND | AFF_CHARM)) {
	    found = FALSE;
	    for (vict = world[ch->in_room].people; vict && !found;
		 vict = vict->next_in_room) {
		if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
		    continue;
		if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
		    continue;

		act("$n jumps to the aid of $N!", FALSE, ch, 0, vict,
		    TO_ROOM);
		hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
		found = TRUE;
	    }
	}
	/* Run Away Mobs */
	if (MOB_FLAGGED(ch, MOB_RUN_AWAY)
	    && !AFF_FLAGGED(ch, AFF_BLIND | AFF_CHARM)) {
	    found = FALSE;
	    for (vict = world[ch->in_room].people; vict && !found;
		 vict = vict->next_in_room) {
		if (ch == vict || !FIGHTING(vict))
		    continue;
		if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
		    continue;
		do_flee(ch, NULL, 0, 0);
		found = TRUE;
	    }
	}
	if (GET_POS(ch) > POS_RESTING && !FIGHTING(ch)) {
	    /* SLEEPER MOBS */
	    if (MOB_FLAGGED(ch, MOB_SLEEPS) && number(0, 2)) {
		if (IS_AFFECTED(ch, AFF_INVISIBLE) && IS_DAY) {
                    if(IS_NPC_HIGHHUMAN(ch)) {
                      command_interpreter(ch, "open door");
                    }
		    REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
		    REMOVE_BIT(AFF_FLAGS(ch), AFF_SLEEP);
		    act(wake_msg[(int) GET_RACE(ch)], FALSE, ch, 0, 0,
			TO_ROOM);
		} else if (!IS_AFFECTED(ch, AFF_INVISIBLE) && IS_NIGHT) {
		    if(IS_NPC_HIGHHUMAN(ch)) {
                      command_interpreter(ch, "close door");
                    }
                    SET_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
		    SET_BIT(AFF_FLAGS(ch), AFF_SLEEP);
		    act(sleep_msg[(int) GET_RACE(ch)], FALSE, ch, 0, 0,
			TO_ROOM);
		}
	    }

	    /* NOCTURNAL MOBS */
	    if (MOB_FLAGGED(ch, MOB_NOCTURNAL) && number(0, 2)) {
		if (IS_AFFECTED(ch, AFF_INVISIBLE) && IS_NIGHT) {
		    REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
		    REMOVE_BIT(AFF_FLAGS(ch), AFF_SLEEP);
		    act(wake_msg[(int) GET_RACE(ch)], FALSE, ch, 0, 0,
			TO_ROOM);
		} else if (!IS_AFFECTED(ch, AFF_INVISIBLE) && IS_DAY) {
		    SET_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
		    SET_BIT(AFF_FLAGS(ch), AFF_SLEEP);
		    act(sleep_msg[(int) GET_RACE(ch)], FALSE, ch, 0, 0,
			TO_ROOM);
		}
	    }

	    /* HIBERNATE MOBS */
	    if (MOB_FLAGGED(ch, MOB_HIBERNATE) && number(0, 2)) {
		if (IS_AFFECTED(ch, AFF_INVISIBLE)
		    && (time_info.month >= 4 && time_info.month <= 13)) {
		    REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
		    REMOVE_BIT(AFF_FLAGS(ch), AFF_SLEEP);
		    act(hwake_msg[(int) GET_RACE(ch)], FALSE, ch, 0, 0,
			TO_ROOM);
		} else {
		    SET_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
		    SET_BIT(AFF_FLAGS(ch), AFF_SLEEP);
		    act(hsleep_msg[(int) GET_RACE(ch)], FALSE, ch, 0, 0,
			TO_ROOM);
		}
	    }


	    /* EMOTE MOBS */
	    if (!MOB_FLAGGED(ch, MOB_NOEMOTE) && !number(0, 30)
		&& !AFF_FLAGGED(ch, AFF_SLEEP)) {
		act(general_emote_msg[(int) GET_RACE(ch)][number(0, 3)],
		    FALSE, ch, 0, 0, TO_ROOM);
	    }
	}

/* Humans and Goblins should be aware of weapons and eq in their inv and pick
   the best one to use.
*/
// If I am not wielding a weapon, but I have one in inv, wield it.
	if ((IS_NPC_HIGHHUMAN(ch) || IS_NPC_GOBLIN(ch))
	    &&
	    (((ch)->mob_specials.newitemarmor != 0
	      || (ch)->mob_specials.newitem != 0)
	     && !(mob_index[GET_MOB_RNUM(ch)].func == shop_keeper))) {

	    if (!GET_EQ(ch, WEAR_WIELD)) {
		for (obj = ch->carrying; obj; obj = obj->next_content) {
		    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
			perform_wear(ch, obj, WEAR_WIELD);
		    }
		}
	    }
// If I don't have any eq on, but I have some in inv, wear it.
	    for (w = 0; w < NUM_WEARS; w++) {
		if (!GET_EQ(ch, w)) {
		    for (obj = ch->carrying; obj; obj = obj->next_content) {
			if (GET_OBJ_TYPE(obj) == ITEM_ARMOR
			    || GET_OBJ_TYPE(obj) == ITEM_WORN) {
			    if (find_eq_pos(ch, obj, 0) == w) {
				perform_wear(ch, obj, w);
			    }
			}
		    }
		}
	    }

// Check and see if the weapon I am using is better or worse than the one I picked up. Wield the best one.

	    if (((ch)->mob_specials.newitem != 0)
		&& !(mob_index[GET_MOB_RNUM(ch)].func == shop_keeper)) {
		if (GET_EQ(ch, WEAR_WIELD)) {
		    wield_obj = ch->equipment[WEAR_WIELD];
		    best_obj = wield_obj;
		    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			if (wield_obj->affected[i].location ==
			    APPLY_DAMROLL) a1 =
				wield_obj->affected[i].modifier;
		    }
		    max1 =
			(((GET_OBJ_VAL(wield_obj, 2) + a1 + 1) / 2.0) *
			 GET_OBJ_VAL(wield_obj, 1));
		} else {
		    wield_obj = NULL;
		    best_obj = NULL;
		    max1 = 1;
		}
		a1 = 0;
		a2 = 0;
		for (obj = ch->carrying; obj; obj = obj->next_content) {
		    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON
			&& !OBJ_FLAGGED(obj, ITEM_THROW)) {
			for (i = 0; i < MAX_OBJ_AFFECT; i++) {
			    if (obj->affected[i].location == APPLY_DAMROLL)
				a2 = obj->affected[i].modifier;
			}
			dam2 =
			    (((GET_OBJ_VAL(obj, 2) + a2 + 1) / 2.0) *
			     GET_OBJ_VAL(obj, 1));
			if (dam2 > max1) {
			    if (!invalid_align(ch, obj)) {
				best_obj = obj;
				for (i = 0; i < MAX_OBJ_AFFECT; i++) {
				    if (best_obj->affected[i].location ==
					APPLY_DAMROLL) a2 =
					    best_obj->affected[i].modifier;
				}
				max1 =
				    (((GET_OBJ_VAL
				       (best_obj,
					2) + a2 +
				       1) / 2.0) * GET_OBJ_VAL(best_obj,
							       1));}} else
			    (ch)->mob_specials.newitem = 0;
		    }
		}
		if (GET_EQ(ch, WEAR_WIELD)) {
		    if (wield_obj != best_obj) {
			perform_remove(ch, WEAR_WIELD);
			perform_wear(ch, best_obj, WEAR_WIELD);
			(ch)->mob_specials.newitem = 0;
		    }
		} else {
		    if (best_obj != NULL)
			perform_wear(ch, best_obj, WEAR_WIELD);
		    (ch)->mob_specials.newitem = 0;
		}
	    }
// Same as above, but for ARMOR
	    for (w = 0; w < NUM_WEARS; w++) {
		if (((ch)->mob_specials.newitemarmor != 0)
		    && !(mob_index[GET_MOB_RNUM(ch)].func == shop_keeper)) {
		    if (GET_EQ(ch, w)) {
			wield_obj = ch->equipment[w];
			best_obj = wield_obj;
			max1 = GET_OBJ_VAL(wield_obj, 0);
		    } else {
			wield_obj = NULL;
			best_obj = NULL;
			max1 = 1;
			continue;
		    }
		    for (obj = ch->carrying; obj; obj = obj->next_content) {
			if (
			    (GET_OBJ_TYPE(obj) == ITEM_ARMOR
			     || GET_OBJ_TYPE(obj) == ITEM_WORN)
			    && (find_eq_pos(ch, obj, 0) == w)) {

			    dam2 = GET_OBJ_VAL(obj, 0);
			    if (dam2 > max1) {
				if (!invalid_align(ch, obj)) {

				    best_obj = obj;
				    max1 = GET_OBJ_VAL(best_obj, 0) + a2;
				}
			    } else {

				(ch)->mob_specials.newitemarmor = 0;
			    }
			}
		    }
		    if (GET_EQ(ch, w)) {
			if (wield_obj != best_obj) {
			    perform_remove(ch, w);
			    perform_wear(ch, best_obj, w);
			    (ch)->mob_specials.newitemarmor = 0;
			}
		    } else {
			if (best_obj != NULL)
			    perform_wear(ch, best_obj, w);
			(ch)->mob_specials.newitemarmor = 0;
		    }
		}
	    }
	}
	if (IS_NPC_GOBLIN(ch)) {
	    // Goblins loot corpses.
	    for (obj = world[ch->in_room].contents; obj;
		 obj = obj->next_content) {
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER
		    && GET_OBJ_VAL(obj, 3) == 1) {
		    for (best_obj = obj->contains; best_obj != NULL;
			 best_obj = obj->next_content) {
			perform_get_from_container(ch, best_obj, obj, 0);
			break;
		    }

		}
	    }
	    // Goblins Hide and Sneak when possible.
	    if (!number(0, 5) && !AFF_FLAGGED(ch, AFF_INVISIBLE)) {
		SET_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
		act("$n disappears into a hiding place.", TRUE, ch, 0,
		    FALSE, TO_ROOM);
	    }
	    if (!number(0, 50) && AFF_FLAGGED(ch, AFF_INVISIBLE)) {
		REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
		act("$n steps out of $s hiding place.", TRUE, ch, 0, FALSE,
		    TO_ROOM);
	    }
	    // END OF HIDE
	}
	// END OF GOBLINS
	/* Add new mobile actions here */
	if (IS_NPC_LIVESTOCK(ch)) {
	    if (!number(0, 20) && ch->master) {
		r_num = real_object(89);
		obj = read_object(r_num, REAL);
		obj_to_room(obj, ch->in_room);
		act("With a grunt, $n poops.", TRUE, ch, 0, FALSE,
		    TO_ROOM);
	    }
	    if (!IS_NPC_PIG(ch)) {
		if (!number(0, 5)) {
		    if (SECT(ch->in_room) == SECT_CITY
			|| SECT(ch->in_room) == SECT_INSIDE
			|| SECT(ch->in_room) == SECT_BRIDGE
			|| SECT(ch->in_room) == SECT_ROAD) {
			break;
		    }
		    livestock_eat_from_room(ch);

		}
	    }
	}


    }				/* end for() */
}



/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
    memory_rec *tmp;
    bool present = FALSE;

    if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
	return;

    for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
	if (tmp->id == GET_IDNUM(victim))
	    present = TRUE;

    if (!present) {
	CREATE(tmp, memory_rec, 1);
	tmp->next = MEMORY(ch);
	tmp->id = GET_IDNUM(victim);
	MEMORY(ch) = tmp;
    }
}


/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
    memory_rec *curr, *prev = NULL;

    if (!(curr = MEMORY(ch)))
	return;

    while (curr && curr->id != GET_IDNUM(victim)) {
	prev = curr;
	curr = curr->next;
    }

    if (!curr)
	return;			/* person wasn't there at all. */

    if (curr == MEMORY(ch))
	MEMORY(ch) = curr->next;
    else
	prev->next = curr->next;

    free(curr);
}


/* erase ch's memory */
void clearMemory(struct char_data *ch)
{
    memory_rec *curr, *next;

    if (!IS_NPC(ch)) {
	return;
    }
    if (!MEMORY(ch)) {
	return;
    }

    curr = MEMORY(ch);

    while (curr) {
	next = curr->next;
	free(curr);
	curr = next;
    }

    MEMORY(ch) = NULL;
}

// This is so much easier to read.

void move_mob(struct char_data *ch) {
  int door = 0;

  if(MOB_FLAGGED(ch, MOB_SENTINEL) || GET_POS(ch) != POS_STANDING) {
    return;
  }

  door = number(0, 18);

  if(door >= NUM_OF_DIRS) { return; }
  if(!CAN_GO(ch, door)) { return; }

  if(ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB | ROOM_DEATH)) {
    return;
  }

  if(MOB_FLAGGED(ch, MOB_STAY_ZONE) && (world[EXIT(ch, door)->to_room].zone != world[ch->in_room].zone)) {
    return;
  }

  if(MOB_FLAGGED(ch, MOB_STAY_SECT) && (SECT(ch->in_room) != SECT(EXIT(ch, door)->to_room))) {
    return;
  }

  if ((SECT(EXIT(ch, door)->to_room) == SECT_CITY)
        && (GET_RACE(ch) == RACE_NPC_AVIAN
        || GET_RACE(ch) == RACE_NPC_MAMMAL
        || GET_RACE(ch) == RACE_NPC_INSECT
        || GET_RACE(ch) == RACE_NPC_CRAB
        || GET_RACE(ch) == RACE_NPC_WOLF)) {
        act("$n shies away from the city.", FALSE, ch, 0, 0,
             TO_ROOM);

  }  

  if((MOB_FLAGGED(ch, MOB_STAY_CITY)) && ((SECT(EXIT(ch, door)->to_room) != SECT_CITY) && (SECT(EXIT(ch, door)->to_room) != SECT_INSIDE) && (SECT(EXIT(ch, door)->to_room) != SECT_BRIDGE))) {
    return;
  }

  perform_move(ch, door, 1);

}
