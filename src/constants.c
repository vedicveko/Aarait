/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"	/* alias_data */

cpp_extern const char *circlemud_version =
    "Aarait - code version 1.0B - library version 1.0B";

/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */


/* cardinal directions */
const char *dirs[] = {
    "north",
    "east",
    "south",
    "west",
    "up",
    "down",
    "\n"
};


/* ROOM_x */
const char *room_bits[] = {
    "DARK",
    "DEATH",
    "!MOB",
    "INDOORS",
    "PEACEFUL",
    "SOUNDPROOF",
    "!TRACK",
    "!MAGIC",
    "TUNNEL",
    "PRIVATE",
    "GODROOM",
    "HOUSE",
    "HCRSH",
    "ATRIUM",
    "OLC",
    "*",			/* BFS MARK */
    "VIEW_DESC",
    "SALT_FISH",
    "NOVIEW",
    "FERRY",
    "\n"
};


/* EX_x */
const char *exit_bits[] = {
    "DOOR",
    "CLOSED",
    "LOCKED",
    "PICKPROOF",
    "\n"
};


/* SECT_ */
const char *sector_types[] = {
    "Inside",
    "City",
    "Field",
    "Forest",
    "Hills",
    "Mountains",
    "Water (Swim)",
    "Water (No Swim)",
    "Underwater",
    "In Flight",
    "Road",
    "Rock Mountain",
    "Snow Mountain",
    "Bridge",
    "Blank",
    "Ruins",
    "Jungle",
    "Swamp",
    "Lava",
    "Farm",
    "Acorn",
    "Empty",


    "\n"
};


/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const char *genders[] = {
    "Neutral",
    "Male",
    "Female",
    "\n"
};


/* POS_x */
const char *position_types[] = {
    "Dead",
    "Mortally wounded",
    "Incapacitated",
    "Stunned",
    "Sleeping",
    "Resting",
    "Sitting",
    "Fighting",
    "Standing",
    "\n"
};


/* PLR_x */
const char *player_bits[] = {
    "KILLER",
    "THIEF",
    "FROZEN",
    "DONTSET",
    "WRITING",
    "MAILING",
    "CSH",
    "SITEOK",
    "NOSHOUT",
    "NOTITLE",
    "DELETED",
    "LOADRM",
    "!WIZL",
    "!DEL",
    "INVST",
    "CRYO",
    "PIG_DONE",
    "GOB_DONE",
    "FISH_DONE",
    "QUESTOR",
    "BOT",
    "\n"
};


/* MOB_x */
const char *action_bits[] = {
    "SPEC",
    "SENTINEL",
    "SCAVENGER",
    "ISNPC",
    "AWARE",
    "AGGR",
    "STAY-ZONE",
    "WIMPY",
    "AGGR_EVIL",
    "AGGR_GOOD",
    "AGGR_NEUTRAL",
    "MEMORY",
    "HELPER",
    "HERD",
    "RUN_AWAY",
    "SLEEPS",
    "HIBERNATE",
    "NOEMOTE",
    "UNUSED1",
    "UNUSED2",
    "UNUSED3",
    "NOCTURNAL",
    "JOBMASTER",
    "STAY_CITY",
    "ACORN_EATER",
    "LEADER",
    "STAY_SECT",
    "MEMORY",
    "\n"
};


/* PRF_x */
const char *preference_bits[] = {
    "BRIEF",
    "COMPACT",
    "DEAF",
    "NOTELL",
    "DISPHP",
    "DISPMANA",
    "DISPMOVE",
    "AUTOEXIT",
    "NOHASSLE",
    "QUEST",
    "SUMMONABLE",
    "NOREPEAT",
    "HOLYLIGHT",
    "COLOR_1",
    "COLOR_2",
    "NOWIZ",
    "LOG1 ",
    "LOG2",
    "NOAUCT",
    "NOGOSS",
    "NOGRATZ",
    "ROOMFLAGS",
    "CLS",
    "AUTOLOOT",
    "TIPS",
    "DEBUG",
    "AUTOBAIT",
    "\n"
};


/* AFF_x */
const char *affected_bits[] = {
    "BLIND",
    "INVIS",
    "DET-ALIGN",
    "DET-INVIS",
    "DET-MAGIC",
    "SENSE-LIFE",
    "WATWALK",
    "SANCT",
    "GROUP",
    "CURSE",
    "INFRA",
    "POISON",
    "PROT-EVIL",
    "PROT-GOOD",
    "SLEEP",
    "!TRACK",
    "UNUSED",
    "UNUSED",
    "SNEAK",
    "HIDE",
    "UNUSED",
    "CHARM",
    "FISHING",
    "\n"
};


/* CON_x */
const char *connected_types[] = {
    "Playing",
    "Disconnecting",
    "Get name",
    "Confirm name",
    "Get password",
    "Get new PW",
    "Confirm new PW",
    "Select sex",
    "Select class",
    "Reading MOTD",
    "Main Menu",
    "Get descript.",
    "Changing PW 1",
    "Changing PW 2",
    "Changing PW 3",
    "Self-Delete 1",
    "Self-Delete 2",
    "Disconnecting",
    "Object edit",
    "Room edit",
    "Zone edit",
    "Mobile edit",
    "Shop edit",
    "Text edit",
    "Choose Ansi",
    "Assembly Edit",
    "QEDIT",
    "\n"
};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const char *where[] = {
    "<used as light>      ",
    "<worn on finger>     ",
    "<worn on finger>     ",
    "<worn around neck>   ",
    "<worn around neck>   ",
    "<worn on body>       ",
    "<worn on head>       ",
    "<worn on legs>       ",
    "<worn on feet>       ",
    "<worn on hands>      ",
    "<worn on arms>       ",
    "<worn as shield>     ",
    "<worn about body>    ",
    "<worn about waist>   ",
    "<worn around wrist>  ",
    "<worn around wrist>  ",
    "<wielded>            ",
    "<held>               ",
};


/* WEAR_x - for stat */
const char *equipment_types[] = {
    "Used as light",
    "Worn on right finger",
    "Worn on left finger",
    "First worn around Neck",
    "Second worn around Neck",
    "Worn on body",
    "Worn on head",
    "Worn on legs",
    "Worn on feet",
    "Worn on hands",
    "Worn on arms",
    "Worn as shield",
    "Worn about body",
    "Worn around waist",
    "Worn around right wrist",
    "Worn around left wrist",
    "Wielded",
    "Held",
    "\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[] = {
    "UNDEFINED",
    "LIGHT",
    "SCROLL",
    "WAND",
    "STAFF",
    "WEAPON",
    "FIRE WEAPON",
    "MISSILE",
    "TREASURE",
    "ARMOR",
    "POTION",
    "WORN",
    "OTHER",
    "TRASH",
    "TRAP",
    "CONTAINER",
    "NOTE",
    "LIQ CONTAINER",
    "KEY",
    "FOOD",
    "MONEY",
    "PEN",
    "BOAT",
    "FOUNTAIN",
    "FISH",
    "POLE",
    "BAIT",
    "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
    "TAKE",
    "FINGER",
    "NECK",
    "BODY",
    "HEAD",
    "LEGS",
    "FEET",
    "HANDS",
    "ARMS",
    "SHIELD",
    "ABOUT",
    "WAIST",
    "WRIST",
    "WIELD",
    "HOLD",
    "OFFHAND",
    "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = {
    "BODYPART",
    "THROW",
    "!RENT",
    "!DONATE",
    "!INVIS",
    "INVISIBLE",
    "MAGIC",
    "!DROP",
    "BLESS",
    "!GOOD",
    "!EVIL",
    "!NEUTRAL",
    "!PIG FARMER",
    "!GOBLIN",
    "!FISHERMAN",
    "UNUSED",
    "!SELL",
    "BOW",
    "CROSSBOW",
    "TWO_HANDED",
    "DECAYS",
    "\n"
};


/* APPLY_x */
const char *apply_types[] = {
    "NONE",
    "STR",
    "DEX",
    "INT",
    "WIS",
    "CON",
    "CHA",
    "CLASS",
    "LEVEL",
    "AGE",
    "CHAR_WEIGHT",
    "CHAR_HEIGHT",
    "MAXMANA",
    "MAXHIT",
    "MAXMOVE",
    "GOLD",
    "EXP",
    "ARMOR",
    "HITROLL",
    "DAMROLL",
    "SAVING_PARA",
    "SAVING_ROD",
    "SAVING_PETRI",
    "SAVING_BREATH",
    "SAVING_SPELL",
    "\n"
};


/* CONT_x */
const char *container_bits[] = {
    "CLOSEABLE",
    "PICKPROOF",
    "CLOSED",
    "LOCKED",
    "\n",
};


/* LIQ_x */
const char *drinks[] = {
    "water",
    "beer",
    "wine",
    "ale",
    "dark ale",
    "whisky",
    "lemonade",
    "firebreather",
    "local speciality",
    "slime mold juice",
    "milk",
    "tea",
    "coffee",
    "blood",
    "salt water",
    "clear water",
    "\n"
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] = {
    "water",
    "beer",
    "wine",
    "ale",
    "ale",
    "whisky",
    "lemonade",
    "firebreather",
    "local",
    "juice",
    "milk",
    "tea",
    "coffee",
    "blood",
    "salt",
    "water",
    "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
    {0, 1, 10},
    {3, 2, 5},
    {5, 2, 5},
    {2, 2, 5},
    {1, 2, 5},
    {6, 1, 4},
    {0, 1, 8},
    {10, 0, 0},
    {3, 3, 3},
    {0, 4, -8},
    {0, 3, 6},
    {0, 1, 6},
    {0, 1, 6},
    {0, 2, -1},
    {0, 1, -2},
    {0, 0, 13}
};


/* color of the various drinks */
const char *color_liquid[] = {
    "clear",
    "brown",
    "clear",
    "brown",
    "dark",
    "golden",
    "red",
    "green",
    "clear",
    "light green",
    "white",
    "brown",
    "black",
    "red",
    "clear",
    "crystal clear",
    "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] = {
    "less than half ",
    "about half ",
    "more than half ",
    ""
};


/* str, int, wis, dex, con applies **************************************/


/* [ch] strength apply (all) */
cpp_extern const struct str_app_type str_app[] = {
    {-5, -4, 0, 0},		/* str = 0 */
    {-5, -4, 3, 1},		/* str = 1 */
    {-3, -2, 3, 2},
    {-3, -1, 10, 3},
    {-2, -1, 25, 4},
    {-2, -1, 55, 5},		/* str = 5 */
    {-1, 0, 80, 6},
    {-1, 0, 90, 7},
    {0, 0, 100, 8},
    {0, 0, 100, 9},
    {0, 0, 115, 10},		/* str = 10 */
    {0, 0, 115, 11},
    {0, 0, 140, 12},
    {0, 0, 140, 13},
    {0, 0, 170, 14},
    {0, 0, 170, 15},		/* str = 15 */
    {0, 1, 195, 16},
    {1, 1, 220, 18},
    {1, 2, 255, 20},		/* str = 18 */
    {3, 7, 640, 40},
    {3, 8, 700, 40},		/* str = 20 */
    {4, 9, 810, 40},
    {4, 10, 970, 40},
    {5, 11, 1130, 40},
    {6, 12, 1440, 40},
    {7, 14, 1750, 40},		/* str = 25 */
    {1, 3, 280, 22},		/* str = 18/0 - 18-50 */
    {2, 3, 305, 24},		/* str = 18/51 - 18-75 */
    {2, 4, 330, 26},		/* str = 18/76 - 18-90 */
    {2, 5, 380, 28},		/* str = 18/91 - 18-99 */
    {3, 6, 480, 30}		/* str = 18/100 */
};



/* [dex] skill apply (thieves only) */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
    {-99, -99, -90, -99, -60},	/* dex = 0 */
    {-90, -90, -60, -90, -50},	/* dex = 1 */
    {-80, -80, -40, -80, -45},
    {-70, -70, -30, -70, -40},
    {-60, -60, -30, -60, -35},
    {-50, -50, -20, -50, -30},	/* dex = 5 */
    {-40, -40, -20, -40, -25},
    {-30, -30, -15, -30, -20},
    {-20, -20, -15, -20, -15},
    {-15, -10, -10, -20, -10},
    {-10, -5, -10, -15, -5},	/* dex = 10 */
    {-5, 0, -5, -10, 0},
    {0, 0, 0, -5, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},		/* dex = 15 */
    {0, 5, 0, 0, 0},
    {5, 10, 0, 5, 5},
    {10, 15, 5, 10, 10},	/* dex = 18 */
    {15, 20, 10, 15, 15},
    {15, 20, 10, 15, 15},	/* dex = 20 */
    {20, 25, 10, 15, 20},
    {20, 25, 15, 20, 20},
    {25, 25, 15, 20, 20},
    {25, 30, 15, 25, 25},
    {25, 30, 15, 25, 25}	/* dex = 25 */
};



/* [dex] apply (all) */
cpp_extern const struct dex_app_type dex_app[] = {
    {-7, -7, 6},		/* dex = 0 */
    {-6, -6, 5},		/* dex = 1 */
    {-4, -4, 5},
    {-3, -3, 4},
    {-2, -2, 3},
    {-1, -1, 2},		/* dex = 5 */
    {0, 0, 1},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},			/* dex = 10 */
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, -1},			/* dex = 15 */
    {1, 1, -2},
    {2, 2, -3},
    {2, 2, -4},			/* dex = 18 */
    {3, 3, -4},
    {3, 3, -4},			/* dex = 20 */
    {4, 4, -5},
    {4, 4, -5},
    {4, 4, -5},
    {5, 5, -6},
    {5, 5, -6}			/* dex = 25 */
};



/* [con] apply (all) */
cpp_extern const struct con_app_type con_app[] = {
    {-4, 20},			/* con = 0 */
    {-3, 25},			/* con = 1 */
    {-2, 30},
    {-2, 35},
    {-1, 40},
    {-1, 45},			/* con = 5 */
    {-1, 50},
    {0, 55},
    {0, 60},
    {0, 65},
    {0, 70},			/* con = 10 */
    {0, 75},
    {0, 80},
    {0, 85},
    {0, 88},
    {1, 90},			/* con = 15 */
    {2, 95},
    {2, 97},
    {3, 99},			/* con = 18 */
    {3, 99},
    {4, 99},			/* con = 20 */
    {5, 99},
    {5, 99},
    {5, 99},
    {6, 99},
    {6, 99}			/* con = 25 */
};



/* [int] apply (all) */
cpp_extern const struct int_app_type int_app[] = {
    {3},			/* int = 0 */
    {5},			/* int = 1 */
    {7},
    {8},
    {9},
    {10},			/* int = 5 */
    {11},
    {12},
    {13},
    {15},
    {17},			/* int = 10 */
    {19},
    {22},
    {25},
    {30},
    {35},			/* int = 15 */
    {40},
    {45},
    {50},			/* int = 18 */
    {53},
    {55},			/* int = 20 */
    {56},
    {57},
    {58},
    {59},
    {60}			/* int = 25 */
};


/* [wis] apply (all) */
cpp_extern const struct wis_app_type wis_app[] = {
    {0},			/* wis = 0 */
    {0},			/* wis = 1 */
    {0},
    {0},
    {0},
    {0},			/* wis = 5 */
    {0},
    {0},
    {0},
    {0},
    {0},			/* wis = 10 */
    {0},
    {2},
    {2},
    {3},
    {3},			/* wis = 15 */
    {3},
    {4},
    {5},			/* wis = 18 */
    {6},
    {6},			/* wis = 20 */
    {6},
    {6},
    {7},
    {7},
    {7}				/* wis = 25 */
};

const char *pc_race_types[] = {
    "Human",
    "\n"
};

const char *npc_class_types[] = {
    "Other",
    "Farmer",
    "Fisherman",
    "Pig Farmer",
    "Clergy",
    "Guard",
    "Miller",
    "Thief",
    "Merchant",
    "\n",
};

const char *npc_race_types[] = {
    "Other",
    "Chicken",
    "Avian",			// Bonus to DEX, FLY
    "Mammal",			// Bonus to INT
    "Sheep",			// WATERWALK
    "Goat",			// Bonus to STR, CON 
    "Insect",			// Bonus to CON, DEX, HELPER
    "Humanoid",			// Bonus to INT and WIS, MEMORY
    "Skeleton",			// AGGR, PROT_GOOD, Bonus to 
    "Cow",			// Bonus to all but DEX, NOBASH, AWARE, NOTRACK
    "Goblin",			// Bonus to DEX, wimp, SNEAK
    "Pig",			// Pig Farmer pigs
    "Wolf",			// Acorn Forest Wolves
    "Crab",			// Acorn Forest Crabs
    "Fish",			// Waterwalk
    "\n"
};



int rev_dir[] = {
    2,
    3,
    0,
    1,
    5,
    4
};


/* Not used in sprinttype(). */
const char *weekdays[] = {
    "the Day of the Moon",
    "the Day of the Bull",
    "the Day of the Deception",
    "the Day of Thunder",
    "the Day of Freedom",
    "the day of the Great Gods",
    "the Day of the Sun"
};


/* Not used in sprinttype(). */
const char *month_name[] = {
    "Month of Winter",		/* 0 */
    "Month of the Winter Wolf",
    "Month of the Frost Giant",
    "Month of the Old Forces",
    "Month of the Grand Struggle",
    "Month of the Spring",
    "Month of Nature",
    "Month of Futility",
    "Month of the Dragon",
    "Month of the Sun",
    "Month of the Heat",
    "Month of the Battle",
    "Month of the Dark Shades",
    "Month of the Shadows",
    "Month of the Long Shadows",
    "Month of the Ancient Darkness",
    "Month of the Great Evil"
};


/*
 * Definitions necessary for MobProg support in OasisOLC
 */
const char *mobprog_types[] = {
    "INFILE",
    "ACT",
    "SPEECH",
    "RAND",
    "FIGHT",
    "DEATH",
    "HITPRCNT",
    "ENTRY",
    "GREET",
    "ALL_GREET",
    "GIVE",
    "BRIBE",
    "\n"
};

const char *noble_houses[] = {
    "House de Tourneville",	// Biased to Goblin Slayers
    "House Veko",		// Biased to Fisherman 
    "House Esinon",		// Biased to Pig Farmers
    "House Fenix",		// Biased to Fisherman
    "House Kensington",		// Biased to Goblin Slayers
    "House Bennyson",		// Biased to Pig Farmers
    "\n"
};


const char *no_house_noble_houses[] = {
    "de Tourneville",		// Biased to Goblin Slayers
    "Veko",			// Biased to Fisherman 
    "Esinon",			// Biased to Pig Farmers
    "Fenix",			// Biased to Fisherman
    "Kensington",		// Biased to Goblin Slayers
    "Bennyson",			// Biased to Pig Farmers
    "\n"
};


const char *rank[][3] = {
    {"Serf", "Serf", "Serf"},
    {"Citizen", "Citizen", "Citizen"},
    {"Lord", "Lord", "Lady"},
    {"Baron", "Baron", "Baroness"},
    {"VisCount", "VisCount", "VisCountess"},
    {"Count", "Count", "Countess"},
    {"Earl", "Earl", "Earless"},
    {"Marquee", "Marquee", "Maquessa"},
    {"Duke", "Duke", "Duchess"},
    {"Prince", "Prince", "Princess"},
    {"Emperor", "Emperor", "Empress"},
    {"/cBGrand Poohbah/c0", "/cBGrand Poohbah/c0", "/cBGrand Poohbah/c0"},
    {"Dweeb", "Dweeb", "Dweebess"},
    {"Geek", "Geek", "Geekess"},
    {"Loser", "Loser", "Loser"}

};

const char *stat_names[] = {
    "NONE",
    "strength",
    "dexterity",
    "intelligence",
    "wisdom",
    "constitution",
    "charisma",
    "\n"
};

int house_load_rooms[] = {
    104,			// De Tourneville
    104,			// Veko
    104,			// Esinon
    104,			// Fenix
    104,			// Kensington
    104,			// Bennyson  


};

const char *skill_names[] = {
    "PLACE HOLDER",
    "Backstab", "Bash", "Hide", "Kick", "Pick Lock",
    "unused1", "Rescue", "Sneak", "Steal", "Track",
    "Scan", "Pig Farming", "Goblin Slaying", "Fishing",
    "Throw", "UNUSED2", "UNUSED3", "Ambush", "unused4",
    "Hit", "Sting",		// NO MORE ON THIS LINE
    "Whip", "Slash", "Bite", "Bludgeon", "Crush",
    "Pound", "Claw", "Maul", "Thrash", "Pierce",
    "Blast", "Punch", "Stab", "Cleave", "Block", "Parry",
    "Assemble", "Bake", "Brew", "Craft", "Fletch", "Knit",
    "Make", "Mix", "Thatch", "BLANK", "Weave", "Bows",
    "Cross Bows", "Repair", "Tame",
    "\n"
};

const int job_needs[][2] = {
// FORMAT: GOLD, POINTS
// GOBLIN SLAYERS
    {35000, 90},
// FISHERMAN
    {8000, 90},
// PIGFARMERS
    {10000, 90},
    {-1, -1}
};

/* Constants for Assemblies    *****************************************/
const char *AssemblyTypes[] = {
    "assemble",
    "bake",
    "brew",
    "craft",
    "fletch",
    "knit",
    "make",
    "mix",
    "thatch",
    "weave",
    "\n"
};

const char *jtypes[] = {
    "NULL",
    "Kill",
    "S OBJ",
    "G OBJ",
    "ESCORT",
    "F MOB",
    "GOFISH",
    "F OBJ",
    "\n"
};

const int ability_modifer[] = {
    -6, -5, -4, -4, -3, -3, -2, -2, -1, -1, 0, 0,	// 10, 11
    1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,	//26-27
    9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15,	//40-41
    15, 16, 16, 17, 17,		// 44- 45
    18, 18, 19, 19, 20, 20
};

const char *mob_sizes[] = {
    "COLOSSAL", "BUG", "BUG", "BUG",
    "GARGANTUAN", "BUG",
    "HUGE",
    "LARGE",
    "MEDIUM",
    "SMALL",
    "TINY", "BUG",
    "DIMINUTIVE", "BUG", "BUG", "BUG",
    "FINE",
    "\n"
};

const char *coin_types[] = {
    "copper",
    "silver",
    "gold",
    "platinum",
    "mixed"
};

const int fish_vnums[] = {
    300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311,
    312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323,
    324, 325, 326, 327, 328, 329, 330, 331, 332, 450, 500, 501,
    502, 503, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514,
    515, 516, 517, 518, 519, 520, 521, 522, 523, 524, 525, 526,
    527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538,
    1500, 1501, 1502, 1503, 1504, 1505, 1506, 1507, 1508, 1509,
    1510, 1511, 1512, 1513, 1514, 1515, 1516, 1517, 1518, 1519,
    1520, 1521, 1522, 1523, 1524, 1525
};

const char *fish_names[] = {
    "Channel Catfish", "Blue Catfish", "Bullhead Catfish",
    "Dace", "Spotted Gar", "Alligator Gar", "Stripper Bass",
    "Black Crappie", "Black Bass", "Loach", "Pike", "Golden Trout",
    "Red Crab", "Blue Crab", "Crawfish", "Clam", "Bitterling",
    "Rainbow Trout", "Crucian", "Pumpkinseed", "Bluegill", "Salmon",
    "Longnose Gar", "Carp", "Bigmouth Buffalo", "Smallmouth Buffalo",
    "Muskie", "Paddlefish", "Smelt", "Drum", "Green Turtle", "Eel",
    "Snake", "Bennyfish", "Char", "Three-Spined Stickleback", "Rudd",
    "Ayu", "Cutthroat Trout", "Gila Trout", "Brown Trout", "Darter",
    "Roughskin Sculpin", "Sculpin", "Shortnose Gar", "Bowfin", "Shad",
    "Brook Trout", "Redfin Pickerel", "Muskellunge", "Chain Pickerel",
    "White Catfish", "Flathead Catfish", "White Bass", "Yellow Bass",
    "Striped Bass", "Shadow Bass", "Rock Bass", "Flier",
	"Redbreast Sunfish",
    "Warmouth", "Longear Sunfish", "Redear Sunfish", "Spotted Sunfish",
    "Redeye Bass", "Smallmouth Bass", "Spotted Bass", "Largemouth Bass",
    "Yellow Perch", "Sauger", "Walleye", "White Crappie",
    "Stingray", "Blue Shark", "Bull Shark", "Reticulated Moray",
    "Leopard Toadfish", "Flounder", "Rock Sea Bass", "Samp Grouper",
    "Red Snapper", "Short Bigeye", "Yellow Jack", "Cigar Fish",
    "King Mackerel", "Dogfish", "Saltwater Bennyfish", "Bonefish",
    "Blue Marlin", "Sailfish", "Tuna", "Squid", "Giant Squid",
    "Cod", "Jellyfish", "Octopus", "Sea Dragon",
    "\n"
};


const char *livestock_afraid_vocals[] = {
    "BUG",
    "clucks",
    "BUG",
    "BUG",
    "baaas",
    "bahbaaahs",
    "BUG",
    "BUG",
    "Bug",
    "mooos",
    "Bug",
    "squeals",
    "Bug",
    "Bug",
    "Bug",
    "\n"
};

const char *items_to_eat[] = {
    "a twig",
    "some grass",
    "some leaves",
    "some long brown grass",
    "some moss",
    "a leafy twig",
    "some purple grass",
    "some sort of fungus",
    "a green bush",
    "a wild flower",
    "some tree bark",
    "a leaf",
    "some berries",
    "\n"
};

