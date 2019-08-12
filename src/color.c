/***************************************************************************
 * Permission is hereby granted to use this code without restrictions as   *
 * long as this permission and the copyright notice below is included.     *
 *                                                                         *
 * Mud Telopt Handler 1.5 Copyright 2009-2019 Igor van den Hoven.          *
 ***************************************************************************/

/*
	For xterm 256 color foreground colors use:
	
	<aaa> to <fff> for RGB foreground colors

	<g00> to <g23> for grayscale foreground colors


	For xterm 256 color background colors use:

	<AAA> to <FFF> for RGB background colors

	<G00> to <G23> for grayscale background colors

	Background color support is commented out by default


	With 256 colors disabled colors are converted to 16 color ANSI.
*/

/*
	For 32 color codes use:

	^a - dark azure                 ^A - azure
	^b - dark blue                  ^B - blue
	^c - dark cyan                  ^C - cyan
	^e - dark ebony                 ^E - ebony
	^g - dark green                 ^G - green
	^j - dark jade                  ^J - jade
	^l - dark lime                  ^L - lime
	^m - dark magenta               ^M - magenta
	^o - dark orange                ^O - orange
	^p - dark pink                  ^P - pink
	^r - dark red                   ^R - red
	^s - dark silver                ^S - silver
	^t - dark tan                   ^T - tan
	^v - dark violet                ^V - violet
	^w - dark white                 ^W - white
	^y - dark yellow                ^Y - yellow


	With 256 colors disabled colors are converted to 16 color ANSI.
*/

#include <stdio.h>
#include <string.h>

/*
	256 to 16 color conversion table
*/

char *ansi_colors[256] =
{
	"\e[0;30m", "\e[0;31m", "\e[0;32m", "\e[0;33m", "\e[0;34m", "\e[0;35m", "\e[0;36m", "\e[0;37m",
	"\e[1;30m", "\e[1;31m", "\e[1;32m", "\e[1;33m", "\e[1;34m", "\e[1;35m", "\e[1;36m", "\e[1;37m",

	"\e[0;30m", "\e[0;34m", "\e[0;34m", "\e[0;34m", "\e[1;34m", "\e[1;34m",
	"\e[0;32m", "\e[0;36m", "\e[0;36m", "\e[0;34m", "\e[1;34m", "\e[1;34m",
	"\e[0;32m", "\e[0;36m", "\e[0;36m", "\e[0;36m", "\e[1;34m", "\e[1;34m",
	"\e[0;32m", "\e[0;32m", "\e[0;36m", "\e[0;36m", "\e[0;36m", "\e[1;36m",
	"\e[1;32m", "\e[1;32m", "\e[1;32m", "\e[0;36m", "\e[1;36m", "\e[1;36m",
	"\e[1;32m", "\e[1;32m", "\e[1;32m", "\e[1;36m", "\e[1;36m", "\e[1;36m",

	"\e[0;31m", "\e[0;35m", "\e[0;35m", "\e[0;34m", "\e[1;34m", "\e[1;34m",
	"\e[0;33m", "\e[1;30m", "\e[0;34m", "\e[0;34m", "\e[1;34m", "\e[1;34m",
	"\e[0;33m", "\e[0;32m", "\e[0;36m", "\e[0;36m", "\e[1;34m", "\e[1;34m",
	"\e[0;32m", "\e[0;32m", "\e[0;36m", "\e[0;36m", "\e[0;36m", "\e[1;36m",
	"\e[1;32m", "\e[1;32m", "\e[1;32m", "\e[0;36m", "\e[1;36m", "\e[1;36m",
	"\e[1;32m", "\e[1;32m", "\e[1;32m", "\e[1;36m", "\e[1;36m", "\e[1;36m",

	"\e[0;31m", "\e[0;35m", "\e[0;35m", "\e[0;35m", "\e[1;34m", "\e[1;34m",
	"\e[0;33m", "\e[0;31m", "\e[0;35m", "\e[0;35m", "\e[1;34m", "\e[1;34m",
	"\e[0;33m", "\e[0;33m", "\e[0;37m", "\e[0;34m", "\e[1;34m", "\e[1;34m",
	"\e[0;33m", "\e[0;33m", "\e[0;32m", "\e[0;36m", "\e[0;36m", "\e[1;34m",
	"\e[1;32m", "\e[1;32m", "\e[1;32m", "\e[0;36m", "\e[1;36m", "\e[1;36m",
	"\e[1;32m", "\e[1;32m", "\e[1;32m", "\e[1;32m", "\e[1;36m", "\e[1;36m",

	"\e[0;31m", "\e[0;31m", "\e[0;35m", "\e[0;35m", "\e[0;35m", "\e[1;35m",
	"\e[0;31m", "\e[0;31m", "\e[0;35m", "\e[0;35m", "\e[0;35m", "\e[1;35m",
	"\e[0;33m", "\e[0;33m", "\e[0;31m", "\e[0;35m", "\e[0;35m", "\e[1;34m",
	"\e[0;33m", "\e[0;33m", "\e[0;33m", "\e[0;37m", "\e[1;34m", "\e[1;34m",
	"\e[0;33m", "\e[0;33m", "\e[0;33m", "\e[1;32m", "\e[1;36m", "\e[1;36m",
	"\e[1;33m", "\e[1;33m", "\e[1;32m", "\e[1;32m", "\e[1;36m", "\e[1;36m",

	"\e[1;31m", "\e[1;31m", "\e[1;31m", "\e[0;35m", "\e[1;35m", "\e[1;35m",
	"\e[1;31m", "\e[1;31m", "\e[1;31m", "\e[0;35m", "\e[1;35m", "\e[1;35m",
	"\e[1;31m", "\e[1;31m", "\e[1;31m", "\e[0;35m", "\e[1;35m", "\e[1;35m",
	"\e[0;33m", "\e[0;33m", "\e[0;33m", "\e[1;31m", "\e[1;35m", "\e[1;35m",
	"\e[1;33m", "\e[1;33m", "\e[1;33m", "\e[1;33m", "\e[1;37m", "\e[1;37m",
	"\e[1;33m", "\e[1;33m", "\e[1;33m", "\e[1;33m", "\e[1;37m", "\e[1;37m",

	"\e[1;31m", "\e[1;31m", "\e[1;31m", "\e[1;35m", "\e[1;35m", "\e[1;35m",
	"\e[1;31m", "\e[1;31m", "\e[1;31m", "\e[1;35m", "\e[1;35m", "\e[1;35m",
	"\e[1;31m", "\e[1;31m", "\e[1;31m", "\e[1;31m", "\e[1;35m", "\e[1;35m",
	"\e[1;33m", "\e[1;33m", "\e[1;31m", "\e[1;31m", "\e[1;35m", "\e[1;35m",
	"\e[1;33m", "\e[1;33m", "\e[1;33m", "\e[1;33m", "\e[1;37m", "\e[1;37m",
	"\e[1;33m", "\e[1;33m", "\e[1;33m", "\e[1;33m", "\e[1;37m", "\e[1;37m",

	"\e[1;30m", "\e[1;30m", "\e[1;30m", "\e[1;30m", "\e[1;30m", "\e[1;30m",
	"\e[1;30m", "\e[1;30m", "\e[1;30m", "\e[1;30m", "\e[1;30m", "\e[1;30m",
	"\e[0;37m", "\e[0;37m", "\e[0;37m", "\e[0;37m", "\e[0;37m", "\e[0;37m",
	"\e[1;37m", "\e[1;37m", "\e[1;37m", "\e[1;37m", "\e[1;37m", "\e[1;37m"
};

/*
	32 to 256 color conversion table
*/

char *alphabet_colors_dark[26] =
{
	"<abd>", "<aad>", "<add>", "", "<g04>", "", "<ada>", "", "", "<adb>", "", "<bda>", "<dad>",
	"", "<dba>", "<dab>", "", "<daa>", "<ccc>", "<cba>", "", "<bad>", "<ddd>", "", "<dda>", ""
};

char *alphabet_colors_bold[26] =
{
	"<acf>", "<aaf>", "<aff>", "", "<bbb>", "", "<afa>", "", "", "<afc>", "", "<cfa>", "<faf>",
	"", "<fca>", "<fac>", "", "<faa>", "<eee>", "<eda>", "", "<caf>", "<fff>", "", "<ffa>", ""
};


// Make sure that the output buffer remains at least 4 times larger than the user input buffer.

// colors should either be 0 for no colors, 16 for ansi colors, or 256 for xterm 256 colors.

int substitute_color(char *input, char *output, int colors)
{
	char *pti, *pto, old[6] = { 0 };

	pti = input;
	pto = output;

	while (*pti)
	{
		switch (*pti)
		{
			case '^':
				if (isalpha(pti[1]))
				{
					if (pti[2] == '^' && isalpha(pti[3]))
					{
						pti += 2;
						continue;
					}

					if (strncmp(old, pti, 2) && colors)
					{
						if (pti[1] >= 'a' && pti[1] <= 'z')
						{
							pto += substitute_color(alphabet_colors_dark[pti[1] - 'a'], pto, colors);
						}
						else
						{
							pto += substitute_color(alphabet_colors_bold[pti[1] - 'A'], pto, colors);
						}
					}
					pti += sprintf(old, "%c%c", pti[0], pti[1]);
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
				if (pti[1] >= 'a' && pti[1] <= 'f' && pti[2] >= 'a' && pti[2] <= 'f' && pti[3] >= 'a' && pti[3] <= 'f' && pti[4] == '>')
				{
					if (strncmp(old, pti, 5) && colors)
					{
						if (colors == 256)
						{
							pto += sprintf(pto, "\033[38;5;%dm", 16 + (pti[1] - 'a') * 36 + (pti[2] - 'a') * 6 + (pti[3] - 'a'));
						}
						else
						{
							pto += substitute_color(ansi_colors[16 + (pti[1] - 'a') * 36 + (pti[2] - 'a') * 6 + (pti[3] - 'a')], pto, colors);
						}
					}
					pti += sprintf(old, "<%c%c%c>", pti[1], pti[2], pti[3]);
				}
				else if (pti[1] == 'g' && isdigit((int) pti[2]) && isdigit((int) pti[3]) && (pti[2] - '0') * 10 + (pti[3] - '0') >= 0 && (pti[2] - '0') * 10 + (pti[3] - '0') < 24 && pti[4] == '>')
				{
					if (strncmp(old, pti, 5) && colors)
					{
						if (colors == 256)
						{
							pto += sprintf(pto, "\033[38;5;%dm", 232 + (pti[2] - '0') * 10 + (pti[3] - '0'));
						}
						else
						{
							pto += substitute_color(ansi_colors[232 + (pti[2] - '0') * 10 + (pti[3] - '0')], pto, colors);
						}
					}
					pti += sprintf(old, "<%c%c%c>", pti[1], pti[2], pti[3]);
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
