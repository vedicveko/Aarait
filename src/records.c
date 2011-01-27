// The new records system.
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

void display_records(int class);
struct record_data *records = NULL;

// new do records, take argument for each class record or displays them all.
ACMD(do_trecords) {
   char arg[255]; 

   one_argument(argument, arg);

   if(is_abbrev(arg, "fisherman")) {
      display_records(CLASS_FISHERMAN);
      return;
   }
   else if(is_abbrev(arg, "goblin")) {
      display_records(CLASS_GOBLIN_SLAYER);
      return;
   }
   else if(is_abbrev(arg, "pig")) {
      display_records(CLASS_PIG_FARMER);
      return;
   }
   else if(is_abbrev(arg, "misc")) {
      display_records(CLASS_JOBLESS);
      return;
   }
   else {
      send_to_char
        ("Please specify the type of record you would like to see.\r\n", ch);
      send_to_char
        ("Your choices are: FISHERMAN, GOBLIN, PIG, and MISC.\r\n", ch); 
      return;
  }
}


// boot_records, reads the records from file, sets up memory.

void boot_records() {

  CREATE(records, struct record_data, 1); //NUMBER OF RECORDS);

}

// save_records saves all of the records.
// This also fixes up the webpage includes. Take class as arg

// display_records_class one function to display records for each class.
// takes class as arg
void display_records(int class) {
   switch(class) {
     case CLASS_FISHERMAN:

     return;

     default:
      log("Got to default case in display records");
      return;
   }

}

// update_record, takes character, value and records number as argument.

