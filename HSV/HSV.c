/* sorry for the terrible mess around here.. I wasn't expecting any visitors in here.. */
#include <stdio.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <clib/exec_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/intuition_protos.h>
#include <graphics/gfxbase.h>

struct Library *IntuitionBase;
struct Library *GadToolsBase;
struct Library *GfxBase;
BOOL terminated;
struct TextAttr Topaz80 = {(UBYTE *) "topaz.font", 8, 0, 0,};
struct Gadget *gadgets[10];
UWORD rgb[256][3], backup[256][3];
void hsv2rgb (void);
void rgb2hsv (void);
int prefsfilelength = 106;
UWORD hue, sat, val, red, green, blue, curcol = 3;
long numberofcolours = 4;
long screendepth = 2;
struct Window *my_window;
char version[]= "$VER: HSV 0.99\n";

UBYTE prefsfile[] =
{
  /* 0000: */ 0x46, 0x4F, 0x52, 0x4D, 0x00, 0x00, 0x00, 0x62, 0x49, 0x4C, 0x42, 0x4D, 0x42, 0x4D, 0x48, 0x44,	/* FORM...bILBMBMHD */
  /* 0010: */ 0x00, 0x00, 0x00, 0x14, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,	/* ................ */
  /* 0020: */ 0x00, 0x00, 0x0A, 0x0B, 0x01, 0x40, 0x00, 0xC8, 0x43, 0x4D, 0x41, 0x50, 0x00, 0x00, 0x00, 0x30,	/* .....@.ÈCMAP...0 */
  /* 0030: */ 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x66, 0x88, 0xBB, 0x00, 0x00, 0xFF, 0xFF,	/* ªªª......f.».... */
  /* 0040: */ 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x66, 0x22, 0x00, 0xEE, 0x55, 0x00, 0x99, 0xFF,	/* ........f".îU... */
  /* 0050: */ 0x11, 0xEE, 0xBB, 0x00, 0x55, 0x55, 0xFF, 0x99, 0x22, 0xFF, 0x00, 0xFF, 0x88, 0xCC, 0xCC, 0xCC,	/* .î».UU.."....ÌÌÌ */
  /* 0060: */ 0x42, 0x4F, 0x44, 0x59, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00	/* BODY......       */
};

void
quit ()
{
  terminated = TRUE;
}

void
writeprefs ()
{
  FILE *fp;
  int i;
  for (i = 0; i < numberofcolours; i++)
    {
      prefsfile[3 * i + 48] = rgb[i][0] + 16 * rgb[i][0];
      prefsfile[3 * i + 48 + 1] = rgb[i][1] + 16 * rgb[i][1];
      prefsfile[3 * i + 48 + 2] = rgb[i][2] + 16 * rgb[i][2];
    }
  fp = fopen ("ENVARC:palette.prefs", "wb");
  if (fp != NULL)
    for (i = 0; i < prefsfilelength; i++)
      fprintf (fp, "%c", prefsfile[i]);
  close (fp);
  quit ();
}

void
setgadgets (struct Window *mywin)
{
  GT_SetGadgetAttrs (gadgets[0], mywin, NULL, GTSL_Level, hue, TAG_END);
  GT_SetGadgetAttrs (gadgets[1], mywin, NULL, GTSL_Level, sat, TAG_END);
  GT_SetGadgetAttrs (gadgets[2], mywin, NULL, GTSL_Level, val, TAG_END);
  GT_SetGadgetAttrs (gadgets[3], mywin, NULL, GTSL_Level, red, TAG_END);
  GT_SetGadgetAttrs (gadgets[4], mywin, NULL, GTSL_Level, green, TAG_END);
  GT_SetGadgetAttrs (gadgets[5], mywin, NULL, GTSL_Level, blue, TAG_END);
}

void
somethinghappened (UWORD id, UWORD code, struct Window *mywin)
{
  long i;
  if (id <= 2)
    {
      /* hsv changed so recalculate and set rgb-sliders */
      if (id == 0)
	hue = code;
      else if (id == 1)
	sat = code;
      else
	val = code;
      hsv2rgb ();
      GT_SetGadgetAttrs (gadgets[3], mywin, NULL, GTSL_Level, red, TAG_END);
      GT_SetGadgetAttrs (gadgets[4], mywin, NULL, GTSL_Level, green, TAG_END);
      GT_SetGadgetAttrs (gadgets[5], mywin, NULL, GTSL_Level, blue, TAG_END);
    }
  else if ((id > 2) && (id <= 5))
    {
      /* rgb changed so recalculate and set hsv-sliders */
      if (id == 3)
	red = code;
      else if (id == 4)
	green = code;
      else
	blue = code;
      rgb2hsv ();
      GT_SetGadgetAttrs (gadgets[0], mywin, NULL, GTSL_Level, hue, TAG_END);
      GT_SetGadgetAttrs (gadgets[1], mywin, NULL, GTSL_Level, sat, TAG_END);
      GT_SetGadgetAttrs (gadgets[2], mywin, NULL, GTSL_Level, val, TAG_END);
    }
  else if (id == 6)
    {
      /* cancel, colours to startup-values */
      for (i = 0; i < numberofcolours; i++)
	SetRGB4 (&mywin->WScreen->ViewPort, i, backup[i][0], backup[i][1], backup[i][2]);
      quit ();
      red = backup[curcol][0];
      green = backup[curcol][1];
      blue = backup[curcol][2];
    }
  else if (id == 7)		/* palette */
    {
      rgb[curcol][0] = red;	/* save old colour */
      rgb[curcol][1] = green;
      rgb[curcol][2] = blue;
      curcol = code;
      red = rgb[curcol][0];	/* set new colour */
      green = rgb[curcol][1];
      blue = rgb[curcol][2];
      rgb2hsv ();
      setgadgets (mywin);
    }
  else if (id == 8)
    {
      writeprefs ();		/* save */
      quit ();
    }
  else if (id == 9)
    quit ();			/* cancel */
  SetRGB4 (&mywin->WScreen->ViewPort, curcol, red, green, blue);
}

void
processwindowevents (struct Window *mywin)
{
  struct IntuiMessage *imsg;
  terminated = FALSE;
  while (!terminated)
    {
      Wait (1 << mywin->UserPort->mp_SigBit);
      while ((!terminated) && (imsg = GT_GetIMsg (mywin->UserPort)))
	{
	  switch (imsg->Class)
	    {
	    case MOUSEMOVE:
	    case IDCMP_GADGETUP:
	      somethinghappened ((UWORD) ((struct Gadget *) imsg->IAddress)->GadgetID, imsg->Code, mywin);
	      break;
	    case IDCMP_CLOSEWINDOW:
	      terminated = TRUE;
	      break;
	    case IDCMP_REFRESHWINDOW:
	      GT_BeginRefresh (mywin);
	      GT_EndRefresh (mywin, TRUE);
	      break;
	    }
	  GT_ReplyIMsg (imsg);
	}
    }
}

void
gadtoolswindow (void)
{
  struct Screen *mysc;
  struct Window *mywin;
  struct Gadget *glist, *gad;
  struct NewGadget ng;
  UWORD *colour_table;
  struct DrawInfo *screen_drawinfo = NULL;
  BOOL savebutdisabled=FALSE;

  void *vi;
  int i;
  glist = NULL;
  if ((mysc = LockPubScreen (NULL)) != NULL)
    if ((screen_drawinfo = GetScreenDrawInfo (mysc)) != NULL)
      {
	if ((screendepth = screen_drawinfo->dri_Depth)>4) savebutdisabled=TRUE;
	numberofcolours = (1 << screendepth);
	if (numberofcolours<4) curcol=0;
	if ((vi = GetVisualInfo (mysc, TAG_END)) != NULL)
	  {
	    gad = CreateContext (&glist);
	    ng.ng_TextAttr = &Topaz80;
	    ng.ng_VisualInfo = vi;
	    ng.ng_LeftEdge = 65;
	    ng.ng_TopEdge = 30 + mysc->WBorTop + (mysc->Font->ta_YSize + 1);
	    ng.ng_Width = 150;
	    ng.ng_Height = 12;
	    ng.ng_GadgetText = (UBYTE *) "H:   ";
	    ng.ng_GadgetID = 0;
	    ng.ng_Flags = PLACETEXT_LEFT;
	    gadgets[0] = gad = CreateGadget (SLIDER_KIND, gad, &ng,
					     GTSL_Min, 0,
					     GTSL_Max, 359,
					     GTSL_Level, hue,
					     GTSL_LevelFormat, "%3ld",
					     GTSL_LevelPlace, PLACETEXT_LEFT,
					     GTSL_MaxLevelLen, 3,
					     TAG_END);
	    ng.ng_TopEdge += 15;
	    ng.ng_GadgetID = 1;
	    ng.ng_GadgetText = (UBYTE *) "S:   ";
	    gadgets[1] = gad = CreateGadget (SLIDER_KIND, gad, &ng,
					     GTSL_Min, 0,
					     GTSL_Max, 100,
					     GTSL_Level, sat,
					     GTSL_LevelFormat, "%3ld",
					     GTSL_MaxLevelLen, 3,
					     TAG_END);
	    ng.ng_TopEdge += 15;
	    ng.ng_GadgetID = 2;
	    ng.ng_GadgetText = (UBYTE *) "V:   ";
	    gadgets[2] = gad = CreateGadget (SLIDER_KIND, gad, &ng,
					     GTSL_Min, 0,
					     GTSL_Max, 100,
					     GTSL_Level, val,
					     GTSL_LevelFormat, "%3ld",
					     GTSL_MaxLevelLen, 3,
					     TAG_END);
	    ng.ng_LeftEdge = 270;
	    ng.ng_TopEdge = 30 + mysc->WBorTop + (mysc->Font->ta_YSize + 1);
	    ng.ng_GadgetID = 3;
	    ng.ng_GadgetText = (UBYTE *) "R:  ";
	    gadgets[3] = gad = CreateGadget (SLIDER_KIND, gad, &ng,
					     GTSL_Min, 0,
					     GTSL_Max, 15,
					     GTSL_Level, red,
					     GTSL_LevelPlace, PLACETEXT_LEFT,
					     GTSL_LevelFormat, "%2ld",
					     GTSL_MaxLevelLen, 2,
					     TAG_END);
	    ng.ng_GadgetID = 4;
	    ng.ng_TopEdge += 15;
	    ng.ng_GadgetText = (UBYTE *) "G:  ";
	    gadgets[4] = gad = CreateGadget (SLIDER_KIND, gad, &ng,
					     GTSL_Min, 0,
					     GTSL_Max, 15,
					     GTSL_Level, green,
					     GTSL_LevelPlace, PLACETEXT_LEFT,
					     GTSL_LevelFormat, "%2ld",
					     GTSL_MaxLevelLen, 2,
					     TAG_END);
	    ng.ng_GadgetID = 5;
	    ng.ng_TopEdge += 15;
	    ng.ng_GadgetText = (UBYTE *) "B:  ";
	    gadgets[5] = gad = CreateGadget (SLIDER_KIND, gad, &ng,
					     GTSL_Min, 0,
					     GTSL_Max, 15,
					     GTSL_Level, blue,
					     GTSL_LevelPlace, PLACETEXT_LEFT,
					     GTSL_LevelFormat, "%2ld",
					     GTSL_MaxLevelLen, 2,
					     TAG_END);
	    ng.ng_TopEdge = 90;
	    ng.ng_LeftEdge = 265;
	    ng.ng_Width = 75;
	    ng.ng_Height = 12;
	    ng.ng_GadgetText = (UBYTE *) "Cancel";
	    ng.ng_GadgetID = 6;
	    ng.ng_Flags = 0;
	    gadgets[6] = gad = CreateGadget (BUTTON_KIND, gad, &ng, TAG_END);

	    ng.ng_TopEdge = 15;
	    ng.ng_LeftEdge = 117;
	    ng.ng_Height = 20;
	    ng.ng_Width = 200;
	    ng.ng_GadgetID = 7;
	    ng.ng_GadgetText = (UBYTE *) "";

	    gadgets[7] = gad = CreateGadget (PALETTE_KIND, gad, &ng, GTPA_Depth, screendepth, GTPA_Color, 3, GTPA_IndicatorWidth, 40, TAG_END);

	    ng.ng_TopEdge = 90;
	    ng.ng_LeftEdge = 95;
	    ng.ng_Width = 75;
	    ng.ng_Height = 12;
	    ng.ng_GadgetText = (UBYTE *) "Save";
	    ng.ng_GadgetID = 8;
	    ng.ng_Flags = 0;
	    gadgets[8] = gad = CreateGadget (BUTTON_KIND, gad, &ng,GA_Disabled,savebutdisabled, TAG_END);

	    ng.ng_TopEdge = 90;
	    ng.ng_LeftEdge = 180;
	    ng.ng_Width = 75;
	    ng.ng_Height = 12;
	    ng.ng_GadgetText = (UBYTE *) "Use";
	    ng.ng_GadgetID = 9;
	    ng.ng_Flags = 0;
	    gadgets[9] = gad = CreateGadget (BUTTON_KIND, gad, &ng, TAG_END);



	    if (gad != NULL)
	      {
		if ((mywin = OpenWindowTags (NULL,
					     WA_Title, "HSV 0.99, FE",
					     WA_Gadgets, glist,
					     WA_AutoAdjust, TRUE,
					     WA_Width, 435,
					     WA_InnerHeight, 97,
					     WA_DragBar, TRUE,
					     WA_DepthGadget, TRUE,
					     WA_Activate, TRUE,
					     WA_CloseGadget, TRUE,
					     WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | SLIDERIDCMP | BUTTONIDCMP,
					     WA_PubScreen, mysc,
					     TAG_END)) != NULL)
		  {
		    colour_table = (UWORD *) mywin->WScreen->ViewPort.ColorMap->ColorTable;
		    for (i = 0; i < numberofcolours; i++)
		      {
			backup[i][0] = (rgb[i][0] = (colour_table[i] & 0xF00) >> 8);
			backup[i][1] = (rgb[i][1] = (colour_table[i] & 0x0F0) >> 4);
			backup[i][2] = (rgb[i][2] = colour_table[i] & 0x00F);
		      }
		    red = rgb[curcol][0];
		    green = rgb[curcol][1];
		    blue = rgb[curcol][2];
		    rgb2hsv ();
		    setgadgets (mywin);
		    GT_RefreshWindow (mywin, NULL);
		    processwindowevents (mywin);
		    CloseWindow (mywin);
		  }
	      }
	    FreeGadgets (glist);
	    FreeVisualInfo (vi);
	  }
	UnlockPubScreen (NULL, mysc);
      }
}

void
rgb2hsv (void)
{
  float h, s, v, r, g, b, ma, mi, d;
  r = (float) red / 15;
  g = (float) green / 15;
  b = (float) blue / 15;

  ma = (r > b) ? r : b;
  ma = (ma > g) ? ma : g;
  mi = (r < b) ? r : b;
  mi = (mi < g) ? mi : g;
  v = ma;
  if (ma != 0)
    s = (ma - mi) / ma;
  else
    s = 0;
  if (s == 0)
    h = 0;
  else
    {
      d = ma - mi;
      if (r == ma)
	h = (g - b) / d;
      else if (g == ma)
	h = 2 + (b - r) / d;
      else
	h = 4 + (r - g) / d;
      h *= 60;
      if (h < 0)
	h += 360;
    }
  hue = h;
  sat = 100 * s;
  val = 100 * v;
}

void
hsv2rgb (void)
{
  float h, s, v, r, g, b, p1, p2, p3;
  int i = 0;
  float f = 0;
  h = (float) hue / 60;
  v = (float) val / 100;
  s = (float) sat / 100;
  while ((f = h - i) > 1)
    i++;
  p1 = v * (1 - s);
  p2 = v * (1 - (s * f));
  p3 = v * (1 - (s * (1 - f)));
  switch (i)
    {
    case 0:
      r = v;
      g = p3;
      b = p1;
      break;
    case 1:
      r = p2;
      g = v;
      b = p1;
      break;
    case 2:
      r = p1;
      g = v;
      b = p3;
      break;
    case 3:
      r = p1;
      g = p2;
      b = v;
      break;
    case 4:
      r = p3;
      g = p1;
      b = v;
      break;
    case 5:
      r = v;
      g = p1;
      b = p2;
      break;
    }
  red = (UWORD) (r * 15);
  green = (UWORD) (g * 15);
  blue = (UWORD) (b * 15);
}

void
main (void)
{
  if ((IntuitionBase = OpenLibrary ((UBYTE *) "intuition.library", 37l)) != NULL)
    {
      if ((GadToolsBase = OpenLibrary ((UBYTE *) "gadtools.library", 37l)) != NULL)
	{
	  if ((GfxBase = OpenLibrary ((UBYTE *) "graphics.library", 0l)) != NULL)
	    {
	      gadtoolswindow ();
	      CloseLibrary (GfxBase);
	    }
	  CloseLibrary (GadToolsBase);
	}
      CloseLibrary (IntuitionBase);
    }
}
