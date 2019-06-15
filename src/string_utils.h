#ifndef INCLUDE_STRING_UTILS
#define INCLUDE_STRING_UTILS

#if 0
printf("[lll][llll*] : %d\n", string_contains("lll", "llll*"));
printf("[lllll][l*lop] : %d\n", string_contains("lllll", "l*lop"));
printf("[22lllll][l*l] : %d\n", string_contains("22lllll", "l*l"));

printf("[hello world][h?lo] : %d\n", string_contains("hello world", "h?lo"));
printf("[\"hello sailor\"][sailor] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "sailor"));
printf("[\"hello sailor\"][*sailor] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "*sailor"));
printf("\n");

printf("[\"hello sailor\"][*sailor\"] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "*sailor\""));
printf("[\"hello sailor\"][*sailor*] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "*sailor*"));
printf("[\"hello sailor\"][sailor*] : %d\n", string_contains(" wsdf asd \"hello sailor\" asdf asdf ", "sailor*"));
printf("[22lllll pi23hjp rbksje LSKJDh l][LS*] : %d\n",
	   string_contains("22lllll pi23hjp rbksje LSKJDh l", "LS*"));
printf("[22lllll lal][l*l] : %d\n", string_contains("22lllll lal", "l*l"));
printf("[22lllll][*l*l] : %d\n", string_contains("lllll", "*l*l"));
printf("[hello world][hello] : %d\n", string_contains("hello world", "hello"));
printf("[hello world][h?llo] : %d\n", string_contains("hello world", "h?llo"));
printf("[hello world][h????] : %d\n", string_contains("hello world", "h????"));
printf("[hello world][h*lo] : %d\n", string_contains("hello world", "h*lo"));
printf("[hello world][*] : %d\n", string_contains("hello world", "*"));
printf("[hello world][h*] : %d\n", string_contains("hello world", "h*"));
printf("[hello world][*o] : %d\n", string_contains("hello world", "*o"));
printf("[hello world][h*o] : %d\n", string_contains("hello world", "h*o"));
printf("[hello world][*lo] : %d\n", string_contains("hello world", "*lo"));
printf("[hello world][hel*lo] : %d\n", string_contains("hello world", "hel*lo"));

printf("[lllll][l*l] : %d\n", string_contains("lllll", "l*l"));
printf("[llllllll][l*llll] : %d\n", string_contains("lllll", "l*llll"));
printf("[llllllll][l*lll] : %d\n", string_contains("lllll", "l*lll"));
printf("[llllllll][llll*l] : %d\n", string_contains("lllll", "llll*l"));
printf("[llllllll][*] : %d\n", string_contains("lllll", "*"));
printf("[lllll][l?lll] : %d\n", string_contains("lllll", "l?lll"));

printf("[lllll][lllll] : %d\n", string_contains("lllll", "lllll"));
printf("[lllll][*llll] : %d\n", string_contains("lllll", "*llll"));
printf("[lllll][llll*] : %d\n", string_contains("lllll", "llll*"));
printf("[lllll][*llll*] : %d\n", string_contains("lllll", "*llll*"));
printf("[lllll][*lllll*] : %d\n", string_contains("lllll", "*lllll*"));
printf("[lllll][*ll*] : %d\n", string_contains("lllll", "*ll*"));
#endif

u8 string_match(char *first, char *second);
u8 string_contains(char *big, char *small);
void string_trim(char *string);

#endif