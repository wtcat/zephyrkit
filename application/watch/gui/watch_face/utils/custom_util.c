#include "gx_api.h"

/*************************************************************************************/
VOID screen_switch(GX_WIDGET *parent, GX_WIDGET **current_screen,
				   GX_WIDGET *new_screen)
{
	gx_widget_detach(*current_screen);
	gx_widget_attach(parent, new_screen);
	*current_screen = new_screen;
}

/*************************************************************************************/
UINT string_length_get(GX_CONST GX_CHAR *input_string, UINT max_string_length)
{
	UINT length = 0;

	if (input_string) {
		/* Traverse the string.  */
		for (length = 0; input_string[length]; length++) {
			/* Check if the string length is bigger than the max string length.
			 */
			if (length >= max_string_length) {
				break;
			}
		}
	}

	return length;
}