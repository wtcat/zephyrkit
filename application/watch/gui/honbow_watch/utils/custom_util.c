#include "gx_api.h"

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