// Aarait 20010920

extern const char *coin_types[];

/* Public money functions in utils.c*/
void convert_copper(struct char_data *ch, int coppers);
int convert_to_copper(int amount, int type);
int convert_all_to_copper(struct char_data *ch);
void make_change(struct char_data *ch, int cost, int type);
char list_from_copper(int amount, char *cost, int style);
void add_money_to_char(struct char_data *ch, int amount, int type);
void remove_money_from_char(struct char_data *ch, int amount, int type);

/* money handling definitions */
#define BUYING 2
#define SELLING 1
#define CONDENSED_LIST 0
#define UNCONDENSED_LIST 1


/* Constants for coin types */
/* Note: These are set up to mirror the obj_val of the ITEM_MONEY */

#define COPPER_COINS 0
#define SILVER_COINS 1
#define GOLD_COINS 2
#define PLAT_COINS 3

#define MIXED_COINS 4   /* This is used for corpses creating money with multiple coin types in it. */

/* Money conversion Utilities */
#define MONEY_CONV_FACTOR  10

#define COPPER_PER_SILVER (MONEY_CONV_FACTOR)
#define SILVER_PER_GOLD 	(MONEY_CONV_FACTOR)
#define COPPER_PER_GOLD 	(COPPER_PER_SILVER * SILVER_PER_GOLD)
#define GOLD_PER_PLAT 		(MONEY_CONV_FACTOR)
#define SILVER_PER_PLAT		(SILVER_PER_GOLD * GOLD_PER_PLAT)
#define COPPER_PER_PLAT		(COPPER_PER_SILVER * SILVER_PER_PLAT)


