/*
 * Copyright (C) 2005 Jimi Xenidis <jimix@watson.ibm.com>, IBM Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 * 
 * $Id$
 */

#include <test.h>
#include <partition.h>
#include <sysipc.h>
#include <xh.h>

#define CANVAS_WIDTH 450
#define CANVAS_HEIGHT 355

#define OBJECT_NAME "bounce"

#define OBJECT1_DIAM 30

#define TRUE 1

#define MAXCOLOR 200
#define MAXLENGTH 256
#define MAXBALLS  1
#define DEC_BALL  0

#define SCREENCOLOR "black"
#define BALL1COLOR  "yellow"
#define BALL0COLOR  "green"

#define GRAPH_CHANNEL 3
#define MIN_DEC_BITS  20
#define MAX_DEC_BITS  21
#define DEC_INCR      1
#define DEFAULT_DEC_COUNT 327680
#define DEC_OUTPUT 0

uval  decr_count = DEFAULT_DEC_COUNT;
int shift_bits = 16; 
int exit_app=0;

const char *colors[] = {
	"red", "green", "yellow", "orange",
	 "pink", "magenta", "white", "blue",
	/* FIXME: why repeat */
	"red", "green", "yellow", "orange",
	"pink", "magenta", "white", "blue"
};

/* define the object structure */
struct obj {
        const char *color;                  /* ball color as a string */
	char _pad[sizeof (char *) - 4];
        uval32 left;                     /* object co-ordinates */
	uval32 right;
	uval32 upper;
	uval32 lower; 
        uval32 delta_x;                  /* delta values to move the object */
	uval32 delta_y;
        uval32 channel;         /* channel to use for sending the commands */
        /* buffer to store commands before sending to tcl */
        char tkoutbuffer[MAXLENGTH];
        char name[32 + (MAXLENGTH % 8)]; /* ball name will be sent to tcl   */

};

struct obj decBall;         /* ball used for drawing in decrement interrupt ( channel 1) */
struct obj ball[MAXBALLS];  /* balls drawn in screen1 in test_os ( channel 2) */



void  update_position(struct obj *);
void  update_display(struct obj *);
int process_input(void);

uval32 prevExitTime=0;
uval32 prevProcessingTime=0;
char s[256];
uval32 entryTime,processingTime;
int curDiff;
uval32 prevExcpTime=0 ;
uval32 test_on = 0;
uval32 results_ready = 0;
uval32 skipNextSample = 0;
char tkgraphbuffer[MAXLENGTH];
uval32 startDecTime;
uval32 endDecTime;
uval32 decTimeIncr;
uval32 maxEntries;
int curIndex;
uval32 sampleIndex;
int totalProcessed;
int displayFlag=0;
uval32 prevEntry=0;
uval dec_processing_time=15000; /* dec interrupt processing time */
struct profileTime{
uval32 prevExit;
uval32 entry;
uval32 processing;
uval32 prevProcessing;
uval32 curTime;
   int decDelay;
};

/* 16 different dec timings and for each one 10,000 entries */
#define MAX_ENTRIES 1000
#define MAX_ITEMS   60
struct profileTime allTimings[MAX_ITEMS][MAX_ENTRIES];  
struct profileTime curSample;

static __inline__ uval32 mftbl(void);
static inline uval get16chars(char *buf);
void send_to_out(const char *tk_cmd, int chan);


static void
processTimingInfo(void)
{
        int diffPercent;
	int diff;
	int procPercent=0;
	uval32 temp;

	temp = mftbl();

	if (skipNextSample == 1){
	  skipNextSample = 0; 
	  prevProcessingTime = processingTime;
	  return;
	}

	
	if (test_on){
	  allTimings[curIndex][sampleIndex].prevExit = prevExitTime;
	  allTimings[curIndex][sampleIndex].entry    = entryTime;
	  allTimings[curIndex][sampleIndex].processing = processingTime;
	  allTimings[curIndex][sampleIndex].prevProcessing = prevProcessingTime;
	  allTimings[curIndex][sampleIndex].curTime = temp;
	  allTimings[curIndex][sampleIndex].decDelay = curSample.decDelay;
	}

        /* calculate the time between prev exit and this entry */ 
	curDiff = entryTime - prevExitTime;
        diff = curDiff - decr_count;


	diffPercent = (diff*100)/decr_count;
	procPercent = ((processingTime - prevProcessingTime)*100)/prevProcessingTime;

	if (displayFlag){
	       if (procPercent > 10 || procPercent < -10) {
		       sprintf(s, "********Processing Time = %d, diff = %d, percent = %d\n",
			       processingTime, (processingTime - prevProcessingTime), procPercent);
	       }

#ifdef SHOW_PERCENT
	       if (diffPercent > 10 || diffPercent < -10) {
		       if (diff > 6000) {
			       sprintf(s, "\n%d::TimeSpent In  = %d , "
				       "Out= %d, diff = %d, percent =%d, "
				       "dec= %ld, %d\n", 
				       mftbl(),
				       processingTime - entryTime,
				       curDiff, diff, diffPercent, decr_count,
				       entryTime - prevEntry);
			       hputs(s);
			       sprintf(s, "%d::Time interval = %d\n",
				       mftbl(), temp-prevExcpTime);
			       hputs(s);
			       prevExcpTime = temp;
		       }
	       } else {
		       hputs(".");
	       }
#endif
	}


	if (test_on){
	  sampleIndex++;
	  if (sampleIndex >= maxEntries) {
	    curIndex ++;
	    sampleIndex = 0;
	    totalProcessed = curIndex;
	    if ((decr_count + decTimeIncr) > endDecTime  || curIndex >= MAX_ITEMS)
	      {
		decr_count =DEFAULT_DEC_COUNT ;
		results_ready = 1;
	      }
	    else decr_count = decr_count + decTimeIncr;	
	    skipNextSample = 1;
	  }
	}
	
	prevProcessingTime = processingTime;
}

static void
init_graph(void)
{
	send_to_out("set w .screen\n", GRAPH_CHANNEL);
	send_to_out("frame $w\n", GRAPH_CHANNEL);
	send_to_out("pack $w -side top -fill both -expand 1\n", GRAPH_CHANNEL);
	send_to_out("scrollbar $w.xscroll -orient horizontal -command \"$w.canvas xview\"\n", GRAPH_CHANNEL);
	send_to_out("scrollbar $w.yscroll -orient vertical   -command \"$w.canvas yview\"\n", GRAPH_CHANNEL);
	send_to_out("pack $w.xscroll -side bottom -fill x\n", GRAPH_CHANNEL);
	send_to_out("pack $w.yscroll -side right -fill y\n", GRAPH_CHANNEL);
	send_to_out("canvas $w.canvas -xscrollcommand \"$w.xscroll set\" -yscrollcommand \"$w.yscroll set\" -width 600 -height 600 -scrollregion {0 0 80000 5000} -background black\n", GRAPH_CHANNEL);
	send_to_out("grid $w.canvas $w.yscroll -sticky news\n", GRAPH_CHANNEL);
	send_to_out("grid $w.xscroll -sticky ew\n", GRAPH_CHANNEL);

	send_to_out("pack $w\n", GRAPH_CHANNEL);
}

static void DrawLine(int x1, int y1, int x2, int y2, const char *color)
{
  sprintf(tkgraphbuffer, "$w.canvas create line %d %d %d %d -fill %s\n",x1, 5000-y1, x2, 5000-y2, color);
	send_to_out(tkgraphbuffer, GRAPH_CHANNEL);
}

static void DrawText(int x, int y, char *text, const char *color)
{
  sprintf(tkgraphbuffer, "$w.canvas create text %d %d -text \"%s\" -fill %s  -width 50000\n",x,5000-y, text, color);
	send_to_out(tkgraphbuffer, GRAPH_CHANNEL);
}

static void
GenerateGraph(void)
{
	uval32 temp;
	uval32 decTime, startTime;
	int i;
	uval j;
	int diff;

	int prevX, prevY, x, y, totalTicks;
	int xoffset = 0;


	hputs("Start a console window with the following options \n"
	      "  $ console <thinwire host>:<console port + 3> | "
	      "cat - | wish -\n\n");
	

        send_to_out("\v", GRAPH_CHANNEL);

	init_graph();
	DrawLine(0,0, 50000-1, 0, "green");

	for (i = 0; i < totalProcessed; i++) {
		prevExcpTime = 0;
		decTime = startDecTime + i * decTimeIncr;
		
		prevX = 0;
		prevY = 0;
		totalTicks = 0;
		startTime = 0;


		for (j = 0; j < maxEntries; j++) {
			int k = i;
			uval t;
			prevExitTime = allTimings[k][j].prevExit;
			entryTime = allTimings[k][j].entry;
			processingTime = allTimings[k][j].processing;
			prevProcessingTime = allTimings[k][j].prevProcessing;
			temp = allTimings[k][j].curTime;
			
			/* calculate the time between prev exit and
			 * this entry */ 
			curDiff = entryTime - prevExitTime;
			diff = curDiff - decTime;

			t = (temp - startTime) /
				(startDecTime + (i * decTimeIncr));

			x = xoffset + t;
			y = allTimings[k][j].decDelay/30;

			totalTicks += curDiff + processingTime;


			if (j != 0) {
				int t;
				t = sizeof(colors)/sizeof(colors[0]);
				DrawLine(prevX, prevY+50, x, y+50,
					 colors[i%t]);
				prevX = x;
				prevY = y;
			} else {
				int t;
				t = sizeof(colors)/sizeof(colors[0]);
				startTime = temp;
				prevX = xoffset;
				prevY = 0;
				sprintf(s, "%d",
					startDecTime + i * decTimeIncr);
				DrawText(prevX+50, prevY+15, s, colors[i%t]);
			}
		}
		xoffset = x;
	}
}

static __inline__ uval32 mftbl(void) {
    uval32 ret;
    __asm__ ("mftbl %0": "=&r" (ret) : /* inputs */);
    return ret;
}

static __inline__ uval32 mftbu(void) {
    uval32 ret;
    __asm__ ("mftbu %0": "=&r" (ret) : /* inputs */);
    return ret;
}

static void
adjust_freq(void)
{
        /* reset decrement register */
	mtdec(decr_count);
}


static uval
xh_dec(uval ex __attribute__ ((unused)),
       uval *regs __attribute__ ((unused)))
{
 	/* record entry time for profiling */
        prevEntry = entryTime;
	entryTime = mftbl();

       /* save dec count value */
        curSample.decDelay =  mfdec() * -1;
	
	sprintf(s, "%ld:: Interrupt delay  = %d\n", mftb(),
		curSample.decDelay);
	hputs(s);

	/* record entry time for profiling */
        prevEntry = entryTime;
	entryTime = mftbl();


        /* update position and redraw object */
	update_position(&decBall);
	update_display(&decBall);
	
	/* see if there is a pending input from keyboard */
	process_input();
	
	processingTime = mftbl();

	/* see if there are any abnormal behavior */
	processTimingInfo();

#ifdef DECB_DEBUG
	sprintf(s, "%d::Time in = %d\n", mftbl(), mftbl()-entryTime);
	hputs(s);
#endif
	prevExitTime = mftbl();
#ifdef DECB_DEBUG
	adjust_freq();
#endif
	mtdec(decr_count - curSample.decDelay);

  return 0;
}

/*
 * Write the string to stdout
 */
void
send_to_out(const char *tk_cmd, int chan)
{
	uval len;

	len = strlen(tk_cmd);
	hcall_put_term_string(chan, len, tk_cmd);
}

/*
 * TK commands to setup initial graphic screen 
 */
static void
init_screen(struct obj *pObj)
{
	sprintf(pObj->tkoutbuffer, "canvas .screen -width %i -height %i\n",
		CANVAS_WIDTH, CANVAS_HEIGHT);
	send_to_out(pObj->tkoutbuffer, pObj->channel);
	sprintf(pObj->tkoutbuffer, ".screen config -background %s\n", SCREENCOLOR);
	send_to_out(pObj->tkoutbuffer, pObj->channel);
	sprintf(pObj->tkoutbuffer, "pack .screen\n");
	send_to_out(pObj->tkoutbuffer, pObj->channel);
}

/*
 * Create graphic object
 */
static void
init_object(struct obj *pObj, int id)
{
	/* Set initial position */
	pObj->left = 0;
	pObj->right = OBJECT1_DIAM;
	pObj->upper = 0;
	pObj->lower = OBJECT1_DIAM;
	
	/* Set change delta's */
	pObj->delta_x = 2*id;
	pObj->delta_y = 4*id;


	sprintf(pObj->name, "%s%d", OBJECT_NAME, id);
	
	sprintf(pObj->tkoutbuffer,
		".screen create oval %i %i %i %i "
		"-tags %s -fill %s -outline %s\n",
		pObj->left, pObj->upper,
		pObj->right, pObj->lower,
		pObj->name,
		pObj->color,
		pObj->color);
	send_to_out(pObj->tkoutbuffer, pObj->channel);
}


/*
 * Update the change to the object's current position
 */
void
update_position(struct obj * pObj)
{
	int x_left_new;
	int x_right_new;
	int y_upper_new;
	int y_lower_new;
        

	/* calculate new position */
	x_left_new  = pObj->left  + pObj->delta_x;
	x_right_new = pObj->right + pObj->delta_x;
	y_upper_new = pObj->upper + pObj->delta_y;
	y_lower_new = pObj->lower + pObj->delta_y;

	/*  see if it goes beyond X visible area */
	if ((x_left_new < 0) || (x_right_new > CANVAS_WIDTH)) {
	        /* change directions */
		pObj->delta_x = pObj->delta_x * -1;
		x_left_new = pObj->left + pObj-> delta_x;
		x_right_new = pObj->right + pObj->delta_x;
	}

	/* see if it goes beyond Y visible area */
	if ((y_upper_new < 0) || (y_lower_new > CANVAS_HEIGHT)) {
	        /* change directions	  */
		pObj->delta_y = pObj->delta_y * -1;
		y_upper_new = pObj->upper + pObj->delta_y;
		y_lower_new = pObj->lower + pObj->delta_y;
	}
	pObj->left = x_left_new;
	pObj->right = x_right_new;
	pObj->upper = y_upper_new;
	pObj->lower = y_lower_new;
}


/*
 * update_display: Generate command to update object position on screen
 */
void
update_display(struct obj *pObj)
{

	/* No color change, just move existing object */
	sprintf(pObj->tkoutbuffer, ".screen move %s %i %i\n",
		pObj->name,
		pObj->delta_x,
		pObj->delta_y);

	send_to_out(pObj->tkoutbuffer, pObj->channel);

	sprintf(pObj->tkoutbuffer, "update idletasks\n");
	send_to_out(pObj->tkoutbuffer, pObj->channel);
}

/*
 * exit_wish: Send "exit" command to wish, so it will terminate
 */

static void
exit_wish(struct obj *pObj)
{
  sprintf(pObj->tkoutbuffer, "exit\n");
  send_to_out(pObj->tkoutbuffer, pObj->channel);
}


static void
disp_menu(void)
{
	hputs("\n\n"
	      "< :  Decrease interrupt frequency\n"
	      "> :  Increase interrupt frequency\n"
	      "r :  Reset interrupt frequency\n"
	      "t :  Run a full test and stop\n"
	      "s :  Stop the test\n"
	      "q :  Quit\n\n"
	      "Enter Input: "
	      );
}




int 
process_input()
{
	char s[40];
        uval ret_vals[(16 / sizeof (uval)) + 1];
	uval rc;


	  rc = hcall_get_term_char(ret_vals, 0);
	  assert (rc == H_Success,
			"hcall_get_term_char: failed\n");

	  if (ret_vals[0] != 0) {
	    memcpy(s, (char *)&ret_vals[1], ret_vals[0]);

	    if ((s[0] == 'q') || (s[0] == 'Q')) {
		exit_app = 1;
		shift_bits = 20;
		decr_count = (1 << shift_bits);
		adjust_freq();		
		return 0; /* quit app */
	      }

	    switch (s[0]) {
	    case '>':  /* increase decrement int frequency */
	      if (shift_bits > 2){
		shift_bits -= 1;
		skipNextSample = 1;
	      }
	      decr_count = (5 << shift_bits);
	      adjust_freq();
	      break;

	    case '<': /* decrease int freq */
	      if (shift_bits < 29){	      
		shift_bits += 1;
		skipNextSample = 1;
	      }
	      decr_count = (5 << shift_bits);
	      adjust_freq();
	      break;	    

	    /*
	    case 'd':
	    case 'D':
	      displayFlag = displayFlag ^ 1;
	      break;
	      */

	    case 'r':   /* reset int frequency */
  	    case 'R': 
	      shift_bits = 16;
	      adjust_freq();
	      break;

	    case 's':
	    case 'S':
	      xh_table[XH_DEC] = NULL;
	      if (test_on)  results_ready = 1;
	      break;
	       

	    case 't':
	    case 'T':
	      
	      xh_table[XH_DEC] = NULL;
	      shift_bits = MIN_DEC_BITS;
	      sampleIndex = 0;
	      test_on = 1;
	      skipNextSample = 1;
	      maxEntries = MAX_ENTRIES;
	      curIndex = 0;
	      sampleIndex = 0;
	      totalProcessed = 0;
	      hputs("Do you want to enter decrement counter? ( Press Y or N) ");
	      get16chars(s);
	      if (s[0] == 'y' || s[1] == 'Y'){
		uval32 temp;
		hputs("Enter starting value for the counter: ");
		get16chars(s);
		startDecTime = strtoul(s, NULL, 0);
		
		hputs("Enter final value for the coount: ");
		get16chars(s);
		endDecTime = strtoul(s, NULL, 0);

		hputs("Enter Increment value for the counter: ");
		get16chars(s);
		decTimeIncr = strtoul(s, NULL, 0);

		hputs("How many iterations? (1 to 10000): ");
		get16chars(s);
		temp = strtoul(s, NULL, 0);
		if (temp <= MAX_ENTRIES) {
		  maxEntries = temp;
		}
		sprintf(s, "start = %d, final= %d, incr = %d, iterations =%d\n", 
			startDecTime, endDecTime, decTimeIncr, maxEntries);
		hputs(s);
	      }
	      else{
		startDecTime = DEFAULT_DEC_COUNT+1024;
		endDecTime = startDecTime + 3*64*1024;
		decTimeIncr = 64*1024;
		maxEntries = 200;
	      }
	      decr_count = startDecTime;
	      xh_table[XH_DEC] = xh_dec;

	      adjust_freq();
	      break;
	    }
	    disp_menu();
	    sprintf(s, "Enter Input (Dec= %ld): ", decr_count);
	    hputs(s); 
	  }

  return 1;
}





/*
 * Generates TK commands for bouncing ball and sends to
 * stdout.
 */

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	uval32 temp, temp1;

	hputs("\n\n"
	      "This program uses this partition's VTTY channel 1.\n"
	      "Normal console is channel 0.\n"
	      "Therefore channel 1 is the console port + 1.\n"
	      "You can also look for a thinwire message like:\n"
	      "  \"thinwire: "
	      "awaiting connection on channel 33 (port 40634).\"\n"
	      "Which will indicate the port thinwire wants to use.\n\n"
	      "So to see the balls bounce you need to run:\n\n"
	      "  $ console <thinwire host>:<console port + 1> | "
	      "cat - | wish -\n"
	      "  $ console <thinwire host>:<console port + 2> | "
	      "cat - | wish -\n\n" );
	int ballNumber;

	
	/* initialize dec ball screen */
	decBall.channel = 1;
	decBall.color = BALL0COLOR;
        init_screen(&decBall);
	init_object(&decBall, 1);
        send_to_out("\v", decBall.channel);

	/* initialize screen 1 */
        ball[0].channel =  2;
  	ball[0].color  = BALL1COLOR;
	init_screen(&ball[0]);
	init_object(&ball[0], 1);
        send_to_out("\v", ball[0].channel);

	/* initialize additional balls in screen 1 */
	for (ballNumber=1; ballNumber < MAXBALLS; ballNumber++)	  {
	     ball[ballNumber].channel =  2;
  	     ball[ballNumber].color  = BALL1COLOR;
	     init_object(&ball[ballNumber], ballNumber+1);
	  }

	/* we are ready to receive decrement interrupt, 
	 * set our receiver address in xh_table 
	 */

	do {
	  temp = mftbl();
	  xh_dec(0, NULL);
	  temp1 = mftbl();
	  dec_processing_time = temp1 - temp + 1000;
	}while(temp1 < temp);  /* in case the timer low value been reset, redo it */

	sampleIndex = 0;
	totalProcessed = 0;
	test_on = 0;
	results_ready = 0;
	xh_table[XH_DEC] = xh_dec;


	
	disp_menu();

	/*
	 * Read, munge, and send data to stdout until
	 * some termination msg is received
	 */
	
	while(!exit_app && results_ready == 0){
	   for (ballNumber=0; ballNumber < MAXBALLS; ballNumber++){
		update_position(&ball[ballNumber]);
		update_display(&ball[ballNumber]);
		}
	  process_input();
	  
	  
	}
	

	xh_table[XH_DEC] = NULL;


	  GenerateGraph();



	/* Done, clean up by telling wish to exit */
	for (ballNumber=0; ballNumber < MAXBALLS; ballNumber++)	  
	  exit_wish(&ball[ballNumber]);	  
	
	exit_wish(&decBall);

	return 0;
}


