#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"

typedef struct {
  short col, row;
} Pos;

int shapes[] = {0, 1, 2, 3};
int currentShape = 0;

Pos positions[] = {
  {12,12},			/* upper left */
  {12, screenHeight-30}, 	/* lower left */
  {screenWidth -20, screenHeight-30}, /* lower right */
  {screenWidth -20, 12},	       /* upper right */
  {screenWidth / 2, screenHeight/2}      /* middle */
};
#define NUM_POSITIONS 5

unsigned short sqColors[] = {COLOR_RED, COLOR_GREEN, COLOR_ORANGE, COLOR_BLUE};
#define NUM_SQCOLORS 4
#define BG_COLOR COLOR_BLACK

char current_position = 0, current_color = 0;
  
// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!! 

#define LED BIT6		/* note that bit zero req'd for display */

#define SWITCHES 15

int redrawScreen = 1;

static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

int switches = 0;

void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;

  if (switches & SWITCHES) { 	/* a switch is depresssed */
    redrawScreen = 1;
    for (char swNum = 0; swNum < 4; swNum++) { /* respond to lowest button pressed */
      int swFlag = 1 << swNum;
      if (switches & swFlag) {
  currentShape = shapes[swNum];

	break;
      }
    }
  }
}

void wdt_c_handler()
{
  static int sec2Count = 0;
  static int sec1Count = 0;
  if (sec2Count++ >= 062) {		/* 2/sec */
    sec2Count = 0;
    current_color = (current_color+1) % NUM_SQCOLORS;
    redrawScreen = 1;
  }
  if (sec1Count++ >= 250) {		/* 1/sec */
    sec1Count = 0;
    current_position = (current_position+1) % NUM_POSITIONS;
    redrawScreen = 1;
  }
}
  
void update_shape();

void main()
{
  
  P1DIR |= LED;		/**< Green led on when CPU on */
  P1OUT |= LED;
  configureClocks();
  lcd_init();
  switch_init();
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  clearScreen(BG_COLOR);
  while (1) {			/* forever */
    if (redrawScreen) {
      drawBoard();
      redrawScreen = 0;
      update_shape();
    }
    P1OUT &= ~LED;	/* led off */
    or_sr(0x10);	/**< CPU OFF */
    P1OUT |= LED;	/* led on */
  }
}

void drawBoard() {
  for (int i = 5; i <= screenWidth-5; i++) {
    for (int j = 5; j <= screenHeight-5; j++) {
      if (i == 5 || i == (screenWidth -5) || j == 5 || j == (screenHeight-5)) {
        fillRectangle(i, j, 2, 2, COLOR_WHITE);
      }
    }
  }
}

    
    
void
update_shape()
{
  static char last_position = 0, last_color = 0;
  redrawScreen = 0;
  int pos, color;
  and_sr(~8);  // mask interrupts (GIE = 0)
  pos = current_position;	/* read state variables */
  color = current_color;
  or_sr(8); // unmask interrupts
  if (pos == last_position && color == last_color) /* nothing to redraw */
    return;

  /* erase old shape */
  short col = positions[last_position].col;
  short row = positions[last_position].row;
  if (pos != last_position)    /* erase if position changed */
    switch (currentShape) {
      case 0:
        fillRectangle(col-5, row-5, 10, 30, BG_COLOR); 
        fillRectangle(col+5, row-5, 10, 10, BG_COLOR);
        break;
      case 2:
        fillRectangle(col-5, row, 10, 20, BG_COLOR);
        fillRectangle(col+5, row-10, 10, 20, BG_COLOR);
    }

  /* draw new shape */
  col = positions[pos].col;
  row = positions[pos].row;
    switch (currentShape) {
      case 0:
        fillRectangle(col-5, row-5, 10, 30, sqColors[color]); 
        fillRectangle(col+5, row-5, 10, 10, sqColors[color]);
        break;
      case 1:
        fillRectangle(col-5, row-5, 10, 30, sqColors[color]); 
        fillRectangle(col+5, row-5, 10, 10, sqColors[color]);
        break;
      case 2:
        fillRectangle(col-5, row, 10, 20, sqColors[color]);
        fillRectangle(col+5, row-10, 10, 20,  sqColors[color]);
        break;
      case 3:
        fillRectangle(col-5, row, 10, 20, sqColors[color]);
        fillRectangle(col+5, row-10, 10, 20,  sqColors[color]);
        break;
    }
//  fillRectangle(col-5, row-5, 10, 30, sqColors[color]); /* draw new shape */
//  fillRectangle(col+5, row-5, 10, 10, sqColors[color]);
  /* remember color & pos for next redraw */
  last_position = pos;
  last_color = color;
}


/* Switch on S2 */
void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}
