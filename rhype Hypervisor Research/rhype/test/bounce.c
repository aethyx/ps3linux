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
 *
 */
#include <test.h>
#include <partition.h>
#include <sysipc.h>
#include <xh.h>

#define CANVAS_WIDTH 450
#define CANVAS_HEIGHT 355

#define OBJECT1 "bounce1"
#define OBJECT2 "bounce2"
#define DELAY_OBJ "delay1"
#define DELAYMAX 4

#define OBJECT1_DIAM 30
#define OBJECT2_DIAM 20

#define DATAFILE "colorcmds"

#define TRUE 1

#define MAXCOLOR 200
#define MAXLENGTH 256

#define SCREENCOLOR "black"
#define BALL1COLOR  "yellow"

int	ball_count;
int	nodelay = 1;

char tkoutbuffer[MAXLENGTH];

int x_left, x_right, y_upper, y_lower, delta_x, delta_y;
int x_left_delay, x_right_delay, y_upper_delay, y_lower_delay;

/*
 * Write to the string to stdout
 */
static void
send_to_out(const char *tk_cmd)
{
	uval len;

	len = strlen(tk_cmd);
	hcall_put_term_string(1, len, tk_cmd);
}

/*
 * TK commands to setup initial graphic screen 
 */
static void
init_screen(void)
{
	sprintf(tkoutbuffer, "canvas .screen -width %i -height %i\n",
		CANVAS_WIDTH, CANVAS_HEIGHT);
	send_to_out(tkoutbuffer);
	sprintf(tkoutbuffer, ".screen config -background %s\n", SCREENCOLOR);
	send_to_out(tkoutbuffer);
	sprintf(tkoutbuffer, "pack .screen\n");
	send_to_out(tkoutbuffer);
}

/*
 * Create graphic object
 */
static void
init_object(void)
{
	/* Set initial position */
	x_left = 0;
	x_right = OBJECT1_DIAM;
	y_upper = 0;
	y_lower = OBJECT1_DIAM;
	
	/* Set change delta's */
	delta_x = 2;
	delta_y = 4;
	
	sprintf(tkoutbuffer,
		".screen create oval %i %i %i %i "
		"-tags %s -fill %s -outline %s\n",
		x_left, y_upper,
		x_right, y_lower,
		OBJECT1,
		BALL1COLOR,
		BALL1COLOR);
	send_to_out(tkoutbuffer);
}


/*
 * Update the change to the object's current position
 */
static void
update_position(void)
{
	int x_left_new;
	int x_right_new;
	int y_upper_new;
	int y_lower_new;

	x_left_new = x_left + delta_x;
	x_right_new = x_right + delta_x;
	y_upper_new = y_upper + delta_y;
	y_lower_new = y_lower + delta_y;

	if ((x_left_new < 0) || (x_right_new > CANVAS_WIDTH)) {
		delta_x = delta_x * -1;
		x_left_new = x_left + delta_x;
		x_right_new = x_right + delta_x;
	}
	if ((y_upper_new < 0) || (y_lower_new > CANVAS_HEIGHT)) {
		delta_y = delta_y * -1;
		y_upper_new = y_upper + delta_y;
		y_lower_new = y_lower + delta_y;
	}
	x_left = x_left_new;
	x_right = x_right_new;
	y_upper = y_upper_new;
	y_lower = y_lower_new;
}


/*
 * update_display: Generate command to update object position on screen
 */
static void
update_display(void)
{

	/* No color change, just move existing object */
	sprintf(tkoutbuffer, ".screen move %s %i %i\n",
		OBJECT1,
		delta_x,
		delta_y);

	send_to_out(tkoutbuffer);

	sprintf(tkoutbuffer, "update idletasks\n");
	send_to_out(tkoutbuffer);
}

/*
 * Create swelling object
 */
static void
init_delay_object(void)
{
        /* Set initial position */
        x_left_delay = CANVAS_WIDTH + 5;
        x_right_delay = CANVAS_WIDTH + 15;
        y_upper_delay = CANVAS_HEIGHT + 5;
        y_lower_delay = CANVAS_HEIGHT + 15;

        sprintf(tkoutbuffer,
		".screen create oval %i %i %i %i "
		"-outline black -tags %s -fill green\n",
                x_left_delay,
                y_upper_delay,
                x_right_delay,
                y_lower_delay,
                DELAY_OBJ);
        send_to_out(tkoutbuffer);
}

static void
update_delay_object(void)
{
	int count;
	int dcnt;
	int d_delta = 2;

	if (nodelay) {
		return;
	}

	for (dcnt = 0; dcnt < 2; dcnt++) {
		for (count = 0; count < DELAYMAX; count++) {
			x_left_delay = x_left_delay - d_delta;
			x_right_delay = x_right_delay + d_delta;
			y_upper_delay = y_upper_delay - d_delta;
			y_lower_delay = y_lower_delay + d_delta;
			
			/* Delete old object */
			sprintf(tkoutbuffer, ".screen delete %s\n", DELAY_OBJ);
			send_to_out(tkoutbuffer);

			/* Create new object with new dimensions */
			sprintf(tkoutbuffer,
				".screen create oval %i %i %i %i "
				"-outline black -tags %s -fill black\n",
				x_left_delay,
				y_upper_delay,
				x_right_delay,
				y_lower_delay,
				DELAY_OBJ);
			send_to_out(tkoutbuffer);
		}
		/* reverse sign on d_delta, so object will shrink */
		d_delta = d_delta * -1;
	}
}

/*
 * exit_wish: Send "exit" command to wish, so it will terminate
 */

static void
exit_wish(void)
{
	sprintf(tkoutbuffer, "exit\n");
	send_to_out(tkoutbuffer);
}

/*
 * Generates TK commands for bouncing ball and sends to
 * stdout.
 */

uval
test_os(uval argc __attribute__ ((unused)),
	uval argv[] __attribute__ ((unused)))
{
	int	move;

	hputs("\n\n"
	      "This program uses this partition's VTTY channel 1.\n"
	      "Normal console is channel 0.\n"
	      "Therefore channel 1 is the console port + 1.\n"
	      "You can also look for a thinwire message like:\n"
	      "  \"thinwire: "
	      "awaiting connection on channel 33 (port 40634).\"\n"
	      "Which will indicate the port thinwire wants to use.\n\n"
	      "So do see the balls bounce you need to run:\n"
	      "  $ console <thinwire host>:<console port + 1> | "
	      "cat - | wish -\n" );

	send_to_out("\v");

	/*
	 * Read, munge, and send data to stdout until
	 * some termination msg is received
	 */
	init_screen();
	init_object();

	/* Delay, so ball does not move too fast */
	init_delay_object();

/* #define BOUNCE_MOVE_LIMIT 5000 */
#if BOUNCE_MOVE_LIMIT > 0 
	for (move = 0; move < BOUNCE_MOVE_LIMIT; move++) {
#else
	move = move;
	for (;;) {
#endif		
		update_position();
		update_display();
		update_delay_object();
	}

	/* Done, clean up by telling wish to exit */
	exit_wish();
	return 0;
}

