// Aarait 20010920
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "coins.h"

void convert_copper(struct char_data *ch, int coppers)
{
    int plat, silver, gold;

    plat = (coppers / 1000);
    coppers -= (plat * 1000);
    add_money_to_char(ch, plat, PLAT_COINS);

    gold = coppers / 100;
    coppers -= (gold * 100);
    add_money_to_char(ch, gold, GOLD_COINS);

    silver = coppers / 10;
    coppers -= (silver * 10);
    add_money_to_char(ch, silver, SILVER_COINS);

    add_money_to_char(ch, coppers, COPPER_COINS);

}

int convert_to_copper(int amount, int type)
{
    if (type == COPPER_COINS)
	amount = amount;
    else if (type == SILVER_COINS)
	amount *= COPPER_PER_SILVER;
    else if (type == GOLD_COINS)
	amount *= COPPER_PER_GOLD;
    else if (type == PLAT_COINS)
	amount *= COPPER_PER_PLAT;

    return (amount);
}

char list_from_copper(int amount, char *cost, int style)
{
    int plat = 0, gold = 0, silver = 0, copper = 0, remainder, is_prev =
	FALSE;

    remainder = amount;

    if (remainder >= COPPER_PER_PLAT) {
	plat = (remainder / COPPER_PER_PLAT);
	remainder = (remainder % COPPER_PER_PLAT);
	if (style != CONDENSED_LIST)
	    sprintf(cost, "%d platinum", plat);
	else
	    sprintf(cost, "%dP", plat);
	is_prev = TRUE;
    }
    if (plat == 0 && style == CONDENSED_LIST)
	sprintf(cost, "0P");
    if (remainder >= COPPER_PER_GOLD) {
	gold = (remainder / COPPER_PER_GOLD);
	remainder = (remainder % COPPER_PER_GOLD);
	if ((is_prev == TRUE) && (remainder == 0)
	    && style != CONDENSED_LIST)
	    sprintf(cost + strlen(cost), " and %d gold", gold);
	else if (is_prev == TRUE && style != CONDENSED_LIST)
	    sprintf(cost + strlen(cost), ", %d gold", gold);
	else if (style != CONDENSED_LIST) {
	    sprintf(cost, "%d gold", gold);
	    is_prev = TRUE;
	} else
	    sprintf(cost + strlen(cost), " %dG", gold);
    }
    if (gold == 0 && style == CONDENSED_LIST)
	sprintf(cost + strlen(cost), " 0G");
    if (remainder >= COPPER_PER_SILVER) {
	silver = (remainder / COPPER_PER_SILVER);
	remainder = (remainder % COPPER_PER_SILVER);
	if ((is_prev == TRUE) && (remainder == 0)
	    && style != CONDENSED_LIST)
	    sprintf(cost + strlen(cost), " and %d silver", silver);
	else if (is_prev == TRUE && style != CONDENSED_LIST)
	    sprintf(cost + strlen(cost), ", %d silver", silver);
	else if (style != CONDENSED_LIST) {
	    sprintf(cost, "%d silver", silver);
	    is_prev = TRUE;
	} else
	    sprintf(cost + strlen(cost), " %dS", silver);
    }
    if (silver == 0 && style == CONDENSED_LIST)
	sprintf(cost + strlen(cost), " 0S");
    if (remainder > 0) {
	copper = remainder;
	if (is_prev == TRUE && style != CONDENSED_LIST)
	    sprintf(cost + strlen(cost), " and %d copper", copper);
	else if (style != CONDENSED_LIST)
	    sprintf(cost, "%d copper", copper);
	else
	    sprintf(cost + strlen(cost), " %dC", copper);
    }
    if (copper == 0 && style == CONDENSED_LIST)
	sprintf(cost + strlen(cost), " 0G");

    return (*cost);
}

int convert_all_to_copper(struct char_data *ch)
{
    int amount;

    amount =
	((GET_MONEY(ch, PLAT_COINS) * COPPER_PER_PLAT) +
	 (GET_MONEY(ch, GOLD_COINS) * COPPER_PER_GOLD) +
	 (GET_MONEY(ch, SILVER_COINS) * COPPER_PER_SILVER) + GET_MONEY(ch,
								       COPPER_COINS));

    return (amount);
}


int get_gold_weight(int amount, int type)
{
    int weight;

    if (type == SILVER_COINS)
	weight = (amount * 3);
    else if (type == GOLD_COINS)
	weight = (amount * 2);
    else if (type == PLAT_COINS)
	weight = (amount * 1);
    else
	weight = (amount * 4);

    return (weight);
}

void add_money_to_char(struct char_data *ch, int amount, int type)
{
    if (amount == 0)
	return;
    else if (amount < 0) {
        sprintf(buf, "%s: [Negative amount passed to remove money.. shifting to add]", GET_NAME(ch));
	log(buf);
	remove_money_from_char(ch, (amount * -1), type);
	return;
    }
    GET_MONEY(ch, type) += amount;

//    sprintf(buf, "Adding money to %s, %d %s coins weight %d", GET_NAME(ch),
//	    amount, coin_types[type], get_gold_weight(amount, type));
//    log(buf);

    return;
}

void remove_money_from_char(struct char_data *ch, int amount, int type)
{
    if (amount == 0)
	return;
    else if (amount < 0) {
        sprintf(buf, "%s :[Negative amount passed to remove money.. shifting to add]", GET_NAME(ch));
	log(buf);
	add_money_to_char(ch, (amount * -1), type);
	return;
    }
    GET_MONEY(ch, type) -= amount;

//    sprintf(buf, "Removing money from %s, %d %s coins, weight %d",
//	    GET_NAME(ch), amount, coin_types[type], get_gold_weight(amount,
//								    type));
  //  log(buf);


    return;
}


void conv_money(struct char_data *ch, int fromtype, int totype)
{
    if (fromtype > 3 || totype > 3) {
	send_to_char("Fubar...", ch);
	log("SYSERR: conv_money().  Invalid coin type to convert");
	return;
    }

    if (totype < fromtype) {	/* ie., 1 plat into 10 gold */
	if (GET_MONEY(ch, fromtype) < 1) {	/* We don't have any money to convert! */
	    switch (fromtype) {
	    case COPPER_COINS:	/* Okay, we're trying to convert copper to silver, but we don't have copper */
		send_to_char
		    ("Oh goodness, how embarassing, you don't seem to have enough money.\r\n",
		     ch);
		return;
		break;
	    case SILVER_COINS:	/* Silver to gold, but don't have silver */
		conv_money(ch, COPPER_COINS, SILVER_COINS);
		break;
	    case GOLD_COINS:	/* Gold to plat, but don't have gold */
		conv_money(ch, SILVER_COINS, GOLD_COINS);
		break;
	    default:
                sprintf(buf, "SYSERR: Convert money() %s %d", GET_NAME(ch),
                        fromtype);
		log(buf);
		break;
	    }
	}
	remove_money_from_char(ch, 1, fromtype);
	add_money_to_char(ch, (MONEY_CONV_FACTOR ^ (fromtype - totype)),
			  totype);
    } else {			/* ie., 10 gold into 1 plat */
	if (GET_MONEY(ch, totype) <
	    (MONEY_CONV_FACTOR ^ (totype - fromtype))) {
	    switch (fromtype) {
	    case PLAT_COINS:	/* We can't possibly convert platinum into something bigger... */
		send_to_char("Er, what?\r\n", ch);
		return;
		break;
	    case GOLD_COINS:	/* Trying to make gold into platinum, but no gold... lets grab some silver */
		conv_money(ch, SILVER_COINS, GOLD_COINS);
		break;
	    case SILVER_COINS:
		conv_money(ch, COPPER_COINS, SILVER_COINS);
		break;
	    default:
		log("SYSERR: Convert money() 2.  Not sure how we got here, but can't be good...");
		break;
	    }
	}
	remove_money_from_char(ch,
			       (MONEY_CONV_FACTOR ^ (totype - fromtype)),
			       fromtype);
	add_money_to_char(ch, 1, totype);
    }
}

/* cost must be in the form of coppers */
void make_change(struct char_data *ch, int cost, int type)
{
    int remainder;

    if (cost < 0) {
	log("SYSERR: Invalid cost sent to makechange");
	return;
    } else if (cost == 0)
	return;


    if (type != BUYING && type != SELLING) {
	log("SYSERR: Invalid type sent to makechange");
	return;
    }

    if (type == BUYING) {
	if (cost > GET_MONEY(ch, COPPER_COINS)) {	/* Don't have enough coppers, use what we have and try silver */
	    cost -= GET_MONEY(ch, COPPER_COINS);
	    remove_money_from_char(ch, GET_MONEY(ch, COPPER_COINS),
				   COPPER_COINS);

	    if (cost > (GET_MONEY(ch, SILVER_COINS) * 10)) {	/* Don't have enough silver use it and try gold */
		cost -= (GET_MONEY(ch, SILVER_COINS) * 10);
		remove_money_from_char(ch, GET_MONEY(ch, SILVER_COINS),
				       SILVER_COINS);

		if (cost > (GET_MONEY(ch, GOLD_COINS) * COPPER_PER_GOLD)) {	/* Don't have enough gold use it and try plat */
		    cost -= (GET_MONEY(ch, GOLD_COINS) * COPPER_PER_GOLD);
		    remove_money_from_char(ch, GET_MONEY(ch, GOLD_COINS),
					   GOLD_COINS);
		    while (cost >
			   (GET_MONEY(ch, GOLD_COINS) * COPPER_PER_GOLD)) {
			conv_money(ch, PLAT_COINS, GOLD_COINS);
			while (cost > COPPER_PER_GOLD
			       && (GET_MONEY(ch, GOLD_COINS) > 0)) {
			    remove_money_from_char(ch, 1, GOLD_COINS);
			    cost -= COPPER_PER_GOLD;
			}
		    }
		    while (cost >
			   (GET_MONEY(ch, SILVER_COINS) *
			    COPPER_PER_SILVER)) {
			conv_money(ch, GOLD_COINS, SILVER_COINS);
			while (cost > COPPER_PER_SILVER
			       && (GET_MONEY(ch, SILVER_COINS) > 0)) {
			    remove_money_from_char(ch, 1, SILVER_COINS);
			    cost -= COPPER_PER_SILVER;
			}
		    }
		    while (cost > GET_MONEY(ch, COPPER_COINS)) {
			conv_money(ch, SILVER_COINS, COPPER_COINS);
			while (cost > 0
			       && (GET_MONEY(ch, COPPER_COINS) > 0)) {
			    remove_money_from_char(ch, 1, COPPER_COINS);
			    cost -= 1;
			}
		    }
		} else {	/* Using Gold to Pay */
		    while (cost >
			   (GET_MONEY(ch, SILVER_COINS) *
			    COPPER_PER_SILVER)) {
			conv_money(ch, GOLD_COINS, SILVER_COINS);
			while (cost > COPPER_PER_SILVER
			       && (GET_MONEY(ch, SILVER_COINS) > 0)) {
			    remove_money_from_char(ch, 1, SILVER_COINS);
			    cost -= COPPER_PER_SILVER;
			}
		    }
		    while (cost > GET_MONEY(ch, COPPER_COINS)) {
			conv_money(ch, SILVER_COINS, COPPER_COINS);
			while (cost > 0
			       && (GET_MONEY(ch, COPPER_COINS) > 0)) {
			    remove_money_from_char(ch, 1, COPPER_COINS);
			    cost -= 1;
			}
		    }
		}
	    } else {		/* Using silvers to pay... */
		while (cost > GET_MONEY(ch, COPPER_COINS)) {
		    conv_money(ch, SILVER_COINS, COPPER_COINS);
		    while (cost > 0 && (GET_MONEY(ch, COPPER_COINS) > 0)) {
			remove_money_from_char(ch, 1, COPPER_COINS);
			cost -= 1;
		    }
		}
	    }
	} else
	    GET_MONEY(ch, COPPER_COINS) -= cost;
    } else {
	remainder = cost;
	if (remainder >= 1000) {
	    add_money_to_char(ch, (remainder / COPPER_PER_PLAT),
			      PLAT_COINS);
	    remainder = (remainder % COPPER_PER_PLAT);
	}
	if (remainder >= 100) {
	    add_money_to_char(ch, (remainder / COPPER_PER_GOLD),
			      GOLD_COINS);
	    remainder = (remainder % COPPER_PER_GOLD);
	}
	if (remainder >= 10) {
	    add_money_to_char(ch, (remainder / COPPER_PER_SILVER),
			      SILVER_COINS);
	    remainder = (remainder % COPPER_PER_SILVER);
	}
	if (remainder > 0) {
	    add_money_to_char(ch, remainder, COPPER_COINS);
	}
    }
    return;
}
