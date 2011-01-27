/************************************************************************
 * Generic OLC Library - Shops / genshp.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genshp.h"
#include "genzon.h"

extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct shop_data *shop_index;
extern struct zone_data *zone_table;
extern int top_shop;
extern zone_rnum top_of_zone_table;

/*-------------------------------------------------------------------*/

void copy_shop(struct shop_data *tshop, struct shop_data *fshop)
{
  /*
   * Copy basic information over.
   */
  S_NUM(tshop) = S_NUM(fshop);
  S_KEEPER(tshop) = S_KEEPER(fshop);
  S_OPEN1(tshop) = S_OPEN1(fshop);
  S_CLOSE1(tshop) = S_CLOSE1(fshop);
  S_OPEN2(tshop) = S_OPEN2(fshop);
  S_CLOSE2(tshop) = S_CLOSE2(fshop);
  S_BANK(tshop) = S_BANK(fshop);
  S_BROKE_TEMPER(tshop) = S_BROKE_TEMPER(fshop);
  S_BITVECTOR(tshop) = S_BITVECTOR(fshop);
  S_NOTRADE(tshop) = S_NOTRADE(fshop);
  S_SORT(tshop) = S_SORT(fshop);
  S_BUYPROFIT(tshop) = S_BUYPROFIT(fshop);
  S_SELLPROFIT(tshop) = S_SELLPROFIT(fshop);
  S_FUNC(tshop) = S_FUNC(fshop);

  /*
   * Copy lists over.
   */
  copy_list(&(S_ROOMS(tshop)), S_ROOMS(fshop));
  copy_list(&(S_PRODUCTS(tshop)), S_PRODUCTS(fshop));
  copy_type_list(&(tshop->type), fshop->type);

  /*
   * Copy notification strings over.
   */
  free_shop_strings(tshop);
  S_NOITEM1(tshop) = str_dup(S_NOITEM1(fshop));
  S_NOITEM2(tshop) = str_dup(S_NOITEM2(fshop));
  S_NOCASH1(tshop) = str_dup(S_NOCASH1(fshop));
  S_NOCASH2(tshop) = str_dup(S_NOCASH2(fshop));
  S_NOBUY(tshop) = str_dup(S_NOBUY(fshop));
  S_BUY(tshop) = str_dup(S_BUY(fshop));
  S_SELL(tshop) = str_dup(S_SELL(fshop));
}

/*-------------------------------------------------------------------*/

/*
 * Copy a -1 terminated integer array list.
 */
void copy_list(sh_int **tlist, sh_int *flist)
{
  int num_items, i;

  if (*tlist)
    free(*tlist);

  /*
   * Count number of entries.
   */
  for (i = 0; flist[i] != -1; i++);
  num_items = i + 1;

  /*
   * Make space for entries.
   */
  CREATE(*tlist, sh_int, num_items);

  /*
   * Copy entries over.
   */
  for (i = 0; i < num_items; i++)
    (*tlist)[i] = flist[i];
}

/*-------------------------------------------------------------------*/

/*
 * Copy a -1 terminated (in the type field) shop_buy_data 
 * array list.
 */
void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist)
{
  int num_items, i;

  if (*tlist)
    free_type_list(tlist);

  /*
   * Count number of entries.
   */
  for (i = 0; BUY_TYPE(flist[i]) != -1; i++);
  num_items = i + 1;

  /*
   * Make space for entries.
   */
  CREATE(*tlist, struct shop_buy_data, num_items);

  /*
   * Copy entries over.
   */
  for (i = 0; i < num_items; i++) {
    (*tlist)[i].type = flist[i].type;
    if (BUY_WORD(flist[i]))
      BUY_WORD((*tlist)[i]) = str_dup(BUY_WORD(flist[i]));
  }
}

/*-------------------------------------------------------------------*/

void remove_from_type_list(struct shop_buy_data **list, int num)
{
  int i, num_items;
  struct shop_buy_data *nlist;

  /*
   * Count number of entries.
   */
  for (i = 0; (*list)[i].type != -1; i++);

  if (num < 0 || num >= i)
    return;
  num_items = i;

  CREATE(nlist, struct shop_buy_data, num_items);

  for (i = 0; i < num_items; i++)
    nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

  free(BUY_WORD((*list)[num]));
  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

void add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *newl)
{
  int i, num_items;
  struct shop_buy_data *nlist;

  /*
   * Count number of entries.
   */
  for (i = 0; (*list)[i].type != -1; i++);
  num_items = i;

  /*
   * Make a new list and slot in the new entry.
   */
  CREATE(nlist, struct shop_buy_data, num_items + 2);

  for (i = 0; i < num_items; i++)
    nlist[i] = (*list)[i];
  nlist[num_items] = *newl;
  nlist[num_items + 1].type = -1;

  /*
   * Out with the old, in with the new.
   */
  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

void add_to_int_list(sh_int **list, sh_int newi)
{
  sh_int i, num_items, *nlist;

  /*
   * Count number of entries.
   */
  for (i = 0; (*list)[i] != -1; i++);
  num_items = i;

  /*
   * Make a new list and slot in the new entry.
   */
  CREATE(nlist, sh_int, num_items + 2);

  for (i = 0; i < num_items; i++)
    nlist[i] = (*list)[i];
  nlist[num_items] = newi;
  nlist[num_items + 1] = -1;

  /*
   * Out with the old, in with the new.
   */
  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

void remove_from_int_list(sh_int **list, sh_int num)
{
  sh_int i, num_items, *nlist;

  /*
   * Count number of entries.
   */
  for (i = 0; (*list)[i] != -1; i++);

  if (num >= i || num < 0)
    return;
  num_items = i;

  CREATE(nlist, sh_int, num_items);

  for (i = 0; i < num_items; i++)
    nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

/*
 * Free all the notice character strings in a shop structure.
 */
void free_shop_strings(struct shop_data *shop)
{
  if (S_NOITEM1(shop)) {
    free(S_NOITEM1(shop));
    S_NOITEM1(shop) = NULL;
  }
  if (S_NOITEM2(shop)) {
    free(S_NOITEM2(shop));
    S_NOITEM2(shop) = NULL;
  }
  if (S_NOCASH1(shop)) {
    free(S_NOCASH1(shop));
    S_NOCASH1(shop) = NULL;
  }
  if (S_NOCASH2(shop)) {
    free(S_NOCASH2(shop));
    S_NOCASH2(shop) = NULL;
  }
  if (S_NOBUY(shop)) {
    free(S_NOBUY(shop));
    S_NOBUY(shop) = NULL;
  }
  if (S_BUY(shop)) {
    free(S_BUY(shop));
    S_BUY(shop) = NULL;
  }
  if (S_SELL(shop)) {
    free(S_SELL(shop));
    S_SELL(shop) = NULL;
  }
}

/*-------------------------------------------------------------------*/

/*
 * Free a type list and all the strings it contains.
 */
void free_type_list(struct shop_buy_data **list)
{
  int i;

  for (i = 0; (*list)[i].type != -1; i++)
    if (BUY_WORD((*list)[i]))
      free(BUY_WORD((*list)[i]));

  free(*list);
  *list = NULL;
}

/*-------------------------------------------------------------------*/

/*
 * Free up the whole shop structure and it's content.
 */
void free_shop(struct shop_data *shop)
{
  free_shop_strings(shop);
  free_type_list(&(S_NAMELISTS(shop)));
  free(S_ROOMS(shop));
  free(S_PRODUCTS(shop));
  free(shop);
}

/*-------------------------------------------------------------------*/

/*
 * Ew, linear search, O(n)
 */
int real_shop(int vshop_num)
{
  int rshop_num;

  for (rshop_num = 0; rshop_num <= top_shop - top_shop_offset; rshop_num++)
    if (SHOP_NUM(rshop_num) == vshop_num)
      return rshop_num;

  return NOWHERE;
}

/*-------------------------------------------------------------------*/

/*
 * Generic string modifier for shop keeper messages.
 */
void modify_string(char **str, char *new_s)
{
  char *pointer;

  /*
   * Check the '%s' is present, if not, add it.
   */
  if (*new_s != '%') {
    sprintf(buf, "%%s %s", new_s);
    pointer = buf;
  } else
    pointer = new_s;

  if (*str)
    free(*str);
  *str = str_dup(pointer);
}

/*-------------------------------------------------------------------*/

int add_shop(struct shop_data *nshp)
{
  shop_rnum rshop;
  int found = 0;
  zone_rnum rznum = real_zone_by_thing(S_NUM(nshp));

  /*
   * The shop already exists, just update it.
   */
  if ((rshop = real_shop(S_NUM(nshp))) != NOWHERE) {
    copy_shop(&shop_index[rshop], nshp);
    if (rznum != NOWHERE)
      add_to_save_list(zone_table[rznum].number, SL_SHP);
    else
      mudlog("SYSERR: GenOLC: Cannot determine shop zone.", BRF, LVL_BUILDER, TRUE);
    return rshop;
  }

  top_shop++;
  RECREATE(shop_index, struct shop_data, top_shop - top_shop_offset + 1);

  for (rshop = top_shop - top_shop_offset; rshop > 0; rshop--) {
    if (nshp->vnum > SHOP_NUM(rshop - 1)) {
      found = rshop;

      /* Make a "nofree" variant and remove these later. */
      shop_index[rshop].in_room = NULL;
      shop_index[rshop].producing = NULL;
      shop_index[rshop].type = NULL;

      copy_shop(&shop_index[rshop], nshp);
      break;
    }
    shop_index[rshop] = shop_index[rshop - 1];
  }

  if (!found) {
    /* Make a "nofree" variant and remove these later. */
    shop_index[rshop].in_room = NULL;
    shop_index[rshop].producing = NULL;
    shop_index[rshop].type = NULL;

    copy_shop(&shop_index[0], nshp);
  }

  if (rznum != NOWHERE)
    add_to_save_list(zone_table[rznum].number, SL_SHP);
  else
    mudlog("SYSERR: GenOLC: Cannot determine shop zone.", BRF, LVL_BUILDER, TRUE);

  return rshop;
}

/*-------------------------------------------------------------------*/

int save_shops(zone_rnum zone_num)
{
  int i, j, rshop, vzone, top;
  FILE *shop_file;
  char fname[64];
  struct shop_data *shop;

  if (zone_num < 0 || zone_num > top_of_zone_table) {
    log("SYSERR: GenOLC: save_mobiles: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
    return FALSE;
  }

  vzone = zone_table[zone_num].number;
  top = zone_table[zone_num].top;

  sprintf(fname, "%s/%d.new", SHP_PREFIX, vzone);
  if (!(shop_file = fopen(fname, "w"))) {
    mudlog("SYSERR: OLC: Cannot open shop file!", BRF, LVL_GOD, TRUE);
    return FALSE;
  } else if (fprintf(shop_file, "CircleMUD v3.0 Shop File~\n") < 0) {
    mudlog("SYSERR: OLC: Cannot write to shop file!", BRF, LVL_GOD, TRUE);
    fclose(shop_file);
    return FALSE;
  }
  /*
   * Search database for shops in this zone.
   */
  for (i = vzone * 100; i <= top; i++) {
    if ((rshop = real_shop(i)) != -1) {
      fprintf(shop_file, "#%d~\n", i);
      shop = shop_index + rshop;

      /*
       * Save the products.
       */
      for (j = 0; S_PRODUCT(shop, j) != -1; j++)
	fprintf(shop_file, "%d\n", obj_index[S_PRODUCT(shop, j)].vnum);

      /*
       * Save the rates.
       */
      fprintf(shop_file, "-1\n%1.2f\n%1.2f\n", S_BUYPROFIT(shop), S_SELLPROFIT(shop));

      /*
       * Save the buy types and namelists.
       */
      j = -1;
      do {
	j++;
	fprintf(shop_file, "%d%s\n", S_BUYTYPE(shop, j),
		S_BUYWORD(shop, j) ? S_BUYWORD(shop, j) : "");
      } while (S_BUYTYPE(shop, j) != -1);

      /*
       * Save messages'n'stuff.
       * Added some small'n'silly defaults as sanity checks.
       */
      fprintf(shop_file,
	      "%s~\n%s~\n%s~\n%s~\n%s~\n%s~\n%s~\n"
	      "%d\n%ld\n%d\n%d\n",
	      S_NOITEM1(shop) ? S_NOITEM1(shop) : "%s Ke?!",
	      S_NOITEM2(shop) ? S_NOITEM2(shop) : "%s Ke?!",
	      S_NOBUY(shop) ? S_NOBUY(shop) : "%s Ke?!",
	      S_NOCASH1(shop) ? S_NOCASH1(shop) : "%s Ke?!",
	      S_NOCASH2(shop) ? S_NOCASH2(shop) : "%s Ke?!",
	      S_BUY(shop) ? S_BUY(shop) : "%s Ke?! %d?",
	      S_SELL(shop) ? S_SELL(shop) : "%s Ke?! %d?",
	      S_BROKE_TEMPER(shop),
	      S_BITVECTOR(shop),
	      mob_index[S_KEEPER(shop)].vnum,
	      S_NOTRADE(shop)
	      );

      /*
       * Save the rooms.
       */
      j = -1;
      do {
	j++;
	fprintf(shop_file, "%d\n", S_ROOM(shop, j));
      } while (S_ROOM(shop, j) != -1);

      /*
       * Save open/closing times 
       */
      fprintf(shop_file, "%d\n%d\n%d\n%d\n", S_OPEN1(shop), S_CLOSE1(shop),
		S_OPEN2(shop), S_CLOSE2(shop));
    }
  }
  fprintf(shop_file, "$~\n");
  fclose(shop_file);
  sprintf(buf2, "%s/%d.shp", SHP_PREFIX, vzone);
  remove(buf2);
  rename(fname, buf2);

  remove_from_save_list(vzone, SL_SHP);
  return TRUE;
}
