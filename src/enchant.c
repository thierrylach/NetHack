/* NetHack 3.7	enchant.c	$NHDT-Date: $  $NHDT-Branch: $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/*-Copyright (c) Thierry Lach, 2022. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

boolean u_can_enchant(void);
int doenchant(void);
int enchant_ok(struct obj*);

#define ugod_is_angry() (u.ualign.record < 0)
#define on_altar() IS_ALTAR(levl[u.ux][u.uy].typ)
#define on_shrine() ((levl[u.ux][u.uy].altarmask & AM_SHRINE) != 0)
#define a_align(x, y) ((aligntyp) Amask2align(levl[x][y].altarmask & AM_MASK))

#define NECOMP 6

/* Enchantment components */
#define NCHCMPT(c1, c2, c3, c4, c5, c6) \
    {                                   \
        c1, c2, c3, c4, c5, c6          \
    }

#define NCH(v1, v2, v3, v4, v5, v6) {v1,v2,v3,v4,v5,v6}
#define NO_NCH    {0,0,0}                               /* no component */
#define NCHITEM(v1, v2)    {v1,v2,0}                    /* item component */
#define NCHCORPSE(v1)  {CORPSE,BUC_ALLBKNOWN,v1}        /* corpse component */
#define NCH_NOT_CURSED BUC_UNCURSED|BUC_BLESSED
#define NCH_ANY_ALIGN BUC_CURSED|BUC_UNCURSED|BUC_BLESSED


/* Enchantment component */
struct enchcomp {
    short otyp;                             /* type of object to use */
    int flags;                              /* blessed/cursed flags */
    short corpse;                           /* type of corpse */
};

/* enchantment information (from, to, min level, components) */
struct enchinfo {
    short otyp;                             /* type of object to enchant */
    short result;                           /* object enchanted to */
    short minlvl;                           /* minimum experience level to enchant */
    int success;                            /* success percentage
                                             * should be low because of all of the modifiers added to it
                                             * including the person's level
                                             */
    boolean init;                           /* initialize the object?  This avoids putting objects in bags */
    struct enchcomp complist[NECOMP];       /* required components - include the base object */
};

static struct enchinfo enchantable[] = {
                    NCH(OILSKIN_SACK, BAG_OF_HOLDING, 10, 20, FALSE,
                        NCHCMPT(NCHITEM(OILSKIN_SACK, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHITEM(GARNET, NCH_ANY_ALIGN), NO_NCH, NO_NCH)),
                    NCH(SACK, BAG_OF_HOLDING, 12, 10, FALSE,
                        NCHCMPT(NCHITEM(SACK, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHITEM(GARNET, NCH_NOT_CURSED), NO_NCH, NO_NCH)),
                    NCH(LARGE_BOX, ICE_BOX, 16, 10, FALSE,
                        NCHCMPT(NCHITEM(SACK, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHITEM(GLOB_OF_BROWN_PUDDING, NCH_NOT_CURSED), NO_NCH, NO_NCH)),
                    NCH(CHEST, ICE_BOX, 13, 20, FALSE,
                        NCHCMPT(NCHITEM(SACK, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHITEM(GLOB_OF_BROWN_PUDDING, NCH_NOT_CURSED), NO_NCH, NO_NCH)),
                    NCH(TIN_WHISTLE, MAGIC_WHISTLE, 12, 10, TRUE,
                        NCHCMPT(NCHITEM(TIN_WHISTLE, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHCORPSE(PM_RUST_MONSTER), NO_NCH, NO_NCH)),
                    NCH(BRASS_LANTERN, MAGIC_LAMP, 10, 10, TRUE,
                        NCHCMPT(NCHITEM(BRASS_LANTERN, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHCORPSE(PM_RED_MOLD), NCHITEM(RUBY, NCH_ANY_ALIGN), NO_NCH)),
                    NCH(OIL_LAMP, MAGIC_LAMP, 14, 20, TRUE,
                        NCHCMPT(NCHITEM(OIL_LAMP, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                               NCHCORPSE(PM_RED_MOLD), NCHITEM(RUBY, NCH_NOT_CURSED), NO_NCH)),
                    NCH(WOODEN_FLUTE, MAGIC_FLUTE, 12, 10, TRUE,
                        NCHCMPT(NCHITEM(WOODEN_FLUTE, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHCORPSE(PM_OCHRE_JELLY), NCHITEM(POT_SLEEPING, NCH_ANY_ALIGN), NO_NCH)),
                    NCH(BUGLE, HORN_OF_PLENTY, 16, 10, TRUE,
                        NCHCMPT(NCHITEM(BUGLE, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHITEM(RIN_SLOW_DIGESTION, BUC_BLESSED), NCHCORPSE(PM_GELATINOUS_CUBE), NCHITEM(SCR_FOOD_DETECTION, NCH_NOT_CURSED))),
                    NCH(WOODEN_HARP, MAGIC_HARP, 15, 10, TRUE,
                        NCHCMPT(NCHITEM(WOODEN_HARP, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHITEM(SPE_NOVEL, NCH_NOT_CURSED), NCHITEM(SCR_TAMING, NCH_NOT_CURSED), NO_NCH)),
                    NCH(LEATHER_DRUM, DRUM_OF_EARTHQUAKE, 18, 5, TRUE,
                        NCHCMPT(NCHITEM(LEATHER_DRUM, NCH_NOT_CURSED), NCHITEM(POT_WATER, BUC_BLESSED), NCHITEM(POT_POLYMORPH, NCH_NOT_CURSED),
                                NCHITEM(SCR_EARTH, NCH_NOT_CURSED), NO_NCH, NO_NCH)),
                    NCH(0, 0, 0, 0, 0,
                        NCHCMPT(NO_NCH, NO_NCH, NO_NCH, NO_NCH, NO_NCH, NO_NCH))
};

/* can hero enchant at all? */
boolean
u_can_enchant(void)
{
    aligntyp altaralign = a_align(u.ux, u.uy);
    schar intell = ACURR(A_INT);
    schar wis = ACURR(A_WIS);
    schar dext = ACURR(A_DEX);

    if (Unaware) {
        You("are dreaming!");
        return FALSE;
    }
    if (Blind) {
        You_cant("see!");
        return FALSE;
    }
    if (Hallucination) {
        You_cant("see what components to use!");
        return FALSE;
    }
    if (Levitation) {
        You_cant("reach the floor!");
        return FALSE;
    }
    if (u.uswallow) {
        You_cant("move enough!");
        return FALSE;
    }
    if (nohands(g.youmonst.data)) {
        You_cant("hold anything!");
        return FALSE;
    }

    if (intell < 20) {
        You("are not smart enough!");
        return FALSE;
    }
    if (wis < 20) {
        You("are not wise enough!");
        return FALSE;
    }
    if (dext < 15) {
        You("are not agile enough!");
        return FALSE;
    }
    if (u.ulevel < 10) {
        You("are not experienced enough!");
        return FALSE;
    }
    if (calc_capacity(0) > 2) {
        You("are too encumbered.");
        return FALSE;
    }
    if (u.uhunger <= 10) {
        You("are too hungry to enchant!");
        return FALSE;
    }
    /* Must be on a co-aligned alter */
    if (!on_altar()) {
        You("are not standing on an altar.");
        return FALSE;
    }
    if (u.ualign.type != altaralign) {
        You_cant("enchant here!");
        return FALSE;
    }
    return TRUE;
}

/* getobj callback for object to enchant */
int
enchant_ok(struct obj* obj)
{
    if (!obj) {
        return GETOBJ_EXCLUDE;
    }
    register int i;
    for (i = 0; enchantable[i].otyp != 0; i++) {
        if (enchantable[i].otyp == obj->otyp)
            return GETOBJ_SUGGEST;
    }
    return GETOBJ_EXCLUDE_SELECTABLE;
}

void
set_unused(void)
{
    struct obj* otmp;
    for (otmp = g.invent; otmp; otmp = otmp->nobj) {
        otmp->in_use = FALSE;
    }
}

int
is_usable_for_enchant(struct enchcomp* component, struct obj* obj) {
    debugpline2("Comparing %s to %s", "", "");
    if (component->otyp == CORPSE && obj->corpsenm != component->corpse) {
        return FALSE;
    }
    if (obj->blessed && (component->flags && BUC_BLESSED)) {
        return TRUE;
    }
    if (obj->cursed && (component->flags && BUC_CURSED)) {
        return TRUE;
    }
    if ((!(obj->blessed)) && (!(obj->cursed)) && (component->flags && BUC_UNCURSED)) {
        return TRUE;
    }
    return FALSE;
}

//struct posscomp {                   /* possible component */
//    struct posscomp* next;          /* pointer to next in chain*/
//    boolean selected;               /* is component selected for use? */
//    boolean ignored;                 /* ignored because another of that component has been selected*/
//    struct obj* comp;               /* pointer to object in inventory */
//};

/* callback for enchanting an item */
int
doenchant(void)
{
    register struct obj* otmp;
    struct enchinfo *trying = NULL;     /* requirements for enchanting obj */
    struct enchcomp* component = NULL;
    struct obj* isearch = NULL;
    char posscomp[26];                  /* inventory letters of possible components */
    boolean found1;
    aligntyp altaralign = a_align(u.ux, u.uy);


    register int i;
    for (i = 0; i < 26; i++)
        posscomp[i] = '\0';

    /* Look to see if you have something that can be enchanted */
    otmp = getobj("enchant", enchant_ok, GETOBJ_NOFLAGS);
    if (!otmp) {
        return 0;
    }

    for (i = 0; enchantable[i].otyp != 0; i++) {
        if (enchantable[i].otyp == otmp->otyp)
            trying = &enchantable[i];
    }
     if (!trying) {
        impossible("Did not find object type to enchant");
    }

    if (!u_can_enchant()) {
        return 0;
    }
    /* This means that you can retry, but the message is annoying */
    if (Glib && rnd(10) < 3) {
        Your("hands are too slippery");
        return 0;
    }

    if (trying) {
        /* Check to see if you are of sufficient level for this specific item */
        if (u.ulevel < trying->minlvl) {
            You("are not experienced enough!");
            return FALSE;
        }

        /* Look to see if you have all of the required components */
        // TODO: Ensure you have all of the required components
        for (i = 0; i < 6; i++) {
            component = &(trying->complist[i]);
            if (!(component->otyp)) {
                continue;
            }
            found1 = FALSE;
            for (isearch = g.invent; isearch; isearch = isearch->nobj) {
                if (isearch->otyp == component->otyp) {
                    if (!is_usable_for_enchant(component, isearch))
                        continue;
                    //isearch->in_use = TRUE;
                    // TODO: Add to the possible component list
                    ///* Add to the possible component list*/
                    //tcomp = &{possible, false, false, isearch};
                    //possible = tcomp;
                    found1 = TRUE;
                }
            }
            if (!found1) {
                You("do not have all of the necessary components needed to enchant.");
                return 0;
            }
        }


        // TODO: Select the specific components in case of multiple
        //otmp = getobj("components", components_ok, GETOBJ_NOFLAGS);
        // display_cinventory ?

        /* if we exit prematurely after this point, we need to call set_unused to ensure that we don't unintentionally use up items*/
        //set_unused();

        /* Additional adjustments to success */
        int success = rnd(100);
        success += u.ulevel;                            /* add level bonus */
        success += P_SKILL(P_ENCHANTMENT_SPELL);        /* Add enhancement bonus */
        success += Luck;                                /* add luck modifier */
        if (on_shrine())
            success += 10;                               /* shrines help a lot */

        // TODO: Plus for each blessed component, minus for each cursed
        // 

        if (ugod_is_angry()) {
            godvoice(altaralign, "Thou hast angered me more. I decline thy request.");
            change_luck(-5);
        }
        else {
            /* Enchant the item */
            otmp = mksobj(trying->result, trying->init, FALSE);
            otmp->cursed = FALSE;                           /* will never be cursed */
#if 0
            /* additional adjustments can be incorporated here */
            switch (otmp->otyp) {
            case BAG_OF_HOLDING:
            case ICE_BOX:
            case HORN_OF_PLENTY:
            case MAGIC_FLUTE:
            case MAGIC_WHISTLE:
            case MAGIC_HARP:
            case MAGIC_LAMP:
            case DRUM_OF_EARTHQUAKE:
                break;
            default:
                impossible("Unhandled enchanted item");
            }
#endif
            otmp = addinv(otmp);
            godvoice(altaralign, "Thou hast done well. I grant thy boon.");
            record_achievement(ACH_CRFT);
        }

        /* Remove the components, even if god is angry */
        for (isearch = g.invent; isearch; isearch = isearch->nobj) {
            if (isearch->in_use) {
                useup(isearch);
                isearch = g.invent;     /* restart the search because the inventory chain may have changed */
            }
        }

    }


    return 0;
}

/*enchant.c*/
