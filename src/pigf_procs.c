// Part of Aarait.

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
#include "coins.h"

/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct spell_info_type spell_info[];
extern int guild_info[][3];
extern struct char_data *character_list;

/* extern functions */
void add_follower(struct char_data *ch, struct char_data *leader);
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);
ACMD(do_tell);
void stop_follower(struct char_data *ch);
ACMD(do_follow);
void fighting_injuries(struct char_data *ch);

void wolf_hunger()
{
    struct char_data *ch;
    struct char_data *next_ch;

    // Let's Use GET_MOB_HUNGER for hunger as a cop out.
    for (ch = character_list; ch; ch = next_ch) {
	next_ch = ch->next;
       
	if (!GET_MOB_HUNGER(ch)) {
	    continue;
	}

	GET_MOB_HUNGER(ch)--;
	if (!GET_MOB_HUNGER(ch)) {
	    continue;
	}
	GET_MOB_HUNGER(ch)--;

	// if a wolf gets really hungry and is following a player, it should stop following
	if (IS_NPC_WOLF(ch) && GET_MOB_HUNGER(ch) < 6 && ch->master) {
	    stop_follower(ch);

	}

    }
}

void acorn_trees()
{

    int r, obj_num, znum = world[real_room(201)].zone, DROP = 0;
    struct obj_data *acorn;

    for (r = 200; r < 297; r++) {
	if (!number(0, 200) && real_room(r)
	    && SECT(real_room(r)) == SECT_FOREST) {

	    obj_num = real_object(200);
	    acorn = read_object(obj_num, REAL);
	    obj_to_room(acorn, real_room(r));
	    DROP = 1;
	}

    }

    if (DROP) {
	send_to_zone_outdoor(znum,
			     "In the distance, you hear an acorn fall to the ground with a rustle of leaves.\r\n");

    }
    DROP = 0;

    for (r = 700; r < 799; r++) {
	if (!number(0, 400) && real_room(r)
	    && SECT(real_room(r)) == SECT_FOREST) {

	    obj_num = real_object(200);
	    acorn = read_object(obj_num, REAL);
	    obj_to_room(acorn, real_room(r));
	    DROP = 1;
	    znum = world[real_room(700)].zone;
	}

    }

    if (DROP) {
	send_to_zone_outdoor(znum,
			     "In the distance, you hear an acorn fall to the ground with a rustle of leaves.\r\n");

    }
    DROP = 0;

    for (r = 800; r < 811; r++) {
	if (!number(0, 400) && real_room(r)
	    && SECT(real_room(r)) == SECT_FOREST) {


	    obj_num = real_object(200);
	    acorn = read_object(obj_num, REAL);
	    obj_to_room(acorn, real_room(r));
	    DROP = 1;
	    znum = world[real_room(800)].zone;
	}

    }
    if (DROP) {
	send_to_zone_outdoor(znum,
			     "In the distance, you hear an acorn fall to the ground with a rustle of leaves.\r\n");

    }

    for (r = 530; r < 586; r++) {
	if (!number(0, 300) && real_room(r)) {

	    obj_num = real_object(599);
	    acorn = read_object(obj_num, REAL);
	    obj_to_room(acorn, real_room(r));
	}

    }

}

void load_acorn(int rnum, int how_many)
{

    int r, obj_num, znum, n, DROP = 0;
    struct obj_data *acorn;

    znum = world[real_room(rnum)].zone;

    for (n = 0; n <= how_many; n++) {
	r = number(rnum, (rnum + 99));
	if (real_room(r)) {

	    obj_num = real_object(200);
	    acorn = read_object(obj_num, REAL);
	    obj_to_room(acorn, real_room(r));
	    DROP = 1;
	}

    }

    if (DROP) {
	send_to_zone_outdoor(znum,
			     "In the distance, you hear an acorn fall to the ground with a rustle of leaves.\r\n");
    }

    obj_num = real_object(599);
    acorn = read_object(obj_num, REAL);
    obj_to_room(acorn, real_room(220));

}

const char *allyton_indoor_day_messages[][4] = {
// SKY_CLOUDLESS

    {"You hear muffled speech from outside.",
     "A rat screeches from the inside of a wall.",
     "You hear someone shouting outside.",
     "You hear a clanking noise."},
// SKY_CLOUDY 

    {"The room becomes dimmer as clouds obscure the sun.",
     "The room becomes brighter as the clouds move away from the sun.",
     "The room becomes dimmer as clouds obscure the sun.",
     "The room becomes brighter as the clouds move away from the sun."},
// SKY_RAINING

    {"Rain makes a soft pitter, patter noise on the roof.",
     "Rain makes a loud thumping noise on the roof.",
     "You can hear rain falling off of the buildings gutter on onto the ground.",
     "Rain cascades off of the building."},
// SKY_LIGHTNING

    {"Thunder booms in the distance.",
     "A thunder clap causes the building to vibrate.",
     "Thunder booms in the distance.",
     "A thunder clap causes the building to vibrate."},

};
const char *allyton_outdoor_day_messages[][4] = {
// SKY_CLOUDLESS

    {"The annoying sound of a dog's incessant barking can be heard nearby.",
     "Someone swears loudly a few streets over.",
     "The sounds of laughter can be heard from the next building.",
     "You hear a pig oink."},
// SKY_CLOUDY 

    {"The moving clouds obscure the sun.",
     "The cloudy sky causes dappled shadows to appear.",
     "The cloudy sky begins to clear.",
     "A child can be heard shouting in the distance."},
// SKY_RAINING

    {"The rain begins forming small puddles in the street.",
     "Rains falls in a steady cascade from a nearby building.",
     "The rain lightens up a bit.", "It begins raining harder."},
// SKY_LIGHTNING

    {"Lightning flashes near Benny Mountain.",
     "A clap of thunder causes the air to vibrate.",
     "Thunder booms in the distance.",
     "Lightning flashes to the south."},
};
const char *allyton_indoor_night_messages[][4] = {
// SKY_CLOUDLESS

    {"A cricket chirps.", "A cricket chirps.", "A cricket chirps.",
     "A cricket chirps."},
// SKY_CLOUDY 

    {"A cricket chirps.", "A cricket chirps.", "A cricket chirps.",
     "A cricket chirps."},
// SKY_RAINING

    {"Rain makes a soft pitter, patter noise on the roof.",
     "Rain makes a loud thumping noise on the roof.",
     "You can hear rain falling off of the building's gutter on onto the ground.",
     "Rain makes a soft pitter, patter noise on the roof."},
// SKY_LIGHTNING

    {"Thunder booms in the distance.",
     "A thunder clap causes the building to vibrate.",
     "Thunder booms in the distance.",
     "A lightning strike causes the building to light up for a brief secound."},
};
const char *allyton_outdoor_night_messages[][4] = {
// SKY_CLOUDLESS

    {"You hear a cricket chirp.",
     "You hear the droning of night insects.", "You hear a cricket chirp.",
     "You hear the droning of night insects."},
// SKY_CLOUDY 

    {"You hear a cricket chirp.",
     "You hear the droning of night insects.", "You hear a cricket chirp.",
     "You hear the droning of night insects."},
// SKY_RAINING

    {"The rain drenches you.",
     "The rain makes soft pitter patter noises striking the ground.",
     "The rain falls from the sky in sheets.",
     "Rain falls off of the tip of your nose."},
// SKY_LIGHTNING

    {"Thunder booms in the distance.",
     "A thunder clap causes the building to vibrate.",
     "Thunder booms in the distance.",
     "A lightning strike causes your surroundings to light up for a brief secound."},
};

const char *creaknlake_outdoor_day_messages[][4] = {

    {"The field grass sways in the wind.",
     "A grasshopper jumps around wildly.",
     "A turkey gobbles in the distance.",
     "The field grass sways gently in the wind."},
    {"The field grass sways in the wind.",
     "A grasshopper jumps around wildly.",
     "A turkey gobbles in the distance.",
     "The field grass sways gently in the wind."},
    {"The rain drenches you.",
     "The rain makes soft pitter patter noises striking the ground.",
     "Rain falls from the sky in sheets.",
     "Rain weighs down the field grass."},
    {"Thunder booms in the distance.",
     "Lightening strikes the east peak of Benny Mountain",
     "Thunder booms in the distance.",
     "A lightning strike causes your surroundings to painfully brighten for a secound."},

};
void zone_emotes(void)
{
    int allyton = world[real_room(104)].zone;
//  int bennymountain = world[real_room(400)].zone;
//  int acornforest = world[real_room(202)].zone;
    int creaknlake = world[real_room(303)].zone;

    // ALLYTON
    if (allyton != 0) {
	if (IS_DAY) {
	    sprintf(buf, "%s\r\n",
		    allyton_outdoor_day_messages[weather_info.sky][number
								   (0,
								    3)]);
	    send_to_zone_outdoor(allyton, buf);
	    sprintf(buf, "%s\r\n",
		    allyton_indoor_day_messages[weather_info.sky][number
								  (0, 3)]);
	    send_to_zone_indoor(allyton, buf);
	} else {
	    sprintf(buf, "%s\r\n",
		    allyton_outdoor_night_messages[weather_info.sky][number
								     (0,
								      3)]);
	    send_to_zone_outdoor(allyton, buf);
	    sprintf(buf, "%s\r\n",
		    allyton_indoor_night_messages[weather_info.sky][number
								    (0,
								     3)]);
	    send_to_zone_indoor(allyton, buf);

	}
    }
    // END ALLYTON
/*
   BENNY MOUNTAIN
  if (bennymountain != 0) {
    if(IS_DAY) {
       sprintf(buf, "%s\r\n", bennymountain_outdoor_day_messages[weather_info.sky][number(0, 3)]);
       send_to_zone_outdoor(bennymountain, buf);
       sprintf(buf, "%s\r\n", bennymountain_indoor_day_messages[weather_info.sky][number(0, 3)]);
       send_to_zone_indoor(bennymountain, buf);
     }
    else {
       sprintf(buf, "%s\r\n", bennymountain_outdoor_night_messages[weather_info.sky][number(0, 3)]);
       send_to_zone_outdoor(bennymountain, buf);
       sprintf(buf, "%s\r\n", bennymountain_indoor_night_messages[weather_info.sky][number(0, 3)]);
       send_to_zone_indoor(bennymountain, buf);

    }
 
  }
   END BENNY MOUNTAIN
*/
    // CREAK N LAKE 
    if (creaknlake != 0) {
	if (IS_DAY) {
	    sprintf(buf, "%s\r\n",
		    creaknlake_outdoor_day_messages[weather_info.sky]
		    [number(0, 3)]);
	    send_to_zone_outdoor(creaknlake, buf);
	}
/*    else {
       sprintf(buf, "%s\r\n", creaknlake_outdoor_night_messages[weather_info.sky][number(0, 3)]);
       send_to_zone_outdoor(creaknlake, buf);

    } */

    }
//   END CREAKNLAKE
/*   ACORN FOREST
  if (acornforest != 0) {
    if(IS_DAY) {
       sprintf(buf, "%s\r\n", acornforest_outdoor_day_messages[weather_info.sky][number(0, 3)]);
       send_to_zone_outdoor(acornforest, buf);
     }
   else {
       sprintf(buf, "%s\r\n", acornforest_outdoor_night_messages[weather_info.sky][number(0, 3)]);
       send_to_zone_outdoor(acornforest, buf);

    }

  } */
    //  END ACORN FOREST

}

#define PIG_PRICE(m) (GET_MOB_WEIGHT(m) * 50)
SPECIAL(pig_trader)
{

    char buy_pig_name[MAX_INPUT_LENGTH];
    struct char_data *buy_pig = NULL;
    int buy_pig_room = 0;
    struct obj_data *obj;
    struct char_data *stable_pig = NULL;
    int r_num, found = 0;
    struct char_data *butcher = (struct char_data *) me;
    struct follow_type *f, *next_fol;

    if (!cmd)
	return FALSE;

    if (GET_ROOM_VNUM(ch->in_room) == 159) {
	buy_pig_room = real_room(160);
    } else if (GET_ROOM_VNUM(ch->in_room) == 1897) {
	buy_pig_room = real_room(1999);
    }

    if (CMD_IS("list")) {
	act("$n asks the stable man for a price list.", FALSE, ch, 0, 0,
	    TO_ROOM);
	send_to_char("I have the following livestock for sale.\r\n", ch);

	for (buy_pig = world[buy_pig_room].people; buy_pig;
	     buy_pig = buy_pig->next_in_room) {
	    sprintf(buf, "%3d Copper - %s\r\n", PIG_PRICE(buy_pig),
		    GET_NAME(buy_pig));
	    send_to_char(buf, ch);
	}
	return TRUE;

    } else if (CMD_IS("buy")) {

	argument = one_argument(argument, buf);
	argument = one_argument(argument, buy_pig_name);

	if (!(buy_pig = get_char_room(buf, buy_pig_room))) {
	    sprintf(buf, "%s, I do not have that livestock.",
		    GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}
	if (convert_all_to_copper(ch) < PIG_PRICE(buy_pig)) {
	    sprintf(buf, "%s, You do not have enough money.",
		    GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	for (f = ch->followers; f; f = next_fol) {
	    next_fol = f->next;
	    if (IS_NPC(f->follower) && IS_NPC_PIG(f->follower)
		&& IS_NPC_PIG(buy_pig)) {
		found = 1;
	    }
	}

	if (found) {
	    sprintf(buf, "But, you already own a pig %s!", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	found = 0;

	act("$n buys $N.", FALSE, ch, 0, buy_pig, TO_ROOM);
	send_to_char("The stable man takes your money.\r\n", ch);

	make_change(ch, PIG_PRICE(buy_pig), BUYING);

	buy_pig = read_mobile(GET_MOB_RNUM(buy_pig), REAL);
	GET_EXP(buy_pig) = 0;

	char_to_room(buy_pig, ch->in_room);
	do_follow(buy_pig, GET_NAME(ch), 0, 0);

	IS_CARRYING_W(buy_pig) = 1000;
	IS_CARRYING_N(buy_pig) = 100;
	GET_MOB_HUNGER(buy_pig) = 1;

	sprintf(buf, "%s, The butcher is always open!", GET_NAME(ch));
	mobsay(butcher, buf);
	if (IS_NPC_PIG(ch)) {
	    load_acorn(200, 5);
	}

	return TRUE;
    } else if (CMD_IS("stable")) {
	act("$n tries to stable some livestock.", FALSE, ch, 0, 0,
	    TO_ROOM);

	for (f = ch->followers; f; f = next_fol) {
	    next_fol = f->next;
	    if (IS_NPC_PIG(f->follower)) {
		found = 1;
		break;
	    }
	}

	if (!found) {
	    sprintf(buf, "You don't have any livestock here, %s",
		    GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (!f->follower) {
	    log("No f->follower in butcher stable");
	    send_to_char("Bug, please report.\r\n", ch);
	    return TRUE;
	}

	found = 0;

	r_num = real_object(1260);	// token of stable_pig recovery
	obj = read_object(r_num, REAL);

	if (!obj) {
	    log("No obj in butcher stable");
	    send_to_char("Bug, please report.\r\n", ch);
	    return TRUE;
	}

	GET_OBJ_VAL(obj, 1) = GET_MOB_VNUM(f->follower);
	GET_OBJ_VAL(obj, 0) = GET_MOB_WEIGHT(f->follower);

	obj_to_char(obj, ch);

	send_to_char
	    ("The stable man gives you a shiny gold token.  DON'T LOSE IT.\r\n",
	     ch);
	act("The stable man gives $n something.", FALSE, ch, 0, 0,
	    TO_ROOM);
	act("The stable man takes $n away to a nice safe place.", FALSE,
	    f->follower, 0, 0, TO_ROOM);
	extract_char(f->follower);
	return 1;


    } else if (CMD_IS("redeem")) {
	act("$n tries to redeem a token.", FALSE, ch, 0, 0, TO_ROOM);
	if (!
	    (obj =
	     get_obj_in_list_vis(ch, "stable_pig_token", ch->carrying))) {
	    sprintf(buf,
		    "%s, You don't seem to have any tokens to claim your livestock.",
		    GET_NAME(ch));
	    mobsay(butcher, buf);
	    return 1;
	}

	if (convert_all_to_copper(ch) < 25) {
	    sprintf(buf, "Redeeming your token will cost 25 coins %s.",
		    GET_NAME(ch));
	    mobsay(butcher, buf);
	    mobsay(butcher, "You can not afford it!");
	    return TRUE;
	}

	r_num = GET_OBJ_VAL(obj, 1);


	if (!r_num) {
	    send_to_char("Ack, It's a bug squish it!\r\n", ch);
	    return 1;
	}
	stable_pig = read_mobile(r_num, VIRTUAL);

	GET_MOB_WEIGHT(stable_pig) = GET_OBJ_VAL(obj, 0);
	GET_MOB_HUNGER(stable_pig) = 1;
	send_to_room("The stable man takes 25 coins for his services.\r\n",
		     ch->in_room);
	send_to_char("The stable man takes back the shiny gold token.\r\n",
		     ch);
	act("The stable man takes back a token from $n.", FALSE, ch, 0, 0,
	    TO_ROOM);
	obj_from_char(obj);

	send_to_char("The stable man returns your pig back to you.\r\n",
		     ch);
	act("The stable man returns $n's livestock.", FALSE, ch, 0, 0,
	    TO_ROOM);
	GET_EXP(stable_pig) = 0;
	make_change(ch, 25, BUYING);
	char_to_room(stable_pig, ch->in_room);
	do_follow(stable_pig, GET_NAME(ch), 0, 0);
	if (IS_NPC_PIG(stable_pig)) {
	    load_acorn(200, 5);
	}
	return 1;
    } else {
	return FALSE;
    }

}

SPECIAL(butcher)
{
    int found = 0, ammount = 0, percent = 0;
    struct char_data *vict;
    struct char_data *butcher = (struct char_data *) me;

    if (CMD_IS("sell")) {
	one_argument(argument, arg);
	vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);

	act("$n tries to sell a pig", FALSE, ch, 0, 0, TO_ROOM);

	if (!vict) {
	    sprintf(buf, "Sell who, %s?", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (!IS_NPC_PIG(vict)) {
	    sprintf(buf, "That's not a pig, %s!", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (GET_MAX_HIT(vict) > 0)
	    percent = (100 * GET_HIT(vict)) / GET_MAX_HIT(vict);
	else
	    percent = -1;	/* How could MAX_HIT be < 1?? */

	if (percent < 75) {
	    sprintf(buf, "That pig is all bruised up %s!", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return 1;
	}

	ammount = GET_MOB_WEIGHT(vict) * 10;

	if (GET_MOB_WEIGHT(vict) < 5) {
	    sprintf(buf, "That pig is just to small for me to buy %s",
		    GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (GET_MOB_WEIGHT(vict) > 51 && !found) {
	    sprintf(buf,
		    "Now, that's a big fat pig, %s. I'll give you %d coins for him!",
		    GET_NAME(ch), ammount);
	    found = 1;
	} else if (GET_MOB_WEIGHT(vict) > 25 && !found) {
	    sprintf(buf,
		    "That's a nice sized pig, %s. I'll give you %d coins for him.",
		    GET_NAME(ch), ammount);
	    found = 1;
	} else if (GET_MOB_WEIGHT(vict) > 10 && !found) {
	    sprintf(buf,
		    "He's a wee bit on the scrawny side, %s. I'll give you %d coins for him.",
		    GET_NAME(ch), ammount);
	    found = 1;
	}

	else {
	    sprintf(buf,
		    "My %s, that's a small one. Here are %d coins for your trouble.",
		    GET_NAME(ch), ammount);
	    found = 1;
	}

	mobsay(butcher, buf);

	make_change(ch, ammount, SELLING);

	if (IS_PIG_FARMER(ch)) {
	    improve_skill(ch, SKILL_PIG_FARMING, 5);
	    improve_points(ch, 5);
	}

	GET_POUNDS_OF_PIG(ch) += GET_MOB_WEIGHT(vict);
	if (GET_MOB_WEIGHT(vict) > GET_PIG_WEIGHT(ch)) {
	    GET_PIG_WEIGHT(ch) = GET_MOB_WEIGHT(vict);
	}

	stop_follower(vict);
	extract_char(vict);

	GET_PIGS_RAISED(ch)++;

	return TRUE;
    }
    return FALSE;
}

SPECIAL(livestock_butcher)
{
    int ammount = 0, percent = 0, multi = 0;
    struct char_data *vict = NULL;
    struct char_data *butcher = (struct char_data *) me;

    switch (GET_RACE(ch)) {
    case RACE_NPC_CHICKEN:
	multi = 50;
	break;
    case RACE_NPC_COW:
	multi = 30;
	break;
    case RACE_NPC_GOAT:
    case RACE_NPC_SHEEP:
	multi = 35;
	break;
    default:
	multi = 1;
	break;
    }

    ammount = GET_MOB_WEIGHT(vict) * multi;

    if (CMD_IS("value")) {

	one_argument(argument, arg);
	vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);

	act("$n tries to value some livestock", FALSE, ch, 0, 0, TO_ROOM);

	if (!vict) {
	    sprintf(buf, "Value what, %s?", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (!IS_NPC_LIVESTOCK(vict)) {
	    sprintf(buf, "That's not livestock, %s!", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (IS_NPC_PIG(vict)) {
	    sprintf(buf,
		    "You'll get more money for that pig at the pig market,%s.",
		    GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	sprintf(buf, "I'll give you %d coins for him!", ammount);

	mobsay(butcher, buf);
	return TRUE;

    }


    if (CMD_IS("sell")) {
	one_argument(argument, arg);
	vict = get_char_vis(ch, arg, FIND_CHAR_ROOM);

	act("$n tries to sell some livestock", FALSE, ch, 0, 0, TO_ROOM);

	if (!vict) {
	    sprintf(buf, "Sell who, %s?", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (!IS_NPC_LIVESTOCK(vict)) {
	    sprintf(buf, "That's not livestock, %s!", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (IS_NPC_PIG(vict)) {
	    sprintf(buf,
		    "You'll get more money for that pig at the pig market, %s.",
		    GET_NAME(ch));
	    mobsay(butcher, buf);
	    return TRUE;
	}

	if (GET_MAX_HIT(vict) > 0)
	    percent = (100 * GET_HIT(vict)) / GET_MAX_HIT(vict);
	else
	    percent = -1;	/* How could MAX_HIT be < 1?? */

	if (percent < 75) {
	    sprintf(buf, "I don't buy injured animals, %s!", GET_NAME(ch));
	    mobsay(butcher, buf);
	    return 1;
	}

	sprintf(buf, "I'll give you %d coins for him!", ammount);

	mobsay(butcher, buf);

	make_change(ch, ammount, SELLING);

	act("The butcher takes $N from you.", FALSE, ch, 0, vict, TO_CHAR);
	act("The butcher takes $N from $n.", FALSE, ch, 0, vict, TO_ROOM);

	stop_follower(vict);
	extract_char(vict);

	return TRUE;
    }
    return FALSE;
}
