/***************************************************************************
 * MUD 32 colors 1.6 by Igor van den Hoven.                                *
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdlib.h>
#include "color.h"



int substitute_color(char *input, char *output, int colors)
{
	char *pti, *pto, old_f[7] = { 0 }, old_b[7] = { 0 };

	pti = input;
	pto = output;

	while (*pti)
	{
		switch (*pti)
		{
			case '^':
				if (is_32c(pti[1]))
				{
					if (pti[2] == '^' && is_32c(pti[3]))
					{
						pti += 2;
						continue;
					}

					if (colors)
					{
						if (pti[1] == '?')
						{
							pto += substitute_color(tc_rnd(), pto, colors);
						}
						else if (strncmp(old_f, pti, 2))
						{
							if (pti[1] >= 'a' && pti[1] <= 'z')
							{
								pto += substitute_color(alphabet_fgc_dark[pti[1] - 'a'], pto, colors < 256 ? colors : 256);
							}
							else
							{
								pto += substitute_color(alphabet_fgc_bold[pti[1] - 'A'], pto, colors < 256 ? colors : 256);
							}
						}
					}
					pti += sprintf(old_f, "%c%c", pti[0], pti[1]);
				}
				else
				{
					if (pti[1] == '^')
					{
						pti++;
					}
					*pto++ = *pti++;
				}
				break;

			case '<':
				if (toupper((int) pti[1]) == 'F' && isxdigit((int) pti[2]) && isxdigit((int) pti[3]) && isxdigit((int) pti[4]) && pti[5] == '>')
				{
					if (strncasecmp(old_f, pti, 6) && colors)
					{
						if (colors == 4096)
						{
							pto += sprintf(pto, "\e[38;2;%d;%d;%dm", tc_val(pti[2]), tc_val(pti[3]), tc_val(pti[4]));
						}
						else if (colors == 256)
						{
							pto += sprintf(pto, "\033[38;5;%dm", 16 + x_256c_val(pti[2]) * 36 + x_256c_val(pti[3]) * 6 + x_256c_val(pti[4]));
						}
						else
						{
							pto += substitute_color(ansi_colors_f[16 + x_256c_val(pti[2]) * 36 + x_256c_val(pti[3]) * 6 + x_256c_val(pti[4])], pto, colors);
						}
					}
					pti += sprintf(old_f, "<F%c%c%c>", pti[2], pti[3], pti[4]);
				}
				else if (toupper((int) pti[1]) == 'B' && isxdigit((int) pti[2]) && isxdigit((int) pti[3]) && isxdigit((int) pti[4]) && pti[5] == '>')
				{
					if (strncasecmp(old_b, pti, 6) && colors)
					{
						if (colors == 4096)
						{
							pto += sprintf(pto, "\e[48;2;%d;%d;%dm", tc_val(pti[2]), tc_val(pti[3]), tc_val(pti[4]));
						}
						else if (colors == 256)
						{
							pto += sprintf(pto, "\033[48;5;%dm", 16 + x_256c_val(pti[2]) * 36 + x_256c_val(pti[3]) * 6 + x_256c_val(pti[4]));
						}
						else
						{
							pto += substitute_color(ansi_colors_b[16 + x_256c_val(pti[2]) * 36 + x_256c_val(pti[3]) * 6 + x_256c_val(pti[4])], pto, colors);
						}
					}
					pti += sprintf(old_b, "<F%c%c%c>", pti[2], pti[3], pti[4]);
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			default:
				*pto++ = *pti++;
				break;
		}
	}
	*pto = 0;

	return pto - output;
}

int main(int argc, char **argv)
{
	char in[2000], out[12000];
	int cnt, rnd;

	strcpy(in,
		"\n^W"
		"     ^^a - ^adark azure            ^W^^A - ^Aazure^W\n"
		"     ^^b - ^bdark blue             ^W^^B - ^Bblue^W\n"
		"     ^^c - ^cdark cyan             ^W^^C - ^Ccyan^W\n"
		"     ^^e - ^edark ebony            ^W^^E - ^Eebony^W\n"
		"     ^^g - ^gdark green            ^W^^G - ^Ggreen^W\n"
		"     ^^j - ^jdark jade             ^W^^J - ^Jjade^W\n"
		"     ^^l - ^ldark lime             ^W^^L - ^Llime^W\n"
		"     ^^m - ^mdark magenta          ^W^^M - ^Mmagenta^W\n"
		"     ^^o - ^odark orange           ^W^^O - ^Oorange^W\n"
		"     ^^p - ^pdark pink             ^W^^P - ^Ppink^W\n"
		"     ^^r - ^rdark red              ^W^^R - ^Rred^W\n"
		"     ^^s - ^sdark silver           ^W^^S - ^Ssilver^W\n"
		"     ^^t - ^tdark tan              ^W^^T - ^Ttan^W\n"
		"     ^^v - ^vdark violet           ^W^^V - ^Vviolet^W\n"
		"     ^^w - ^wdark white            ^W^^W - ^Wwhite^W\n"
		"     ^^y - ^ydark yellow           ^W^^Y - ^Yyellow^W\n");

	substitute_color(in, out, 4096);
	printf("%s\n", out);

	srand(time(NULL));

	rnd = rand();

	// init

	for (cnt = 0 ; cnt < 25 ; cnt++)
	{
		strcpy(in + cnt * 4, "^?##");
	}

	srand(rnd);

	printf("\n\e[1;37mrandom 4096 color values.\n\n");

	for (cnt = 0 ; cnt < 6 ; cnt++)
	{
		substitute_color(in, out, 4096);

		printf("%s\n", out);
	}

	srand(rnd);

	printf("\n\e[1;37mrandom 4096 color values downgraded to 256.\n\n");

	for (cnt = 0 ; cnt < 6 ; cnt++)
	{
		substitute_color(in, out, 256);

		printf("%s\n", out);
	}

	srand(rnd);

	printf("\n\e[1;37mrandom 4096 color values downgraded to 16.\n\n");

	for (cnt = 0 ; cnt < 6 ; cnt++)
	{
		substitute_color(in, out, 16);

		printf("%s\n", out);
	}

	return 0;
}
