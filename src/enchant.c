/* NetHack 3.7	enchant.c	$NHDT-Date: $  $NHDT-Branch: $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/*-Copyright (c) Thierry Lach, 2022. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static boolean u_can_enchant(void);
static int enchant(void);
static int enchant_ok(struct obj*);

#define NECOMP 6

/* Enchantment components */
#define NCHCMPT(c1, c2, c3, c4, c5, c6)   \
    {                                   \
        c1, c2, c3, c4, c5, c6          \
    }

#define NCH(v1, v2, v3, v4) {v1,(v2),v3,v4}
#define NO_NCH    {0,0,0}                               /* no component */
#define NCHITEM(v1, v2)    ( v1, v2, 0 }                /* item component */
#define NCHCORPSE(v1)  { CORPSE, BUC_ALLBKNOWN, v1 }      /* corpse component */

/* Enchantment component */
struct enchcomp {
    short obj;                              /* object to use */
    int flags;                              /* blessed/cursed flags */
    short corpse;                           /* type of corpse */
};

/* enchantment information (from, to, min level, components) */
struct enchinfo {
    short obj;                              /* object to enchant */
    short result;                           /* object to */
    short minlvl;                           /* minimum experience level to enchant */
    struct enchcomp complist[NECOMP];       /* required components - include the base object */
};
static struct enchinfo enchantable[] = {
                    NCH(OILSKIN_SACK, BAG_OF_HOLDING, 10,
                        NCHCMPT(NCHITEM(OILSKIN_SACK, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NO_NCH, NO_NCH, NO_NCH, NO_NCH)),
                    NCH(SACK, BAG_OF_HOLDING, 12,
                        NCHCMPT(NCHITEM(SACK, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NO_NCH, NO_NCH, NO_NCH, NO_NCH)),
                    NCH(LARGE_BOX, ICE_BOX, 16,
                        NCHCMPT(NCHITEM(SACK, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(GLOB_OF_BROWN_PUDDING, BUC_UNCURSED | BUC_BLESSED), NO_NCH, NO_NCH, NO_NCH)),
                    NCH(CHEST, ICE_BOX, 13,
                        NCHCMPT(NCHITEM(SACK, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(GLOB_OF_BROWN_PUDDING, BUC_UNCURSED | BUC_BLESSED), NO_NCH, NO_NCH, NO_NCH)),
                    NCH(TIN_WHISTLE, MAGIC_WHISTLE, 12,
                        NCHCMPT(NCHITEM(TIN_WHISTLE, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHCORPSE(PM_RUST_MONSTER), NO_NCH, NO_NCH, NO_NCH)),
                    NCH(BRASS_LANTERN, MAGIC_LAMP, 10,
                        NCHCMPT(NCHITEM(BRASS_LANTERN, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHCORPSE(PM_RED_MOLD, BUC_UNCURSED | BUC_BLESSED), NO_NCH, NO_NCH, NO_NCH)),
                    NCH(OIL_LAMP, MAGIC_LAMP, 12,
                        NCHCMPT(NCHITEM(OIL_LAMP, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHCORPSE(PM_RED_MOLD, BUC_UNCURSED | BUC_BLESSED), NO_NCH, NO_NCH, NO_NCH)),
                    NCH(WOODEN_FLUTE, MAGIC_FLUTE, 12,
                        NCHCMPT(NCHITEM(WOODEN_FLUTE, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHCORPSE(PM_SHRIEKER), NO_NCH, NO_NCH, NO_NCH)),
                    NCH(WOODEN_HARP, MAGIC_HARP, 14,
                        NCHCMPT(NCHITEM(WOODEN_HARP, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NO_NCH, NO_NCH, NO_NCH, NO_NCH)),
                    NCH(LEATHER_DRUM, DRUM_OF_EARTHQUAKE, 18,
                        NCHCMPT(NCHITEM(LEATHER_DRUM, BUC_UNCURSED | BUC_BLESSED), NCHITEM(POT_WATER, BUC_BLESSED), NO_NCH, NO_NCH, NO_NCH, NO_NCH)),
                    NCH(0, 0, 0,
                        NCHCMPT(NO_NCH, NO_NCH, NO_NCH, NO_NCH, NO_NCH, NO_NCH))
};

/* can hero enchant at all */
static boolean
u_can_enchant(void)
{
    if (u.uswallow) {
        pline("You cannot move enough to enchant!");
        return FALSE;
    }
    if (nohands(g.youmonst.data)) {
        You_cant("even hold anything!");
        return FALSE;
    }

    int intell = ACURR(A_INT);
    if (intell < 17) {
        You("are not smart enough!");
        return FALSE;
    }
    if (u.ulevel < 10) {
        You("are not experienced enough!");
        return FALSE;
    }
    if (check_capacity((char *) 0))
        return FALSE;

    // TODO: Are you on a co-aligned altar?

    // TODO: Is your god pleased with you?

    return TRUE;
}

/* getobj callback for object to enchant */
static int
enchant_ok(struct obj* obj)
{
    switch (obj->otyp) {
    case OILSKIN_SACK: // BAG_OF_HOLDING
    case SACK: // BAG_OF_HOLDING
    case TIN_WHISTLE: // MAGIC_WHISTLE
    case BRASS_LANTERN: // MAGIC_LAMP
    case OIL_LAMP: // MAGIC_LAMP
    case WOODEN_FLUTE: // MAGIC_FLUTE:
    case WOODEN_HARP: // MAGIC_HARP:
    case LEATHER_DRUM: //DRUM_OF_EARTHQUAKE:
        return GETOBJ_SUGGEST;
    default:
        return GETOBJ_EXCLUDE_SELECTABLE;
    }
    return GETOBJ_EXCLUDE_SELECTABLE;
}

/* occupation callback for enchanting an item */
static int
enchant(void)
{
    register struct obj* otmp;
    /* Look to see if you have something that can be enchanted */
    otmp = getobj("enchant", enchant_ok, GETOBJ_NOFLAGS);


    /* Look to see if you have all of the required components */
    // TODO: Ensure you have all of the required components

    /* Enchant the item and remove the components */
    // TODO: Enchant the item
    // TODO: Remove the components
    return 0;
}

/*enchant.c*/
