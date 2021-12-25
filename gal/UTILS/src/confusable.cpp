// ReSharper disable CommentTypo
// ReSharper disable StringLiteralTypo
#include <utils/confusable.hpp>

#include <compare>
#include <algorithm>

namespace
{
	struct confusable
	{
		unsigned codepoint;
		char text[5];

		friend constexpr bool operator==(const confusable& lhs, const confusable& rhs) noexcept
		{
			return lhs.codepoint == rhs.codepoint;
		}

		friend constexpr auto operator<=>(const confusable& lhs, const confusable& rhs) noexcept
		{
			return lhs.codepoint <=> rhs.codepoint;
		}
	};

	// https://www.unicode.org/Public/security/latest/confusables.txt
	constexpr confusable g_confusable[] =
	{
			{34, "\""},     // MA#* ( " ¡ú '' ) QUOTATION MARK ¡ú APOSTROPHE, APOSTROPHE# # Converted to a quote.
			{48, "O"},      // MA# ( 0 ¡ú O ) DIGIT ZERO ¡ú LATIN CAPITAL LETTER O#
			{49, "l"},      // MA# ( 1 ¡ú l ) DIGIT ONE ¡ú LATIN SMALL LETTER L#
			{73, "l"},      // MA# ( I ¡ú l ) LATIN CAPITAL LETTER I ¡ú LATIN SMALL LETTER L#
			{96, "'"},      // MA#* ( ` ¡ú ' ) GRAVE ACCENT ¡ú APOSTROPHE# ¡ú¨A¡ú¡ú£à¡ú¡ú¡®¡ú
			{109, "rn"},    // MA# ( m ¡ú rn ) LATIN SMALL LETTER M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N#
			{124, "l"},     // MA#* ( | ¡ú l ) VERTICAL LINE ¡ú LATIN SMALL LETTER L#
			{160, " "},     // MA#* ( ? ¡ú   ) NO-BREAK SPACE ¡ú SPACE#
			{180, "'"},     // MA#* ( ? ¡ú ' ) ACUTE ACCENT ¡ú APOSTROPHE# ¡ú?¡ú¡ú?¡ú
			{184, ","},     // MA#* ( ? ¡ú , ) CEDILLA ¡ú COMMA#
			{198, "AE"},    // MA# ( ? ¡ú AE ) LATIN CAPITAL LETTER AE ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER E#
			{215, "x"},     // MA#* ( ¡Á ¡ú x ) MULTIPLICATION SIGN ¡ú LATIN SMALL LETTER X#
			{230, "ae"},    // MA# ( ? ¡ú ae ) LATIN SMALL LETTER AE ¡ú LATIN SMALL LETTER A, LATIN SMALL LETTER E#
			{305, "i"},     // MA# ( ? ¡ú i ) LATIN SMALL LETTER DOTLESS I ¡ú LATIN SMALL LETTER I#
			{306, "lJ"},    // MA# ( ? ¡ú lJ ) LATIN CAPITAL LIGATURE IJ ¡ú LATIN SMALL LETTER L, LATIN CAPITAL LETTER J# ¡úIJ¡ú
			{307, "ij"},    // MA# ( ? ¡ú ij ) LATIN SMALL LIGATURE IJ ¡ú LATIN SMALL LETTER I, LATIN SMALL LETTER J#
			{329, "'n"},    // MA# ( ? ¡ú 'n ) LATIN SMALL LETTER N PRECEDED BY APOSTROPHE ¡ú APOSTROPHE, LATIN SMALL LETTER N# ¡ú?n¡ú
			{338, "OE"},    // MA# ( ? ¡ú OE ) LATIN CAPITAL LIGATURE OE ¡ú LATIN CAPITAL LETTER O, LATIN CAPITAL LETTER E#
			{339, "oe"},    // MA# ( ? ¡ú oe ) LATIN SMALL LIGATURE OE ¡ú LATIN SMALL LETTER O, LATIN SMALL LETTER E#
			{383, "f"},     // MA# ( ? ¡ú f ) LATIN SMALL LETTER LONG S ¡ú LATIN SMALL LETTER F#
			{385, "'B"},    // MA# ( ? ¡ú 'B ) LATIN CAPITAL LETTER B WITH HOOK ¡ú APOSTROPHE, LATIN CAPITAL LETTER B# ¡ú?B¡ú
			{388, "b"},     // MA# ( ? ¡ú b ) LATIN CAPITAL LETTER TONE SIX ¡ú LATIN SMALL LETTER B#
			{391, "C'"},    // MA# ( ? ¡ú C' ) LATIN CAPITAL LETTER C WITH HOOK ¡ú LATIN CAPITAL LETTER C, APOSTROPHE# ¡úC?¡ú
			{394, "'D"},    // MA# ( ? ¡ú 'D ) LATIN CAPITAL LETTER D WITH HOOK ¡ú APOSTROPHE, LATIN CAPITAL LETTER D# ¡ú?D¡ú
			{397, "g"},     // MA# ( ? ¡ú g ) LATIN SMALL LETTER TURNED DELTA ¡ú LATIN SMALL LETTER G#
			{403, "G'"},    // MA# ( ? ¡ú G' ) LATIN CAPITAL LETTER G WITH HOOK ¡ú LATIN CAPITAL LETTER G, APOSTROPHE# ¡úG?¡ú
			{406, "l"},     // MA# ( ? ¡ú l ) LATIN CAPITAL LETTER IOTA ¡ú LATIN SMALL LETTER L#
			{408, "K'"},    // MA# ( ? ¡ú K' ) LATIN CAPITAL LETTER K WITH HOOK ¡ú LATIN CAPITAL LETTER K, APOSTROPHE# ¡úK?¡ú
			{416, "O'"},    // MA# ( ? ¡ú O' ) LATIN CAPITAL LETTER O WITH HORN ¡ú LATIN CAPITAL LETTER O, APOSTROPHE# ¡úO?¡ú
			{417, "o'"},    // MA# ( ? ¡ú o' ) LATIN SMALL LETTER O WITH HORN ¡ú LATIN SMALL LETTER O, APOSTROPHE# ¡úo?¡ú
			{420, "'P"},    // MA# ( ? ¡ú 'P ) LATIN CAPITAL LETTER P WITH HOOK ¡ú APOSTROPHE, LATIN CAPITAL LETTER P# ¡ú?P¡ú
			{422, "R"},     // MA# ( ? ¡ú R ) LATIN LETTER YR ¡ú LATIN CAPITAL LETTER R#
			{423, "2"},     // MA# ( ? ¡ú 2 ) LATIN CAPITAL LETTER TONE TWO ¡ú DIGIT TWO#
			{428, "'T"},    // MA# ( ? ¡ú 'T ) LATIN CAPITAL LETTER T WITH HOOK ¡ú APOSTROPHE, LATIN CAPITAL LETTER T# ¡ú?T¡ú
			{435, "'Y"},    // MA# ( ? ¡ú 'Y ) LATIN CAPITAL LETTER Y WITH HOOK ¡ú APOSTROPHE, LATIN CAPITAL LETTER Y# ¡ú?Y¡ú
			{439, "3"},     // MA# ( ? ¡ú 3 ) LATIN CAPITAL LETTER EZH ¡ú DIGIT THREE#
			{444, "5"},     // MA# ( ? ¡ú 5 ) LATIN CAPITAL LETTER TONE FIVE ¡ú DIGIT FIVE#
			{445, "s"},     // MA# ( ? ¡ú s ) LATIN SMALL LETTER TONE FIVE ¡ú LATIN SMALL LETTER S#
			{448, "l"},     // MA# ( ? ¡ú l ) LATIN LETTER DENTAL CLICK ¡ú LATIN SMALL LETTER L#
			{449, "ll"},    // MA# ( ? ¡ú ll ) LATIN LETTER LATERAL CLICK ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡ú¡¬¡ú¡ú¡Î¡ú¡ú||¡ú
			{451, "!"},     // MA# ( ? ¡ú ! ) LATIN LETTER RETROFLEX CLICK ¡ú EXCLAMATION MARK#
			{455, "LJ"},    // MA# ( ? ¡ú LJ ) LATIN CAPITAL LETTER LJ ¡ú LATIN CAPITAL LETTER L, LATIN CAPITAL LETTER J#
			{456, "Lj"},    // MA# ( ? ¡ú Lj ) LATIN CAPITAL LETTER L WITH SMALL LETTER J ¡ú LATIN CAPITAL LETTER L, LATIN SMALL LETTER J#
			{457, "lj"},    // MA# ( ? ¡ú lj ) LATIN SMALL LETTER LJ ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER J#
			{458, "NJ"},    // MA# ( ? ¡ú NJ ) LATIN CAPITAL LETTER NJ ¡ú LATIN CAPITAL LETTER N, LATIN CAPITAL LETTER J#
			{459, "Nj"},    // MA# ( ? ¡ú Nj ) LATIN CAPITAL LETTER N WITH SMALL LETTER J ¡ú LATIN CAPITAL LETTER N, LATIN SMALL LETTER J#
			{460, "nj"},    // MA# ( ? ¡ú nj ) LATIN SMALL LETTER NJ ¡ú LATIN SMALL LETTER N, LATIN SMALL LETTER J#
			{497, "DZ"},    // MA# ( ? ¡ú DZ ) LATIN CAPITAL LETTER DZ ¡ú LATIN CAPITAL LETTER D, LATIN CAPITAL LETTER Z#
			{498, "Dz"},    // MA# ( ? ¡ú Dz ) LATIN CAPITAL LETTER D WITH SMALL LETTER Z ¡ú LATIN CAPITAL LETTER D, LATIN SMALL LETTER Z#
			{499, "dz"},    // MA# ( ? ¡ú dz ) LATIN SMALL LETTER DZ ¡ú LATIN SMALL LETTER D, LATIN SMALL LETTER Z#
			{540, "3"},     // MA# ( ? ¡ú 3 ) LATIN CAPITAL LETTER YOGH ¡ú DIGIT THREE# ¡ú?¡ú
			{546, "8"},     // MA# ( ? ¡ú 8 ) LATIN CAPITAL LETTER OU ¡ú DIGIT EIGHT#
			{547, "8"},     // MA# ( ? ¡ú 8 ) LATIN SMALL LETTER OU ¡ú DIGIT EIGHT#
			{577, "?"},     // MA# ( ? ¡ú ? ) LATIN CAPITAL LETTER GLOTTAL STOP ¡ú QUESTION MARK# ¡ú?¡ú
			{593, "a"},     // MA# ( ¨» ¡ú a ) LATIN SMALL LETTER ALPHA ¡ú LATIN SMALL LETTER A#
			{609, "g"},     // MA# ( ¨À ¡ú g ) LATIN SMALL LETTER SCRIPT G ¡ú LATIN SMALL LETTER G#
			{611, "y"},     // MA# ( ? ¡ú y ) LATIN SMALL LETTER GAMMA ¡ú LATIN SMALL LETTER Y# ¡ú¦Ã¡ú
			{617, "i"},     // MA# ( ? ¡ú i ) LATIN SMALL LETTER IOTA ¡ú LATIN SMALL LETTER I#
			{618, "i"},     // MA# ( ? ¡ú i ) LATIN LETTER SMALL CAPITAL I ¡ú LATIN SMALL LETTER I# ¡ú?¡ú
			{623, "w"},     // MA# ( ? ¡ú w ) LATIN SMALL LETTER TURNED M ¡ú LATIN SMALL LETTER W#
			{651, "u"},     // MA# ( ? ¡ú u ) LATIN SMALL LETTER V WITH HOOK ¡ú LATIN SMALL LETTER U#
			{655, "y"},     // MA# ( ? ¡ú y ) LATIN LETTER SMALL CAPITAL Y ¡ú LATIN SMALL LETTER Y# ¡ú?¡ú¡ú¦Ã¡ú
			{660, "?"},     // MA# ( ? ¡ú ? ) LATIN LETTER GLOTTAL STOP ¡ú QUESTION MARK#
			{675, "dz"},    // MA# ( ? ¡ú dz ) LATIN SMALL LETTER DZ DIGRAPH ¡ú LATIN SMALL LETTER D, LATIN SMALL LETTER Z#
			{678, "ts"},    // MA# ( ? ¡ú ts ) LATIN SMALL LETTER TS DIGRAPH ¡ú LATIN SMALL LETTER T, LATIN SMALL LETTER S#
			{682, "ls"},    // MA# ( ? ¡ú ls ) LATIN SMALL LETTER LS DIGRAPH ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER S#
			{683, "lz"},    // MA# ( ? ¡ú lz ) LATIN SMALL LETTER LZ DIGRAPH ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER Z#
			{697, "'"},     // MA# ( ? ¡ú ' ) MODIFIER LETTER PRIME ¡ú APOSTROPHE#
			{698, "\""},    // MA# ( ? ¡ú '' ) MODIFIER LETTER DOUBLE PRIME ¡ú APOSTROPHE, APOSTROPHE# ¡ú"¡ú# Converted to a quote.
			{699, "'"},     // MA# ( ? ¡ú ' ) MODIFIER LETTER TURNED COMMA ¡ú APOSTROPHE# ¡ú¡®¡ú
			{700, "'"},     // MA# ( ? ¡ú ' ) MODIFIER LETTER APOSTROPHE ¡ú APOSTROPHE# ¡ú¡ä¡ú
			{701, "'"},     // MA# ( ? ¡ú ' ) MODIFIER LETTER REVERSED COMMA ¡ú APOSTROPHE# ¡ú¡®¡ú
			{702, "'"},     // MA# ( ? ¡ú ' ) MODIFIER LETTER RIGHT HALF RING ¡ú APOSTROPHE# ¡ú?¡ú¡ú¡ä¡ú
			{706, "<"},     // MA#* ( ? ¡ú < ) MODIFIER LETTER LEFT ARROWHEAD ¡ú LESS-THAN SIGN#
			{707, ">"},     // MA#* ( ? ¡ú > ) MODIFIER LETTER RIGHT ARROWHEAD ¡ú GREATER-THAN SIGN#
			{708, "^"},     // MA#* ( ? ¡ú ^ ) MODIFIER LETTER UP ARROWHEAD ¡ú CIRCUMFLEX ACCENT#
			{710, "^"},     // MA# ( ? ¡ú ^ ) MODIFIER LETTER CIRCUMFLEX ACCENT ¡ú CIRCUMFLEX ACCENT#
			{712, "'"},     // MA# ( ? ¡ú ' ) MODIFIER LETTER VERTICAL LINE ¡ú APOSTROPHE#
			{714, "'"},     // MA# ( ¨@ ¡ú ' ) MODIFIER LETTER ACUTE ACCENT ¡ú APOSTROPHE# ¡ú?¡ú¡ú¡ä¡ú
			{715, "'"},     // MA# ( ¨A ¡ú ' ) MODIFIER LETTER GRAVE ACCENT ¡ú APOSTROPHE# ¡ú£à¡ú¡ú¡®¡ú
			{720, ":"},     // MA# ( ? ¡ú : ) MODIFIER LETTER TRIANGULAR COLON ¡ú COLON#
			{727, "-"},     // MA#* ( ? ¡ú - ) MODIFIER LETTER MINUS SIGN ¡ú HYPHEN-MINUS#
			{731, "i"},     // MA#* ( ? ¡ú i ) OGONEK ¡ú LATIN SMALL LETTER I# ¡ú?¡ú¡ú?¡ú¡ú¦É¡ú
			{732, "~"},     // MA#* ( ? ¡ú ~ ) SMALL TILDE ¡ú TILDE#
			{733, "\""},    // MA#* ( ? ¡ú '' ) DOUBLE ACUTE ACCENT ¡ú APOSTROPHE, APOSTROPHE# ¡ú"¡ú# Converted to a quote.
			{750, "\""},    // MA# ( ? ¡ú '' ) MODIFIER LETTER DOUBLE APOSTROPHE ¡ú APOSTROPHE, APOSTROPHE# ¡ú¡å¡ú¡ú"¡ú# Converted to a quote.
			{756, "'"},     // MA#* ( ? ¡ú ' ) MODIFIER LETTER MIDDLE GRAVE ACCENT ¡ú APOSTROPHE# ¡ú¨A¡ú¡ú£à¡ú¡ú¡®¡ú
			{758, "\""},    // MA#* ( ? ¡ú '' ) MODIFIER LETTER MIDDLE DOUBLE ACUTE ACCENT ¡ú APOSTROPHE, APOSTROPHE# ¡ú?¡ú¡ú"¡ú# Converted to a quote.
			{760, ":"},     // MA#* ( ? ¡ú : ) MODIFIER LETTER RAISED COLON ¡ú COLON#
			{884, "'"},     // MA# ( ? ¡ú ' ) GREEK NUMERAL SIGN ¡ú APOSTROPHE# ¡ú¡ä¡ú
			{890, "i"},     // MA#* ( ? ¡ú i ) GREEK YPOGEGRAMMENI ¡ú LATIN SMALL LETTER I# ¡ú?¡ú¡ú¦É¡ú
			{894, ";"},     // MA#* ( ? ¡ú ; ) GREEK QUESTION MARK ¡ú SEMICOLON#
			{895, "J"},     // MA# ( ? ¡ú J ) GREEK CAPITAL LETTER YOT ¡ú LATIN CAPITAL LETTER J#
			{900, "'"},     // MA#* ( ? ¡ú ' ) GREEK TONOS ¡ú APOSTROPHE# ¡ú?¡ú
			{913, "A"},     // MA# ( ¦¡ ¡ú A ) GREEK CAPITAL LETTER ALPHA ¡ú LATIN CAPITAL LETTER A#
			{914, "B"},     // MA# ( ¦¢ ¡ú B ) GREEK CAPITAL LETTER BETA ¡ú LATIN CAPITAL LETTER B#
			{917, "E"},     // MA# ( ¦¥ ¡ú E ) GREEK CAPITAL LETTER EPSILON ¡ú LATIN CAPITAL LETTER E#
			{918, "Z"},     // MA# ( ¦¦ ¡ú Z ) GREEK CAPITAL LETTER ZETA ¡ú LATIN CAPITAL LETTER Z#
			{919, "H"},     // MA# ( ¦§ ¡ú H ) GREEK CAPITAL LETTER ETA ¡ú LATIN CAPITAL LETTER H#
			{921, "l"},     // MA# ( ¦© ¡ú l ) GREEK CAPITAL LETTER IOTA ¡ú LATIN SMALL LETTER L#
			{922, "K"},     // MA# ( ¦ª ¡ú K ) GREEK CAPITAL LETTER KAPPA ¡ú LATIN CAPITAL LETTER K#
			{924, "M"},     // MA# ( ¦¬ ¡ú M ) GREEK CAPITAL LETTER MU ¡ú LATIN CAPITAL LETTER M#
			{925, "N"},     // MA# ( ¦­ ¡ú N ) GREEK CAPITAL LETTER NU ¡ú LATIN CAPITAL LETTER N#
			{927, "O"},     // MA# ( ¦¯ ¡ú O ) GREEK CAPITAL LETTER OMICRON ¡ú LATIN CAPITAL LETTER O#
			{929, "P"},     // MA# ( ¦± ¡ú P ) GREEK CAPITAL LETTER RHO ¡ú LATIN CAPITAL LETTER P#
			{932, "T"},     // MA# ( ¦³ ¡ú T ) GREEK CAPITAL LETTER TAU ¡ú LATIN CAPITAL LETTER T#
			{933, "Y"},     // MA# ( ¦´ ¡ú Y ) GREEK CAPITAL LETTER UPSILON ¡ú LATIN CAPITAL LETTER Y#
			{935, "X"},     // MA# ( ¦¶ ¡ú X ) GREEK CAPITAL LETTER CHI ¡ú LATIN CAPITAL LETTER X#
			{945, "a"},     // MA# ( ¦Á ¡ú a ) GREEK SMALL LETTER ALPHA ¡ú LATIN SMALL LETTER A#
			{947, "y"},     // MA# ( ¦Ã ¡ú y ) GREEK SMALL LETTER GAMMA ¡ú LATIN SMALL LETTER Y#
			{953, "i"},     // MA# ( ¦É ¡ú i ) GREEK SMALL LETTER IOTA ¡ú LATIN SMALL LETTER I#
			{957, "v"},     // MA# ( ¦Í ¡ú v ) GREEK SMALL LETTER NU ¡ú LATIN SMALL LETTER V#
			{959, "o"},     // MA# ( ¦Ï ¡ú o ) GREEK SMALL LETTER OMICRON ¡ú LATIN SMALL LETTER O#
			{961, "p"},     // MA# ( ¦Ñ ¡ú p ) GREEK SMALL LETTER RHO ¡ú LATIN SMALL LETTER P#
			{963, "o"},     // MA# ( ¦Ò ¡ú o ) GREEK SMALL LETTER SIGMA ¡ú LATIN SMALL LETTER O#
			{965, "u"},     // MA# ( ¦Ô ¡ú u ) GREEK SMALL LETTER UPSILON ¡ú LATIN SMALL LETTER U# ¡ú?¡ú
			{978, "Y"},     // MA# ( ? ¡ú Y ) GREEK UPSILON WITH HOOK SYMBOL ¡ú LATIN CAPITAL LETTER Y#
			{988, "F"},     // MA# ( ? ¡ú F ) GREEK LETTER DIGAMMA ¡ú LATIN CAPITAL LETTER F#
			{1000, "2"},    // MA# ( ? ¡ú 2 ) COPTIC CAPITAL LETTER HORI ¡ú DIGIT TWO# ¡ú?¡ú
			{1009, "p"},    // MA# ( ? ¡ú p ) GREEK RHO SYMBOL ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{1010, "c"},    // MA# ( ? ¡ú c ) GREEK LUNATE SIGMA SYMBOL ¡ú LATIN SMALL LETTER C#
			{1011, "j"},    // MA# ( ? ¡ú j ) GREEK LETTER YOT ¡ú LATIN SMALL LETTER J#
			{1017, "C"},    // MA# ( ? ¡ú C ) GREEK CAPITAL LUNATE SIGMA SYMBOL ¡ú LATIN CAPITAL LETTER C#
			{1018, "M"},    // MA# ( ? ¡ú M ) GREEK CAPITAL LETTER SAN ¡ú LATIN CAPITAL LETTER M#
			{1029, "S"},    // MA# ( ? ¡ú S ) CYRILLIC CAPITAL LETTER DZE ¡ú LATIN CAPITAL LETTER S#
			{1030, "l"},    // MA# ( ? ¡ú l ) CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I ¡ú LATIN SMALL LETTER L#
			{1032, "J"},    // MA# ( ? ¡ú J ) CYRILLIC CAPITAL LETTER JE ¡ú LATIN CAPITAL LETTER J#
			{1040, "A"},    // MA# ( §¡ ¡ú A ) CYRILLIC CAPITAL LETTER A ¡ú LATIN CAPITAL LETTER A#
			{1042, "B"},    // MA# ( §£ ¡ú B ) CYRILLIC CAPITAL LETTER VE ¡ú LATIN CAPITAL LETTER B#
			{1045, "E"},    // MA# ( §¦ ¡ú E ) CYRILLIC CAPITAL LETTER IE ¡ú LATIN CAPITAL LETTER E#
			{1047, "3"},    // MA# ( §© ¡ú 3 ) CYRILLIC CAPITAL LETTER ZE ¡ú DIGIT THREE#
			{1050, "K"},    // MA# ( §¬ ¡ú K ) CYRILLIC CAPITAL LETTER KA ¡ú LATIN CAPITAL LETTER K#
			{1052, "M"},    // MA# ( §® ¡ú M ) CYRILLIC CAPITAL LETTER EM ¡ú LATIN CAPITAL LETTER M#
			{1053, "H"},    // MA# ( §¯ ¡ú H ) CYRILLIC CAPITAL LETTER EN ¡ú LATIN CAPITAL LETTER H#
			{1054, "O"},    // MA# ( §° ¡ú O ) CYRILLIC CAPITAL LETTER O ¡ú LATIN CAPITAL LETTER O#
			{1056, "P"},    // MA# ( §² ¡ú P ) CYRILLIC CAPITAL LETTER ER ¡ú LATIN CAPITAL LETTER P#
			{1057, "C"},    // MA# ( §³ ¡ú C ) CYRILLIC CAPITAL LETTER ES ¡ú LATIN CAPITAL LETTER C#
			{1058, "T"},    // MA# ( §´ ¡ú T ) CYRILLIC CAPITAL LETTER TE ¡ú LATIN CAPITAL LETTER T#
			{1059, "Y"},    // MA# ( §µ ¡ú Y ) CYRILLIC CAPITAL LETTER U ¡ú LATIN CAPITAL LETTER Y#
			{1061, "X"},    // MA# ( §· ¡ú X ) CYRILLIC CAPITAL LETTER HA ¡ú LATIN CAPITAL LETTER X#
			{1067, "bl"},   // MA# ( §½ ¡ú bl ) CYRILLIC CAPITAL LETTER YERU ¡ú LATIN SMALL LETTER B, LATIN SMALL LETTER L# ¡ú§¾?¡ú¡ú§¾1¡ú
			{1068, "b"},    // MA# ( §¾ ¡ú b ) CYRILLIC CAPITAL LETTER SOFT SIGN ¡ú LATIN SMALL LETTER B# ¡ú?¡ú
			{1070, "lO"},   // MA# ( §À ¡ú lO ) CYRILLIC CAPITAL LETTER YU ¡ú LATIN SMALL LETTER L, LATIN CAPITAL LETTER O# ¡úIO¡ú
			{1072, "a"},    // MA# ( §Ñ ¡ú a ) CYRILLIC SMALL LETTER A ¡ú LATIN SMALL LETTER A#
			{1073, "6"},    // MA# ( §Ò ¡ú 6 ) CYRILLIC SMALL LETTER BE ¡ú DIGIT SIX#
			{1075, "r"},    // MA# ( §Ô ¡ú r ) CYRILLIC SMALL LETTER GHE ¡ú LATIN SMALL LETTER R#
			{1077, "e"},    // MA# ( §Ö ¡ú e ) CYRILLIC SMALL LETTER IE ¡ú LATIN SMALL LETTER E#
			{1086, "o"},    // MA# ( §à ¡ú o ) CYRILLIC SMALL LETTER O ¡ú LATIN SMALL LETTER O#
			{1088, "p"},    // MA# ( §â ¡ú p ) CYRILLIC SMALL LETTER ER ¡ú LATIN SMALL LETTER P#
			{1089, "c"},    // MA# ( §ã ¡ú c ) CYRILLIC SMALL LETTER ES ¡ú LATIN SMALL LETTER C#
			{1091, "y"},    // MA# ( §å ¡ú y ) CYRILLIC SMALL LETTER U ¡ú LATIN SMALL LETTER Y#
			{1093, "x"},    // MA# ( §ç ¡ú x ) CYRILLIC SMALL LETTER HA ¡ú LATIN SMALL LETTER X#
			{1109, "s"},    // MA# ( ? ¡ú s ) CYRILLIC SMALL LETTER DZE ¡ú LATIN SMALL LETTER S#
			{1110, "i"},    // MA# ( ? ¡ú i ) CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I ¡ú LATIN SMALL LETTER I#
			{1112, "j"},    // MA# ( ? ¡ú j ) CYRILLIC SMALL LETTER JE ¡ú LATIN SMALL LETTER J#
			{1121, "w"},    // MA# ( ? ¡ú w ) CYRILLIC SMALL LETTER OMEGA ¡ú LATIN SMALL LETTER W#
			{1140, "V"},    // MA# ( ? ¡ú V ) CYRILLIC CAPITAL LETTER IZHITSA ¡ú LATIN CAPITAL LETTER V#
			{1141, "v"},    // MA# ( ? ¡ú v ) CYRILLIC SMALL LETTER IZHITSA ¡ú LATIN SMALL LETTER V#
			{1169, "r'"},   // MA# ( ? ¡ú r' ) CYRILLIC SMALL LETTER GHE WITH UPTURN ¡ú LATIN SMALL LETTER R, APOSTROPHE# ¡ú§Ô?¡ú
			{1198, "Y"},    // MA# ( ? ¡ú Y ) CYRILLIC CAPITAL LETTER STRAIGHT U ¡ú LATIN CAPITAL LETTER Y#
			{1199, "y"},    // MA# ( ? ¡ú y ) CYRILLIC SMALL LETTER STRAIGHT U ¡ú LATIN SMALL LETTER Y# ¡ú¦Ã¡ú
			{1211, "h"},    // MA# ( ? ¡ú h ) CYRILLIC SMALL LETTER SHHA ¡ú LATIN SMALL LETTER H#
			{1213, "e"},    // MA# ( ? ¡ú e ) CYRILLIC SMALL LETTER ABKHASIAN CHE ¡ú LATIN SMALL LETTER E#
			{1216, "l"},    // MA# ( ? ¡ú l ) CYRILLIC LETTER PALOCHKA ¡ú LATIN SMALL LETTER L#
			{1231, "i"},    // MA# ( ? ¡ú i ) CYRILLIC SMALL LETTER PALOCHKA ¡ú LATIN SMALL LETTER I# ¡ú?¡ú
			{1236, "AE"},   // MA# ( ? ¡ú AE ) CYRILLIC CAPITAL LIGATURE A IE ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER E# ¡ú?¡ú
			{1237, "ae"},   // MA# ( ? ¡ú ae ) CYRILLIC SMALL LIGATURE A IE ¡ú LATIN SMALL LETTER A, LATIN SMALL LETTER E# ¡ú§Ñ§Ö¡ú
			{1248, "3"},    // MA# ( ? ¡ú 3 ) CYRILLIC CAPITAL LETTER ABKHASIAN DZE ¡ú DIGIT THREE# ¡ú?¡ú
			{1281, "d"},    // MA# ( ? ¡ú d ) CYRILLIC SMALL LETTER KOMI DE ¡ú LATIN SMALL LETTER D#
			{1292, "G"},    // MA# ( ? ¡ú G ) CYRILLIC CAPITAL LETTER KOMI SJE ¡ú LATIN CAPITAL LETTER G#
			{1307, "q"},    // MA# ( ? ¡ú q ) CYRILLIC SMALL LETTER QA ¡ú LATIN SMALL LETTER Q#
			{1308, "W"},    // MA# ( ? ¡ú W ) CYRILLIC CAPITAL LETTER WE ¡ú LATIN CAPITAL LETTER W#
			{1309, "w"},    // MA# ( ? ¡ú w ) CYRILLIC SMALL LETTER WE ¡ú LATIN SMALL LETTER W#
			{1357, "U"},    // MA# ( ? ¡ú U ) ARMENIAN CAPITAL LETTER SEH ¡ú LATIN CAPITAL LETTER U#
			{1359, "S"},    // MA# ( ? ¡ú S ) ARMENIAN CAPITAL LETTER TIWN ¡ú LATIN CAPITAL LETTER S#
			{1365, "O"},    // MA# ( ? ¡ú O ) ARMENIAN CAPITAL LETTER OH ¡ú LATIN CAPITAL LETTER O#
			{1370, "'"},    // MA#* ( ? ¡ú ' ) ARMENIAN APOSTROPHE ¡ú APOSTROPHE# ¡ú¡¯¡ú
			{1373, "'"},    // MA#* ( ? ¡ú ' ) ARMENIAN COMMA ¡ú APOSTROPHE# ¡ú¨A¡ú¡ú£à¡ú¡ú¡®¡ú
			{1377, "w"},    // MA# ( ? ¡ú w ) ARMENIAN SMALL LETTER AYB ¡ú LATIN SMALL LETTER W# ¡ú?¡ú
			{1379, "q"},    // MA# ( ? ¡ú q ) ARMENIAN SMALL LETTER GIM ¡ú LATIN SMALL LETTER Q#
			{1382, "q"},    // MA# ( ? ¡ú q ) ARMENIAN SMALL LETTER ZA ¡ú LATIN SMALL LETTER Q#
			{1392, "h"},    // MA# ( ? ¡ú h ) ARMENIAN SMALL LETTER HO ¡ú LATIN SMALL LETTER H#
			{1400, "n"},    // MA# ( ? ¡ú n ) ARMENIAN SMALL LETTER VO ¡ú LATIN SMALL LETTER N#
			{1404, "n"},    // MA# ( ? ¡ú n ) ARMENIAN SMALL LETTER RA ¡ú LATIN SMALL LETTER N#
			{1405, "u"},    // MA# ( ? ¡ú u ) ARMENIAN SMALL LETTER SEH ¡ú LATIN SMALL LETTER U#
			{1409, "g"},    // MA# ( ? ¡ú g ) ARMENIAN SMALL LETTER CO ¡ú LATIN SMALL LETTER G#
			{1412, "f"},    // MA# ( ? ¡ú f ) ARMENIAN SMALL LETTER KEH ¡ú LATIN SMALL LETTER F#
			{1413, "o"},    // MA# ( ? ¡ú o ) ARMENIAN SMALL LETTER OH ¡ú LATIN SMALL LETTER O#
			{1417, ":"},    // MA#* ( ? ¡ú : ) ARMENIAN FULL STOP ¡ú COLON#
			{1472, "l"},    // MA#* ( ??? ¡ú l ) HEBREW PUNCTUATION PASEQ ¡ú LATIN SMALL LETTER L# ¡ú|¡ú
			{1475, ":"},    // MA#* ( ??? ¡ú : ) HEBREW PUNCTUATION SOF PASUQ ¡ú COLON#
			{1493, "l"},    // MA# ( ??? ¡ú l ) HEBREW LETTER VAV ¡ú LATIN SMALL LETTER L#
			{1496, "v"},    // MA# ( ??? ¡ú v ) HEBREW LETTER TET ¡ú LATIN SMALL LETTER V#
			{1497, "'"},    // MA# ( ??? ¡ú ' ) HEBREW LETTER YOD ¡ú APOSTROPHE#
			{1503, "l"},    // MA# ( ??? ¡ú l ) HEBREW LETTER FINAL NUN ¡ú LATIN SMALL LETTER L#
			{1505, "o"},    // MA# ( ??? ¡ú o ) HEBREW LETTER SAMEKH ¡ú LATIN SMALL LETTER O#
			{1520, "ll"},   // MA# ( ??? ¡ú ll ) HEBREW LIGATURE YIDDISH DOUBLE VAV ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡ú????¡ú
			{1521, "l'"},   // MA# ( ??? ¡ú l' ) HEBREW LIGATURE YIDDISH VAV YOD ¡ú LATIN SMALL LETTER L, APOSTROPHE# ¡ú????¡ú
			{1522, "\""},   // MA# ( ??? ¡ú '' ) HEBREW LIGATURE YIDDISH DOUBLE YOD ¡ú APOSTROPHE, APOSTROPHE# ¡ú????¡ú# Converted to a quote.
			{1523, "'"},    // MA#* ( ??? ¡ú ' ) HEBREW PUNCTUATION GERESH ¡ú APOSTROPHE#
			{1524, "\""},   // MA#* ( ??? ¡ú '' ) HEBREW PUNCTUATION GERSHAYIM ¡ú APOSTROPHE, APOSTROPHE# ¡ú"¡ú# Converted to a quote.
			{1549, ","},    // MA#* ( ??? ¡ú , ) ARABIC DATE SEPARATOR ¡ú COMMA# ¡ú???¡ú
			{1575, "l"},    // MA# ( ??? ¡ú l ) ARABIC LETTER ALEF ¡ú LATIN SMALL LETTER L# ¡ú1¡ú
			{1607, "o"},    // MA# ( ??? ¡ú o ) ARABIC LETTER HEH ¡ú LATIN SMALL LETTER O#
			{1632, "."},    // MA# ( ??? ¡ú . ) ARABIC-INDIC DIGIT ZERO ¡ú FULL STOP#
			{1633, "l"},    // MA# ( ??? ¡ú l ) ARABIC-INDIC DIGIT ONE ¡ú LATIN SMALL LETTER L# ¡ú1¡ú
			{1637, "o"},    // MA# ( ??? ¡ú o ) ARABIC-INDIC DIGIT FIVE ¡ú LATIN SMALL LETTER O#
			{1639, "V"},    // MA# ( ??? ¡ú V ) ARABIC-INDIC DIGIT SEVEN ¡ú LATIN CAPITAL LETTER V#
			{1643, ","},    // MA#* ( ??? ¡ú , ) ARABIC DECIMAL SEPARATOR ¡ú COMMA#
			{1645, "*"},    // MA#* ( ??? ¡ú * ) ARABIC FIVE POINTED STAR ¡ú ASTERISK#
			{1726, "o"},    // MA# ( ??? ¡ú o ) ARABIC LETTER HEH DOACHASHMEE ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{1729, "o"},    // MA# ( ??? ¡ú o ) ARABIC LETTER HEH GOAL ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{1748, "-"},    // MA#* ( ??? ¡ú - ) ARABIC FULL STOP ¡ú HYPHEN-MINUS# ¡ú©\¡ú
			{1749, "o"},    // MA# ( ??? ¡ú o ) ARABIC LETTER AE ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{1776, "."},    // MA# ( ? ¡ú . ) EXTENDED ARABIC-INDIC DIGIT ZERO ¡ú FULL STOP# ¡ú???¡ú
			{1777, "l"},    // MA# ( ? ¡ú l ) EXTENDED ARABIC-INDIC DIGIT ONE ¡ú LATIN SMALL LETTER L# ¡ú1¡ú
			{1781, "o"},    // MA# ( ? ¡ú o ) EXTENDED ARABIC-INDIC DIGIT FIVE ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{1783, "V"},    // MA# ( ? ¡ú V ) EXTENDED ARABIC-INDIC DIGIT SEVEN ¡ú LATIN CAPITAL LETTER V# ¡ú???¡ú
			{1793, "."},    // MA#* ( ??? ¡ú . ) SYRIAC SUPRALINEAR FULL STOP ¡ú FULL STOP#
			{1794, "."},    // MA#* ( ??? ¡ú . ) SYRIAC SUBLINEAR FULL STOP ¡ú FULL STOP#
			{1795, ":"},    // MA#* ( ??? ¡ú : ) SYRIAC SUPRALINEAR COLON ¡ú COLON#
			{1796, ":"},    // MA#* ( ??? ¡ú : ) SYRIAC SUBLINEAR COLON ¡ú COLON#
			{1984, "O"},    // MA# ( ??? ¡ú O ) NKO DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{1994, "l"},    // MA# ( ??? ¡ú l ) NKO LETTER A ¡ú LATIN SMALL LETTER L# ¡ú¨O¡ú¡ú?¡ú
			{2036, "'"},    // MA# ( ??? ¡ú ' ) NKO HIGH TONE APOSTROPHE ¡ú APOSTROPHE# ¡ú¡¯¡ú
			{2037, "'"},    // MA# ( ??? ¡ú ' ) NKO LOW TONE APOSTROPHE ¡ú APOSTROPHE# ¡ú¡®¡ú
			{2042, "_"},    // MA# ( ??? ¡ú _ ) NKO LAJANYALAN ¡ú LOW LINE#
			{2307, ":"},    // MA# ( ? ¡ú : ) DEVANAGARI SIGN VISARGA ¡ú COLON#
			{2406, "o"},    // MA# ( ? ¡ú o ) DEVANAGARI DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{2429, "?"},    // MA# ( ? ¡ú ? ) DEVANAGARI LETTER GLOTTAL STOP ¡ú QUESTION MARK#
			{2534, "O"},    // MA# ( ? ¡ú O ) BENGALI DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{2538, "8"},    // MA# ( ? ¡ú 8 ) BENGALI DIGIT FOUR ¡ú DIGIT EIGHT#
			{2541, "9"},    // MA# ( ? ¡ú 9 ) BENGALI DIGIT SEVEN ¡ú DIGIT NINE#
			{2662, "o"},    // MA# ( ? ¡ú o ) GURMUKHI DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{2663, "9"},    // MA# ( ? ¡ú 9 ) GURMUKHI DIGIT ONE ¡ú DIGIT NINE#
			{2666, "8"},    // MA# ( ? ¡ú 8 ) GURMUKHI DIGIT FOUR ¡ú DIGIT EIGHT#
			{2691, ":"},    // MA# ( ? ¡ú : ) GUJARATI SIGN VISARGA ¡ú COLON#
			{2790, "o"},    // MA# ( ? ¡ú o ) GUJARATI DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{2819, "8"},    // MA# ( ? ¡ú 8 ) ORIYA SIGN VISARGA ¡ú DIGIT EIGHT#
			{2848, "O"},    // MA# ( ? ¡ú O ) ORIYA LETTER TTHA ¡ú LATIN CAPITAL LETTER O# ¡ú?¡ú¡ú0¡ú
			{2918, "O"},    // MA# ( ? ¡ú O ) ORIYA DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{2920, "9"},    // MA# ( ? ¡ú 9 ) ORIYA DIGIT TWO ¡ú DIGIT NINE#
			{3046, "o"},    // MA# ( ? ¡ú o ) TAMIL DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{3074, "o"},    // MA# ( ? ¡ú o ) TELUGU SIGN ANUSVARA ¡ú LATIN SMALL LETTER O#
			{3174, "o"},    // MA# ( ? ¡ú o ) TELUGU DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{3202, "o"},    // MA# ( ? ¡ú o ) KANNADA SIGN ANUSVARA ¡ú LATIN SMALL LETTER O#
			{3302, "o"},    // MA# ( ? ¡ú o ) KANNADA DIGIT ZERO ¡ú LATIN SMALL LETTER O# ¡ú?¡ú
			{3330, "o"},    // MA# ( ? ¡ú o ) MALAYALAM SIGN ANUSVARA ¡ú LATIN SMALL LETTER O#
			{3360, "o"},    // MA# ( ? ¡ú o ) MALAYALAM LETTER TTHA ¡ú LATIN SMALL LETTER O#
			{3430, "o"},    // MA# ( ? ¡ú o ) MALAYALAM DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{3437, "9"},    // MA# ( ? ¡ú 9 ) MALAYALAM DIGIT SEVEN ¡ú DIGIT NINE#
			{3458, "o"},    // MA# ( ? ¡ú o ) SINHALA SIGN ANUSVARAYA ¡ú LATIN SMALL LETTER O#
			{3664, "o"},    // MA# ( ? ¡ú o ) THAI DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{3792, "o"},    // MA# ( ? ¡ú o ) LAO DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{4125, "o"},    // MA# ( ? ¡ú o ) MYANMAR LETTER WA ¡ú LATIN SMALL LETTER O#
			{4160, "o"},    // MA# ( ? ¡ú o ) MYANMAR DIGIT ZERO ¡ú LATIN SMALL LETTER O#
			{4327, "y"},    // MA# ( ? ¡ú y ) GEORGIAN LETTER QAR ¡ú LATIN SMALL LETTER Y#
			{4351, "o"},    // MA# ( ? ¡ú o ) GEORGIAN LETTER LABIAL SIGN ¡ú LATIN SMALL LETTER O#
			{4608, "U"},    // MA# ( ? ¡ú U ) ETHIOPIC SYLLABLE HA ¡ú LATIN CAPITAL LETTER U# ¡ú?¡ú
			{4816, "O"},    // MA# ( ? ¡ú O ) ETHIOPIC SYLLABLE PHARYNGEAL A ¡ú LATIN CAPITAL LETTER O# ¡ú?¡ú
			{5024, "D"},    // MA# ( ? ¡ú D ) CHEROKEE LETTER A ¡ú LATIN CAPITAL LETTER D#
			{5025, "R"},    // MA# ( ? ¡ú R ) CHEROKEE LETTER E ¡ú LATIN CAPITAL LETTER R#
			{5026, "T"},    // MA# ( ? ¡ú T ) CHEROKEE LETTER I ¡ú LATIN CAPITAL LETTER T#
			{5028, "O'"},   // MA# ( ? ¡ú O' ) CHEROKEE LETTER U ¡ú LATIN CAPITAL LETTER O, APOSTROPHE# ¡ú?¡ú¡úO?¡ú
			{5029, "i"},    // MA# ( ? ¡ú i ) CHEROKEE LETTER V ¡ú LATIN SMALL LETTER I#
			{5033, "Y"},    // MA# ( ? ¡ú Y ) CHEROKEE LETTER GI ¡ú LATIN CAPITAL LETTER Y#
			{5034, "A"},    // MA# ( ? ¡ú A ) CHEROKEE LETTER GO ¡ú LATIN CAPITAL LETTER A#
			{5035, "J"},    // MA# ( ? ¡ú J ) CHEROKEE LETTER GU ¡ú LATIN CAPITAL LETTER J#
			{5036, "E"},    // MA# ( ? ¡ú E ) CHEROKEE LETTER GV ¡ú LATIN CAPITAL LETTER E#
			{5038, "?"},    // MA# ( ? ¡ú ? ) CHEROKEE LETTER HE ¡ú QUESTION MARK# ¡ú?¡ú¡ú?¡ú
			{5043, "W"},    // MA# ( ? ¡ú W ) CHEROKEE LETTER LA ¡ú LATIN CAPITAL LETTER W#
			{5047, "M"},    // MA# ( ? ¡ú M ) CHEROKEE LETTER LU ¡ú LATIN CAPITAL LETTER M#
			{5051, "H"},    // MA# ( ? ¡ú H ) CHEROKEE LETTER MI ¡ú LATIN CAPITAL LETTER H#
			{5053, "Y"},    // MA# ( ? ¡ú Y ) CHEROKEE LETTER MU ¡ú LATIN CAPITAL LETTER Y# ¡ú?¡ú
			{5056, "G"},    // MA# ( ? ¡ú G ) CHEROKEE LETTER NAH ¡ú LATIN CAPITAL LETTER G#
			{5058, "h"},    // MA# ( ? ¡ú h ) CHEROKEE LETTER NI ¡ú LATIN SMALL LETTER H#
			{5059, "Z"},    // MA# ( ? ¡ú Z ) CHEROKEE LETTER NO ¡ú LATIN CAPITAL LETTER Z#
			{5070, "4"},    // MA# ( ? ¡ú 4 ) CHEROKEE LETTER SE ¡ú DIGIT FOUR#
			{5071, "b"},    // MA# ( ? ¡ú b ) CHEROKEE LETTER SI ¡ú LATIN SMALL LETTER B#
			{5074, "R"},    // MA# ( ? ¡ú R ) CHEROKEE LETTER SV ¡ú LATIN CAPITAL LETTER R#
			{5076, "W"},    // MA# ( ? ¡ú W ) CHEROKEE LETTER TA ¡ú LATIN CAPITAL LETTER W#
			{5077, "S"},    // MA# ( ? ¡ú S ) CHEROKEE LETTER DE ¡ú LATIN CAPITAL LETTER S#
			{5081, "V"},    // MA# ( ? ¡ú V ) CHEROKEE LETTER DO ¡ú LATIN CAPITAL LETTER V#
			{5082, "S"},    // MA# ( ? ¡ú S ) CHEROKEE LETTER DU ¡ú LATIN CAPITAL LETTER S#
			{5086, "L"},    // MA# ( ? ¡ú L ) CHEROKEE LETTER TLE ¡ú LATIN CAPITAL LETTER L#
			{5087, "C"},    // MA# ( ? ¡ú C ) CHEROKEE LETTER TLI ¡ú LATIN CAPITAL LETTER C#
			{5090, "P"},    // MA# ( ? ¡ú P ) CHEROKEE LETTER TLV ¡ú LATIN CAPITAL LETTER P#
			{5094, "K"},    // MA# ( ? ¡ú K ) CHEROKEE LETTER TSO ¡ú LATIN CAPITAL LETTER K#
			{5095, "d"},    // MA# ( ? ¡ú d ) CHEROKEE LETTER TSU ¡ú LATIN SMALL LETTER D#
			{5102, "6"},    // MA# ( ? ¡ú 6 ) CHEROKEE LETTER WV ¡ú DIGIT SIX#
			{5107, "G"},    // MA# ( ? ¡ú G ) CHEROKEE LETTER YU ¡ú LATIN CAPITAL LETTER G#
			{5108, "B"},    // MA# ( ? ¡ú B ) CHEROKEE LETTER YV ¡ú LATIN CAPITAL LETTER B#
			{5120, "="},    // MA#* ( ? ¡ú = ) CANADIAN SYLLABICS HYPHEN ¡ú EQUALS SIGN#
			{5167, "V"},    // MA# ( ? ¡ú V ) CANADIAN SYLLABICS PE ¡ú LATIN CAPITAL LETTER V#
			{5171, ">"},    // MA# ( ? ¡ú > ) CANADIAN SYLLABICS PO ¡ú GREATER-THAN SIGN#
			{5176, "<"},    // MA# ( ? ¡ú < ) CANADIAN SYLLABICS PA ¡ú LESS-THAN SIGN#
			{5194, "'"},    // MA# ( ? ¡ú ' ) CANADIAN SYLLABICS WEST-CREE P ¡ú APOSTROPHE# ¡ú?¡ú
			{5196, "U"},    // MA# ( ? ¡ú U ) CANADIAN SYLLABICS TE ¡ú LATIN CAPITAL LETTER U#
			{5223, "U'"},   // MA# ( ? ¡ú U' ) CANADIAN SYLLABICS TTE ¡ú LATIN CAPITAL LETTER U, APOSTROPHE# ¡ú??¡ú¡ú?'¡ú
			{5229, "P"},    // MA# ( ? ¡ú P ) CANADIAN SYLLABICS KI ¡ú LATIN CAPITAL LETTER P#
			{5231, "d"},    // MA# ( ? ¡ú d ) CANADIAN SYLLABICS KO ¡ú LATIN SMALL LETTER D#
			{5254, "P'"},   // MA# ( ? ¡ú P' ) CANADIAN SYLLABICS SOUTH-SLAVEY KIH ¡ú LATIN CAPITAL LETTER P, APOSTROPHE# ¡ú??¡ú
			{5255, "d'"},   // MA# ( ? ¡ú d' ) CANADIAN SYLLABICS SOUTH-SLAVEY KOH ¡ú LATIN SMALL LETTER D, APOSTROPHE# ¡ú??¡ú
			{5261, "J"},    // MA# ( ? ¡ú J ) CANADIAN SYLLABICS CO ¡ú LATIN CAPITAL LETTER J#
			{5290, "L"},    // MA# ( ? ¡ú L ) CANADIAN SYLLABICS MA ¡ú LATIN CAPITAL LETTER L#
			{5311, "2"},    // MA# ( ? ¡ú 2 ) CANADIAN SYLLABICS SAYISI M ¡ú DIGIT TWO#
			{5441, "x"},    // MA# ( ? ¡ú x ) CANADIAN SYLLABICS SAYISI YI ¡ú LATIN SMALL LETTER X# ¡ú?¡ú
			{5500, "H"},    // MA# ( ? ¡ú H ) CANADIAN SYLLABICS NUNAVUT H ¡ú LATIN CAPITAL LETTER H#
			{5501, "x"},    // MA# ( ? ¡ú x ) CANADIAN SYLLABICS HK ¡ú LATIN SMALL LETTER X# ¡ú?¡ú¡ú?¡ú
			{5511, "R"},    // MA# ( ? ¡ú R ) CANADIAN SYLLABICS TLHI ¡ú LATIN CAPITAL LETTER R#
			{5551, "b"},    // MA# ( ? ¡ú b ) CANADIAN SYLLABICS AIVILIK B ¡ú LATIN SMALL LETTER B#
			{5556, "F"},    // MA# ( ? ¡ú F ) CANADIAN SYLLABICS BLACKFOOT WE ¡ú LATIN CAPITAL LETTER F#
			{5573, "A"},    // MA# ( ? ¡ú A ) CANADIAN SYLLABICS CARRIER GHO ¡ú LATIN CAPITAL LETTER A#
			{5598, "D"},    // MA# ( ? ¡ú D ) CANADIAN SYLLABICS CARRIER THE ¡ú LATIN CAPITAL LETTER D#
			{5610, "D"},    // MA# ( ? ¡ú D ) CANADIAN SYLLABICS CARRIER PE ¡ú LATIN CAPITAL LETTER D# ¡ú?¡ú
			{5616, "M"},    // MA# ( ? ¡ú M ) CANADIAN SYLLABICS CARRIER GO ¡ú LATIN CAPITAL LETTER M#
			{5623, "B"},    // MA# ( ? ¡ú B ) CANADIAN SYLLABICS CARRIER KHE ¡ú LATIN CAPITAL LETTER B#
			{5741, "X"},    // MA#* ( ? ¡ú X ) CANADIAN SYLLABICS CHI SIGN ¡ú LATIN CAPITAL LETTER X#
			{5742, "x"},    // MA#* ( ? ¡ú x ) CANADIAN SYLLABICS FULL STOP ¡ú LATIN SMALL LETTER X#
			{5760, " "},    // MA#* ( ? ¡ú   ) OGHAM SPACE MARK ¡ú SPACE#
			{5810, "<"},    // MA# ( ? ¡ú < ) RUNIC LETTER KAUNA ¡ú LESS-THAN SIGN#
			{5815, "X"},    // MA# ( ? ¡ú X ) RUNIC LETTER GEBO GYFU G ¡ú LATIN CAPITAL LETTER X#
			{5825, "l"},    // MA# ( ? ¡ú l ) RUNIC LETTER ISAZ IS ISS I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{5836, "'"},    // MA# ( ? ¡ú ' ) RUNIC LETTER SHORT-TWIG-SOL S ¡ú APOSTROPHE#
			{5845, "K"},    // MA# ( ? ¡ú K ) RUNIC LETTER OPEN-P ¡ú LATIN CAPITAL LETTER K#
			{5846, "M"},    // MA# ( ? ¡ú M ) RUNIC LETTER EHWAZ EH E ¡ú LATIN CAPITAL LETTER M#
			{5868, ":"},    // MA#* ( ? ¡ú : ) RUNIC MULTIPLE PUNCTUATION ¡ú COLON#
			{5869, "+"},    // MA#* ( ? ¡ú + ) RUNIC CROSS PUNCTUATION ¡ú PLUS SIGN#
			{5941, "/"},    // MA#* ( ? ¡ú / ) PHILIPPINE SINGLE PUNCTUATION ¡ú SOLIDUS#
			{6147, ":"},    // MA#* ( ? ¡ú : ) MONGOLIAN FULL STOP ¡ú COLON#
			{6153, ":"},    // MA#* ( ? ¡ú : ) MONGOLIAN MANCHU FULL STOP ¡ú COLON#
			{7379, "\""},   // MA#* ( ? ¡ú '' ) VEDIC SIGN NIHSHVASA ¡ú APOSTROPHE, APOSTROPHE# ¡ú¡å¡ú¡ú"¡ú# Converted to a quote.
			{7428, "c"},    // MA# ( ? ¡ú c ) LATIN LETTER SMALL CAPITAL C ¡ú LATIN SMALL LETTER C#
			{7439, "o"},    // MA# ( ? ¡ú o ) LATIN LETTER SMALL CAPITAL O ¡ú LATIN SMALL LETTER O#
			{7441, "o"},    // MA# ( ? ¡ú o ) LATIN SMALL LETTER SIDEWAYS O ¡ú LATIN SMALL LETTER O#
			{7452, "u"},    // MA# ( ? ¡ú u ) LATIN LETTER SMALL CAPITAL U ¡ú LATIN SMALL LETTER U#
			{7456, "v"},    // MA# ( ? ¡ú v ) LATIN LETTER SMALL CAPITAL V ¡ú LATIN SMALL LETTER V#
			{7457, "w"},    // MA# ( ? ¡ú w ) LATIN LETTER SMALL CAPITAL W ¡ú LATIN SMALL LETTER W#
			{7458, "z"},    // MA# ( ? ¡ú z ) LATIN LETTER SMALL CAPITAL Z ¡ú LATIN SMALL LETTER Z#
			{7462, "r"},    // MA# ( ? ¡ú r ) GREEK LETTER SMALL CAPITAL GAMMA ¡ú LATIN SMALL LETTER R# ¡ú§Ô¡ú
			{7531, "ue"},   // MA# ( ? ¡ú ue ) LATIN SMALL LETTER UE ¡ú LATIN SMALL LETTER U, LATIN SMALL LETTER E#
			{7555, "g"},    // MA# ( ? ¡ú g ) LATIN SMALL LETTER G WITH PALATAL HOOK ¡ú LATIN SMALL LETTER G#
			{7564, "y"},    // MA# ( ? ¡ú y ) LATIN SMALL LETTER V WITH PALATAL HOOK ¡ú LATIN SMALL LETTER Y#
			{7837, "f"},    // MA# ( ? ¡ú f ) LATIN SMALL LETTER LONG S WITH HIGH STROKE ¡ú LATIN SMALL LETTER F#
			{7935, "y"},    // MA# ( ? ¡ú y ) LATIN SMALL LETTER Y WITH LOOP ¡ú LATIN SMALL LETTER Y#
			{8125, "'"},    // MA#* ( ? ¡ú ' ) GREEK KORONIS ¡ú APOSTROPHE# ¡ú¡¯¡ú
			{8126, "i"},    // MA# ( ? ¡ú i ) GREEK PROSGEGRAMMENI ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{8127, "'"},    // MA#* ( ? ¡ú ' ) GREEK PSILI ¡ú APOSTROPHE# ¡ú¡¯¡ú
			{8128, "~"},    // MA#* ( ? ¡ú ~ ) GREEK PERISPOMENI ¡ú TILDE# ¡ú?¡ú
			{8175, "'"},    // MA#* ( ? ¡ú ' ) GREEK VARIA ¡ú APOSTROPHE# ¡ú¨A¡ú¡ú£à¡ú¡ú¡®¡ú
			{8189, "'"},    // MA#* ( ? ¡ú ' ) GREEK OXIA ¡ú APOSTROPHE# ¡ú?¡ú¡ú?¡ú¡ú?¡ú
			{8190, "'"},    // MA#* ( ? ¡ú ' ) GREEK DASIA ¡ú APOSTROPHE# ¡ú?¡ú¡ú¡ä¡ú
			{8192, " "},    // MA#* ( ? ¡ú   ) EN QUAD ¡ú SPACE#
			{8193, " "},    // MA#* ( ? ¡ú   ) EM QUAD ¡ú SPACE#
			{8194, " "},    // MA#* ( ? ¡ú   ) EN SPACE ¡ú SPACE#
			{8195, " "},    // MA#* ( ? ¡ú   ) EM SPACE ¡ú SPACE#
			{8196, " "},    // MA#* ( ? ¡ú   ) THREE-PER-EM SPACE ¡ú SPACE#
			{8197, " "},    // MA#* ( ? ¡ú   ) FOUR-PER-EM SPACE ¡ú SPACE#
			{8198, " "},    // MA#* ( ? ¡ú   ) SIX-PER-EM SPACE ¡ú SPACE#
			{8199, " "},    // MA#* ( ? ¡ú   ) FIGURE SPACE ¡ú SPACE#
			{8200, " "},    // MA#* ( ? ¡ú   ) PUNCTUATION SPACE ¡ú SPACE#
			{8201, " "},    // MA#* ( ? ¡ú   ) THIN SPACE ¡ú SPACE#
			{8202, " "},    // MA#* ( ? ¡ú   ) HAIR SPACE ¡ú SPACE#
			{8208, "-"},    // MA#* ( ©\ ¡ú - ) HYPHEN ¡ú HYPHEN-MINUS#
			{8209, "-"},    // MA#* ( ? ¡ú - ) NON-BREAKING HYPHEN ¡ú HYPHEN-MINUS#
			{8210, "-"},    // MA#* ( ? ¡ú - ) FIGURE DASH ¡ú HYPHEN-MINUS#
			{8211, "-"},    // MA#* ( ¨C ¡ú - ) EN DASH ¡ú HYPHEN-MINUS#
			{8214, "ll"},   // MA#* ( ¡¬ ¡ú ll ) DOUBLE VERTICAL LINE ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡ú¡Î¡ú¡ú||¡ú
			{8216, "'"},    // MA#* ( ¡® ¡ú ' ) LEFT SINGLE QUOTATION MARK ¡ú APOSTROPHE#
			{8217, "'"},    // MA#* ( ¡¯ ¡ú ' ) RIGHT SINGLE QUOTATION MARK ¡ú APOSTROPHE#
			{8218, ","},    // MA#* ( ? ¡ú , ) SINGLE LOW-9 QUOTATION MARK ¡ú COMMA#
			{8219, "'"},    // MA#* ( ? ¡ú ' ) SINGLE HIGH-REVERSED-9 QUOTATION MARK ¡ú APOSTROPHE# ¡ú¡ä¡ú
			{8220, "\""},   // MA#* ( ¡° ¡ú '' ) LEFT DOUBLE QUOTATION MARK ¡ú APOSTROPHE, APOSTROPHE# ¡ú"¡ú# Converted to a quote.
			{8221, "\""},   // MA#* ( ¡± ¡ú '' ) RIGHT DOUBLE QUOTATION MARK ¡ú APOSTROPHE, APOSTROPHE# ¡ú"¡ú# Converted to a quote.
			{8223, "\""},   // MA#* ( ? ¡ú '' ) DOUBLE HIGH-REVERSED-9 QUOTATION MARK ¡ú APOSTROPHE, APOSTROPHE# ¡ú¡°¡ú¡ú"¡ú# Converted to a quote.
			{8228, "."},    // MA#* ( ? ¡ú . ) ONE DOT LEADER ¡ú FULL STOP#
			{8229, ".."},   // MA#* ( ¨E ¡ú .. ) TWO DOT LEADER ¡ú FULL STOP, FULL STOP#
			{8230, "..."},  // MA#* ( ¡­ ¡ú ... ) HORIZONTAL ELLIPSIS ¡ú FULL STOP, FULL STOP, FULL STOP#
			{8232, " "},    // MA#* (  ¡ú   ) LINE SEPARATOR ¡ú SPACE#
			{8233, " "},    // MA#* (  ¡ú   ) PARAGRAPH SEPARATOR ¡ú SPACE#
			{8239, " "},    // MA#* ( ? ¡ú   ) NARROW NO-BREAK SPACE ¡ú SPACE#
			{8242, "'"},    // MA#* ( ¡ä ¡ú ' ) PRIME ¡ú APOSTROPHE#
			{8243, "\""},   // MA#* ( ¡å ¡ú '' ) DOUBLE PRIME ¡ú APOSTROPHE, APOSTROPHE# ¡ú"¡ú# Converted to a quote.
			{8244, "'''"},  // MA#* ( ? ¡ú ''' ) TRIPLE PRIME ¡ú APOSTROPHE, APOSTROPHE, APOSTROPHE# ¡ú¡ä¡ä¡ä¡ú
			{8245, "'"},    // MA#* ( ¨F ¡ú ' ) REVERSED PRIME ¡ú APOSTROPHE# ¡ú?¡ú¡ú¡®¡ú
			{8246, "\""},   // MA#* ( ? ¡ú '' ) REVERSED DOUBLE PRIME ¡ú APOSTROPHE, APOSTROPHE# ¡ú¨F¨F¡ú# Converted to a quote.
			{8247, "'''"},  // MA#* ( ? ¡ú ''' ) REVERSED TRIPLE PRIME ¡ú APOSTROPHE, APOSTROPHE, APOSTROPHE# ¡ú¨F¨F¨F¡ú
			{8249, "<"},    // MA#* ( ? ¡ú < ) SINGLE LEFT-POINTING ANGLE QUOTATION MARK ¡ú LESS-THAN SIGN#
			{8250, ">"},    // MA#* ( ? ¡ú > ) SINGLE RIGHT-POINTING ANGLE QUOTATION MARK ¡ú GREATER-THAN SIGN#
			{8252, "!!"},   // MA#* ( ? ¡ú !! ) DOUBLE EXCLAMATION MARK ¡ú EXCLAMATION MARK, EXCLAMATION MARK#
			{8257, "/"},    // MA#* ( ? ¡ú / ) CARET INSERTION POINT ¡ú SOLIDUS#
			{8259, "-"},    // MA#* ( ? ¡ú - ) HYPHEN BULLET ¡ú HYPHEN-MINUS# ¡ú©\¡ú
			{8260, "/"},    // MA#* ( ? ¡ú / ) FRACTION SLASH ¡ú SOLIDUS#
			{8263, "??"},   // MA#* ( ? ¡ú ?? ) DOUBLE QUESTION MARK ¡ú QUESTION MARK, QUESTION MARK#
			{8264, "?!"},   // MA#* ( ? ¡ú ?! ) QUESTION EXCLAMATION MARK ¡ú QUESTION MARK, EXCLAMATION MARK#
			{8265, "!?"},   // MA#* ( ? ¡ú !? ) EXCLAMATION QUESTION MARK ¡ú EXCLAMATION MARK, QUESTION MARK#
			{8270, "*"},    // MA#* ( ? ¡ú * ) LOW ASTERISK ¡ú ASTERISK#
			{8275, "~"},    // MA#* ( ? ¡ú ~ ) SWUNG DASH ¡ú TILDE#
			{8279, "''''"}, // MA#* ( ? ¡ú '''' ) QUADRUPLE PRIME ¡ú APOSTROPHE, APOSTROPHE, APOSTROPHE, APOSTROPHE# ¡ú¡ä¡ä¡ä¡ä¡ú
			{8282, ":"},    // MA#* ( ? ¡ú : ) TWO DOT PUNCTUATION ¡ú COLON#
			{8287, " "},    // MA#* ( ? ¡ú   ) MEDIUM MATHEMATICAL SPACE ¡ú SPACE#
			{8360, "Rs"},   // MA#* ( ? ¡ú Rs ) RUPEE SIGN ¡ú LATIN CAPITAL LETTER R, LATIN SMALL LETTER S#
			{8374, "lt"},   // MA#* ( ? ¡ú lt ) LIVRE TOURNOIS SIGN ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER T#
			{8448, "a/c"},  // MA#* ( ? ¡ú a/c ) ACCOUNT OF ¡ú LATIN SMALL LETTER A, SOLIDUS, LATIN SMALL LETTER C#
			{8449, "a/s"},  // MA#* ( ? ¡ú a/s ) ADDRESSED TO THE SUBJECT ¡ú LATIN SMALL LETTER A, SOLIDUS, LATIN SMALL LETTER S#
			{8450, "C"},    // MA# ( ? ¡ú C ) DOUBLE-STRUCK CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{8453, "c/o"},  // MA#* ( ¨G ¡ú c/o ) CARE OF ¡ú LATIN SMALL LETTER C, SOLIDUS, LATIN SMALL LETTER O#
			{8454, "c/u"},  // MA#* ( ? ¡ú c/u ) CADA UNA ¡ú LATIN SMALL LETTER C, SOLIDUS, LATIN SMALL LETTER U#
			{8458, "g"},    // MA# ( ? ¡ú g ) SCRIPT SMALL G ¡ú LATIN SMALL LETTER G#
			{8459, "H"},    // MA# ( ? ¡ú H ) SCRIPT CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{8460, "H"},    // MA# ( ? ¡ú H ) BLACK-LETTER CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{8461, "H"},    // MA# ( ? ¡ú H ) DOUBLE-STRUCK CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{8462, "h"},    // MA# ( ? ¡ú h ) PLANCK CONSTANT ¡ú LATIN SMALL LETTER H#
			{8464, "l"},    // MA# ( ? ¡ú l ) SCRIPT CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{8465, "l"},    // MA# ( ? ¡ú l ) BLACK-LETTER CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{8466, "L"},    // MA# ( ? ¡ú L ) SCRIPT CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{8467, "l"},    // MA# ( ? ¡ú l ) SCRIPT SMALL L ¡ú LATIN SMALL LETTER L#
			{8469, "N"},    // MA# ( ? ¡ú N ) DOUBLE-STRUCK CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{8470, "No"},   // MA#* ( ¡í ¡ú No ) NUMERO SIGN ¡ú LATIN CAPITAL LETTER N, LATIN SMALL LETTER O#
			{8473, "P"},    // MA# ( ? ¡ú P ) DOUBLE-STRUCK CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{8474, "Q"},    // MA# ( ? ¡ú Q ) DOUBLE-STRUCK CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{8475, "R"},    // MA# ( ? ¡ú R ) SCRIPT CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{8476, "R"},    // MA# ( ? ¡ú R ) BLACK-LETTER CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{8477, "R"},    // MA# ( ? ¡ú R ) DOUBLE-STRUCK CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{8481, "TEL"},  // MA#* ( ©Y ¡ú TEL ) TELEPHONE SIGN ¡ú LATIN CAPITAL LETTER T, LATIN CAPITAL LETTER E, LATIN CAPITAL LETTER L#
			{8484, "Z"},    // MA# ( ? ¡ú Z ) DOUBLE-STRUCK CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{8488, "Z"},    // MA# ( ? ¡ú Z ) BLACK-LETTER CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{8490, "K"},    // MA# ( ? ¡ú K ) KELVIN SIGN ¡ú LATIN CAPITAL LETTER K#
			{8492, "B"},    // MA# ( ? ¡ú B ) SCRIPT CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{8493, "C"},    // MA# ( ? ¡ú C ) BLACK-LETTER CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{8494, "e"},    // MA# ( ? ¡ú e ) ESTIMATED SYMBOL ¡ú LATIN SMALL LETTER E#
			{8495, "e"},    // MA# ( ? ¡ú e ) SCRIPT SMALL E ¡ú LATIN SMALL LETTER E#
			{8496, "E"},    // MA# ( ? ¡ú E ) SCRIPT CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{8497, "F"},    // MA# ( ? ¡ú F ) SCRIPT CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{8499, "M"},    // MA# ( ? ¡ú M ) SCRIPT CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{8500, "o"},    // MA# ( ? ¡ú o ) SCRIPT SMALL O ¡ú LATIN SMALL LETTER O#
			{8505, "i"},    // MA# ( ? ¡ú i ) INFORMATION SOURCE ¡ú LATIN SMALL LETTER I#
			{8507, "FAX"},  // MA#* ( ? ¡ú FAX ) FACSIMILE SIGN ¡ú LATIN CAPITAL LETTER F, LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER X#
			{8509, "y"},    // MA# ( ? ¡ú y ) DOUBLE-STRUCK SMALL GAMMA ¡ú LATIN SMALL LETTER Y# ¡ú¦Ã¡ú
			{8517, "D"},    // MA# ( ? ¡ú D ) DOUBLE-STRUCK ITALIC CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{8518, "d"},    // MA# ( ? ¡ú d ) DOUBLE-STRUCK ITALIC SMALL D ¡ú LATIN SMALL LETTER D#
			{8519, "e"},    // MA# ( ? ¡ú e ) DOUBLE-STRUCK ITALIC SMALL E ¡ú LATIN SMALL LETTER E#
			{8520, "i"},    // MA# ( ? ¡ú i ) DOUBLE-STRUCK ITALIC SMALL I ¡ú LATIN SMALL LETTER I#
			{8521, "j"},    // MA# ( ? ¡ú j ) DOUBLE-STRUCK ITALIC SMALL J ¡ú LATIN SMALL LETTER J#
			{8544, "l"},    // MA# ( ¢ñ ¡ú l ) ROMAN NUMERAL ONE ¡ú LATIN SMALL LETTER L# ¡ú?¡ú
			{8545, "ll"},   // MA# ( ¢ò ¡ú ll ) ROMAN NUMERAL TWO ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡úII¡ú
			{8546, "lll"},  // MA# ( ¢ó ¡ú lll ) ROMAN NUMERAL THREE ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡úIII¡ú
			{8547, "lV"},   // MA# ( ¢ô ¡ú lV ) ROMAN NUMERAL FOUR ¡ú LATIN SMALL LETTER L, LATIN CAPITAL LETTER V# ¡úIV¡ú
			{8548, "V"},    // MA# ( ¢õ ¡ú V ) ROMAN NUMERAL FIVE ¡ú LATIN CAPITAL LETTER V#
			{8549, "Vl"},   // MA# ( ¢ö ¡ú Vl ) ROMAN NUMERAL SIX ¡ú LATIN CAPITAL LETTER V, LATIN SMALL LETTER L# ¡úVI¡ú
			{8550, "Vll"},  // MA# ( ¢÷ ¡ú Vll ) ROMAN NUMERAL SEVEN ¡ú LATIN CAPITAL LETTER V, LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡úVII¡ú
			{8551, "Vlll"}, // MA# ( ¢ø ¡ú Vlll ) ROMAN NUMERAL EIGHT ¡ú LATIN CAPITAL LETTER V, LATIN SMALL LETTER L, LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡úVIII¡ú
			{8552, "lX"},   // MA# ( ¢ù ¡ú lX ) ROMAN NUMERAL NINE ¡ú LATIN SMALL LETTER L, LATIN CAPITAL LETTER X# ¡úIX¡ú
			{8553, "X"},    // MA# ( ¢ú ¡ú X ) ROMAN NUMERAL TEN ¡ú LATIN CAPITAL LETTER X#
			{8554, "Xl"},   // MA# ( ¢û ¡ú Xl ) ROMAN NUMERAL ELEVEN ¡ú LATIN CAPITAL LETTER X, LATIN SMALL LETTER L# ¡úXI¡ú
			{8555, "Xll"},  // MA# ( ¢ü ¡ú Xll ) ROMAN NUMERAL TWELVE ¡ú LATIN CAPITAL LETTER X, LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡úXII¡ú
			{8556, "L"},    // MA# ( ? ¡ú L ) ROMAN NUMERAL FIFTY ¡ú LATIN CAPITAL LETTER L#
			{8557, "C"},    // MA# ( ? ¡ú C ) ROMAN NUMERAL ONE HUNDRED ¡ú LATIN CAPITAL LETTER C#
			{8558, "D"},    // MA# ( ? ¡ú D ) ROMAN NUMERAL FIVE HUNDRED ¡ú LATIN CAPITAL LETTER D#
			{8559, "M"},    // MA# ( ? ¡ú M ) ROMAN NUMERAL ONE THOUSAND ¡ú LATIN CAPITAL LETTER M#
			{8560, "i"},    // MA# ( ¢¡ ¡ú i ) SMALL ROMAN NUMERAL ONE ¡ú LATIN SMALL LETTER I#
			{8561, "ii"},   // MA# ( ¢¢ ¡ú ii ) SMALL ROMAN NUMERAL TWO ¡ú LATIN SMALL LETTER I, LATIN SMALL LETTER I#
			{8562, "iii"},  // MA# ( ¢£ ¡ú iii ) SMALL ROMAN NUMERAL THREE ¡ú LATIN SMALL LETTER I, LATIN SMALL LETTER I, LATIN SMALL LETTER I#
			{8563, "iv"},   // MA# ( ¢¤ ¡ú iv ) SMALL ROMAN NUMERAL FOUR ¡ú LATIN SMALL LETTER I, LATIN SMALL LETTER V#
			{8564, "v"},    // MA# ( ¢¥ ¡ú v ) SMALL ROMAN NUMERAL FIVE ¡ú LATIN SMALL LETTER V#
			{8565, "vi"},   // MA# ( ¢¦ ¡ú vi ) SMALL ROMAN NUMERAL SIX ¡ú LATIN SMALL LETTER V, LATIN SMALL LETTER I#
			{8566, "vii"},  // MA# ( ¢§ ¡ú vii ) SMALL ROMAN NUMERAL SEVEN ¡ú LATIN SMALL LETTER V, LATIN SMALL LETTER I, LATIN SMALL LETTER I#
			{8567, "viii"}, // MA# ( ¢¨ ¡ú viii ) SMALL ROMAN NUMERAL EIGHT ¡ú LATIN SMALL LETTER V, LATIN SMALL LETTER I, LATIN SMALL LETTER I, LATIN SMALL LETTER I#
			{8568, "ix"},   // MA# ( ¢© ¡ú ix ) SMALL ROMAN NUMERAL NINE ¡ú LATIN SMALL LETTER I, LATIN SMALL LETTER X#
			{8569, "x"},    // MA# ( ¢ª ¡ú x ) SMALL ROMAN NUMERAL TEN ¡ú LATIN SMALL LETTER X#
			{8570, "xi"},   // MA# ( ? ¡ú xi ) SMALL ROMAN NUMERAL ELEVEN ¡ú LATIN SMALL LETTER X, LATIN SMALL LETTER I#
			{8571, "xii"},  // MA# ( ? ¡ú xii ) SMALL ROMAN NUMERAL TWELVE ¡ú LATIN SMALL LETTER X, LATIN SMALL LETTER I, LATIN SMALL LETTER I#
			{8572, "l"},    // MA# ( ? ¡ú l ) SMALL ROMAN NUMERAL FIFTY ¡ú LATIN SMALL LETTER L#
			{8573, "c"},    // MA# ( ? ¡ú c ) SMALL ROMAN NUMERAL ONE HUNDRED ¡ú LATIN SMALL LETTER C#
			{8574, "d"},    // MA# ( ? ¡ú d ) SMALL ROMAN NUMERAL FIVE HUNDRED ¡ú LATIN SMALL LETTER D#
			{8575, "rn"},   // MA# ( ? ¡ú rn ) SMALL ROMAN NUMERAL ONE THOUSAND ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{8722, "-"},    // MA#* ( ? ¡ú - ) MINUS SIGN ¡ú HYPHEN-MINUS#
			{8725, "/"},    // MA#* ( ¨M ¡ú / ) DIVISION SLASH ¡ú SOLIDUS#
			{8726, "\\"},   // MA#* ( ? ¡ú \ ) SET MINUS ¡ú REVERSE SOLIDUS#
			{8727, "*"},    // MA#* ( ? ¡ú * ) ASTERISK OPERATOR ¡ú ASTERISK#
			{8734, "oo"},   // MA#* ( ¡Þ ¡ú oo ) INFINITY ¡ú LATIN SMALL LETTER O, LATIN SMALL LETTER O# ¡ú?¡ú
			{8739, "l"},    // MA#* ( ¨O ¡ú l ) DIVIDES ¡ú LATIN SMALL LETTER L# ¡ú?¡ú
			{8741, "ll"},   // MA#* ( ¡Î ¡ú ll ) PARALLEL TO ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER L# ¡ú||¡ú
			{8744, "v"},    // MA#* ( ¡Å ¡ú v ) LOGICAL OR ¡ú LATIN SMALL LETTER V#
			{8746, "U"},    // MA#* ( ¡È ¡ú U ) UNION ¡ú LATIN CAPITAL LETTER U# ¡ú?¡ú
			{8758, ":"},    // MA#* ( ¡Ã ¡ú : ) RATIO ¡ú COLON#
			{8764, "~"},    // MA#* ( ? ¡ú ~ ) TILDE OPERATOR ¡ú TILDE#
			{8810, "<<"},   // MA#* ( ? ¡ú << ) MUCH LESS-THAN ¡ú LESS-THAN SIGN, LESS-THAN SIGN#
			{8811, ">>"},   // MA#* ( ? ¡ú >> ) MUCH GREATER-THAN ¡ú GREATER-THAN SIGN, GREATER-THAN SIGN#
			{8868, "T"},    // MA#* ( ? ¡ú T ) DOWN TACK ¡ú LATIN CAPITAL LETTER T#
			{8897, "v"},    // MA#* ( ? ¡ú v ) N-ARY LOGICAL OR ¡ú LATIN SMALL LETTER V# ¡ú¡Å¡ú
			{8899, "U"},    // MA#* ( ? ¡ú U ) N-ARY UNION ¡ú LATIN CAPITAL LETTER U# ¡ú¡È¡ú¡ú?¡ú
			{8920, "<<<"},  // MA#* ( ? ¡ú <<< ) VERY MUCH LESS-THAN ¡ú LESS-THAN SIGN, LESS-THAN SIGN, LESS-THAN SIGN#
			{8921, ">>>"},  // MA#* ( ? ¡ú >>> ) VERY MUCH GREATER-THAN ¡ú GREATER-THAN SIGN, GREATER-THAN SIGN, GREATER-THAN SIGN#
			{8959, "E"},    // MA#* ( ? ¡ú E ) Z NOTATION BAG MEMBERSHIP ¡ú LATIN CAPITAL LETTER E#
			{9075, "i"},    // MA#* ( ? ¡ú i ) APL FUNCTIONAL SYMBOL IOTA ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{9076, "p"},    // MA#* ( ? ¡ú p ) APL FUNCTIONAL SYMBOL RHO ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{9082, "a"},    // MA#* ( ? ¡ú a ) APL FUNCTIONAL SYMBOL ALPHA ¡ú LATIN SMALL LETTER A# ¡ú¦Á¡ú
			{9213, "l"},    // MA#* ( ? ¡ú l ) POWER ON SYMBOL ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{9290, "\\\\"}, // MA#* ( ? ¡ú \\ ) OCR DOUBLE BACKSLASH ¡ú REVERSE SOLIDUS, REVERSE SOLIDUS#
			{9332, "(l)"},  // MA#* ( ¢Å ¡ú (l) ) PARENTHESIZED DIGIT ONE ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, RIGHT PARENTHESIS# ¡ú(1)¡ú
			{9333, "(2)"},  // MA#* ( ¢Æ ¡ú (2) ) PARENTHESIZED DIGIT TWO ¡ú LEFT PARENTHESIS, DIGIT TWO, RIGHT PARENTHESIS#
			{9334, "(3)"},  // MA#* ( ¢Ç ¡ú (3) ) PARENTHESIZED DIGIT THREE ¡ú LEFT PARENTHESIS, DIGIT THREE, RIGHT PARENTHESIS#
			{9335, "(4)"},  // MA#* ( ¢È ¡ú (4) ) PARENTHESIZED DIGIT FOUR ¡ú LEFT PARENTHESIS, DIGIT FOUR, RIGHT PARENTHESIS#
			{9336, "(5)"},  // MA#* ( ¢É ¡ú (5) ) PARENTHESIZED DIGIT FIVE ¡ú LEFT PARENTHESIS, DIGIT FIVE, RIGHT PARENTHESIS#
			{9337, "(6)"},  // MA#* ( ¢Ê ¡ú (6) ) PARENTHESIZED DIGIT SIX ¡ú LEFT PARENTHESIS, DIGIT SIX, RIGHT PARENTHESIS#
			{9338, "(7)"},  // MA#* ( ¢Ë ¡ú (7) ) PARENTHESIZED DIGIT SEVEN ¡ú LEFT PARENTHESIS, DIGIT SEVEN, RIGHT PARENTHESIS#
			{9339, "(8)"},  // MA#* ( ¢Ì ¡ú (8) ) PARENTHESIZED DIGIT EIGHT ¡ú LEFT PARENTHESIS, DIGIT EIGHT, RIGHT PARENTHESIS#
			{9340, "(9)"},  // MA#* ( ¢Í ¡ú (9) ) PARENTHESIZED DIGIT NINE ¡ú LEFT PARENTHESIS, DIGIT NINE, RIGHT PARENTHESIS#
			{9341, "(lO)"}, // MA#* ( ¢Î ¡ú (lO) ) PARENTHESIZED NUMBER TEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, LATIN CAPITAL LETTER O, RIGHT PARENTHESIS# ¡ú(10)¡ú
			{9342, "(ll)"}, // MA#* ( ¢Ï ¡ú (ll) ) PARENTHESIZED NUMBER ELEVEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, LATIN SMALL LETTER L, RIGHT PARENTHESIS# ¡ú(11)¡ú
			{9343, "(l2)"}, // MA#* ( ¢Ð ¡ú (l2) ) PARENTHESIZED NUMBER TWELVE ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, DIGIT TWO, RIGHT PARENTHESIS# ¡ú(12)¡ú
			{9344, "(l3)"}, // MA#* ( ¢Ñ ¡ú (l3) ) PARENTHESIZED NUMBER THIRTEEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, DIGIT THREE, RIGHT PARENTHESIS# ¡ú(13)¡ú
			{9345, "(l4)"}, // MA#* ( ¢Ò ¡ú (l4) ) PARENTHESIZED NUMBER FOURTEEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, DIGIT FOUR, RIGHT PARENTHESIS# ¡ú(14)¡ú
			{9346, "(l5)"}, // MA#* ( ¢Ó ¡ú (l5) ) PARENTHESIZED NUMBER FIFTEEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, DIGIT FIVE, RIGHT PARENTHESIS# ¡ú(15)¡ú
			{9347, "(l6)"}, // MA#* ( ¢Ô ¡ú (l6) ) PARENTHESIZED NUMBER SIXTEEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, DIGIT SIX, RIGHT PARENTHESIS# ¡ú(16)¡ú
			{9348, "(l7)"}, // MA#* ( ¢Õ ¡ú (l7) ) PARENTHESIZED NUMBER SEVENTEEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, DIGIT SEVEN, RIGHT PARENTHESIS# ¡ú(17)¡ú
			{9349, "(l8)"}, // MA#* ( ¢Ö ¡ú (l8) ) PARENTHESIZED NUMBER EIGHTEEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, DIGIT EIGHT, RIGHT PARENTHESIS# ¡ú(18)¡ú
			{9350, "(l9)"}, // MA#* ( ¢× ¡ú (l9) ) PARENTHESIZED NUMBER NINETEEN ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, DIGIT NINE, RIGHT PARENTHESIS# ¡ú(19)¡ú
			{9351, "(2O)"}, // MA#* ( ¢Ø ¡ú (2O) ) PARENTHESIZED NUMBER TWENTY ¡ú LEFT PARENTHESIS, DIGIT TWO, LATIN CAPITAL LETTER O, RIGHT PARENTHESIS# ¡ú(20)¡ú
			{9352, "l."},   // MA#* ( ¢± ¡ú l. ) DIGIT ONE FULL STOP ¡ú LATIN SMALL LETTER L, FULL STOP# ¡ú1.¡ú
			{9353, "2."},   // MA#* ( ¢² ¡ú 2. ) DIGIT TWO FULL STOP ¡ú DIGIT TWO, FULL STOP#
			{9354, "3."},   // MA#* ( ¢³ ¡ú 3. ) DIGIT THREE FULL STOP ¡ú DIGIT THREE, FULL STOP#
			{9355, "4."},   // MA#* ( ¢´ ¡ú 4. ) DIGIT FOUR FULL STOP ¡ú DIGIT FOUR, FULL STOP#
			{9356, "5."},   // MA#* ( ¢µ ¡ú 5. ) DIGIT FIVE FULL STOP ¡ú DIGIT FIVE, FULL STOP#
			{9357, "6."},   // MA#* ( ¢¶ ¡ú 6. ) DIGIT SIX FULL STOP ¡ú DIGIT SIX, FULL STOP#
			{9358, "7."},   // MA#* ( ¢· ¡ú 7. ) DIGIT SEVEN FULL STOP ¡ú DIGIT SEVEN, FULL STOP#
			{9359, "8."},   // MA#* ( ¢¸ ¡ú 8. ) DIGIT EIGHT FULL STOP ¡ú DIGIT EIGHT, FULL STOP#
			{9360, "9."},   // MA#* ( ¢¹ ¡ú 9. ) DIGIT NINE FULL STOP ¡ú DIGIT NINE, FULL STOP#
			{9361, "lO."},  // MA#* ( ¢º ¡ú lO. ) NUMBER TEN FULL STOP ¡ú LATIN SMALL LETTER L, LATIN CAPITAL LETTER O, FULL STOP# ¡ú10.¡ú
			{9362, "ll."},  // MA#* ( ¢» ¡ú ll. ) NUMBER ELEVEN FULL STOP ¡ú LATIN SMALL LETTER L, LATIN SMALL LETTER L, FULL STOP# ¡ú11.¡ú
			{9363, "l2."},  // MA#* ( ¢¼ ¡ú l2. ) NUMBER TWELVE FULL STOP ¡ú LATIN SMALL LETTER L, DIGIT TWO, FULL STOP# ¡ú12.¡ú
			{9364, "l3."},  // MA#* ( ¢½ ¡ú l3. ) NUMBER THIRTEEN FULL STOP ¡ú LATIN SMALL LETTER L, DIGIT THREE, FULL STOP# ¡ú13.¡ú
			{9365, "l4."},  // MA#* ( ¢¾ ¡ú l4. ) NUMBER FOURTEEN FULL STOP ¡ú LATIN SMALL LETTER L, DIGIT FOUR, FULL STOP# ¡ú14.¡ú
			{9366, "l5."},  // MA#* ( ¢¿ ¡ú l5. ) NUMBER FIFTEEN FULL STOP ¡ú LATIN SMALL LETTER L, DIGIT FIVE, FULL STOP# ¡ú15.¡ú
			{9367, "l6."},  // MA#* ( ¢À ¡ú l6. ) NUMBER SIXTEEN FULL STOP ¡ú LATIN SMALL LETTER L, DIGIT SIX, FULL STOP# ¡ú16.¡ú
			{9368, "l7."},  // MA#* ( ¢Á ¡ú l7. ) NUMBER SEVENTEEN FULL STOP ¡ú LATIN SMALL LETTER L, DIGIT SEVEN, FULL STOP# ¡ú17.¡ú
			{9369, "l8."},  // MA#* ( ¢Â ¡ú l8. ) NUMBER EIGHTEEN FULL STOP ¡ú LATIN SMALL LETTER L, DIGIT EIGHT, FULL STOP# ¡ú18.¡ú
			{9370, "l9."},  // MA#* ( ¢Ã ¡ú l9. ) NUMBER NINETEEN FULL STOP ¡ú LATIN SMALL LETTER L, DIGIT NINE, FULL STOP# ¡ú19.¡ú
			{9371, "2O."},  // MA#* ( ¢Ä ¡ú 2O. ) NUMBER TWENTY FULL STOP ¡ú DIGIT TWO, LATIN CAPITAL LETTER O, FULL STOP# ¡ú20.¡ú
			{9372, "(a)"},  // MA#* ( ? ¡ú (a) ) PARENTHESIZED LATIN SMALL LETTER A ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER A, RIGHT PARENTHESIS#
			{9373, "(b)"},  // MA#* ( ? ¡ú (b) ) PARENTHESIZED LATIN SMALL LETTER B ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER B, RIGHT PARENTHESIS#
			{9374, "(c)"},  // MA#* ( ? ¡ú (c) ) PARENTHESIZED LATIN SMALL LETTER C ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER C, RIGHT PARENTHESIS#
			{9375, "(d)"},  // MA#* ( ? ¡ú (d) ) PARENTHESIZED LATIN SMALL LETTER D ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER D, RIGHT PARENTHESIS#
			{9376, "(e)"},  // MA#* ( ? ¡ú (e) ) PARENTHESIZED LATIN SMALL LETTER E ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER E, RIGHT PARENTHESIS#
			{9377, "(f)"},  // MA#* ( ? ¡ú (f) ) PARENTHESIZED LATIN SMALL LETTER F ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER F, RIGHT PARENTHESIS#
			{9378, "(g)"},  // MA#* ( ? ¡ú (g) ) PARENTHESIZED LATIN SMALL LETTER G ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER G, RIGHT PARENTHESIS#
			{9379, "(h)"},  // MA#* ( ? ¡ú (h) ) PARENTHESIZED LATIN SMALL LETTER H ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER H, RIGHT PARENTHESIS#
			{9380, "(i)"},  // MA#* ( ? ¡ú (i) ) PARENTHESIZED LATIN SMALL LETTER I ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER I, RIGHT PARENTHESIS#
			{9381, "(j)"},  // MA#* ( ? ¡ú (j) ) PARENTHESIZED LATIN SMALL LETTER J ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER J, RIGHT PARENTHESIS#
			{9382, "(k)"},  // MA#* ( ? ¡ú (k) ) PARENTHESIZED LATIN SMALL LETTER K ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER K, RIGHT PARENTHESIS#
			{9383, "(l)"},  // MA#* ( ? ¡ú (l) ) PARENTHESIZED LATIN SMALL LETTER L ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, RIGHT PARENTHESIS#
			{9384, "(rn)"}, // MA#* ( ? ¡ú (rn) ) PARENTHESIZED LATIN SMALL LETTER M ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER R, LATIN SMALL LETTER N, RIGHT PARENTHESIS# ¡ú(m)¡ú
			{9385, "(n)"},  // MA#* ( ? ¡ú (n) ) PARENTHESIZED LATIN SMALL LETTER N ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER N, RIGHT PARENTHESIS#
			{9386, "(o)"},  // MA#* ( ? ¡ú (o) ) PARENTHESIZED LATIN SMALL LETTER O ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER O, RIGHT PARENTHESIS#
			{9387, "(p)"},  // MA#* ( ? ¡ú (p) ) PARENTHESIZED LATIN SMALL LETTER P ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER P, RIGHT PARENTHESIS#
			{9388, "(q)"},  // MA#* ( ? ¡ú (q) ) PARENTHESIZED LATIN SMALL LETTER Q ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER Q, RIGHT PARENTHESIS#
			{9389, "(r)"},  // MA#* ( ? ¡ú (r) ) PARENTHESIZED LATIN SMALL LETTER R ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER R, RIGHT PARENTHESIS#
			{9390, "(s)"},  // MA#* ( ? ¡ú (s) ) PARENTHESIZED LATIN SMALL LETTER S ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER S, RIGHT PARENTHESIS#
			{9391, "(t)"},  // MA#* ( ? ¡ú (t) ) PARENTHESIZED LATIN SMALL LETTER T ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER T, RIGHT PARENTHESIS#
			{9392, "(u)"},  // MA#* ( ? ¡ú (u) ) PARENTHESIZED LATIN SMALL LETTER U ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER U, RIGHT PARENTHESIS#
			{9393, "(v)"},  // MA#* ( ? ¡ú (v) ) PARENTHESIZED LATIN SMALL LETTER V ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER V, RIGHT PARENTHESIS#
			{9394, "(w)"},  // MA#* ( ? ¡ú (w) ) PARENTHESIZED LATIN SMALL LETTER W ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER W, RIGHT PARENTHESIS#
			{9395, "(x)"},  // MA#* ( ? ¡ú (x) ) PARENTHESIZED LATIN SMALL LETTER X ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER X, RIGHT PARENTHESIS#
			{9396, "(y)"},  // MA#* ( ? ¡ú (y) ) PARENTHESIZED LATIN SMALL LETTER Y ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER Y, RIGHT PARENTHESIS#
			{9397, "(z)"},  // MA#* ( ? ¡ú (z) ) PARENTHESIZED LATIN SMALL LETTER Z ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER Z, RIGHT PARENTHESIS#
			{9585, "/"},    // MA#* ( ¨u ¡ú / ) BOX DRAWINGS LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT ¡ú SOLIDUS#
			{9587, "X"},    // MA#* ( ¨w ¡ú X ) BOX DRAWINGS LIGHT DIAGONAL CROSS ¡ú LATIN CAPITAL LETTER X#
			{10088, "("},   // MA#* ( ? ¡ú ( ) MEDIUM LEFT PARENTHESIS ORNAMENT ¡ú LEFT PARENTHESIS#
			{10089, ")"},   // MA#* ( ? ¡ú ) ) MEDIUM RIGHT PARENTHESIS ORNAMENT ¡ú RIGHT PARENTHESIS#
			{10094, "<"},   // MA#* ( ? ¡ú < ) HEAVY LEFT-POINTING ANGLE QUOTATION MARK ORNAMENT ¡ú LESS-THAN SIGN# ¡ú?¡ú
			{10095, ">"},   // MA#* ( ? ¡ú > ) HEAVY RIGHT-POINTING ANGLE QUOTATION MARK ORNAMENT ¡ú GREATER-THAN SIGN# ¡ú?¡ú
			{10098, "("},   // MA#* ( ? ¡ú ( ) LIGHT LEFT TORTOISE SHELL BRACKET ORNAMENT ¡ú LEFT PARENTHESIS# ¡ú¡²¡ú
			{10099, ")"},   // MA#* ( ? ¡ú ) ) LIGHT RIGHT TORTOISE SHELL BRACKET ORNAMENT ¡ú RIGHT PARENTHESIS# ¡ú¡³¡ú
			{10100, "{"},   // MA#* ( ? ¡ú { ) MEDIUM LEFT CURLY BRACKET ORNAMENT ¡ú LEFT CURLY BRACKET#
			{10101, "}"},   // MA#* ( ? ¡ú } ) MEDIUM RIGHT CURLY BRACKET ORNAMENT ¡ú RIGHT CURLY BRACKET#
			{10133, "+"},   // MA#* ( ? ¡ú + ) HEAVY PLUS SIGN ¡ú PLUS SIGN#
			{10134, "-"},   // MA#* ( ? ¡ú - ) HEAVY MINUS SIGN ¡ú HYPHEN-MINUS# ¡ú?¡ú
			{10187, "/"},   // MA#* ( ? ¡ú / ) MATHEMATICAL RISING DIAGONAL ¡ú SOLIDUS#
			{10189, "\\"},  // MA#* ( ? ¡ú \ ) MATHEMATICAL FALLING DIAGONAL ¡ú REVERSE SOLIDUS#
			{10201, "T"},   // MA#* ( ? ¡ú T ) LARGE DOWN TACK ¡ú LATIN CAPITAL LETTER T#
			{10539, "x"},   // MA#* ( ? ¡ú x ) RISING DIAGONAL CROSSING FALLING DIAGONAL ¡ú LATIN SMALL LETTER X#
			{10540, "x"},   // MA#* ( ? ¡ú x ) FALLING DIAGONAL CROSSING RISING DIAGONAL ¡ú LATIN SMALL LETTER X#
			{10741, "\\"},  // MA#* ( ? ¡ú \ ) REVERSE SOLIDUS OPERATOR ¡ú REVERSE SOLIDUS#
			{10744, "/"},   // MA#* ( ? ¡ú / ) BIG SOLIDUS ¡ú SOLIDUS#
			{10745, "\\"},  // MA#* ( ? ¡ú \ ) BIG REVERSE SOLIDUS ¡ú REVERSE SOLIDUS#
			{10784, ">>"},  // MA#* ( ? ¡ú >> ) Z NOTATION SCHEMA PIPING ¡ú GREATER-THAN SIGN, GREATER-THAN SIGN# ¡ú?¡ú
			{10799, "x"},   // MA#* ( ? ¡ú x ) VECTOR OR CROSS PRODUCT ¡ú LATIN SMALL LETTER X# ¡ú¡Á¡ú
			{10868, "::="}, // MA#* ( ? ¡ú ::= ) DOUBLE COLON EQUAL ¡ú COLON, COLON, EQUALS SIGN#
			{10869, "=="},  // MA#* ( ? ¡ú == ) TWO CONSECUTIVE EQUALS SIGNS ¡ú EQUALS SIGN, EQUALS SIGN#
			{10870, "==="}, // MA#* ( ? ¡ú === ) THREE CONSECUTIVE EQUALS SIGNS ¡ú EQUALS SIGN, EQUALS SIGN, EQUALS SIGN#
			{10917, "><"},  // MA#* ( ? ¡ú >< ) GREATER-THAN BESIDE LESS-THAN ¡ú GREATER-THAN SIGN, LESS-THAN SIGN#
			{11003, "///"}, // MA#* ( ? ¡ú /// ) TRIPLE SOLIDUS BINARY RELATION ¡ú SOLIDUS, SOLIDUS, SOLIDUS#
			{11005, "//"},  // MA#* ( ? ¡ú // ) DOUBLE SOLIDUS OPERATOR ¡ú SOLIDUS, SOLIDUS#
			{11397, "r"},   // MA# ( ? ¡ú r ) COPTIC SMALL LETTER GAMMA ¡ú LATIN SMALL LETTER R# ¡ú§Ô¡ú
			{11406, "H"},   // MA# ( ? ¡ú H ) COPTIC CAPITAL LETTER HATE ¡ú LATIN CAPITAL LETTER H# ¡ú¦§¡ú
			{11410, "l"},   // MA# ( ? ¡ú l ) COPTIC CAPITAL LETTER IAUDA ¡ú LATIN SMALL LETTER L# ¡ú?¡ú
			{11412, "K"},   // MA# ( ? ¡ú K ) COPTIC CAPITAL LETTER KAPA ¡ú LATIN CAPITAL LETTER K# ¡ú¦ª¡ú
			{11416, "M"},   // MA# ( ? ¡ú M ) COPTIC CAPITAL LETTER MI ¡ú LATIN CAPITAL LETTER M#
			{11418, "N"},   // MA# ( ? ¡ú N ) COPTIC CAPITAL LETTER NI ¡ú LATIN CAPITAL LETTER N#
			{11422, "O"},   // MA# ( ? ¡ú O ) COPTIC CAPITAL LETTER O ¡ú LATIN CAPITAL LETTER O#
			{11423, "o"},   // MA# ( ? ¡ú o ) COPTIC SMALL LETTER O ¡ú LATIN SMALL LETTER O#
			{11426, "P"},   // MA# ( ? ¡ú P ) COPTIC CAPITAL LETTER RO ¡ú LATIN CAPITAL LETTER P#
			{11427, "p"},   // MA# ( ? ¡ú p ) COPTIC SMALL LETTER RO ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{11428, "C"},   // MA# ( ? ¡ú C ) COPTIC CAPITAL LETTER SIMA ¡ú LATIN CAPITAL LETTER C# ¡ú?¡ú
			{11429, "c"},   // MA# ( ? ¡ú c ) COPTIC SMALL LETTER SIMA ¡ú LATIN SMALL LETTER C# ¡ú?¡ú
			{11430, "T"},   // MA# ( ? ¡ú T ) COPTIC CAPITAL LETTER TAU ¡ú LATIN CAPITAL LETTER T#
			{11432, "Y"},   // MA# ( ? ¡ú Y ) COPTIC CAPITAL LETTER UA ¡ú LATIN CAPITAL LETTER Y#
			{11436, "X"},   // MA# ( ? ¡ú X ) COPTIC CAPITAL LETTER KHI ¡ú LATIN CAPITAL LETTER X# ¡ú§·¡ú
			{11450, "-"},   // MA# ( ? ¡ú - ) COPTIC CAPITAL LETTER DIALECT-P NI ¡ú HYPHEN-MINUS# ¡ú?¡ú
			{11462, "/"},   // MA# ( ? ¡ú / ) COPTIC CAPITAL LETTER OLD COPTIC ESH ¡ú SOLIDUS#
			{11466, "9"},   // MA# ( ? ¡ú 9 ) COPTIC CAPITAL LETTER DIALECT-P HORI ¡ú DIGIT NINE#
			{11468, "3"},   // MA# ( ? ¡ú 3 ) COPTIC CAPITAL LETTER OLD COPTIC HORI ¡ú DIGIT THREE# ¡ú?¡ú¡ú?¡ú
			{11472, "L"},   // MA# ( ? ¡ú L ) COPTIC CAPITAL LETTER L-SHAPED HA ¡ú LATIN CAPITAL LETTER L#
			{11474, "6"},   // MA# ( ? ¡ú 6 ) COPTIC CAPITAL LETTER OLD COPTIC HEI ¡ú DIGIT SIX#
			{11513, "\\\\"},// MA#* ( ? ¡ú \\ ) COPTIC OLD NUBIAN FULL STOP ¡ú REVERSE SOLIDUS, REVERSE SOLIDUS#
			{11576, "V"},   // MA# ( ? ¡ú V ) TIFINAGH LETTER YADH ¡ú LATIN CAPITAL LETTER V#
			{11577, "E"},   // MA# ( ? ¡ú E ) TIFINAGH LETTER YADD ¡ú LATIN CAPITAL LETTER E#
			{11599, "l"},   // MA# ( ? ¡ú l ) TIFINAGH LETTER YAN ¡ú LATIN SMALL LETTER L# ¡ú?¡ú
			{11601, "!"},   // MA# ( ? ¡ú ! ) TIFINAGH LETTER TUAREG YANG ¡ú EXCLAMATION MARK#
			{11604, "O"},   // MA# ( ? ¡ú O ) TIFINAGH LETTER YAR ¡ú LATIN CAPITAL LETTER O#
			{11605, "Q"},   // MA# ( ? ¡ú Q ) TIFINAGH LETTER YARR ¡ú LATIN CAPITAL LETTER Q#
			{11613, "X"},   // MA# ( ? ¡ú X ) TIFINAGH LETTER YATH ¡ú LATIN CAPITAL LETTER X#
			{11816, "(("},  // MA#* ( ? ¡ú (( ) LEFT DOUBLE PARENTHESIS ¡ú LEFT PARENTHESIS, LEFT PARENTHESIS#
			{11817, "))"},  // MA#* ( ? ¡ú )) ) RIGHT DOUBLE PARENTHESIS ¡ú RIGHT PARENTHESIS, RIGHT PARENTHESIS#
			{11840, "="},   // MA#* ( ? ¡ú = ) DOUBLE HYPHEN ¡ú EQUALS SIGN#
			{12034, "\\"},  // MA#* ( ? ¡ú \ ) KANGXI RADICAL DOT ¡ú REVERSE SOLIDUS#
			{12035, "/"},   // MA#* ( ? ¡ú / ) KANGXI RADICAL SLASH ¡ú SOLIDUS#
			{12291, "\""},  // MA#* ( ¡¨ ¡ú '' ) DITTO MARK ¡ú APOSTROPHE, APOSTROPHE# ¡ú¡å¡ú¡ú"¡ú# Converted to a quote.
			{12295, "O"},   // MA# ( ©– ¡ú O ) IDEOGRAPHIC NUMBER ZERO ¡ú LATIN CAPITAL LETTER O#
			{12308, "("},   // MA#* ( ¡² ¡ú ( ) LEFT TORTOISE SHELL BRACKET ¡ú LEFT PARENTHESIS#
			{12309, ")"},   // MA#* ( ¡³ ¡ú ) ) RIGHT TORTOISE SHELL BRACKET ¡ú RIGHT PARENTHESIS#
			{12339, "/"},   // MA# ( ? ¡ú / ) VERTICAL KANA REPEAT MARK UPPER HALF ¡ú SOLIDUS#
			{12448, "="},   // MA#* ( ? ¡ú = ) KATAKANA-HIRAGANA DOUBLE HYPHEN ¡ú EQUALS SIGN#
			{12494, "/"},   // MA# ( ¥Î ¡ú / ) KATAKANA LETTER NO ¡ú SOLIDUS# ¡ú?¡ú
			{12755, "/"},   // MA#* ( ? ¡ú / ) CJK STROKE SP ¡ú SOLIDUS# ¡ú?¡ú
			{12756, "\\"},  // MA#* ( ? ¡ú \ ) CJK STROKE D ¡ú REVERSE SOLIDUS# ¡ú?¡ú
			{20022, "\\"},  // MA# ( Ø¼ ¡ú \ ) CJK UNIFIED IDEOGRAPH-4E36 ¡ú REVERSE SOLIDUS# ¡ú?¡ú
			{20031, "/"},   // MA# ( Ø¯ ¡ú / ) CJK UNIFIED IDEOGRAPH-4E3F ¡ú SOLIDUS# ¡ú?¡ú
			{42192, "B"},   // MA# ( ? ¡ú B ) LISU LETTER BA ¡ú LATIN CAPITAL LETTER B#
			{42193, "P"},   // MA# ( ? ¡ú P ) LISU LETTER PA ¡ú LATIN CAPITAL LETTER P#
			{42194, "d"},   // MA# ( ? ¡ú d ) LISU LETTER PHA ¡ú LATIN SMALL LETTER D#
			{42195, "D"},   // MA# ( ? ¡ú D ) LISU LETTER DA ¡ú LATIN CAPITAL LETTER D#
			{42196, "T"},   // MA# ( ? ¡ú T ) LISU LETTER TA ¡ú LATIN CAPITAL LETTER T#
			{42198, "G"},   // MA# ( ? ¡ú G ) LISU LETTER GA ¡ú LATIN CAPITAL LETTER G#
			{42199, "K"},   // MA# ( ? ¡ú K ) LISU LETTER KA ¡ú LATIN CAPITAL LETTER K#
			{42201, "J"},   // MA# ( ? ¡ú J ) LISU LETTER JA ¡ú LATIN CAPITAL LETTER J#
			{42202, "C"},   // MA# ( ? ¡ú C ) LISU LETTER CA ¡ú LATIN CAPITAL LETTER C#
			{42204, "Z"},   // MA# ( ? ¡ú Z ) LISU LETTER DZA ¡ú LATIN CAPITAL LETTER Z#
			{42205, "F"},   // MA# ( ? ¡ú F ) LISU LETTER TSA ¡ú LATIN CAPITAL LETTER F#
			{42207, "M"},   // MA# ( ? ¡ú M ) LISU LETTER MA ¡ú LATIN CAPITAL LETTER M#
			{42208, "N"},   // MA# ( ? ¡ú N ) LISU LETTER NA ¡ú LATIN CAPITAL LETTER N#
			{42209, "L"},   // MA# ( ? ¡ú L ) LISU LETTER LA ¡ú LATIN CAPITAL LETTER L#
			{42210, "S"},   // MA# ( ? ¡ú S ) LISU LETTER SA ¡ú LATIN CAPITAL LETTER S#
			{42211, "R"},   // MA# ( ? ¡ú R ) LISU LETTER ZHA ¡ú LATIN CAPITAL LETTER R#
			{42214, "V"},   // MA# ( ? ¡ú V ) LISU LETTER HA ¡ú LATIN CAPITAL LETTER V#
			{42215, "H"},   // MA# ( ? ¡ú H ) LISU LETTER XA ¡ú LATIN CAPITAL LETTER H#
			{42218, "W"},   // MA# ( ? ¡ú W ) LISU LETTER WA ¡ú LATIN CAPITAL LETTER W#
			{42219, "X"},   // MA# ( ? ¡ú X ) LISU LETTER SHA ¡ú LATIN CAPITAL LETTER X#
			{42220, "Y"},   // MA# ( ? ¡ú Y ) LISU LETTER YA ¡ú LATIN CAPITAL LETTER Y#
			{42222, "A"},   // MA# ( ? ¡ú A ) LISU LETTER A ¡ú LATIN CAPITAL LETTER A#
			{42224, "E"},   // MA# ( ? ¡ú E ) LISU LETTER E ¡ú LATIN CAPITAL LETTER E#
			{42226, "l"},   // MA# ( ? ¡ú l ) LISU LETTER I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{42227, "O"},   // MA# ( ? ¡ú O ) LISU LETTER O ¡ú LATIN CAPITAL LETTER O#
			{42228, "U"},   // MA# ( ? ¡ú U ) LISU LETTER U ¡ú LATIN CAPITAL LETTER U#
			{42232, "."},   // MA# ( ? ¡ú . ) LISU LETTER TONE MYA TI ¡ú FULL STOP#
			{42233, ","},   // MA# ( ? ¡ú , ) LISU LETTER TONE NA PO ¡ú COMMA#
			{42234, ".."},  // MA# ( ? ¡ú .. ) LISU LETTER TONE MYA CYA ¡ú FULL STOP, FULL STOP#
			{42235, ".,"},  // MA# ( ? ¡ú ., ) LISU LETTER TONE MYA BO ¡ú FULL STOP, COMMA#
			{42237, ":"},   // MA# ( ? ¡ú : ) LISU LETTER TONE MYA JEU ¡ú COLON#
			{42238, "-."},  // MA#* ( ? ¡ú -. ) LISU PUNCTUATION COMMA ¡ú HYPHEN-MINUS, FULL STOP#
			{42239, "="},   // MA#* ( ? ¡ú = ) LISU PUNCTUATION FULL STOP ¡ú EQUALS SIGN#
			{42510, "."},   // MA#* ( ? ¡ú . ) VAI FULL STOP ¡ú FULL STOP#
			{42564, "2"},   // MA# ( ? ¡ú 2 ) CYRILLIC CAPITAL LETTER REVERSED DZE ¡ú DIGIT TWO# ¡ú?¡ú
			{42567, "i"},   // MA# ( ? ¡ú i ) CYRILLIC SMALL LETTER IOTA ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{42648, "OO"},  // MA# ( ? ¡ú OO ) CYRILLIC CAPITAL LETTER DOUBLE O ¡ú LATIN CAPITAL LETTER O, LATIN CAPITAL LETTER O#
			{42649, "oo"},  // MA# ( ? ¡ú oo ) CYRILLIC SMALL LETTER DOUBLE O ¡ú LATIN SMALL LETTER O, LATIN SMALL LETTER O#
			{42719, "V"},   // MA# ( ? ¡ú V ) BAMUM LETTER KO ¡ú LATIN CAPITAL LETTER V#
			{42731, "?"},   // MA# ( ? ¡ú ? ) BAMUM LETTER NTUU ¡ú QUESTION MARK# ¡ú?¡ú
			{42735, "2"},   // MA# ( ? ¡ú 2 ) BAMUM LETTER KOGHOM ¡ú DIGIT TWO# ¡ú?¡ú
			{42792, "T3"},  // MA# ( ? ¡ú T3 ) LATIN CAPITAL LETTER TZ ¡ú LATIN CAPITAL LETTER T, DIGIT THREE# ¡úT?¡ú
			{42801, "s"},   // MA# ( ? ¡ú s ) LATIN LETTER SMALL CAPITAL S ¡ú LATIN SMALL LETTER S#
			{42802, "AA"},  // MA# ( ? ¡ú AA ) LATIN CAPITAL LETTER AA ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER A#
			{42803, "aa"},  // MA# ( ? ¡ú aa ) LATIN SMALL LETTER AA ¡ú LATIN SMALL LETTER A, LATIN SMALL LETTER A#
			{42804, "AO"},  // MA# ( ? ¡ú AO ) LATIN CAPITAL LETTER AO ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER O#
			{42805, "ao"},  // MA# ( ? ¡ú ao ) LATIN SMALL LETTER AO ¡ú LATIN SMALL LETTER A, LATIN SMALL LETTER O#
			{42806, "AU"},  // MA# ( ? ¡ú AU ) LATIN CAPITAL LETTER AU ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER U#
			{42807, "au"},  // MA# ( ? ¡ú au ) LATIN SMALL LETTER AU ¡ú LATIN SMALL LETTER A, LATIN SMALL LETTER U#
			{42808, "AV"},  // MA# ( ? ¡ú AV ) LATIN CAPITAL LETTER AV ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER V#
			{42809, "av"},  // MA# ( ? ¡ú av ) LATIN SMALL LETTER AV ¡ú LATIN SMALL LETTER A, LATIN SMALL LETTER V#
			{42810, "AV"},  // MA# ( ? ¡ú AV ) LATIN CAPITAL LETTER AV WITH HORIZONTAL BAR ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER V#
			{42811, "av"},  // MA# ( ? ¡ú av ) LATIN SMALL LETTER AV WITH HORIZONTAL BAR ¡ú LATIN SMALL LETTER A, LATIN SMALL LETTER V#
			{42812, "AY"},  // MA# ( ? ¡ú AY ) LATIN CAPITAL LETTER AY ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER Y#
			{42813, "ay"},  // MA# ( ? ¡ú ay ) LATIN SMALL LETTER AY ¡ú LATIN SMALL LETTER A, LATIN SMALL LETTER Y#
			{42830, "OO"},  // MA# ( ? ¡ú OO ) LATIN CAPITAL LETTER OO ¡ú LATIN CAPITAL LETTER O, LATIN CAPITAL LETTER O#
			{42831, "oo"},  // MA# ( ? ¡ú oo ) LATIN SMALL LETTER OO ¡ú LATIN SMALL LETTER O, LATIN SMALL LETTER O#
			{42842, "2"},   // MA# ( ? ¡ú 2 ) LATIN CAPITAL LETTER R ROTUNDA ¡ú DIGIT TWO#
			{42858, "3"},   // MA# ( ? ¡ú 3 ) LATIN CAPITAL LETTER ET ¡ú DIGIT THREE#
			{42862, "9"},   // MA# ( ? ¡ú 9 ) LATIN CAPITAL LETTER CON ¡ú DIGIT NINE#
			{42871, "tf"},  // MA# ( ? ¡ú tf ) LATIN SMALL LETTER TUM ¡ú LATIN SMALL LETTER T, LATIN SMALL LETTER F#
			{42872, "&"},   // MA# ( ? ¡ú & ) LATIN SMALL LETTER UM ¡ú AMPERSAND#
			{42889, ":"},   // MA#* ( ? ¡ú : ) MODIFIER LETTER COLON ¡ú COLON#
			{42892, "'"},   // MA# ( ? ¡ú ' ) LATIN SMALL LETTER SALTILLO ¡ú APOSTROPHE#
			{42904, "F"},   // MA# ( ? ¡ú F ) LATIN CAPITAL LETTER F WITH STROKE ¡ú LATIN CAPITAL LETTER F#
			{42905, "f"},   // MA# ( ? ¡ú f ) LATIN SMALL LETTER F WITH STROKE ¡ú LATIN SMALL LETTER F#
			{42911, "u"},   // MA# ( ? ¡ú u ) LATIN SMALL LETTER VOLAPUK UE ¡ú LATIN SMALL LETTER U#
			{42923, "3"},   // MA# ( ? ¡ú 3 ) LATIN CAPITAL LETTER REVERSED OPEN E ¡ú DIGIT THREE#
			{42930, "J"},   // MA# ( ? ¡ú J ) LATIN CAPITAL LETTER J WITH CROSSED-TAIL ¡ú LATIN CAPITAL LETTER J#
			{42931, "X"},   // MA# ( ? ¡ú X ) LATIN CAPITAL LETTER CHI ¡ú LATIN CAPITAL LETTER X#
			{42932, "B"},   // MA# ( ? ¡ú B ) LATIN CAPITAL LETTER BETA ¡ú LATIN CAPITAL LETTER B#
			{43826, "e"},   // MA# ( ? ¡ú e ) LATIN SMALL LETTER BLACKLETTER E ¡ú LATIN SMALL LETTER E#
			{43829, "f"},   // MA# ( ? ¡ú f ) LATIN SMALL LETTER LENIS F ¡ú LATIN SMALL LETTER F#
			{43837, "o"},   // MA# ( ? ¡ú o ) LATIN SMALL LETTER BLACKLETTER O ¡ú LATIN SMALL LETTER O#
			{43847, "r"},   // MA# ( ? ¡ú r ) LATIN SMALL LETTER R WITHOUT HANDLE ¡ú LATIN SMALL LETTER R#
			{43848, "r"},   // MA# ( ? ¡ú r ) LATIN SMALL LETTER DOUBLE R ¡ú LATIN SMALL LETTER R#
			{43854, "u"},   // MA# ( ? ¡ú u ) LATIN SMALL LETTER U WITH SHORT RIGHT LEG ¡ú LATIN SMALL LETTER U#
			{43858, "u"},   // MA# ( ? ¡ú u ) LATIN SMALL LETTER U WITH LEFT HOOK ¡ú LATIN SMALL LETTER U#
			{43866, "y"},   // MA# ( ? ¡ú y ) LATIN SMALL LETTER Y WITH SHORT RIGHT LEG ¡ú LATIN SMALL LETTER Y#
			{43875, "uo"},  // MA# ( ? ¡ú uo ) LATIN SMALL LETTER UO ¡ú LATIN SMALL LETTER U, LATIN SMALL LETTER O#
			{43893, "i"},   // MA# ( ? ¡ú i ) CHEROKEE SMALL LETTER V ¡ú LATIN SMALL LETTER I#
			{43905, "r"},   // MA# ( ? ¡ú r ) CHEROKEE SMALL LETTER HU ¡ú LATIN SMALL LETTER R# ¡ú?¡ú¡ú§Ô¡ú
			{43907, "w"},   // MA# ( ? ¡ú w ) CHEROKEE SMALL LETTER LA ¡ú LATIN SMALL LETTER W# ¡ú?¡ú
			{43923, "z"},   // MA# ( ? ¡ú z ) CHEROKEE SMALL LETTER NO ¡ú LATIN SMALL LETTER Z# ¡ú?¡ú
			{43945, "v"},   // MA# ( ? ¡ú v ) CHEROKEE SMALL LETTER DO ¡ú LATIN SMALL LETTER V# ¡ú?¡ú
			{43946, "s"},   // MA# ( ? ¡ú s ) CHEROKEE SMALL LETTER DU ¡ú LATIN SMALL LETTER S# ¡ú?¡ú
			{43951, "c"},   // MA# ( ? ¡ú c ) CHEROKEE SMALL LETTER TLI ¡ú LATIN SMALL LETTER C# ¡ú?¡ú
			{64256, "ff"},  // MA# ( ? ¡ú ff ) LATIN SMALL LIGATURE FF ¡ú LATIN SMALL LETTER F, LATIN SMALL LETTER F#
			{64257, "fi"},  // MA# ( ? ¡ú fi ) LATIN SMALL LIGATURE FI ¡ú LATIN SMALL LETTER F, LATIN SMALL LETTER I#
			{64258, "fl"},  // MA# ( ? ¡ú fl ) LATIN SMALL LIGATURE FL ¡ú LATIN SMALL LETTER F, LATIN SMALL LETTER L#
			{64259, "ffi"}, // MA# ( ? ¡ú ffi ) LATIN SMALL LIGATURE FFI ¡ú LATIN SMALL LETTER F, LATIN SMALL LETTER F, LATIN SMALL LETTER I#
			{64260, "ffl"}, // MA# ( ? ¡ú ffl ) LATIN SMALL LIGATURE FFL ¡ú LATIN SMALL LETTER F, LATIN SMALL LETTER F, LATIN SMALL LETTER L#
			{64262, "st"},  // MA# ( ? ¡ú st ) LATIN SMALL LIGATURE ST ¡ú LATIN SMALL LETTER S, LATIN SMALL LETTER T#
			{64422, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH GOAL ISOLATED FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{64423, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH GOAL FINAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú¡ú???¡ú
			{64424, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH GOAL INITIAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú¡ú???¡ú
			{64425, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH GOAL MEDIAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú¡ú???¡ú
			{64426, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH DOACHASHMEE ISOLATED FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{64427, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH DOACHASHMEE FINAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú¡ú???¡ú
			{64428, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH DOACHASHMEE INITIAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú¡ú???¡ú
			{64429, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH DOACHASHMEE MEDIAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú¡ú???¡ú
			{64830, "("},   // MA#* ( ? ¡ú ( ) ORNATE LEFT PARENTHESIS ¡ú LEFT PARENTHESIS#
			{64831, ")"},   // MA#* ( ? ¡ú ) ) ORNATE RIGHT PARENTHESIS ¡ú RIGHT PARENTHESIS#
			{65072, ":"},   // MA#* ( ©U ¡ú : ) PRESENTATION FORM FOR VERTICAL TWO DOT LEADER ¡ú COLON#
			{65101, "_"},   // MA# ( ©l ¡ú _ ) DASHED LOW LINE ¡ú LOW LINE#
			{65102, "_"},   // MA# ( ©m ¡ú _ ) CENTRELINE LOW LINE ¡ú LOW LINE#
			{65103, "_"},   // MA# ( ©n ¡ú _ ) WAVY LOW LINE ¡ú LOW LINE#
			{65112, "-"},   // MA#* ( ? ¡ú - ) SMALL EM DASH ¡ú HYPHEN-MINUS#
			{65128, "\\"},  // MA#* ( ©… ¡ú \ ) SMALL REVERSE SOLIDUS ¡ú REVERSE SOLIDUS# ¡ú?¡ú
			{65165, "l"},   // MA# ( ??? ¡ú l ) ARABIC LETTER ALEF ISOLATED FORM ¡ú LATIN SMALL LETTER L# ¡ú???¡ú¡ú1¡ú
			{65166, "l"},   // MA# ( ??? ¡ú l ) ARABIC LETTER ALEF FINAL FORM ¡ú LATIN SMALL LETTER L# ¡ú???¡ú¡ú1¡ú
			{65257, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH ISOLATED FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{65258, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH FINAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{65259, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH INITIAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{65260, "o"},   // MA# ( ??? ¡ú o ) ARABIC LETTER HEH MEDIAL FORM ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{65281, "!"},   // MA#* ( £¡ ¡ú ! ) FULLWIDTH EXCLAMATION MARK ¡ú EXCLAMATION MARK# ¡ú?¡ú
			{65282, "\""},  // MA#* ( £¢ ¡ú '' ) FULLWIDTH QUOTATION MARK ¡ú APOSTROPHE, APOSTROPHE# ¡ú¡±¡ú¡ú"¡ú# Converted to a quote.
			{65287, "'"},   // MA#* ( £§ ¡ú ' ) FULLWIDTH APOSTROPHE ¡ú APOSTROPHE# ¡ú¡¯¡ú
			{65306, ":"},   // MA#* ( £º ¡ú : ) FULLWIDTH COLON ¡ú COLON# ¡ú©U¡ú
			{65313, "A"},   // MA# ( £Á ¡ú A ) FULLWIDTH LATIN CAPITAL LETTER A ¡ú LATIN CAPITAL LETTER A# ¡ú§¡¡ú
			{65314, "B"},   // MA# ( £Â ¡ú B ) FULLWIDTH LATIN CAPITAL LETTER B ¡ú LATIN CAPITAL LETTER B# ¡ú¦¢¡ú
			{65315, "C"},   // MA# ( £Ã ¡ú C ) FULLWIDTH LATIN CAPITAL LETTER C ¡ú LATIN CAPITAL LETTER C# ¡ú§³¡ú
			{65317, "E"},   // MA# ( £Å ¡ú E ) FULLWIDTH LATIN CAPITAL LETTER E ¡ú LATIN CAPITAL LETTER E# ¡ú¦¥¡ú
			{65320, "H"},   // MA# ( £È ¡ú H ) FULLWIDTH LATIN CAPITAL LETTER H ¡ú LATIN CAPITAL LETTER H# ¡ú¦§¡ú
			{65321, "l"},   // MA# ( £É ¡ú l ) FULLWIDTH LATIN CAPITAL LETTER I ¡ú LATIN SMALL LETTER L# ¡ú?¡ú
			{65322, "J"},   // MA# ( £Ê ¡ú J ) FULLWIDTH LATIN CAPITAL LETTER J ¡ú LATIN CAPITAL LETTER J# ¡ú?¡ú
			{65323, "K"},   // MA# ( £Ë ¡ú K ) FULLWIDTH LATIN CAPITAL LETTER K ¡ú LATIN CAPITAL LETTER K# ¡ú¦ª¡ú
			{65325, "M"},   // MA# ( £Í ¡ú M ) FULLWIDTH LATIN CAPITAL LETTER M ¡ú LATIN CAPITAL LETTER M# ¡ú¦¬¡ú
			{65326, "N"},   // MA# ( £Î ¡ú N ) FULLWIDTH LATIN CAPITAL LETTER N ¡ú LATIN CAPITAL LETTER N# ¡ú¦­¡ú
			{65327, "O"},   // MA# ( £Ï ¡ú O ) FULLWIDTH LATIN CAPITAL LETTER O ¡ú LATIN CAPITAL LETTER O# ¡ú§°¡ú
			{65328, "P"},   // MA# ( £Ð ¡ú P ) FULLWIDTH LATIN CAPITAL LETTER P ¡ú LATIN CAPITAL LETTER P# ¡ú§²¡ú
			{65331, "S"},   // MA# ( £Ó ¡ú S ) FULLWIDTH LATIN CAPITAL LETTER S ¡ú LATIN CAPITAL LETTER S# ¡ú?¡ú
			{65332, "T"},   // MA# ( £Ô ¡ú T ) FULLWIDTH LATIN CAPITAL LETTER T ¡ú LATIN CAPITAL LETTER T# ¡ú§´¡ú
			{65336, "X"},   // MA# ( £Ø ¡ú X ) FULLWIDTH LATIN CAPITAL LETTER X ¡ú LATIN CAPITAL LETTER X# ¡ú§·¡ú
			{65337, "Y"},   // MA# ( £Ù ¡ú Y ) FULLWIDTH LATIN CAPITAL LETTER Y ¡ú LATIN CAPITAL LETTER Y# ¡ú¦´¡ú
			{65338, "Z"},   // MA# ( £Ú ¡ú Z ) FULLWIDTH LATIN CAPITAL LETTER Z ¡ú LATIN CAPITAL LETTER Z# ¡ú¦¦¡ú
			{65339, "("},   // MA#* ( £Û ¡ú ( ) FULLWIDTH LEFT SQUARE BRACKET ¡ú LEFT PARENTHESIS# ¡ú¡²¡ú
			{65340, "\\"},  // MA#* ( £Ü ¡ú \ ) FULLWIDTH REVERSE SOLIDUS ¡ú REVERSE SOLIDUS# ¡ú?¡ú
			{65341, ")"},   // MA#* ( £Ý ¡ú ) ) FULLWIDTH RIGHT SQUARE BRACKET ¡ú RIGHT PARENTHESIS# ¡ú¡³¡ú
			{65344, "'"},   // MA#* ( £à ¡ú ' ) FULLWIDTH GRAVE ACCENT ¡ú APOSTROPHE# ¡ú¡®¡ú
			{65345, "a"},   // MA# ( £á ¡ú a ) FULLWIDTH LATIN SMALL LETTER A ¡ú LATIN SMALL LETTER A# ¡ú§Ñ¡ú
			{65347, "c"},   // MA# ( £ã ¡ú c ) FULLWIDTH LATIN SMALL LETTER C ¡ú LATIN SMALL LETTER C# ¡ú§ã¡ú
			{65349, "e"},   // MA# ( £å ¡ú e ) FULLWIDTH LATIN SMALL LETTER E ¡ú LATIN SMALL LETTER E# ¡ú§Ö¡ú
			{65351, "g"},   // MA# ( £ç ¡ú g ) FULLWIDTH LATIN SMALL LETTER G ¡ú LATIN SMALL LETTER G# ¡ú¨À¡ú
			{65352, "h"},   // MA# ( £è ¡ú h ) FULLWIDTH LATIN SMALL LETTER H ¡ú LATIN SMALL LETTER H# ¡ú?¡ú
			{65353, "i"},   // MA# ( £é ¡ú i ) FULLWIDTH LATIN SMALL LETTER I ¡ú LATIN SMALL LETTER I# ¡ú?¡ú
			{65354, "j"},   // MA# ( £ê ¡ú j ) FULLWIDTH LATIN SMALL LETTER J ¡ú LATIN SMALL LETTER J# ¡ú?¡ú
			{65356, "l"},   // MA# ( £ì ¡ú l ) FULLWIDTH LATIN SMALL LETTER L ¡ú LATIN SMALL LETTER L# ¡ú¢ñ¡ú¡ú?¡ú
			{65359, "o"},   // MA# ( £ï ¡ú o ) FULLWIDTH LATIN SMALL LETTER O ¡ú LATIN SMALL LETTER O# ¡ú§à¡ú
			{65360, "p"},   // MA# ( £ð ¡ú p ) FULLWIDTH LATIN SMALL LETTER P ¡ú LATIN SMALL LETTER P# ¡ú§â¡ú
			{65363, "s"},   // MA# ( £ó ¡ú s ) FULLWIDTH LATIN SMALL LETTER S ¡ú LATIN SMALL LETTER S# ¡ú?¡ú
			{65366, "v"},   // MA# ( £ö ¡ú v ) FULLWIDTH LATIN SMALL LETTER V ¡ú LATIN SMALL LETTER V# ¡ú¦Í¡ú
			{65368, "x"},   // MA# ( £ø ¡ú x ) FULLWIDTH LATIN SMALL LETTER X ¡ú LATIN SMALL LETTER X# ¡ú§ç¡ú
			{65369, "y"},   // MA# ( £ù ¡ú y ) FULLWIDTH LATIN SMALL LETTER Y ¡ú LATIN SMALL LETTER Y# ¡ú§å¡ú
			{65512, "l"},   // MA#* ( ? ¡ú l ) HALFWIDTH FORMS LIGHT VERTICAL ¡ú LATIN SMALL LETTER L# ¡ú|¡ú
			{66178, "B"},   // MA# ( ?? ¡ú B ) LYCIAN LETTER B ¡ú LATIN CAPITAL LETTER B#
			{66182, "E"},   // MA# ( ?? ¡ú E ) LYCIAN LETTER I ¡ú LATIN CAPITAL LETTER E#
			{66183, "F"},   // MA# ( ?? ¡ú F ) LYCIAN LETTER W ¡ú LATIN CAPITAL LETTER F#
			{66186, "l"},   // MA# ( ?? ¡ú l ) LYCIAN LETTER J ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{66192, "X"},   // MA# ( ?? ¡ú X ) LYCIAN LETTER MM ¡ú LATIN CAPITAL LETTER X#
			{66194, "O"},   // MA# ( ?? ¡ú O ) LYCIAN LETTER U ¡ú LATIN CAPITAL LETTER O#
			{66197, "P"},   // MA# ( ?? ¡ú P ) LYCIAN LETTER R ¡ú LATIN CAPITAL LETTER P#
			{66198, "S"},   // MA# ( ?? ¡ú S ) LYCIAN LETTER S ¡ú LATIN CAPITAL LETTER S#
			{66199, "T"},   // MA# ( ?? ¡ú T ) LYCIAN LETTER T ¡ú LATIN CAPITAL LETTER T#
			{66203, "+"},   // MA# ( ?? ¡ú + ) LYCIAN LETTER H ¡ú PLUS SIGN#
			{66208, "A"},   // MA# ( ?? ¡ú A ) CARIAN LETTER A ¡ú LATIN CAPITAL LETTER A#
			{66209, "B"},   // MA# ( ?? ¡ú B ) CARIAN LETTER P2 ¡ú LATIN CAPITAL LETTER B#
			{66210, "C"},   // MA# ( ?? ¡ú C ) CARIAN LETTER D ¡ú LATIN CAPITAL LETTER C#
			{66213, "F"},   // MA# ( ?? ¡ú F ) CARIAN LETTER R ¡ú LATIN CAPITAL LETTER F#
			{66219, "O"},   // MA# ( ?? ¡ú O ) CARIAN LETTER O ¡ú LATIN CAPITAL LETTER O#
			{66224, "M"},   // MA# ( ?? ¡ú M ) CARIAN LETTER S ¡ú LATIN CAPITAL LETTER M#
			{66225, "T"},   // MA# ( ?? ¡ú T ) CARIAN LETTER C-18 ¡ú LATIN CAPITAL LETTER T#
			{66226, "Y"},   // MA# ( ?? ¡ú Y ) CARIAN LETTER U ¡ú LATIN CAPITAL LETTER Y#
			{66228, "X"},   // MA# ( ?? ¡ú X ) CARIAN LETTER X ¡ú LATIN CAPITAL LETTER X#
			{66255, "H"},   // MA# ( ?? ¡ú H ) CARIAN LETTER E2 ¡ú LATIN CAPITAL LETTER H#
			{66293, "Z"},   // MA#* ( ?? ¡ú Z ) COPTIC EPACT NUMBER THREE HUNDRED ¡ú LATIN CAPITAL LETTER Z#
			{66305, "B"},   // MA# ( ?? ¡ú B ) OLD ITALIC LETTER BE ¡ú LATIN CAPITAL LETTER B#
			{66306, "C"},   // MA# ( ?? ¡ú C ) OLD ITALIC LETTER KE ¡ú LATIN CAPITAL LETTER C#
			{66313, "l"},   // MA# ( ?? ¡ú l ) OLD ITALIC LETTER I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{66321, "M"},   // MA# ( ?? ¡ú M ) OLD ITALIC LETTER SHE ¡ú LATIN CAPITAL LETTER M#
			{66325, "T"},   // MA# ( ?? ¡ú T ) OLD ITALIC LETTER TE ¡ú LATIN CAPITAL LETTER T#
			{66327, "X"},   // MA# ( ?? ¡ú X ) OLD ITALIC LETTER EKS ¡ú LATIN CAPITAL LETTER X#
			{66330, "8"},   // MA# ( ?? ¡ú 8 ) OLD ITALIC LETTER EF ¡ú DIGIT EIGHT#
			{66335, "*"},   // MA# ( ?? ¡ú * ) OLD ITALIC LETTER ESS ¡ú ASTERISK#
			{66336, "l"},   // MA#* ( ?? ¡ú l ) OLD ITALIC NUMERAL ONE ¡ú LATIN SMALL LETTER L# ¡ú??¡ú¡úI¡ú
			{66338, "X"},   // MA#* ( ?? ¡ú X ) OLD ITALIC NUMERAL TEN ¡ú LATIN CAPITAL LETTER X# ¡ú??¡ú
			{66564, "O"},   // MA# ( ?? ¡ú O ) DESERET CAPITAL LETTER LONG O ¡ú LATIN CAPITAL LETTER O#
			{66581, "C"},   // MA# ( ?? ¡ú C ) DESERET CAPITAL LETTER CHEE ¡ú LATIN CAPITAL LETTER C#
			{66587, "L"},   // MA# ( ?? ¡ú L ) DESERET CAPITAL LETTER ETH ¡ú LATIN CAPITAL LETTER L#
			{66592, "S"},   // MA# ( ?? ¡ú S ) DESERET CAPITAL LETTER ZHEE ¡ú LATIN CAPITAL LETTER S#
			{66604, "o"},   // MA# ( ?? ¡ú o ) DESERET SMALL LETTER LONG O ¡ú LATIN SMALL LETTER O#
			{66621, "c"},   // MA# ( ?? ¡ú c ) DESERET SMALL LETTER CHEE ¡ú LATIN SMALL LETTER C#
			{66632, "s"},   // MA# ( ?? ¡ú s ) DESERET SMALL LETTER ZHEE ¡ú LATIN SMALL LETTER S#
			{66740, "R"},   // MA# ( ?? ¡ú R ) OSAGE CAPITAL LETTER BRA ¡ú LATIN CAPITAL LETTER R# ¡ú?¡ú
			{66754, "O"},   // MA# ( ?? ¡ú O ) OSAGE CAPITAL LETTER O ¡ú LATIN CAPITAL LETTER O#
			{66766, "U"},   // MA# ( ?? ¡ú U ) OSAGE CAPITAL LETTER U ¡ú LATIN CAPITAL LETTER U#
			{66770, "7"},   // MA# ( ?? ¡ú 7 ) OSAGE CAPITAL LETTER ZA ¡ú DIGIT SEVEN#
			{66794, "o"},   // MA# ( ?? ¡ú o ) OSAGE SMALL LETTER O ¡ú LATIN SMALL LETTER O#
			{66806, "u"},   // MA# ( ?? ¡ú u ) OSAGE SMALL LETTER U ¡ú LATIN SMALL LETTER U# ¡ú?¡ú
			{66835, "N"},   // MA# ( ?? ¡ú N ) ELBASAN LETTER NE ¡ú LATIN CAPITAL LETTER N#
			{66838, "O"},   // MA# ( ?? ¡ú O ) ELBASAN LETTER O ¡ú LATIN CAPITAL LETTER O#
			{66840, "K"},   // MA# ( ?? ¡ú K ) ELBASAN LETTER QE ¡ú LATIN CAPITAL LETTER K#
			{66844, "C"},   // MA# ( ?? ¡ú C ) ELBASAN LETTER SHE ¡ú LATIN CAPITAL LETTER C#
			{66845, "V"},   // MA# ( ?? ¡ú V ) ELBASAN LETTER TE ¡ú LATIN CAPITAL LETTER V#
			{66853, "F"},   // MA# ( ?? ¡ú F ) ELBASAN LETTER GHE ¡ú LATIN CAPITAL LETTER F#
			{66854, "L"},   // MA# ( ?? ¡ú L ) ELBASAN LETTER GHAMMA ¡ú LATIN CAPITAL LETTER L#
			{66855, "X"},   // MA# ( ?? ¡ú X ) ELBASAN LETTER KHE ¡ú LATIN CAPITAL LETTER X#
			{68176, "."},   // MA#* ( ???? ¡ú . ) KHAROSHTHI PUNCTUATION DOT ¡ú FULL STOP#
			{70864, "O"},   // MA# ( ?? ¡ú O ) TIRHUTA DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú?¡ú¡ú0¡ú
			{71424, "rn"},  // MA# ( ?? ¡ú rn ) AHOM LETTER KA ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{71430, "v"},   // MA# ( ?? ¡ú v ) AHOM LETTER PA ¡ú LATIN SMALL LETTER V#
			{71434, "w"},   // MA# ( ?? ¡ú w ) AHOM LETTER JA ¡ú LATIN SMALL LETTER W#
			{71438, "w"},   // MA# ( ?? ¡ú w ) AHOM LETTER LA ¡ú LATIN SMALL LETTER W#
			{71439, "w"},   // MA# ( ?? ¡ú w ) AHOM LETTER SA ¡ú LATIN SMALL LETTER W#
			{71840, "V"},   // MA# ( ?? ¡ú V ) WARANG CITI CAPITAL LETTER NGAA ¡ú LATIN CAPITAL LETTER V#
			{71842, "F"},   // MA# ( ?? ¡ú F ) WARANG CITI CAPITAL LETTER WI ¡ú LATIN CAPITAL LETTER F#
			{71843, "L"},   // MA# ( ?? ¡ú L ) WARANG CITI CAPITAL LETTER YU ¡ú LATIN CAPITAL LETTER L#
			{71844, "Y"},   // MA# ( ?? ¡ú Y ) WARANG CITI CAPITAL LETTER YA ¡ú LATIN CAPITAL LETTER Y#
			{71846, "E"},   // MA# ( ?? ¡ú E ) WARANG CITI CAPITAL LETTER II ¡ú LATIN CAPITAL LETTER E#
			{71849, "Z"},   // MA# ( ?? ¡ú Z ) WARANG CITI CAPITAL LETTER O ¡ú LATIN CAPITAL LETTER Z#
			{71852, "9"},   // MA# ( ?? ¡ú 9 ) WARANG CITI CAPITAL LETTER KO ¡ú DIGIT NINE#
			{71854, "E"},   // MA# ( ?? ¡ú E ) WARANG CITI CAPITAL LETTER YUJ ¡ú LATIN CAPITAL LETTER E#
			{71855, "4"},   // MA# ( ?? ¡ú 4 ) WARANG CITI CAPITAL LETTER UC ¡ú DIGIT FOUR#
			{71858, "L"},   // MA# ( ?? ¡ú L ) WARANG CITI CAPITAL LETTER TTE ¡ú LATIN CAPITAL LETTER L#
			{71861, "O"},   // MA# ( ?? ¡ú O ) WARANG CITI CAPITAL LETTER AT ¡ú LATIN CAPITAL LETTER O#
			{71864, "U"},   // MA# ( ?? ¡ú U ) WARANG CITI CAPITAL LETTER PU ¡ú LATIN CAPITAL LETTER U#
			{71867, "5"},   // MA# ( ?? ¡ú 5 ) WARANG CITI CAPITAL LETTER HORR ¡ú DIGIT FIVE#
			{71868, "T"},   // MA# ( ?? ¡ú T ) WARANG CITI CAPITAL LETTER HAR ¡ú LATIN CAPITAL LETTER T#
			{71872, "v"},   // MA# ( ?? ¡ú v ) WARANG CITI SMALL LETTER NGAA ¡ú LATIN SMALL LETTER V#
			{71873, "s"},   // MA# ( ?? ¡ú s ) WARANG CITI SMALL LETTER A ¡ú LATIN SMALL LETTER S#
			{71874, "F"},   // MA# ( ?? ¡ú F ) WARANG CITI SMALL LETTER WI ¡ú LATIN CAPITAL LETTER F#
			{71875, "i"},   // MA# ( ?? ¡ú i ) WARANG CITI SMALL LETTER YU ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{71876, "z"},   // MA# ( ?? ¡ú z ) WARANG CITI SMALL LETTER YA ¡ú LATIN SMALL LETTER Z#
			{71878, "7"},   // MA# ( ?? ¡ú 7 ) WARANG CITI SMALL LETTER II ¡ú DIGIT SEVEN#
			{71880, "o"},   // MA# ( ?? ¡ú o ) WARANG CITI SMALL LETTER E ¡ú LATIN SMALL LETTER O#
			{71882, "3"},   // MA# ( ?? ¡ú 3 ) WARANG CITI SMALL LETTER ANG ¡ú DIGIT THREE#
			{71884, "9"},   // MA# ( ?? ¡ú 9 ) WARANG CITI SMALL LETTER KO ¡ú DIGIT NINE#
			{71893, "6"},   // MA# ( ?? ¡ú 6 ) WARANG CITI SMALL LETTER AT ¡ú DIGIT SIX#
			{71894, "9"},   // MA# ( ?? ¡ú 9 ) WARANG CITI SMALL LETTER AM ¡ú DIGIT NINE#
			{71895, "o"},   // MA# ( ?? ¡ú o ) WARANG CITI SMALL LETTER BU ¡ú LATIN SMALL LETTER O#
			{71896, "u"},   // MA# ( ?? ¡ú u ) WARANG CITI SMALL LETTER PU ¡ú LATIN SMALL LETTER U# ¡ú¦Ô¡ú¡ú?¡ú
			{71900, "y"},   // MA# ( ?? ¡ú y ) WARANG CITI SMALL LETTER HAR ¡ú LATIN SMALL LETTER Y# ¡ú?¡ú¡ú¦Ã¡ú
			{71904, "O"},   // MA# ( ?? ¡ú O ) WARANG CITI DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{71907, "rn"},  // MA# ( ?? ¡ú rn ) WARANG CITI DIGIT THREE ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{71909, "Z"},   // MA# ( ?? ¡ú Z ) WARANG CITI DIGIT FIVE ¡ú LATIN CAPITAL LETTER Z#
			{71910, "W"},   // MA# ( ?? ¡ú W ) WARANG CITI DIGIT SIX ¡ú LATIN CAPITAL LETTER W#
			{71913, "C"},   // MA# ( ?? ¡ú C ) WARANG CITI DIGIT NINE ¡ú LATIN CAPITAL LETTER C#
			{71916, "X"},   // MA#* ( ?? ¡ú X ) WARANG CITI NUMBER THIRTY ¡ú LATIN CAPITAL LETTER X#
			{71919, "W"},   // MA#* ( ?? ¡ú W ) WARANG CITI NUMBER SIXTY ¡ú LATIN CAPITAL LETTER W#
			{71922, "C"},   // MA#* ( ?? ¡ú C ) WARANG CITI NUMBER NINETY ¡ú LATIN CAPITAL LETTER C#
			{93960, "V"},   // MA# ( ?? ¡ú V ) MIAO LETTER VA ¡ú LATIN CAPITAL LETTER V#
			{93962, "T"},   // MA# ( ?? ¡ú T ) MIAO LETTER TA ¡ú LATIN CAPITAL LETTER T#
			{93974, "L"},   // MA# ( ?? ¡ú L ) MIAO LETTER LA ¡ú LATIN CAPITAL LETTER L#
			{93992, "l"},   // MA# ( ?? ¡ú l ) MIAO LETTER GHA ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{94005, "R"},   // MA# ( ?? ¡ú R ) MIAO LETTER ZHA ¡ú LATIN CAPITAL LETTER R#
			{94010, "S"},   // MA# ( ?? ¡ú S ) MIAO LETTER SA ¡ú LATIN CAPITAL LETTER S#
			{94011, "3"},   // MA# ( ?? ¡ú 3 ) MIAO LETTER ZA ¡ú DIGIT THREE# ¡ú?¡ú
			{94015, ">"},   // MA# ( ?? ¡ú > ) MIAO LETTER ARCHAIC ZZA ¡ú GREATER-THAN SIGN#
			{94016, "A"},   // MA# ( ?? ¡ú A ) MIAO LETTER ZZYA ¡ú LATIN CAPITAL LETTER A#
			{94018, "U"},   // MA# ( ?? ¡ú U ) MIAO LETTER WA ¡ú LATIN CAPITAL LETTER U#
			{94019, "Y"},   // MA# ( ?? ¡ú Y ) MIAO LETTER AH ¡ú LATIN CAPITAL LETTER Y#
			{94033, "'"},   // MA# ( ?? ¡ú ' ) MIAO SIGN ASPIRATION ¡ú APOSTROPHE# ¡ú?¡ú¡ú¡ä¡ú
			{94034, "'"},   // MA# ( ?? ¡ú ' ) MIAO SIGN REFORMED VOICING ¡ú APOSTROPHE# ¡ú?¡ú¡ú¡®¡ú
			{119060, "{"},  // MA#* ( ?? ¡ú { ) MUSICAL SYMBOL BRACE ¡ú LEFT CURLY BRACKET#
			{119149, "."},  // MA# ( ?? ¡ú . ) MUSICAL SYMBOL COMBINING AUGMENTATION DOT ¡ú FULL STOP#
			{119302, "3"},  // MA#* ( ?? ¡ú 3 ) GREEK VOCAL NOTATION SYMBOL-7 ¡ú DIGIT THREE#
			{119309, "V"},  // MA#* ( ?? ¡ú V ) GREEK VOCAL NOTATION SYMBOL-14 ¡ú LATIN CAPITAL LETTER V#
			{119311, "\\"}, // MA#* ( ?? ¡ú \ ) GREEK VOCAL NOTATION SYMBOL-16 ¡ú REVERSE SOLIDUS#
			{119314, "7"},  // MA#* ( ?? ¡ú 7 ) GREEK VOCAL NOTATION SYMBOL-19 ¡ú DIGIT SEVEN#
			{119315, "F"},  // MA#* ( ?? ¡ú F ) GREEK VOCAL NOTATION SYMBOL-20 ¡ú LATIN CAPITAL LETTER F# ¡ú?¡ú
			{119318, "R"},  // MA#* ( ?? ¡ú R ) GREEK VOCAL NOTATION SYMBOL-23 ¡ú LATIN CAPITAL LETTER R#
			{119338, "L"},  // MA#* ( ?? ¡ú L ) GREEK INSTRUMENTAL NOTATION SYMBOL-23 ¡ú LATIN CAPITAL LETTER L#
			{119350, "<"},  // MA#* ( ?? ¡ú < ) GREEK INSTRUMENTAL NOTATION SYMBOL-40 ¡ú LESS-THAN SIGN#
			{119351, ">"},  // MA#* ( ?? ¡ú > ) GREEK INSTRUMENTAL NOTATION SYMBOL-42 ¡ú GREATER-THAN SIGN#
			{119354, "/"},  // MA#* ( ?? ¡ú / ) GREEK INSTRUMENTAL NOTATION SYMBOL-47 ¡ú SOLIDUS#
			{119355, "\\"}, // MA#* ( ?? ¡ú \ ) GREEK INSTRUMENTAL NOTATION SYMBOL-48 ¡ú REVERSE SOLIDUS# ¡ú??¡ú
			{119808, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL BOLD CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{119809, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL BOLD CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{119810, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL BOLD CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{119811, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL BOLD CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{119812, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL BOLD CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{119813, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL BOLD CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{119814, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL BOLD CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{119815, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL BOLD CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{119816, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{119817, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL BOLD CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{119818, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL BOLD CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{119819, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL BOLD CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{119820, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL BOLD CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{119821, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL BOLD CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{119822, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL BOLD CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{119823, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL BOLD CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{119824, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL BOLD CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{119825, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL BOLD CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{119826, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL BOLD CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{119827, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL BOLD CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{119828, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL BOLD CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{119829, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL BOLD CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{119830, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL BOLD CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{119831, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL BOLD CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{119832, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL BOLD CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{119833, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL BOLD CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{119834, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL BOLD SMALL A ¡ú LATIN SMALL LETTER A#
			{119835, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL BOLD SMALL B ¡ú LATIN SMALL LETTER B#
			{119836, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL BOLD SMALL C ¡ú LATIN SMALL LETTER C#
			{119837, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL BOLD SMALL D ¡ú LATIN SMALL LETTER D#
			{119838, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL BOLD SMALL E ¡ú LATIN SMALL LETTER E#
			{119839, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL BOLD SMALL F ¡ú LATIN SMALL LETTER F#
			{119840, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL BOLD SMALL G ¡ú LATIN SMALL LETTER G#
			{119841, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL BOLD SMALL H ¡ú LATIN SMALL LETTER H#
			{119842, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL BOLD SMALL I ¡ú LATIN SMALL LETTER I#
			{119843, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL BOLD SMALL J ¡ú LATIN SMALL LETTER J#
			{119844, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL BOLD SMALL K ¡ú LATIN SMALL LETTER K#
			{119845, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD SMALL L ¡ú LATIN SMALL LETTER L#
			{119846, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL BOLD SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{119847, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL BOLD SMALL N ¡ú LATIN SMALL LETTER N#
			{119848, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL BOLD SMALL O ¡ú LATIN SMALL LETTER O#
			{119849, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL BOLD SMALL P ¡ú LATIN SMALL LETTER P#
			{119850, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL BOLD SMALL Q ¡ú LATIN SMALL LETTER Q#
			{119851, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL BOLD SMALL R ¡ú LATIN SMALL LETTER R#
			{119852, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL BOLD SMALL S ¡ú LATIN SMALL LETTER S#
			{119853, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL BOLD SMALL T ¡ú LATIN SMALL LETTER T#
			{119854, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL BOLD SMALL U ¡ú LATIN SMALL LETTER U#
			{119855, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL BOLD SMALL V ¡ú LATIN SMALL LETTER V#
			{119856, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL BOLD SMALL W ¡ú LATIN SMALL LETTER W#
			{119857, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL BOLD SMALL X ¡ú LATIN SMALL LETTER X#
			{119858, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL BOLD SMALL Y ¡ú LATIN SMALL LETTER Y#
			{119859, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL BOLD SMALL Z ¡ú LATIN SMALL LETTER Z#
			{119860, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL ITALIC CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{119861, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL ITALIC CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{119862, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL ITALIC CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{119863, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL ITALIC CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{119864, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL ITALIC CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{119865, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL ITALIC CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{119866, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL ITALIC CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{119867, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL ITALIC CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{119868, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL ITALIC CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{119869, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL ITALIC CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{119870, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL ITALIC CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{119871, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL ITALIC CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{119872, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL ITALIC CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{119873, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL ITALIC CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{119874, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL ITALIC CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{119875, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL ITALIC CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{119876, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL ITALIC CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{119877, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL ITALIC CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{119878, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL ITALIC CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{119879, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL ITALIC CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{119880, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL ITALIC CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{119881, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL ITALIC CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{119882, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL ITALIC CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{119883, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL ITALIC CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{119884, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL ITALIC CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{119885, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL ITALIC CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{119886, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL ITALIC SMALL A ¡ú LATIN SMALL LETTER A#
			{119887, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL ITALIC SMALL B ¡ú LATIN SMALL LETTER B#
			{119888, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL ITALIC SMALL C ¡ú LATIN SMALL LETTER C#
			{119889, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL ITALIC SMALL D ¡ú LATIN SMALL LETTER D#
			{119890, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL ITALIC SMALL E ¡ú LATIN SMALL LETTER E#
			{119891, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL ITALIC SMALL F ¡ú LATIN SMALL LETTER F#
			{119892, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL ITALIC SMALL G ¡ú LATIN SMALL LETTER G#
			{119894, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL ITALIC SMALL I ¡ú LATIN SMALL LETTER I#
			{119895, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL ITALIC SMALL J ¡ú LATIN SMALL LETTER J#
			{119896, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL ITALIC SMALL K ¡ú LATIN SMALL LETTER K#
			{119897, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL ITALIC SMALL L ¡ú LATIN SMALL LETTER L#
			{119898, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL ITALIC SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{119899, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL ITALIC SMALL N ¡ú LATIN SMALL LETTER N#
			{119900, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL ITALIC SMALL O ¡ú LATIN SMALL LETTER O#
			{119901, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL ITALIC SMALL P ¡ú LATIN SMALL LETTER P#
			{119902, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL ITALIC SMALL Q ¡ú LATIN SMALL LETTER Q#
			{119903, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL ITALIC SMALL R ¡ú LATIN SMALL LETTER R#
			{119904, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL ITALIC SMALL S ¡ú LATIN SMALL LETTER S#
			{119905, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL ITALIC SMALL T ¡ú LATIN SMALL LETTER T#
			{119906, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL ITALIC SMALL U ¡ú LATIN SMALL LETTER U#
			{119907, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL ITALIC SMALL V ¡ú LATIN SMALL LETTER V#
			{119908, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL ITALIC SMALL W ¡ú LATIN SMALL LETTER W#
			{119909, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL ITALIC SMALL X ¡ú LATIN SMALL LETTER X#
			{119910, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL ITALIC SMALL Y ¡ú LATIN SMALL LETTER Y#
			{119911, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL ITALIC SMALL Z ¡ú LATIN SMALL LETTER Z#
			{119912, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL BOLD ITALIC CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{119913, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL BOLD ITALIC CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{119914, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL BOLD ITALIC CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{119915, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL BOLD ITALIC CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{119916, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL BOLD ITALIC CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{119917, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL BOLD ITALIC CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{119918, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL BOLD ITALIC CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{119919, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL BOLD ITALIC CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{119920, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD ITALIC CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{119921, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL BOLD ITALIC CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{119922, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL BOLD ITALIC CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{119923, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL BOLD ITALIC CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{119924, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL BOLD ITALIC CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{119925, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL BOLD ITALIC CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{119926, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL BOLD ITALIC CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{119927, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL BOLD ITALIC CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{119928, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL BOLD ITALIC CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{119929, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL BOLD ITALIC CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{119930, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL BOLD ITALIC CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{119931, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL BOLD ITALIC CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{119932, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL BOLD ITALIC CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{119933, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL BOLD ITALIC CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{119934, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL BOLD ITALIC CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{119935, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL BOLD ITALIC CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{119936, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL BOLD ITALIC CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{119937, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL BOLD ITALIC CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{119938, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL BOLD ITALIC SMALL A ¡ú LATIN SMALL LETTER A#
			{119939, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL BOLD ITALIC SMALL B ¡ú LATIN SMALL LETTER B#
			{119940, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL BOLD ITALIC SMALL C ¡ú LATIN SMALL LETTER C#
			{119941, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL BOLD ITALIC SMALL D ¡ú LATIN SMALL LETTER D#
			{119942, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL BOLD ITALIC SMALL E ¡ú LATIN SMALL LETTER E#
			{119943, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL BOLD ITALIC SMALL F ¡ú LATIN SMALL LETTER F#
			{119944, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL BOLD ITALIC SMALL G ¡ú LATIN SMALL LETTER G#
			{119945, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL BOLD ITALIC SMALL H ¡ú LATIN SMALL LETTER H#
			{119946, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL BOLD ITALIC SMALL I ¡ú LATIN SMALL LETTER I#
			{119947, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL BOLD ITALIC SMALL J ¡ú LATIN SMALL LETTER J#
			{119948, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL BOLD ITALIC SMALL K ¡ú LATIN SMALL LETTER K#
			{119949, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD ITALIC SMALL L ¡ú LATIN SMALL LETTER L#
			{119950, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL BOLD ITALIC SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{119951, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL BOLD ITALIC SMALL N ¡ú LATIN SMALL LETTER N#
			{119952, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL BOLD ITALIC SMALL O ¡ú LATIN SMALL LETTER O#
			{119953, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL BOLD ITALIC SMALL P ¡ú LATIN SMALL LETTER P#
			{119954, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL BOLD ITALIC SMALL Q ¡ú LATIN SMALL LETTER Q#
			{119955, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL BOLD ITALIC SMALL R ¡ú LATIN SMALL LETTER R#
			{119956, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL BOLD ITALIC SMALL S ¡ú LATIN SMALL LETTER S#
			{119957, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL BOLD ITALIC SMALL T ¡ú LATIN SMALL LETTER T#
			{119958, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL BOLD ITALIC SMALL U ¡ú LATIN SMALL LETTER U#
			{119959, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL BOLD ITALIC SMALL V ¡ú LATIN SMALL LETTER V#
			{119960, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL BOLD ITALIC SMALL W ¡ú LATIN SMALL LETTER W#
			{119961, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL BOLD ITALIC SMALL X ¡ú LATIN SMALL LETTER X#
			{119962, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL BOLD ITALIC SMALL Y ¡ú LATIN SMALL LETTER Y#
			{119963, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL BOLD ITALIC SMALL Z ¡ú LATIN SMALL LETTER Z#
			{119964, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL SCRIPT CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{119966, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL SCRIPT CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{119967, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL SCRIPT CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{119970, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL SCRIPT CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{119973, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL SCRIPT CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{119974, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL SCRIPT CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{119977, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL SCRIPT CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{119978, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SCRIPT CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{119979, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL SCRIPT CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{119980, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL SCRIPT CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{119982, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL SCRIPT CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{119983, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL SCRIPT CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{119984, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL SCRIPT CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{119985, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL SCRIPT CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{119986, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL SCRIPT CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{119987, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL SCRIPT CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{119988, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL SCRIPT CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{119989, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL SCRIPT CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{119990, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL SCRIPT SMALL A ¡ú LATIN SMALL LETTER A#
			{119991, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL SCRIPT SMALL B ¡ú LATIN SMALL LETTER B#
			{119992, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL SCRIPT SMALL C ¡ú LATIN SMALL LETTER C#
			{119993, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL SCRIPT SMALL D ¡ú LATIN SMALL LETTER D#
			{119995, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL SCRIPT SMALL F ¡ú LATIN SMALL LETTER F#
			{119997, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL SCRIPT SMALL H ¡ú LATIN SMALL LETTER H#
			{119998, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL SCRIPT SMALL I ¡ú LATIN SMALL LETTER I#
			{119999, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL SCRIPT SMALL J ¡ú LATIN SMALL LETTER J#
			{120000, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL SCRIPT SMALL K ¡ú LATIN SMALL LETTER K#
			{120001, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SCRIPT SMALL L ¡ú LATIN SMALL LETTER L#
			{120002, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL SCRIPT SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120003, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL SCRIPT SMALL N ¡ú LATIN SMALL LETTER N#
			{120005, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SCRIPT SMALL P ¡ú LATIN SMALL LETTER P#
			{120006, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL SCRIPT SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120007, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL SCRIPT SMALL R ¡ú LATIN SMALL LETTER R#
			{120008, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL SCRIPT SMALL S ¡ú LATIN SMALL LETTER S#
			{120009, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL SCRIPT SMALL T ¡ú LATIN SMALL LETTER T#
			{120010, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL SCRIPT SMALL U ¡ú LATIN SMALL LETTER U#
			{120011, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL SCRIPT SMALL V ¡ú LATIN SMALL LETTER V#
			{120012, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL SCRIPT SMALL W ¡ú LATIN SMALL LETTER W#
			{120013, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL SCRIPT SMALL X ¡ú LATIN SMALL LETTER X#
			{120014, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL SCRIPT SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120015, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL SCRIPT SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120016, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL BOLD SCRIPT CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120017, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL BOLD SCRIPT CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120018, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL BOLD SCRIPT CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{120019, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL BOLD SCRIPT CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120020, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL BOLD SCRIPT CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120021, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL BOLD SCRIPT CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120022, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL BOLD SCRIPT CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120023, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL BOLD SCRIPT CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{120024, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD SCRIPT CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{120025, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL BOLD SCRIPT CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120026, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL BOLD SCRIPT CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120027, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL BOLD SCRIPT CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120028, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL BOLD SCRIPT CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120029, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL BOLD SCRIPT CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{120030, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL BOLD SCRIPT CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120031, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL BOLD SCRIPT CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{120032, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL BOLD SCRIPT CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{120033, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL BOLD SCRIPT CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{120034, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL BOLD SCRIPT CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120035, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL BOLD SCRIPT CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120036, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL BOLD SCRIPT CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120037, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL BOLD SCRIPT CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120038, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL BOLD SCRIPT CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120039, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL BOLD SCRIPT CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120040, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL BOLD SCRIPT CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120041, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL BOLD SCRIPT CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{120042, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL BOLD SCRIPT SMALL A ¡ú LATIN SMALL LETTER A#
			{120043, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL BOLD SCRIPT SMALL B ¡ú LATIN SMALL LETTER B#
			{120044, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL BOLD SCRIPT SMALL C ¡ú LATIN SMALL LETTER C#
			{120045, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL BOLD SCRIPT SMALL D ¡ú LATIN SMALL LETTER D#
			{120046, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL BOLD SCRIPT SMALL E ¡ú LATIN SMALL LETTER E#
			{120047, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL BOLD SCRIPT SMALL F ¡ú LATIN SMALL LETTER F#
			{120048, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL BOLD SCRIPT SMALL G ¡ú LATIN SMALL LETTER G#
			{120049, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL BOLD SCRIPT SMALL H ¡ú LATIN SMALL LETTER H#
			{120050, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL BOLD SCRIPT SMALL I ¡ú LATIN SMALL LETTER I#
			{120051, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL BOLD SCRIPT SMALL J ¡ú LATIN SMALL LETTER J#
			{120052, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL BOLD SCRIPT SMALL K ¡ú LATIN SMALL LETTER K#
			{120053, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD SCRIPT SMALL L ¡ú LATIN SMALL LETTER L#
			{120054, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL BOLD SCRIPT SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120055, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL BOLD SCRIPT SMALL N ¡ú LATIN SMALL LETTER N#
			{120056, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL BOLD SCRIPT SMALL O ¡ú LATIN SMALL LETTER O#
			{120057, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL BOLD SCRIPT SMALL P ¡ú LATIN SMALL LETTER P#
			{120058, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL BOLD SCRIPT SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120059, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL BOLD SCRIPT SMALL R ¡ú LATIN SMALL LETTER R#
			{120060, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL BOLD SCRIPT SMALL S ¡ú LATIN SMALL LETTER S#
			{120061, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL BOLD SCRIPT SMALL T ¡ú LATIN SMALL LETTER T#
			{120062, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL BOLD SCRIPT SMALL U ¡ú LATIN SMALL LETTER U#
			{120063, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL BOLD SCRIPT SMALL V ¡ú LATIN SMALL LETTER V#
			{120064, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL BOLD SCRIPT SMALL W ¡ú LATIN SMALL LETTER W#
			{120065, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL BOLD SCRIPT SMALL X ¡ú LATIN SMALL LETTER X#
			{120066, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL BOLD SCRIPT SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120067, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL BOLD SCRIPT SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120068, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL FRAKTUR CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120069, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL FRAKTUR CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120071, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL FRAKTUR CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120072, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL FRAKTUR CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120073, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL FRAKTUR CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120074, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL FRAKTUR CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120077, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL FRAKTUR CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120078, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL FRAKTUR CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120079, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL FRAKTUR CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120080, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL FRAKTUR CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120081, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL FRAKTUR CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{120082, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL FRAKTUR CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120083, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL FRAKTUR CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{120084, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL FRAKTUR CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{120086, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL FRAKTUR CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120087, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL FRAKTUR CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120088, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL FRAKTUR CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120089, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL FRAKTUR CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120090, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL FRAKTUR CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120091, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL FRAKTUR CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120092, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL FRAKTUR CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120094, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL FRAKTUR SMALL A ¡ú LATIN SMALL LETTER A#
			{120095, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL FRAKTUR SMALL B ¡ú LATIN SMALL LETTER B#
			{120096, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL FRAKTUR SMALL C ¡ú LATIN SMALL LETTER C#
			{120097, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL FRAKTUR SMALL D ¡ú LATIN SMALL LETTER D#
			{120098, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL FRAKTUR SMALL E ¡ú LATIN SMALL LETTER E#
			{120099, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL FRAKTUR SMALL F ¡ú LATIN SMALL LETTER F#
			{120100, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL FRAKTUR SMALL G ¡ú LATIN SMALL LETTER G#
			{120101, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL FRAKTUR SMALL H ¡ú LATIN SMALL LETTER H#
			{120102, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL FRAKTUR SMALL I ¡ú LATIN SMALL LETTER I#
			{120103, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL FRAKTUR SMALL J ¡ú LATIN SMALL LETTER J#
			{120104, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL FRAKTUR SMALL K ¡ú LATIN SMALL LETTER K#
			{120105, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL FRAKTUR SMALL L ¡ú LATIN SMALL LETTER L#
			{120106, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL FRAKTUR SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120107, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL FRAKTUR SMALL N ¡ú LATIN SMALL LETTER N#
			{120108, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL FRAKTUR SMALL O ¡ú LATIN SMALL LETTER O#
			{120109, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL FRAKTUR SMALL P ¡ú LATIN SMALL LETTER P#
			{120110, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL FRAKTUR SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120111, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL FRAKTUR SMALL R ¡ú LATIN SMALL LETTER R#
			{120112, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL FRAKTUR SMALL S ¡ú LATIN SMALL LETTER S#
			{120113, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL FRAKTUR SMALL T ¡ú LATIN SMALL LETTER T#
			{120114, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL FRAKTUR SMALL U ¡ú LATIN SMALL LETTER U#
			{120115, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL FRAKTUR SMALL V ¡ú LATIN SMALL LETTER V#
			{120116, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL FRAKTUR SMALL W ¡ú LATIN SMALL LETTER W#
			{120117, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL FRAKTUR SMALL X ¡ú LATIN SMALL LETTER X#
			{120118, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL FRAKTUR SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120119, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL FRAKTUR SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120120, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL DOUBLE-STRUCK CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120121, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL DOUBLE-STRUCK CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120123, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL DOUBLE-STRUCK CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120124, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL DOUBLE-STRUCK CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120125, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL DOUBLE-STRUCK CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120126, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL DOUBLE-STRUCK CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120128, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL DOUBLE-STRUCK CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{120129, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL DOUBLE-STRUCK CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120130, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL DOUBLE-STRUCK CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120131, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL DOUBLE-STRUCK CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120132, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL DOUBLE-STRUCK CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120134, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL DOUBLE-STRUCK CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120138, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL DOUBLE-STRUCK CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120139, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL DOUBLE-STRUCK CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120140, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL DOUBLE-STRUCK CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120141, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL DOUBLE-STRUCK CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120142, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL DOUBLE-STRUCK CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120143, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL DOUBLE-STRUCK CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120144, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL DOUBLE-STRUCK CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120146, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL DOUBLE-STRUCK SMALL A ¡ú LATIN SMALL LETTER A#
			{120147, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL DOUBLE-STRUCK SMALL B ¡ú LATIN SMALL LETTER B#
			{120148, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL DOUBLE-STRUCK SMALL C ¡ú LATIN SMALL LETTER C#
			{120149, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL DOUBLE-STRUCK SMALL D ¡ú LATIN SMALL LETTER D#
			{120150, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL DOUBLE-STRUCK SMALL E ¡ú LATIN SMALL LETTER E#
			{120151, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL DOUBLE-STRUCK SMALL F ¡ú LATIN SMALL LETTER F#
			{120152, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL DOUBLE-STRUCK SMALL G ¡ú LATIN SMALL LETTER G#
			{120153, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL DOUBLE-STRUCK SMALL H ¡ú LATIN SMALL LETTER H#
			{120154, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL DOUBLE-STRUCK SMALL I ¡ú LATIN SMALL LETTER I#
			{120155, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL DOUBLE-STRUCK SMALL J ¡ú LATIN SMALL LETTER J#
			{120156, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL DOUBLE-STRUCK SMALL K ¡ú LATIN SMALL LETTER K#
			{120157, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL DOUBLE-STRUCK SMALL L ¡ú LATIN SMALL LETTER L#
			{120158, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL DOUBLE-STRUCK SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120159, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL DOUBLE-STRUCK SMALL N ¡ú LATIN SMALL LETTER N#
			{120160, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL DOUBLE-STRUCK SMALL O ¡ú LATIN SMALL LETTER O#
			{120161, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL DOUBLE-STRUCK SMALL P ¡ú LATIN SMALL LETTER P#
			{120162, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL DOUBLE-STRUCK SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120163, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL DOUBLE-STRUCK SMALL R ¡ú LATIN SMALL LETTER R#
			{120164, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL DOUBLE-STRUCK SMALL S ¡ú LATIN SMALL LETTER S#
			{120165, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL DOUBLE-STRUCK SMALL T ¡ú LATIN SMALL LETTER T#
			{120166, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL DOUBLE-STRUCK SMALL U ¡ú LATIN SMALL LETTER U#
			{120167, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL DOUBLE-STRUCK SMALL V ¡ú LATIN SMALL LETTER V#
			{120168, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL DOUBLE-STRUCK SMALL W ¡ú LATIN SMALL LETTER W#
			{120169, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL DOUBLE-STRUCK SMALL X ¡ú LATIN SMALL LETTER X#
			{120170, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL DOUBLE-STRUCK SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120171, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL DOUBLE-STRUCK SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120172, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL BOLD FRAKTUR CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120173, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL BOLD FRAKTUR CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120174, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL BOLD FRAKTUR CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{120175, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL BOLD FRAKTUR CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120176, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL BOLD FRAKTUR CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120177, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL BOLD FRAKTUR CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120178, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL BOLD FRAKTUR CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120179, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL BOLD FRAKTUR CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{120180, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD FRAKTUR CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{120181, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL BOLD FRAKTUR CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120182, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL BOLD FRAKTUR CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120183, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL BOLD FRAKTUR CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120184, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL BOLD FRAKTUR CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120185, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL BOLD FRAKTUR CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{120186, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL BOLD FRAKTUR CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120187, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL BOLD FRAKTUR CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{120188, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL BOLD FRAKTUR CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{120189, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL BOLD FRAKTUR CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{120190, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL BOLD FRAKTUR CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120191, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL BOLD FRAKTUR CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120192, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL BOLD FRAKTUR CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120193, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL BOLD FRAKTUR CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120194, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL BOLD FRAKTUR CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120195, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL BOLD FRAKTUR CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120196, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL BOLD FRAKTUR CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120197, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL BOLD FRAKTUR CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{120198, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL BOLD FRAKTUR SMALL A ¡ú LATIN SMALL LETTER A#
			{120199, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL BOLD FRAKTUR SMALL B ¡ú LATIN SMALL LETTER B#
			{120200, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL BOLD FRAKTUR SMALL C ¡ú LATIN SMALL LETTER C#
			{120201, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL BOLD FRAKTUR SMALL D ¡ú LATIN SMALL LETTER D#
			{120202, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL BOLD FRAKTUR SMALL E ¡ú LATIN SMALL LETTER E#
			{120203, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL BOLD FRAKTUR SMALL F ¡ú LATIN SMALL LETTER F#
			{120204, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL BOLD FRAKTUR SMALL G ¡ú LATIN SMALL LETTER G#
			{120205, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL BOLD FRAKTUR SMALL H ¡ú LATIN SMALL LETTER H#
			{120206, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL BOLD FRAKTUR SMALL I ¡ú LATIN SMALL LETTER I#
			{120207, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL BOLD FRAKTUR SMALL J ¡ú LATIN SMALL LETTER J#
			{120208, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL BOLD FRAKTUR SMALL K ¡ú LATIN SMALL LETTER K#
			{120209, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD FRAKTUR SMALL L ¡ú LATIN SMALL LETTER L#
			{120210, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL BOLD FRAKTUR SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120211, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL BOLD FRAKTUR SMALL N ¡ú LATIN SMALL LETTER N#
			{120212, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL BOLD FRAKTUR SMALL O ¡ú LATIN SMALL LETTER O#
			{120213, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL BOLD FRAKTUR SMALL P ¡ú LATIN SMALL LETTER P#
			{120214, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL BOLD FRAKTUR SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120215, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL BOLD FRAKTUR SMALL R ¡ú LATIN SMALL LETTER R#
			{120216, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL BOLD FRAKTUR SMALL S ¡ú LATIN SMALL LETTER S#
			{120217, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL BOLD FRAKTUR SMALL T ¡ú LATIN SMALL LETTER T#
			{120218, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL BOLD FRAKTUR SMALL U ¡ú LATIN SMALL LETTER U#
			{120219, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL BOLD FRAKTUR SMALL V ¡ú LATIN SMALL LETTER V#
			{120220, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL BOLD FRAKTUR SMALL W ¡ú LATIN SMALL LETTER W#
			{120221, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL BOLD FRAKTUR SMALL X ¡ú LATIN SMALL LETTER X#
			{120222, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL BOLD FRAKTUR SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120223, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL BOLD FRAKTUR SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120224, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL SANS-SERIF CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120225, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL SANS-SERIF CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120226, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL SANS-SERIF CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{120227, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL SANS-SERIF CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120228, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL SANS-SERIF CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120229, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL SANS-SERIF CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120230, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL SANS-SERIF CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120231, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL SANS-SERIF CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{120232, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{120233, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL SANS-SERIF CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120234, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL SANS-SERIF CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120235, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL SANS-SERIF CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120236, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL SANS-SERIF CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120237, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL SANS-SERIF CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{120238, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SANS-SERIF CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120239, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL SANS-SERIF CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{120240, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL SANS-SERIF CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{120241, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL SANS-SERIF CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{120242, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL SANS-SERIF CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120243, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL SANS-SERIF CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120244, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL SANS-SERIF CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120245, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL SANS-SERIF CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120246, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL SANS-SERIF CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120247, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL SANS-SERIF CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120248, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL SANS-SERIF CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120249, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL SANS-SERIF CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{120250, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL SANS-SERIF SMALL A ¡ú LATIN SMALL LETTER A#
			{120251, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL SANS-SERIF SMALL B ¡ú LATIN SMALL LETTER B#
			{120252, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL SANS-SERIF SMALL C ¡ú LATIN SMALL LETTER C#
			{120253, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL SANS-SERIF SMALL D ¡ú LATIN SMALL LETTER D#
			{120254, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL SANS-SERIF SMALL E ¡ú LATIN SMALL LETTER E#
			{120255, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL SANS-SERIF SMALL F ¡ú LATIN SMALL LETTER F#
			{120256, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL SANS-SERIF SMALL G ¡ú LATIN SMALL LETTER G#
			{120257, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL SANS-SERIF SMALL H ¡ú LATIN SMALL LETTER H#
			{120258, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL SANS-SERIF SMALL I ¡ú LATIN SMALL LETTER I#
			{120259, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL SANS-SERIF SMALL J ¡ú LATIN SMALL LETTER J#
			{120260, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL SANS-SERIF SMALL K ¡ú LATIN SMALL LETTER K#
			{120261, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF SMALL L ¡ú LATIN SMALL LETTER L#
			{120262, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL SANS-SERIF SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120263, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL SANS-SERIF SMALL N ¡ú LATIN SMALL LETTER N#
			{120264, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL SANS-SERIF SMALL O ¡ú LATIN SMALL LETTER O#
			{120265, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SANS-SERIF SMALL P ¡ú LATIN SMALL LETTER P#
			{120266, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL SANS-SERIF SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120267, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL SANS-SERIF SMALL R ¡ú LATIN SMALL LETTER R#
			{120268, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL SANS-SERIF SMALL S ¡ú LATIN SMALL LETTER S#
			{120269, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL SANS-SERIF SMALL T ¡ú LATIN SMALL LETTER T#
			{120270, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL SANS-SERIF SMALL U ¡ú LATIN SMALL LETTER U#
			{120271, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL SANS-SERIF SMALL V ¡ú LATIN SMALL LETTER V#
			{120272, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL SANS-SERIF SMALL W ¡ú LATIN SMALL LETTER W#
			{120273, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL SANS-SERIF SMALL X ¡ú LATIN SMALL LETTER X#
			{120274, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL SANS-SERIF SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120275, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL SANS-SERIF SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120276, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL SANS-SERIF BOLD CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120277, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL SANS-SERIF BOLD CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120278, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL SANS-SERIF BOLD CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{120279, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL SANS-SERIF BOLD CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120280, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL SANS-SERIF BOLD CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120281, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL SANS-SERIF BOLD CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120282, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL SANS-SERIF BOLD CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120283, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL SANS-SERIF BOLD CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{120284, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF BOLD CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{120285, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL SANS-SERIF BOLD CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120286, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL SANS-SERIF BOLD CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120287, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL SANS-SERIF BOLD CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120288, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL SANS-SERIF BOLD CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120289, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL SANS-SERIF BOLD CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{120290, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SANS-SERIF BOLD CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120291, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL SANS-SERIF BOLD CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{120292, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL SANS-SERIF BOLD CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{120293, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL SANS-SERIF BOLD CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{120294, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL SANS-SERIF BOLD CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120295, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL SANS-SERIF BOLD CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120296, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL SANS-SERIF BOLD CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120297, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL SANS-SERIF BOLD CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120298, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL SANS-SERIF BOLD CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120299, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL SANS-SERIF BOLD CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120300, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL SANS-SERIF BOLD CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120301, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL SANS-SERIF BOLD CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{120302, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL SANS-SERIF BOLD SMALL A ¡ú LATIN SMALL LETTER A#
			{120303, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL SANS-SERIF BOLD SMALL B ¡ú LATIN SMALL LETTER B#
			{120304, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL SANS-SERIF BOLD SMALL C ¡ú LATIN SMALL LETTER C#
			{120305, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL SANS-SERIF BOLD SMALL D ¡ú LATIN SMALL LETTER D#
			{120306, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL SANS-SERIF BOLD SMALL E ¡ú LATIN SMALL LETTER E#
			{120307, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL SANS-SERIF BOLD SMALL F ¡ú LATIN SMALL LETTER F#
			{120308, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL SANS-SERIF BOLD SMALL G ¡ú LATIN SMALL LETTER G#
			{120309, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL SANS-SERIF BOLD SMALL H ¡ú LATIN SMALL LETTER H#
			{120310, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL SANS-SERIF BOLD SMALL I ¡ú LATIN SMALL LETTER I#
			{120311, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL SANS-SERIF BOLD SMALL J ¡ú LATIN SMALL LETTER J#
			{120312, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL SANS-SERIF BOLD SMALL K ¡ú LATIN SMALL LETTER K#
			{120313, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF BOLD SMALL L ¡ú LATIN SMALL LETTER L#
			{120314, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL SANS-SERIF BOLD SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120315, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL SANS-SERIF BOLD SMALL N ¡ú LATIN SMALL LETTER N#
			{120316, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL SANS-SERIF BOLD SMALL O ¡ú LATIN SMALL LETTER O#
			{120317, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SANS-SERIF BOLD SMALL P ¡ú LATIN SMALL LETTER P#
			{120318, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL SANS-SERIF BOLD SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120319, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL SANS-SERIF BOLD SMALL R ¡ú LATIN SMALL LETTER R#
			{120320, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL SANS-SERIF BOLD SMALL S ¡ú LATIN SMALL LETTER S#
			{120321, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL SANS-SERIF BOLD SMALL T ¡ú LATIN SMALL LETTER T#
			{120322, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL SANS-SERIF BOLD SMALL U ¡ú LATIN SMALL LETTER U#
			{120323, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL SANS-SERIF BOLD SMALL V ¡ú LATIN SMALL LETTER V#
			{120324, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL SANS-SERIF BOLD SMALL W ¡ú LATIN SMALL LETTER W#
			{120325, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL SANS-SERIF BOLD SMALL X ¡ú LATIN SMALL LETTER X#
			{120326, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL SANS-SERIF BOLD SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120327, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL SANS-SERIF BOLD SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120328, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120329, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120330, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{120331, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120332, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120333, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120334, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120335, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{120336, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{120337, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120338, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120339, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120340, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120341, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{120342, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120343, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{120344, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{120345, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{120346, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120347, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120348, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120349, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120350, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120351, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120352, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120353, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL SANS-SERIF ITALIC CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{120354, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL SANS-SERIF ITALIC SMALL A ¡ú LATIN SMALL LETTER A#
			{120355, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL SANS-SERIF ITALIC SMALL B ¡ú LATIN SMALL LETTER B#
			{120356, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL SANS-SERIF ITALIC SMALL C ¡ú LATIN SMALL LETTER C#
			{120357, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL SANS-SERIF ITALIC SMALL D ¡ú LATIN SMALL LETTER D#
			{120358, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL SANS-SERIF ITALIC SMALL E ¡ú LATIN SMALL LETTER E#
			{120359, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL SANS-SERIF ITALIC SMALL F ¡ú LATIN SMALL LETTER F#
			{120360, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL SANS-SERIF ITALIC SMALL G ¡ú LATIN SMALL LETTER G#
			{120361, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL SANS-SERIF ITALIC SMALL H ¡ú LATIN SMALL LETTER H#
			{120362, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL SANS-SERIF ITALIC SMALL I ¡ú LATIN SMALL LETTER I#
			{120363, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL SANS-SERIF ITALIC SMALL J ¡ú LATIN SMALL LETTER J#
			{120364, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL SANS-SERIF ITALIC SMALL K ¡ú LATIN SMALL LETTER K#
			{120365, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF ITALIC SMALL L ¡ú LATIN SMALL LETTER L#
			{120366, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL SANS-SERIF ITALIC SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120367, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL SANS-SERIF ITALIC SMALL N ¡ú LATIN SMALL LETTER N#
			{120368, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL SANS-SERIF ITALIC SMALL O ¡ú LATIN SMALL LETTER O#
			{120369, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SANS-SERIF ITALIC SMALL P ¡ú LATIN SMALL LETTER P#
			{120370, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL SANS-SERIF ITALIC SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120371, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL SANS-SERIF ITALIC SMALL R ¡ú LATIN SMALL LETTER R#
			{120372, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL SANS-SERIF ITALIC SMALL S ¡ú LATIN SMALL LETTER S#
			{120373, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL SANS-SERIF ITALIC SMALL T ¡ú LATIN SMALL LETTER T#
			{120374, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL SANS-SERIF ITALIC SMALL U ¡ú LATIN SMALL LETTER U#
			{120375, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL SANS-SERIF ITALIC SMALL V ¡ú LATIN SMALL LETTER V#
			{120376, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL SANS-SERIF ITALIC SMALL W ¡ú LATIN SMALL LETTER W#
			{120377, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL SANS-SERIF ITALIC SMALL X ¡ú LATIN SMALL LETTER X#
			{120378, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL SANS-SERIF ITALIC SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120379, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL SANS-SERIF ITALIC SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120380, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120381, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120382, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{120383, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120384, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120385, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120386, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120387, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{120388, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{120389, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120390, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120391, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120392, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120393, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{120394, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120395, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{120396, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{120397, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{120398, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120399, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120400, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120401, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120402, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120403, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120404, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120405, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{120406, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL A ¡ú LATIN SMALL LETTER A#
			{120407, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL B ¡ú LATIN SMALL LETTER B#
			{120408, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL C ¡ú LATIN SMALL LETTER C#
			{120409, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL D ¡ú LATIN SMALL LETTER D#
			{120410, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL E ¡ú LATIN SMALL LETTER E#
			{120411, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL F ¡ú LATIN SMALL LETTER F#
			{120412, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL G ¡ú LATIN SMALL LETTER G#
			{120413, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL H ¡ú LATIN SMALL LETTER H#
			{120414, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL I ¡ú LATIN SMALL LETTER I#
			{120415, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL J ¡ú LATIN SMALL LETTER J#
			{120416, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL K ¡ú LATIN SMALL LETTER K#
			{120417, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL L ¡ú LATIN SMALL LETTER L#
			{120418, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120419, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL N ¡ú LATIN SMALL LETTER N#
			{120420, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL O ¡ú LATIN SMALL LETTER O#
			{120421, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL P ¡ú LATIN SMALL LETTER P#
			{120422, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120423, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL R ¡ú LATIN SMALL LETTER R#
			{120424, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL S ¡ú LATIN SMALL LETTER S#
			{120425, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL T ¡ú LATIN SMALL LETTER T#
			{120426, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL U ¡ú LATIN SMALL LETTER U#
			{120427, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL V ¡ú LATIN SMALL LETTER V#
			{120428, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL W ¡ú LATIN SMALL LETTER W#
			{120429, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL X ¡ú LATIN SMALL LETTER X#
			{120430, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120431, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120432, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL MONOSPACE CAPITAL A ¡ú LATIN CAPITAL LETTER A#
			{120433, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL MONOSPACE CAPITAL B ¡ú LATIN CAPITAL LETTER B#
			{120434, "C"},  // MA# ( ?? ¡ú C ) MATHEMATICAL MONOSPACE CAPITAL C ¡ú LATIN CAPITAL LETTER C#
			{120435, "D"},  // MA# ( ?? ¡ú D ) MATHEMATICAL MONOSPACE CAPITAL D ¡ú LATIN CAPITAL LETTER D#
			{120436, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL MONOSPACE CAPITAL E ¡ú LATIN CAPITAL LETTER E#
			{120437, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL MONOSPACE CAPITAL F ¡ú LATIN CAPITAL LETTER F#
			{120438, "G"},  // MA# ( ?? ¡ú G ) MATHEMATICAL MONOSPACE CAPITAL G ¡ú LATIN CAPITAL LETTER G#
			{120439, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL MONOSPACE CAPITAL H ¡ú LATIN CAPITAL LETTER H#
			{120440, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL MONOSPACE CAPITAL I ¡ú LATIN SMALL LETTER L# ¡úI¡ú
			{120441, "J"},  // MA# ( ?? ¡ú J ) MATHEMATICAL MONOSPACE CAPITAL J ¡ú LATIN CAPITAL LETTER J#
			{120442, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL MONOSPACE CAPITAL K ¡ú LATIN CAPITAL LETTER K#
			{120443, "L"},  // MA# ( ?? ¡ú L ) MATHEMATICAL MONOSPACE CAPITAL L ¡ú LATIN CAPITAL LETTER L#
			{120444, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL MONOSPACE CAPITAL M ¡ú LATIN CAPITAL LETTER M#
			{120445, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL MONOSPACE CAPITAL N ¡ú LATIN CAPITAL LETTER N#
			{120446, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL MONOSPACE CAPITAL O ¡ú LATIN CAPITAL LETTER O#
			{120447, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL MONOSPACE CAPITAL P ¡ú LATIN CAPITAL LETTER P#
			{120448, "Q"},  // MA# ( ?? ¡ú Q ) MATHEMATICAL MONOSPACE CAPITAL Q ¡ú LATIN CAPITAL LETTER Q#
			{120449, "R"},  // MA# ( ?? ¡ú R ) MATHEMATICAL MONOSPACE CAPITAL R ¡ú LATIN CAPITAL LETTER R#
			{120450, "S"},  // MA# ( ?? ¡ú S ) MATHEMATICAL MONOSPACE CAPITAL S ¡ú LATIN CAPITAL LETTER S#
			{120451, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL MONOSPACE CAPITAL T ¡ú LATIN CAPITAL LETTER T#
			{120452, "U"},  // MA# ( ?? ¡ú U ) MATHEMATICAL MONOSPACE CAPITAL U ¡ú LATIN CAPITAL LETTER U#
			{120453, "V"},  // MA# ( ?? ¡ú V ) MATHEMATICAL MONOSPACE CAPITAL V ¡ú LATIN CAPITAL LETTER V#
			{120454, "W"},  // MA# ( ?? ¡ú W ) MATHEMATICAL MONOSPACE CAPITAL W ¡ú LATIN CAPITAL LETTER W#
			{120455, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL MONOSPACE CAPITAL X ¡ú LATIN CAPITAL LETTER X#
			{120456, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL MONOSPACE CAPITAL Y ¡ú LATIN CAPITAL LETTER Y#
			{120457, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL MONOSPACE CAPITAL Z ¡ú LATIN CAPITAL LETTER Z#
			{120458, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL MONOSPACE SMALL A ¡ú LATIN SMALL LETTER A#
			{120459, "b"},  // MA# ( ?? ¡ú b ) MATHEMATICAL MONOSPACE SMALL B ¡ú LATIN SMALL LETTER B#
			{120460, "c"},  // MA# ( ?? ¡ú c ) MATHEMATICAL MONOSPACE SMALL C ¡ú LATIN SMALL LETTER C#
			{120461, "d"},  // MA# ( ?? ¡ú d ) MATHEMATICAL MONOSPACE SMALL D ¡ú LATIN SMALL LETTER D#
			{120462, "e"},  // MA# ( ?? ¡ú e ) MATHEMATICAL MONOSPACE SMALL E ¡ú LATIN SMALL LETTER E#
			{120463, "f"},  // MA# ( ?? ¡ú f ) MATHEMATICAL MONOSPACE SMALL F ¡ú LATIN SMALL LETTER F#
			{120464, "g"},  // MA# ( ?? ¡ú g ) MATHEMATICAL MONOSPACE SMALL G ¡ú LATIN SMALL LETTER G#
			{120465, "h"},  // MA# ( ?? ¡ú h ) MATHEMATICAL MONOSPACE SMALL H ¡ú LATIN SMALL LETTER H#
			{120466, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL MONOSPACE SMALL I ¡ú LATIN SMALL LETTER I#
			{120467, "j"},  // MA# ( ?? ¡ú j ) MATHEMATICAL MONOSPACE SMALL J ¡ú LATIN SMALL LETTER J#
			{120468, "k"},  // MA# ( ?? ¡ú k ) MATHEMATICAL MONOSPACE SMALL K ¡ú LATIN SMALL LETTER K#
			{120469, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL MONOSPACE SMALL L ¡ú LATIN SMALL LETTER L#
			{120470, "rn"}, // MA# ( ?? ¡ú rn ) MATHEMATICAL MONOSPACE SMALL M ¡ú LATIN SMALL LETTER R, LATIN SMALL LETTER N# ¡úm¡ú
			{120471, "n"},  // MA# ( ?? ¡ú n ) MATHEMATICAL MONOSPACE SMALL N ¡ú LATIN SMALL LETTER N#
			{120472, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL MONOSPACE SMALL O ¡ú LATIN SMALL LETTER O#
			{120473, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL MONOSPACE SMALL P ¡ú LATIN SMALL LETTER P#
			{120474, "q"},  // MA# ( ?? ¡ú q ) MATHEMATICAL MONOSPACE SMALL Q ¡ú LATIN SMALL LETTER Q#
			{120475, "r"},  // MA# ( ?? ¡ú r ) MATHEMATICAL MONOSPACE SMALL R ¡ú LATIN SMALL LETTER R#
			{120476, "s"},  // MA# ( ?? ¡ú s ) MATHEMATICAL MONOSPACE SMALL S ¡ú LATIN SMALL LETTER S#
			{120477, "t"},  // MA# ( ?? ¡ú t ) MATHEMATICAL MONOSPACE SMALL T ¡ú LATIN SMALL LETTER T#
			{120478, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL MONOSPACE SMALL U ¡ú LATIN SMALL LETTER U#
			{120479, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL MONOSPACE SMALL V ¡ú LATIN SMALL LETTER V#
			{120480, "w"},  // MA# ( ?? ¡ú w ) MATHEMATICAL MONOSPACE SMALL W ¡ú LATIN SMALL LETTER W#
			{120481, "x"},  // MA# ( ?? ¡ú x ) MATHEMATICAL MONOSPACE SMALL X ¡ú LATIN SMALL LETTER X#
			{120482, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL MONOSPACE SMALL Y ¡ú LATIN SMALL LETTER Y#
			{120483, "z"},  // MA# ( ?? ¡ú z ) MATHEMATICAL MONOSPACE SMALL Z ¡ú LATIN SMALL LETTER Z#
			{120484, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL ITALIC SMALL DOTLESS I ¡ú LATIN SMALL LETTER I# ¡ú?¡ú
			{120488, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL BOLD CAPITAL ALPHA ¡ú LATIN CAPITAL LETTER A# ¡ú??¡ú
			{120489, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL BOLD CAPITAL BETA ¡ú LATIN CAPITAL LETTER B# ¡ú¦¢¡ú
			{120492, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL BOLD CAPITAL EPSILON ¡ú LATIN CAPITAL LETTER E# ¡ú??¡ú
			{120493, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL BOLD CAPITAL ZETA ¡ú LATIN CAPITAL LETTER Z# ¡ú¦¦¡ú
			{120494, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL BOLD CAPITAL ETA ¡ú LATIN CAPITAL LETTER H# ¡ú¦§¡ú
			{120496, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD CAPITAL IOTA ¡ú LATIN SMALL LETTER L# ¡ú¦©¡ú
			{120497, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL BOLD CAPITAL KAPPA ¡ú LATIN CAPITAL LETTER K# ¡ú¦ª¡ú
			{120499, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL BOLD CAPITAL MU ¡ú LATIN CAPITAL LETTER M# ¡ú??¡ú
			{120500, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL BOLD CAPITAL NU ¡ú LATIN CAPITAL LETTER N# ¡ú??¡ú
			{120502, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL BOLD CAPITAL OMICRON ¡ú LATIN CAPITAL LETTER O# ¡ú??¡ú
			{120504, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL BOLD CAPITAL RHO ¡ú LATIN CAPITAL LETTER P# ¡ú??¡ú
			{120507, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL BOLD CAPITAL TAU ¡ú LATIN CAPITAL LETTER T# ¡ú¦³¡ú
			{120508, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL BOLD CAPITAL UPSILON ¡ú LATIN CAPITAL LETTER Y# ¡ú¦´¡ú
			{120510, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL BOLD CAPITAL CHI ¡ú LATIN CAPITAL LETTER X# ¡ú¦¶¡ú
			{120514, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL BOLD SMALL ALPHA ¡ú LATIN SMALL LETTER A# ¡ú¦Á¡ú
			{120516, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL BOLD SMALL GAMMA ¡ú LATIN SMALL LETTER Y# ¡ú¦Ã¡ú
			{120522, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL BOLD SMALL IOTA ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{120526, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL BOLD SMALL NU ¡ú LATIN SMALL LETTER V# ¡ú¦Í¡ú
			{120528, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL BOLD SMALL OMICRON ¡ú LATIN SMALL LETTER O# ¡ú??¡ú
			{120530, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL BOLD SMALL RHO ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120532, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL BOLD SMALL SIGMA ¡ú LATIN SMALL LETTER O# ¡ú¦Ò¡ú
			{120534, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL BOLD SMALL UPSILON ¡ú LATIN SMALL LETTER U# ¡ú¦Ô¡ú¡ú?¡ú
			{120544, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL BOLD RHO SYMBOL ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120546, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL ITALIC CAPITAL ALPHA ¡ú LATIN CAPITAL LETTER A# ¡ú¦¡¡ú
			{120547, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL ITALIC CAPITAL BETA ¡ú LATIN CAPITAL LETTER B# ¡ú¦¢¡ú
			{120550, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL ITALIC CAPITAL EPSILON ¡ú LATIN CAPITAL LETTER E# ¡ú¦¥¡ú
			{120551, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL ITALIC CAPITAL ZETA ¡ú LATIN CAPITAL LETTER Z# ¡ú??¡ú
			{120552, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL ITALIC CAPITAL ETA ¡ú LATIN CAPITAL LETTER H# ¡ú¦§¡ú
			{120554, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL ITALIC CAPITAL IOTA ¡ú LATIN SMALL LETTER L# ¡ú¦©¡ú
			{120555, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL ITALIC CAPITAL KAPPA ¡ú LATIN CAPITAL LETTER K# ¡ú??¡ú
			{120557, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL ITALIC CAPITAL MU ¡ú LATIN CAPITAL LETTER M# ¡ú??¡ú
			{120558, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL ITALIC CAPITAL NU ¡ú LATIN CAPITAL LETTER N# ¡ú??¡ú
			{120560, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL ITALIC CAPITAL OMICRON ¡ú LATIN CAPITAL LETTER O# ¡ú??¡ú
			{120562, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL ITALIC CAPITAL RHO ¡ú LATIN CAPITAL LETTER P# ¡ú¦±¡ú
			{120565, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL ITALIC CAPITAL TAU ¡ú LATIN CAPITAL LETTER T# ¡ú¦³¡ú
			{120566, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL ITALIC CAPITAL UPSILON ¡ú LATIN CAPITAL LETTER Y# ¡ú¦´¡ú
			{120568, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL ITALIC CAPITAL CHI ¡ú LATIN CAPITAL LETTER X# ¡ú¦¶¡ú
			{120572, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL ITALIC SMALL ALPHA ¡ú LATIN SMALL LETTER A# ¡ú¦Á¡ú
			{120574, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL ITALIC SMALL GAMMA ¡ú LATIN SMALL LETTER Y# ¡ú¦Ã¡ú
			{120580, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL ITALIC SMALL IOTA ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{120584, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL ITALIC SMALL NU ¡ú LATIN SMALL LETTER V# ¡ú¦Í¡ú
			{120586, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL ITALIC SMALL OMICRON ¡ú LATIN SMALL LETTER O# ¡ú??¡ú
			{120588, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL ITALIC SMALL RHO ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120590, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL ITALIC SMALL SIGMA ¡ú LATIN SMALL LETTER O# ¡ú¦Ò¡ú
			{120592, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL ITALIC SMALL UPSILON ¡ú LATIN SMALL LETTER U# ¡ú¦Ô¡ú¡ú?¡ú
			{120602, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL ITALIC RHO SYMBOL ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120604, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL BOLD ITALIC CAPITAL ALPHA ¡ú LATIN CAPITAL LETTER A# ¡ú¦¡¡ú
			{120605, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL BOLD ITALIC CAPITAL BETA ¡ú LATIN CAPITAL LETTER B# ¡ú¦¢¡ú
			{120608, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL BOLD ITALIC CAPITAL EPSILON ¡ú LATIN CAPITAL LETTER E# ¡ú¦¥¡ú
			{120609, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL BOLD ITALIC CAPITAL ZETA ¡ú LATIN CAPITAL LETTER Z# ¡ú¦¦¡ú
			{120610, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL BOLD ITALIC CAPITAL ETA ¡ú LATIN CAPITAL LETTER H# ¡ú??¡ú
			{120612, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD ITALIC CAPITAL IOTA ¡ú LATIN SMALL LETTER L# ¡ú¦©¡ú
			{120613, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL BOLD ITALIC CAPITAL KAPPA ¡ú LATIN CAPITAL LETTER K# ¡ú??¡ú
			{120615, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL BOLD ITALIC CAPITAL MU ¡ú LATIN CAPITAL LETTER M# ¡ú??¡ú
			{120616, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL BOLD ITALIC CAPITAL NU ¡ú LATIN CAPITAL LETTER N# ¡ú??¡ú
			{120618, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL BOLD ITALIC CAPITAL OMICRON ¡ú LATIN CAPITAL LETTER O# ¡ú??¡ú
			{120620, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL BOLD ITALIC CAPITAL RHO ¡ú LATIN CAPITAL LETTER P# ¡ú¦±¡ú
			{120623, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL BOLD ITALIC CAPITAL TAU ¡ú LATIN CAPITAL LETTER T# ¡ú¦³¡ú
			{120624, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL BOLD ITALIC CAPITAL UPSILON ¡ú LATIN CAPITAL LETTER Y# ¡ú¦´¡ú
			{120626, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL BOLD ITALIC CAPITAL CHI ¡ú LATIN CAPITAL LETTER X# ¡ú??¡ú
			{120630, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL BOLD ITALIC SMALL ALPHA ¡ú LATIN SMALL LETTER A# ¡ú¦Á¡ú
			{120632, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL BOLD ITALIC SMALL GAMMA ¡ú LATIN SMALL LETTER Y# ¡ú¦Ã¡ú
			{120638, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL BOLD ITALIC SMALL IOTA ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{120642, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL BOLD ITALIC SMALL NU ¡ú LATIN SMALL LETTER V# ¡ú¦Í¡ú
			{120644, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL BOLD ITALIC SMALL OMICRON ¡ú LATIN SMALL LETTER O# ¡ú??¡ú
			{120646, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL BOLD ITALIC SMALL RHO ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120648, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL BOLD ITALIC SMALL SIGMA ¡ú LATIN SMALL LETTER O# ¡ú¦Ò¡ú
			{120650, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL BOLD ITALIC SMALL UPSILON ¡ú LATIN SMALL LETTER U# ¡ú¦Ô¡ú¡ú?¡ú
			{120660, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL BOLD ITALIC RHO SYMBOL ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120662, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL SANS-SERIF BOLD CAPITAL ALPHA ¡ú LATIN CAPITAL LETTER A# ¡ú¦¡¡ú
			{120663, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL SANS-SERIF BOLD CAPITAL BETA ¡ú LATIN CAPITAL LETTER B# ¡ú¦¢¡ú
			{120666, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL SANS-SERIF BOLD CAPITAL EPSILON ¡ú LATIN CAPITAL LETTER E# ¡ú¦¥¡ú
			{120667, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL SANS-SERIF BOLD CAPITAL ZETA ¡ú LATIN CAPITAL LETTER Z# ¡ú¦¦¡ú
			{120668, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL SANS-SERIF BOLD CAPITAL ETA ¡ú LATIN CAPITAL LETTER H# ¡ú¦§¡ú
			{120670, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF BOLD CAPITAL IOTA ¡ú LATIN SMALL LETTER L# ¡ú¦©¡ú
			{120671, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL SANS-SERIF BOLD CAPITAL KAPPA ¡ú LATIN CAPITAL LETTER K# ¡ú¦ª¡ú
			{120673, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL SANS-SERIF BOLD CAPITAL MU ¡ú LATIN CAPITAL LETTER M# ¡ú¦¬¡ú
			{120674, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL SANS-SERIF BOLD CAPITAL NU ¡ú LATIN CAPITAL LETTER N# ¡ú¦­¡ú
			{120676, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SANS-SERIF BOLD CAPITAL OMICRON ¡ú LATIN CAPITAL LETTER O# ¡ú¦¯¡ú
			{120678, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL SANS-SERIF BOLD CAPITAL RHO ¡ú LATIN CAPITAL LETTER P# ¡ú¦±¡ú
			{120681, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL SANS-SERIF BOLD CAPITAL TAU ¡ú LATIN CAPITAL LETTER T# ¡ú¦³¡ú
			{120682, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL SANS-SERIF BOLD CAPITAL UPSILON ¡ú LATIN CAPITAL LETTER Y# ¡ú¦´¡ú
			{120684, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL SANS-SERIF BOLD CAPITAL CHI ¡ú LATIN CAPITAL LETTER X# ¡ú¦¶¡ú
			{120688, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL SANS-SERIF BOLD SMALL ALPHA ¡ú LATIN SMALL LETTER A# ¡ú¦Á¡ú
			{120690, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL SANS-SERIF BOLD SMALL GAMMA ¡ú LATIN SMALL LETTER Y# ¡ú¦Ã¡ú
			{120696, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL SANS-SERIF BOLD SMALL IOTA ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{120700, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL SANS-SERIF BOLD SMALL NU ¡ú LATIN SMALL LETTER V# ¡ú¦Í¡ú
			{120702, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL SANS-SERIF BOLD SMALL OMICRON ¡ú LATIN SMALL LETTER O# ¡ú¦Ï¡ú
			{120704, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SANS-SERIF BOLD SMALL RHO ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120706, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL SANS-SERIF BOLD SMALL SIGMA ¡ú LATIN SMALL LETTER O# ¡ú¦Ò¡ú
			{120708, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL SANS-SERIF BOLD SMALL UPSILON ¡ú LATIN SMALL LETTER U# ¡ú¦Ô¡ú¡ú?¡ú
			{120718, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SANS-SERIF BOLD RHO SYMBOL ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120720, "A"},  // MA# ( ?? ¡ú A ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL ALPHA ¡ú LATIN CAPITAL LETTER A# ¡ú¦¡¡ú
			{120721, "B"},  // MA# ( ?? ¡ú B ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL BETA ¡ú LATIN CAPITAL LETTER B# ¡ú¦¢¡ú
			{120724, "E"},  // MA# ( ?? ¡ú E ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL EPSILON ¡ú LATIN CAPITAL LETTER E# ¡ú¦¥¡ú
			{120725, "Z"},  // MA# ( ?? ¡ú Z ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL ZETA ¡ú LATIN CAPITAL LETTER Z# ¡ú¦¦¡ú
			{120726, "H"},  // MA# ( ?? ¡ú H ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL ETA ¡ú LATIN CAPITAL LETTER H# ¡ú¦§¡ú
			{120728, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL IOTA ¡ú LATIN SMALL LETTER L# ¡ú¦©¡ú
			{120729, "K"},  // MA# ( ?? ¡ú K ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL KAPPA ¡ú LATIN CAPITAL LETTER K# ¡ú¦ª¡ú
			{120731, "M"},  // MA# ( ?? ¡ú M ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL MU ¡ú LATIN CAPITAL LETTER M# ¡ú¦¬¡ú
			{120732, "N"},  // MA# ( ?? ¡ú N ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL NU ¡ú LATIN CAPITAL LETTER N# ¡ú¦­¡ú
			{120734, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL OMICRON ¡ú LATIN CAPITAL LETTER O# ¡ú¦¯¡ú
			{120736, "P"},  // MA# ( ?? ¡ú P ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL RHO ¡ú LATIN CAPITAL LETTER P# ¡ú¦±¡ú
			{120739, "T"},  // MA# ( ?? ¡ú T ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL TAU ¡ú LATIN CAPITAL LETTER T# ¡ú¦³¡ú
			{120740, "Y"},  // MA# ( ?? ¡ú Y ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL UPSILON ¡ú LATIN CAPITAL LETTER Y# ¡ú¦´¡ú
			{120742, "X"},  // MA# ( ?? ¡ú X ) MATHEMATICAL SANS-SERIF BOLD ITALIC CAPITAL CHI ¡ú LATIN CAPITAL LETTER X# ¡ú¦¶¡ú
			{120746, "a"},  // MA# ( ?? ¡ú a ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL ALPHA ¡ú LATIN SMALL LETTER A# ¡ú¦Á¡ú
			{120748, "y"},  // MA# ( ?? ¡ú y ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL GAMMA ¡ú LATIN SMALL LETTER Y# ¡ú¦Ã¡ú
			{120754, "i"},  // MA# ( ?? ¡ú i ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL IOTA ¡ú LATIN SMALL LETTER I# ¡ú¦É¡ú
			{120758, "v"},  // MA# ( ?? ¡ú v ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL NU ¡ú LATIN SMALL LETTER V# ¡ú¦Í¡ú
			{120760, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL OMICRON ¡ú LATIN SMALL LETTER O# ¡ú¦Ï¡ú
			{120762, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL RHO ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120764, "o"},  // MA# ( ?? ¡ú o ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL SIGMA ¡ú LATIN SMALL LETTER O# ¡ú¦Ò¡ú
			{120766, "u"},  // MA# ( ?? ¡ú u ) MATHEMATICAL SANS-SERIF BOLD ITALIC SMALL UPSILON ¡ú LATIN SMALL LETTER U# ¡ú¦Ô¡ú¡ú?¡ú
			{120776, "p"},  // MA# ( ?? ¡ú p ) MATHEMATICAL SANS-SERIF BOLD ITALIC RHO SYMBOL ¡ú LATIN SMALL LETTER P# ¡ú¦Ñ¡ú
			{120778, "F"},  // MA# ( ?? ¡ú F ) MATHEMATICAL BOLD CAPITAL DIGAMMA ¡ú LATIN CAPITAL LETTER F# ¡ú?¡ú
			{120782, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL BOLD DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{120783, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL BOLD DIGIT ONE ¡ú LATIN SMALL LETTER L# ¡ú1¡ú
			{120784, "2"},  // MA# ( ?? ¡ú 2 ) MATHEMATICAL BOLD DIGIT TWO ¡ú DIGIT TWO#
			{120785, "3"},  // MA# ( ?? ¡ú 3 ) MATHEMATICAL BOLD DIGIT THREE ¡ú DIGIT THREE#
			{120786, "4"},  // MA# ( ?? ¡ú 4 ) MATHEMATICAL BOLD DIGIT FOUR ¡ú DIGIT FOUR#
			{120787, "5"},  // MA# ( ?? ¡ú 5 ) MATHEMATICAL BOLD DIGIT FIVE ¡ú DIGIT FIVE#
			{120788, "6"},  // MA# ( ?? ¡ú 6 ) MATHEMATICAL BOLD DIGIT SIX ¡ú DIGIT SIX#
			{120789, "7"},  // MA# ( ?? ¡ú 7 ) MATHEMATICAL BOLD DIGIT SEVEN ¡ú DIGIT SEVEN#
			{120790, "8"},  // MA# ( ?? ¡ú 8 ) MATHEMATICAL BOLD DIGIT EIGHT ¡ú DIGIT EIGHT#
			{120791, "9"},  // MA# ( ?? ¡ú 9 ) MATHEMATICAL BOLD DIGIT NINE ¡ú DIGIT NINE#
			{120792, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL DOUBLE-STRUCK DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{120793, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL DOUBLE-STRUCK DIGIT ONE ¡ú LATIN SMALL LETTER L# ¡ú1¡ú
			{120794, "2"},  // MA# ( ?? ¡ú 2 ) MATHEMATICAL DOUBLE-STRUCK DIGIT TWO ¡ú DIGIT TWO#
			{120795, "3"},  // MA# ( ?? ¡ú 3 ) MATHEMATICAL DOUBLE-STRUCK DIGIT THREE ¡ú DIGIT THREE#
			{120796, "4"},  // MA# ( ?? ¡ú 4 ) MATHEMATICAL DOUBLE-STRUCK DIGIT FOUR ¡ú DIGIT FOUR#
			{120797, "5"},  // MA# ( ?? ¡ú 5 ) MATHEMATICAL DOUBLE-STRUCK DIGIT FIVE ¡ú DIGIT FIVE#
			{120798, "6"},  // MA# ( ?? ¡ú 6 ) MATHEMATICAL DOUBLE-STRUCK DIGIT SIX ¡ú DIGIT SIX#
			{120799, "7"},  // MA# ( ?? ¡ú 7 ) MATHEMATICAL DOUBLE-STRUCK DIGIT SEVEN ¡ú DIGIT SEVEN#
			{120800, "8"},  // MA# ( ?? ¡ú 8 ) MATHEMATICAL DOUBLE-STRUCK DIGIT EIGHT ¡ú DIGIT EIGHT#
			{120801, "9"},  // MA# ( ?? ¡ú 9 ) MATHEMATICAL DOUBLE-STRUCK DIGIT NINE ¡ú DIGIT NINE#
			{120802, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SANS-SERIF DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{120803, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF DIGIT ONE ¡ú LATIN SMALL LETTER L# ¡ú1¡ú
			{120804, "2"},  // MA# ( ?? ¡ú 2 ) MATHEMATICAL SANS-SERIF DIGIT TWO ¡ú DIGIT TWO#
			{120805, "3"},  // MA# ( ?? ¡ú 3 ) MATHEMATICAL SANS-SERIF DIGIT THREE ¡ú DIGIT THREE#
			{120806, "4"},  // MA# ( ?? ¡ú 4 ) MATHEMATICAL SANS-SERIF DIGIT FOUR ¡ú DIGIT FOUR#
			{120807, "5"},  // MA# ( ?? ¡ú 5 ) MATHEMATICAL SANS-SERIF DIGIT FIVE ¡ú DIGIT FIVE#
			{120808, "6"},  // MA# ( ?? ¡ú 6 ) MATHEMATICAL SANS-SERIF DIGIT SIX ¡ú DIGIT SIX#
			{120809, "7"},  // MA# ( ?? ¡ú 7 ) MATHEMATICAL SANS-SERIF DIGIT SEVEN ¡ú DIGIT SEVEN#
			{120810, "8"},  // MA# ( ?? ¡ú 8 ) MATHEMATICAL SANS-SERIF DIGIT EIGHT ¡ú DIGIT EIGHT#
			{120811, "9"},  // MA# ( ?? ¡ú 9 ) MATHEMATICAL SANS-SERIF DIGIT NINE ¡ú DIGIT NINE#
			{120812, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL SANS-SERIF BOLD DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{120813, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL SANS-SERIF BOLD DIGIT ONE ¡ú LATIN SMALL LETTER L# ¡ú1¡ú
			{120814, "2"},  // MA# ( ?? ¡ú 2 ) MATHEMATICAL SANS-SERIF BOLD DIGIT TWO ¡ú DIGIT TWO#
			{120815, "3"},  // MA# ( ?? ¡ú 3 ) MATHEMATICAL SANS-SERIF BOLD DIGIT THREE ¡ú DIGIT THREE#
			{120816, "4"},  // MA# ( ?? ¡ú 4 ) MATHEMATICAL SANS-SERIF BOLD DIGIT FOUR ¡ú DIGIT FOUR#
			{120817, "5"},  // MA# ( ?? ¡ú 5 ) MATHEMATICAL SANS-SERIF BOLD DIGIT FIVE ¡ú DIGIT FIVE#
			{120818, "6"},  // MA# ( ?? ¡ú 6 ) MATHEMATICAL SANS-SERIF BOLD DIGIT SIX ¡ú DIGIT SIX#
			{120819, "7"},  // MA# ( ?? ¡ú 7 ) MATHEMATICAL SANS-SERIF BOLD DIGIT SEVEN ¡ú DIGIT SEVEN#
			{120820, "8"},  // MA# ( ?? ¡ú 8 ) MATHEMATICAL SANS-SERIF BOLD DIGIT EIGHT ¡ú DIGIT EIGHT#
			{120821, "9"},  // MA# ( ?? ¡ú 9 ) MATHEMATICAL SANS-SERIF BOLD DIGIT NINE ¡ú DIGIT NINE#
			{120822, "O"},  // MA# ( ?? ¡ú O ) MATHEMATICAL MONOSPACE DIGIT ZERO ¡ú LATIN CAPITAL LETTER O# ¡ú0¡ú
			{120823, "l"},  // MA# ( ?? ¡ú l ) MATHEMATICAL MONOSPACE DIGIT ONE ¡ú LATIN SMALL LETTER L# ¡ú1¡ú
			{120824, "2"},  // MA# ( ?? ¡ú 2 ) MATHEMATICAL MONOSPACE DIGIT TWO ¡ú DIGIT TWO#
			{120825, "3"},  // MA# ( ?? ¡ú 3 ) MATHEMATICAL MONOSPACE DIGIT THREE ¡ú DIGIT THREE#
			{120826, "4"},  // MA# ( ?? ¡ú 4 ) MATHEMATICAL MONOSPACE DIGIT FOUR ¡ú DIGIT FOUR#
			{120827, "5"},  // MA# ( ?? ¡ú 5 ) MATHEMATICAL MONOSPACE DIGIT FIVE ¡ú DIGIT FIVE#
			{120828, "6"},  // MA# ( ?? ¡ú 6 ) MATHEMATICAL MONOSPACE DIGIT SIX ¡ú DIGIT SIX#
			{120829, "7"},  // MA# ( ?? ¡ú 7 ) MATHEMATICAL MONOSPACE DIGIT SEVEN ¡ú DIGIT SEVEN#
			{120830, "8"},  // MA# ( ?? ¡ú 8 ) MATHEMATICAL MONOSPACE DIGIT EIGHT ¡ú DIGIT EIGHT#
			{120831, "9"},  // MA# ( ?? ¡ú 9 ) MATHEMATICAL MONOSPACE DIGIT NINE ¡ú DIGIT NINE#
			{125127, "l"},  // MA#* ( ???? ¡ú l ) MENDE KIKAKUI DIGIT ONE ¡ú LATIN SMALL LETTER L#
			{125131, "8"},  // MA#* ( ???? ¡ú 8 ) MENDE KIKAKUI DIGIT FIVE ¡ú DIGIT EIGHT#
			{126464, "l"},  // MA# ( ???? ¡ú l ) ARABIC MATHEMATICAL ALEF ¡ú LATIN SMALL LETTER L# ¡ú???¡ú¡ú1¡ú
			{126500, "o"},  // MA# ( ???? ¡ú o ) ARABIC MATHEMATICAL INITIAL HEH ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{126564, "o"},  // MA# ( ???? ¡ú o ) ARABIC MATHEMATICAL STRETCHED HEH ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{126592, "l"},  // MA# ( ???? ¡ú l ) ARABIC MATHEMATICAL LOOPED ALEF ¡ú LATIN SMALL LETTER L# ¡ú???¡ú¡ú1¡ú
			{126596, "o"},  // MA# ( ???? ¡ú o ) ARABIC MATHEMATICAL LOOPED HEH ¡ú LATIN SMALL LETTER O# ¡ú???¡ú
			{127232, "O."}, // MA#* ( ?? ¡ú O. ) DIGIT ZERO FULL STOP ¡ú LATIN CAPITAL LETTER O, FULL STOP# ¡ú0.¡ú
			{127233, "O,"}, // MA#* ( ?? ¡ú O, ) DIGIT ZERO COMMA ¡ú LATIN CAPITAL LETTER O, COMMA# ¡ú0,¡ú
			{127234, "l,"}, // MA#* ( ?? ¡ú l, ) DIGIT ONE COMMA ¡ú LATIN SMALL LETTER L, COMMA# ¡ú1,¡ú
			{127235, "2,"}, // MA#* ( ?? ¡ú 2, ) DIGIT TWO COMMA ¡ú DIGIT TWO, COMMA#
			{127236, "3,"}, // MA#* ( ?? ¡ú 3, ) DIGIT THREE COMMA ¡ú DIGIT THREE, COMMA#
			{127237, "4,"}, // MA#* ( ?? ¡ú 4, ) DIGIT FOUR COMMA ¡ú DIGIT FOUR, COMMA#
			{127238, "5,"}, // MA#* ( ?? ¡ú 5, ) DIGIT FIVE COMMA ¡ú DIGIT FIVE, COMMA#
			{127239, "6,"}, // MA#* ( ?? ¡ú 6, ) DIGIT SIX COMMA ¡ú DIGIT SIX, COMMA#
			{127240, "7,"}, // MA#* ( ?? ¡ú 7, ) DIGIT SEVEN COMMA ¡ú DIGIT SEVEN, COMMA#
			{127241, "8,"}, // MA#* ( ?? ¡ú 8, ) DIGIT EIGHT COMMA ¡ú DIGIT EIGHT, COMMA#
			{127242, "9,"}, // MA#* ( ?? ¡ú 9, ) DIGIT NINE COMMA ¡ú DIGIT NINE, COMMA#
			{127248, "(A)"},// MA#* ( ?? ¡ú (A) ) PARENTHESIZED LATIN CAPITAL LETTER A ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER A, RIGHT PARENTHESIS#
			{127249, "(B)"},// MA#* ( ?? ¡ú (B) ) PARENTHESIZED LATIN CAPITAL LETTER B ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER B, RIGHT PARENTHESIS#
			{127250, "(C)"},// MA#* ( ?? ¡ú (C) ) PARENTHESIZED LATIN CAPITAL LETTER C ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER C, RIGHT PARENTHESIS#
			{127251, "(D)"},// MA#* ( ?? ¡ú (D) ) PARENTHESIZED LATIN CAPITAL LETTER D ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER D, RIGHT PARENTHESIS#
			{127252, "(E)"},// MA#* ( ?? ¡ú (E) ) PARENTHESIZED LATIN CAPITAL LETTER E ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER E, RIGHT PARENTHESIS#
			{127253, "(F)"},// MA#* ( ?? ¡ú (F) ) PARENTHESIZED LATIN CAPITAL LETTER F ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER F, RIGHT PARENTHESIS#
			{127254, "(G)"},// MA#* ( ?? ¡ú (G) ) PARENTHESIZED LATIN CAPITAL LETTER G ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER G, RIGHT PARENTHESIS#
			{127255, "(H)"},// MA#* ( ?? ¡ú (H) ) PARENTHESIZED LATIN CAPITAL LETTER H ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER H, RIGHT PARENTHESIS#
			{127256, "(l)"},// MA#* ( ?? ¡ú (l) ) PARENTHESIZED LATIN CAPITAL LETTER I ¡ú LEFT PARENTHESIS, LATIN SMALL LETTER L, RIGHT PARENTHESIS# ¡ú(I)¡ú
			{127257, "(J)"},// MA#* ( ?? ¡ú (J) ) PARENTHESIZED LATIN CAPITAL LETTER J ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER J, RIGHT PARENTHESIS#
			{127258, "(K)"},// MA#* ( ?? ¡ú (K) ) PARENTHESIZED LATIN CAPITAL LETTER K ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER K, RIGHT PARENTHESIS#
			{127259, "(L)"},// MA#* ( ?? ¡ú (L) ) PARENTHESIZED LATIN CAPITAL LETTER L ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER L, RIGHT PARENTHESIS#
			{127260, "(M)"},// MA#* ( ?? ¡ú (M) ) PARENTHESIZED LATIN CAPITAL LETTER M ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER M, RIGHT PARENTHESIS#
			{127261, "(N)"},// MA#* ( ?? ¡ú (N) ) PARENTHESIZED LATIN CAPITAL LETTER N ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER N, RIGHT PARENTHESIS#
			{127262, "(O)"},// MA#* ( ?? ¡ú (O) ) PARENTHESIZED LATIN CAPITAL LETTER O ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER O, RIGHT PARENTHESIS#
			{127263, "(P)"},// MA#* ( ?? ¡ú (P) ) PARENTHESIZED LATIN CAPITAL LETTER P ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER P, RIGHT PARENTHESIS#
			{127264, "(Q)"},// MA#* ( ?? ¡ú (Q) ) PARENTHESIZED LATIN CAPITAL LETTER Q ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER Q, RIGHT PARENTHESIS#
			{127265, "(R)"},// MA#* ( ?? ¡ú (R) ) PARENTHESIZED LATIN CAPITAL LETTER R ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER R, RIGHT PARENTHESIS#
			{127266, "(S)"},// MA#* ( ?? ¡ú (S) ) PARENTHESIZED LATIN CAPITAL LETTER S ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER S, RIGHT PARENTHESIS#
			{127267, "(T)"},// MA#* ( ?? ¡ú (T) ) PARENTHESIZED LATIN CAPITAL LETTER T ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER T, RIGHT PARENTHESIS#
			{127268, "(U)"},// MA#* ( ?? ¡ú (U) ) PARENTHESIZED LATIN CAPITAL LETTER U ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER U, RIGHT PARENTHESIS#
			{127269, "(V)"},// MA#* ( ?? ¡ú (V) ) PARENTHESIZED LATIN CAPITAL LETTER V ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER V, RIGHT PARENTHESIS#
			{127270, "(W)"},// MA#* ( ?? ¡ú (W) ) PARENTHESIZED LATIN CAPITAL LETTER W ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER W, RIGHT PARENTHESIS#
			{127271, "(X)"},// MA#* ( ?? ¡ú (X) ) PARENTHESIZED LATIN CAPITAL LETTER X ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER X, RIGHT PARENTHESIS#
			{127272, "(Y)"},// MA#* ( ?? ¡ú (Y) ) PARENTHESIZED LATIN CAPITAL LETTER Y ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER Y, RIGHT PARENTHESIS#
			{127273, "(Z)"},// MA#* ( ?? ¡ú (Z) ) PARENTHESIZED LATIN CAPITAL LETTER Z ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER Z, RIGHT PARENTHESIS#
			{127274, "(S)"},// MA#* ( ?? ¡ú (S) ) TORTOISE SHELL BRACKETED LATIN CAPITAL LETTER S ¡ú LEFT PARENTHESIS, LATIN CAPITAL LETTER S, RIGHT PARENTHESIS# ¡ú¡²S¡³¡ú
			{128768, "QE"}, // MA#* ( ?? ¡ú QE ) ALCHEMICAL SYMBOL FOR QUINTESSENCE ¡ú LATIN CAPITAL LETTER Q, LATIN CAPITAL LETTER E#
			{128775, "AR"}, // MA#* ( ?? ¡ú AR ) ALCHEMICAL SYMBOL FOR AQUA REGIA-2 ¡ú LATIN CAPITAL LETTER A, LATIN CAPITAL LETTER R#
			{128844, "C"},  // MA#* ( ?? ¡ú C ) ALCHEMICAL SYMBOL FOR CALX ¡ú LATIN CAPITAL LETTER C#
			{128860, "sss"},// MA#* ( ?? ¡ú sss ) ALCHEMICAL SYMBOL FOR STRATUM SUPER STRATUM ¡ú LATIN SMALL LETTER S, LATIN SMALL LETTER S, LATIN SMALL LETTER S#
			{128872, "T"},  // MA#* ( ?? ¡ú T ) ALCHEMICAL SYMBOL FOR CRUCIBLE-4 ¡ú LATIN CAPITAL LETTER T#
			{128875, "MB"}, // MA#* ( ?? ¡ú MB ) ALCHEMICAL SYMBOL FOR BATH OF MARY ¡ú LATIN CAPITAL LETTER M, LATIN CAPITAL LETTER B#
			{128876, "VB"}, // MA#* ( ?? ¡ú VB ) ALCHEMICAL SYMBOL FOR BATH OF VAPOURS ¡ú LATIN CAPITAL LETTER V, LATIN CAPITAL LETTER B#
	};
}

namespace gal
{
	constexpr const char* find_confusable(const std::uint32_t codepoint)
	{
		const auto it = std::ranges::lower_bound(g_confusable, codepoint, std::ranges::less{}, [](const auto& confusable)
												 { return confusable.codepoint; });

		return it != std::ranges::end(g_confusable) && it->codepoint == codepoint ? it->text : nullptr;
	}
}
