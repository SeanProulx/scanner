/*************************************************************
 * File name: scanner.c
 * Author: Joshua Heagle, 040-622-441
 *  	   Sean Proulx, 040-645-429
 * Course: CST8152 - Compilers
 * Assignment: 2
 * Date: March 6th, 2012
 * Professor: Svillen Ranev.
 * Purpose: This is the scanner file. It contains all the necessary
 functions to scan the characters provided the buffer and return the
 appropriate tokens or error token. 
 *	Function list:
 *      char_class(char c);
 *      get_next_state(int, char, int *);
 *		static int iskeyword(char * kw_lexeme);
 *		static long atool(char * lexeme);
 *		Token aa_func02(char *lexeme);
 *		Token aa_func07(char *lexeme);
 *		Token aa_func08(char *lexeme);
 *		Token aa_func09(char *lexeme);
 *		Token aa_func11(char *lexeme);
 *************************************************************/
#include <stdio.h>   /* standard input / output */
#include <ctype.h>   /* conversion functions */
#include <stdlib.h>  /* standard library functions */
#include <string.h>  /* string functions */
#include <limits.h>  /* integer type constants */
#include <float.h>   /* floating-point type constants */

/*#define NDEBUG        to suppress assert() call */
#include <assert.h>  /* assert() prototype */

/* project header files */
#include "buffer.h"
#include "token.h"
#include "table.h"

#define DEBUG  /* for conditional processing */
#undef  DEBUG

/* Global objects - variables */
/* This buffer is used as a repository for string literals.
   It is defined in platy_st.c */
extern Buffer * str_LTBL; /*String literal table  */
int line; /* current line number of the source code */
extern int scerrnum;     /* defined in platy_st.c - run-time error number */


/* No other global variable declarations/definitiond are allowed */

/* Function prototypes */
static int char_class(char c); /* character class function */
static int get_next_state(int, char, int *); /* state machine function */
static int iskeyword(char * kw_lexeme); /*keywords lookup functuion */
static long atool(char * lexeme); /* converts octal string to decimal value */

void scanner_init(Buffer *buf)
{
        ca_addc(buf, '\0'); /* in case EOF is not in the buffer */
        b_reset(str_LTBL);  /* reset the string literal table */
        line = 1;           /*set the source code line number to 1*/
}


/********************************************************************************************
 * Purpose: Scans characters received by the buffer and creates tokens accordingly dependant
 on the platypus specifications. If the character is not part of the grammar then error 
 tokens are provided.
 * Authors: Sean Proulx / Joshua Heagle
 * Versions: 1.0
 * Called functions: b_create(), strlen(), b_destroy(), ca_addc(), isalnum(), ca_setmark()
strcpy(), ca_getmark(), strncpy(), free().
 * Parameters: sc_buf is a pointer to the buffer we want to read characters from.
 * Return value: Error tokens or valid tokens.
 * Algorithm: Check the buffer for specific symbols part of the platypus language and provide
 tokens or error tokens accordingly.
 *******************************************************************************************/
Token malpar_next_token(Buffer * sc_buf)
{
	Token t;				  /*token to be returned */
	unsigned char c;		  /* input symbol */
	Buffer *lex_buf;		  /* temporary buffer for holding lexemes */
	int accept = NOAS;		  /* Not Accepting State */
	int state = 0;			  /* Start state in the Transition Table */
	int lexstart;			  /* current lexeme start offset */
	int i = 0;				  /* Counter for loop */
	static int forward = -1;   /* current input char offset */
	char* temp_Buf;			  /*pointer to the string of the file source*/
	unsigned char nextChar;   /*Used to check the next letter*/

	/* endless loop broken by token returns */
	while (1){ 
		/* Check if the buffer is valid, if not provide run time error message */
		if (sc_buf == NULL) {
			scerrnum = 1; /*Set scanner error number to 1 */
			t.code = ERR_T; /*Set token code to ERR_T and error message */
			strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
			return t;
		}
		else { /* Otherwise, set temp buffer to point to the begining of the buffer and begin scanning */
		temp_Buf = sc_buf->ca_head;
		c = temp_Buf[++forward];
		lexstart = forward;
		}
		/* Special cases or exceptions processing */
		/* If character is a space || horizontal tab || vertical tab || form feed, skip it */
		if (c == ' ' || c == '\t' || c == '\v' || c =='\f') continue;
		/* If character is new line || carriage return, increment line number and skip it */
		if (c == '\n' || c == '\r' ) {line++; continue;}
		/* Separators */
		if (c == '{'){ t.code = LBR_T; return t; }
		if (c == '}'){ t.code = RBR_T; return t; }
		if (c == '('){ t.code = LPR_T; return t; }
		if (c == ')'){ t.code = RPR_T; return t; }
		/* Check if symbol is either '=' or '==' */
		if (c == '=' ){
			/* Check if next character is '=' */
			c = temp_Buf[++forward];
			if ( c == '='){ t.code = REL_OP_T; t.attribute.rel_op = EQ; return t; }
			/* If not, step back */
			--forward;
			t.code = ASS_OP_T;
			return t;
		}
		if (c == '>'){ t.code = REL_OP_T; t.attribute.rel_op = GT; return t;}
		if (c == '<'){
			/* Check if next symbol is string concatenation '<>' */
			c = temp_Buf[++forward];
			if (c == '>'){ t.code = SCC_OP_T; return t; }
			/* If not, step back */
			--forward;
			t.code = REL_OP_T; 
			t.attribute.rel_op = LT; 
			return t;
		}
		/* Check if next symbol ! is followed by either < for comment processing or = for not equal to */
		if (c == '!'){
			nextChar = temp_Buf[++forward];
			if (nextChar == '='){ t.code = REL_OP_T; t.attribute.rel_op = NE; return t; }
			if (nextChar == '<'){
				/* Continue reading until new line is found, if EOF is detected, return error token */
				while (sc_buf->ca_head[++forward] != '\n') {
					if (sc_buf->ca_head[forward] == 0 || sc_buf->ca_head[forward] == (unsigned char)EOF){
						t.code = ERR_T;
						t.attribute.err_lex[0] = c;
						t.attribute.err_lex[1] = nextChar;
						t.attribute.err_lex[2] = '\0';
						--forward;
						line++;
						return t;
					}
				}
				line++;
				continue;
			}
			/* If there was no symbol read after '!', set error token and continue till new line */
			t.code = ERR_T;
			t.attribute.err_lex[0] = c;
			t.attribute.err_lex[1] = nextChar;
			t.attribute.err_lex[2] = '\0';
			while (temp_Buf[++forward] != '\n') {
				if (sc_buf->ca_head[forward] == 0 || sc_buf->ca_head[forward] == (unsigned char)EOF){
					--forward;
					break;
				}
			}
			line++;
			return t;
		}
		if (c == ','){ t.code = COM_T; return t; }
		/* Arithmetic Operators */
		if (c == '+'){ t.code = ART_OP_T; t.attribute.arr_op = PLUS; return t; }
		if (c == '-'){ t.code = ART_OP_T; t.attribute.arr_op = MINUS; return t; }
		if (c == '*'){ t.code = ART_OP_T; t.attribute.arr_op = MULT; return t; }
		if (c == '/'){ t.code = ART_OP_T; t.attribute.arr_op = DIV; return t; }
		if (c == ';') {t.code = EOS_T; return t; }
		/* .AND., .OR. operators*/
		if (c == '.') {
			/* Store next character in buffer into nextChar and check if AND. / OR. follow, if not return error token */
			nextChar = sc_buf->ca_head[forward+1];
			if (temp_Buf[forward+1] == 'A' && temp_Buf[forward+2] == 'N' && temp_Buf[forward+3] == 'D' && temp_Buf[forward+4] == '.'){
				t.code = LOG_OP_T;
				t.attribute.log_op = AND;
				/* Advance buffer ahead 4 */
				forward += 4;
				return t;
			}
			if (temp_Buf[forward+1] == 'O' && temp_Buf[forward+2] == 'R' && temp_Buf[forward+3] == '.'){
				t.code = LOG_OP_T;
				t.attribute.log_op = OR;
				/* Advance buffer ahead 3 */
				forward += 3;
				return t;
			}
			t.code = ERR_T;
			t.attribute.err_lex[0] = c;
			t.attribute.err_lex[1] = '\0';
			return t;

		}
			/* This section does the required algorithm to get the string */
			/* The string is in between " " and is terminated with a \0 */
			if (c == '"'){
				Buffer *b_quote;	  /* New buffer created for the string */
				int checkQuotes = 0;  /* Flag set to break loop if second " is detected */
				/* Create the new buffer using b_create, if error occurs, return error token */
				b_quote = b_create(20, 5,'m');
					if (b_quote == NULL){ 
					scerrnum = 1;
					t.code = ERR_T;
					strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
					return t;
					}
			/* Loop until second quotation marks are found */
			while (checkQuotes != 1){
				c = temp_Buf[++forward];
				/* If new line or carriage return is found, increment line */
				if (c == '\n' || c == '\r') line++;
				if (c == 0 || c == (unsigned char)EOF){
					int length = strlen(b_quote->ca_head);
					/* Check if string is over 20 characters */
					if (length > 20){
						t.attribute.err_lex[0] = '"';
						/* If it is over 20, copy contents into err_lex and append 3 dots to the end .*/
						for (i = 0; i < 17; i++) 	/* i+1 to offset the first " */
							t.attribute.err_lex[i+1] = b_quote->ca_head[i];
						t.attribute.err_lex[i++] = '.';
						t.attribute.err_lex[i++] = '.';
						t.attribute.err_lex[i++] = '.';
					} 
					else {
						for (i = 0; i < 20; i++)
							t.attribute.err_lex[i] = b_quote->ca_head[i];
					}
				
					t.attribute.err_lex[i++] = '\0';
					t.code = ERR_T;
					b_destroy(b_quote);
					--forward;
					return t;
				}
				if (c == '"'){
					/* Attempt to add \0 to the end of the string, return run time error if unable */
					if (ca_addc(b_quote, '\0') == NULL){
						scerrnum = 1;
						t.code = ERR_T;
						strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
						return t;
					}
					/* Once added, set flag to 1 and break loop */
					checkQuotes = 1;
				}
				else{
					if (ca_addc(b_quote, c) == NULL){
						scerrnum = 1;
						t.code = ERR_T;
						strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
						return t;
					}
				}
			}/* End of while loop */

			t.attribute.str_offset = str_LTBL->addc_offset;
			t.code = STR_T;
			for (i = 0; i < b_quote->addc_offset; i++){
				/*check for null return, runtime error */
				if (ca_addc(str_LTBL, b_quote->ca_head[i]) == NULL)
				{
					scerrnum = 1;
					t.code = ERR_T;
					strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
					return t;
				}
			}
			b_destroy(b_quote);
			return t;

		}
		/* Process state transition table */
		if(isalnum(c)){

			ca_setmark(sc_buf,forward);

			while (accept == NOAS){
				/*break loop if state is ES*/
				state = get_next_state(state, c, &accept);
				c = temp_Buf[++forward];
			}
			/* Retract one character */
			--forward;
			/* Get entire string into lex_buf, then gets copied to lexeme */
			lex_buf = b_create(15,10,'m');
			if (lex_buf == NULL){
				scerrnum = 1;
				t.code = ERR_T;
				strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
				return t;
			}
			lexstart = ca_getmark(sc_buf);
			if (accept == ASWR){ --forward; }
			while (lexstart <= forward){/*Copies the string into lex_buf*/
				c = temp_Buf[lexstart++];
				if (ca_addc(lex_buf, c) == NULL){
					scerrnum = 1;
					t.code = ERR_T;
					strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
					return t;
				}
			}

			if (ca_pack(lex_buf) == NULL){
					scerrnum = 1;
					t.code = ERR_T;
					strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
					return t;
			}

			if (ca_addc(lex_buf, '\0') == NULL){
					scerrnum = 1;
					t.code = ERR_T;
					strcpy(t.attribute.err_lex, "RUN TIME ERROR: ");
					return t;
			}
			t = aa_table[state](lex_buf->ca_head);
			b_destroy(lex_buf);
			return t;
		}
		/*Check for EOF*/
		if (c == 0 || c == (unsigned char)EOF) {
			t.code = SEOF_T;
			return t;
		}
		t.code = ERR_T;
		t.attribute.err_lex[0] = c;
		t.attribute.err_lex[1] = '\0';
		return t;
	}
}

/********************************************************************************************
 * Purpose: Determines what the input char is that is passed in to the function.
 * Author: Svillen Ranev
 * History/Versions: 1.0
 * Called functions:
 * Parameters:  int state - int number representing the state number
				char c - the char that is to be tested.
				int *accept - represents the integer value of the accepting function.
 * Return value: int next - representing the next state.
 *******************************************************************************************/
int get_next_state(int state, char c, int *accept)
{
	int col;
	int next;
	col = char_class(c);
	next = st_table[state][col];
#ifdef DEBUG
printf("Input symbol: %c Row: %d Column: %d Next: %d \n",c,state,col,next);
#endif

       assert(next != IS);

#ifdef DEBUG
	if(next == IS){
	  printf("Scanner Error: Illegal state:\n");
	  printf("Input symbol: %c Row: %d Column: %d\n",c,state,col);
	  exit(1);
	}
#endif
	*accept = as_table[next];
	return next;
}

/********************************************************************************************
 * Purpose: Returns the column number by checking the value c.
 * Author: Sean Proulx / Joshua Heagle
 * Versions: 1.0
 * Called functions: isalnum()
					 isdigit()
 * Parameters: char c is the passed argument to be checked.
 * Return value: Can be values from 0 to 6, to determine the column.
 * Algorithm: Checks the variable and sets the column number as specified in table.h
 *******************************************************************************************/
int char_class (char c)
{
	int val; /* Column number */

	if (isalpha(c)) val = 0;
	else if (isdigit(c))
	{
		if (c == '0') val = 1;
		else if (c == '8' || c == '9') val = 3;
		else val = 2;
	}
	else if (c == '.') val = 4;
	else if (c == '#') val = 5;
	else val = 6;
	return val;
}
/********************************************************************************************
 * Purpose: Check string is AVID or SVID and corrects accordingly.
 * Author: Sean Proulx / Joshua Heagle
 * Version: 1.0
 * Called functions: iskeyword()
					 strlen()
					 st_install()
					 st_store()
					 exit()
 * Parameters: Array of characters to be tested.
 * Return value: Returns a Token.
 * Algorithm: Determines if the string passed in is a AVID or SVID, and corrects the string
			format if the (AVID or SVID)name is too long.  The token is set to either AVID
			or SVID.  Passes lexemes to symbol table to populated it. If the Symbol table
			is full exit, and store contents of symbol table to file.  Exits and releases
			memory.
 *******************************************************************************************/
Token aa_func02(char lexeme[]){
	int result = 0; /*Holds the result to keyward match*/
	int i = 0; /*counter*/
	int length; /*length of the passed in string*/
	Token t; /*Token to be assigned to AVID or SVID*/

	result = iskeyword(lexeme);
	/*If iskeyword() did not return -1, then set token accordingly*/
	if (result >= 0){ t.attribute.kwt_idx = result; t.code = KW_T; return t;}
	length = strlen(lexeme);
	if (length > VID_LEN) length = VID_LEN;
	if (lexeme[strlen(lexeme)-1] == '#') t.code = SVID_T;
	else t.code = AVID_T;

	for (i = 0; i < length; i++) 
		t.attribute.vid_lex[i] = lexeme[i];

	if (t.code == SVID_T) 
		t.attribute.vid_lex[length-1] = '#';
	/* Append \0 to end of string */
	t.attribute.vid_lex[length] = '\0';

  return t;
}
/********************************************************************************************
 * Purpose: Accepting Function for integer literal(IL) - decimal constant (DIL)
 * Author: Sean Proulx / Joshua Heagle
 * Version: 1.0
 * Called functions: atol()
					 strlen()
 * Parameters: char lexeme - The string to be determined if it's a valid number.
 * Return value: Returns a Token, or Error Token on failure.
 * Algorithm: Determines if the string is a valid long (not greater than a long can be).
 *******************************************************************************************/
Token aa_func07(char lexeme[]){
	Token t;
	long number;
	int i = 0;
	int length;

	number = atol(lexeme);
	length = strlen(lexeme);
	if (length > ERR_LEN)
		length = ERR_LEN;
   /*PLAT_MAX_NUM used to ensure that integer is ALWAYS 2 bytes,
    since Borland gives a warning*/
	if (length > INL_LEN || (int)number < 0 || number >= PLAT_MAX_NUM){ /*2 byte int*/
		for (i = 0; i < length; i++)
			t.attribute.err_lex[i] = lexeme[i];
		t.attribute.err_lex[length] = '\0';
		t.code = ERR_T;
		return t;
	}
	t.attribute.int_value = (int)number;
	t.code = INL_T;

	return t;
}
/********************************************************************************************
 * Purpose: ACCEPTING FUNCTION FOR THE floating-point literal (FPL)
 * Author: Brian Reed
 * History/Versions: 1.0
 * Called functions: atof()
					 strlen()
 * Parameters: char lexeme - The string to be determined if it's a valid number.
 * Return value: Returns a Token, or Error Token on failure.
 * Algorithm: Determines if the string is a valid float (not greater than a float can be).
 *******************************************************************************************/
Token aa_func08(char lexeme[]){
	Token t;
	double floatNum = 0;
	int i = 0;
	int length;
	length = strlen(lexeme);
	if (length > ERR_LEN)
		length = ERR_LEN;
	floatNum = atof(lexeme);
	if ((floatNum > FLT_MAX || floatNum < FLT_MIN) && floatNum != 0.0){ /*4 byte float*/
		for (i = 0; i < length; i++)
			t.attribute.err_lex[i] = lexeme[i];
		t.attribute.err_lex[length] = '\0';
		t.code = ERR_T;
		return t;
	}

	t.attribute.flt_value = (float)floatNum;
	t.code = FPL_T;

  return t;
}
/********************************************************************************************
 * Purpose: ACCEPTING FUNCTION FOR THE ERROR TOKEN
 * Author: Brian Reed
 * History/Versions: 1.0
 * Called functions: strlen()
 * Parameters: char lexeme to be added to the Token
 * Return value: Error Token.
 * Algorithm: Returns an error token, and the name is corrected if it is too long.
 *******************************************************************************************/
Token aa_func09(char lexeme[]){
	Token t;
	int i = 0;
	int length;
	length = strlen(lexeme);
	if(length > ERR_LEN)
		length = ERR_LEN;
	for (i = 0; i < length; i++)
		t.attribute.err_lex[i] = lexeme[i];
	t.attribute.err_lex[length] = '\0';
	t.code = ERR_T;

	return t;
}
/********************************************************************************************
 * Purpose: FUNCTION FOR THE integer literal(IL) - octal constant (OIL)
 * Author: Brian Reed
 * History/Versions: 1.0
 * Called functions: strlen()
					 atool()
 * Parameters: char lexeme to be added to the Token
 * Return value: Token with octal information, Error Token on failure.
 * Algorithm: Returns an error token, and the name is corrected if it is too long.
 *******************************************************************************************/
Token aa_func11(char lexeme[]){
	Token t;
	int i = 0;
	long convert = 0;
	int length;
	length = strlen(lexeme);

	if (length > ERR_LEN) length = ERR_LEN;
	convert = atool(lexeme);
	if (length > INL_LEN+1 ||convert > INT_MAX || convert < 0){ /*2 byte int*/
		for (i = 0; i < length; i++)
			t.attribute.err_lex[i] = lexeme[i];
		t.attribute.err_lex[length] = '\0';
		t.code = ERR_T;
		return t;
	}

	t.attribute.int_value = (int)convert;
	t.code = INL_T;

  return t;
}
/********************************************************************************************
 * Purpose: Changes an octal in a string form and converts it to a long.
 * Author: Brian Reed
 * History/Versions: 1.0
 * Called functions: strtol()
 * Parameters: char lexeme to be converted to a long.
 * Return value: long representing the lexeme input, converted to long.
 * Algorithm: Takes in the lexeme and passes to strtol function to convert to long.
 *******************************************************************************************/
long atool(char * lexeme){
	long lNumber = 0;
	lNumber = strtol( lexeme, NULL, 8);
	return lNumber;
}
/********************************************************************************************
 * Purpose: Tests if the passed in string is a keyword.
 * Author: Brian Reed
 * History/Versions: 1.0
 * Called functions: strcmp()
 * Parameters: char *kw_lexeme as a string
 * Return value: int i which is the matching keyword in the array kw_table
 * Algorithm: compares the string passed in to the array of keyword text.
 *******************************************************************************************/
int iskeyword(char * kw_lexeme){

	int i = 0; /*Counter for loop*/

	for (i = 0; i < KWT_SIZE; i++){
		if (strcmp(kw_table[i], kw_lexeme) == 0)
			return i;
	}

	return -1;
}
