/* Imports */ 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/* Constants and Global Declarations: */ 
#define MAX_KEYWORDS 100
#define BUFFER_SIZE 2048

// Lexical Analysis:
FILE *inputFile;
FILE *tokenFile;
FILE *errorFile;

char* keywords[30]; // eg. for, do, while, etc...
int table[30][127] = {0}; // transition table for automaton machine (30 states, 127 inputs)
char buffer1[BUFFER_SIZE]; // Dual buffers for reading 
char buffer2[BUFFER_SIZE];
char* currentBuffer = buffer1;

bool is_blank(const char *str) {
    while (*str) {
        if (!isspace((unsigned char)*str))
            return false;
        str++;
    }
    return true;
}

// Define Token Types --> expand upon in future iterations
typedef enum {
    TOKEN_INT,
    TOKEN_IDENTIFIER,
    TOKEN_OPERATOR,
    TOKEN_FLOAT,
    TOKEN_KEYWORD,
    TOKEN_LITERAL,
    TOKEN_EOF, // end of file
    TOKEN_ERROR // error
} TokenType;

// Array to map TokenType to strings
const char *tokenTypeStrings[] = {
    "TOKEN_INT",
    "TOKEN_IDENTIFIER",
    "TOKEN_OPERATOR",
    "TOKEN_FLOAT",
    "TOKEN_KEYWORD",
    "TOKEN_LITERAL",
    "TOKEN_EOF",
    "TOKEN_ERROR"
};

/* Token
    - struct to represent token
    - getNextToken() - parse inputFile char by char and build tokens for lexical analysis 
 */
typedef struct {
    TokenType type;
    char buffer_val1[BUFFER_SIZE]; // dual buffer for storing large chars
    char buffer_val2[BUFFER_SIZE];
    int line; // line and char where token starts
    int character;
} Token;

// Parse file stream for chars
Token getNextToken() {
    // Declarations
    Token token; 
    int currentState = 0;
    char currentChar;
    int ascii = 0; // ascii conversion of char --> used to index transition table
    int bufferIndex = 0;

    // Initialize Buffers:
    memset(token.buffer_val1, '\0', BUFFER_SIZE);
    memset(token.buffer_val2, '\0', BUFFER_SIZE);

    while (1) {
        currentChar = fgetc(inputFile); // Read char by char until token is complete or EOF
        if (currentChar == EOF) {
            token.type = TOKEN_EOF;
            break;
        } 
        
        // Not EOF --> parse char
        else {
            ascii = (int)currentChar; // get ascii
            // printf("Token: %c --> ascii: %d, currentState = %d\n", currentChar, ascii, currentState);

            /* Return current token given following cases
                - relop char --> states 1, 5 and 6 and current char is != to <, > or =
                - digit char --> states 13, 15 and 18 and current char is != to 0-9, . or E
                - a-z char --> states 10 and current char is != a-z
             */
            if ((currentState == 1 || currentState == 5 || currentState == 6) && (ascii < 60 || ascii > 62)) {
                // printf("Return: %c w/ ascii = %d back to the file stream\n", currentChar, ascii);
                ungetc(currentChar, inputFile);
                token.type = TOKEN_OPERATOR;
                return token;
            }

            else if ((currentState == 13 || currentState == 15 || currentState == 18) && (ascii <  48 || ascii > 57) && (ascii != 46) && (ascii != 69)) {
                // printf("Return: %c w/ ascii = %d back to the file stream\n", currentChar, ascii);
                ungetc(currentChar, inputFile);
                token.type = TOKEN_FLOAT;
                return token;
            }

            else if ((currentState == 10) && (ascii < 97 || ascii > 122)) {
                // printf("Return: %c w/ ascii = %d back to the file stream\n", currentChar, ascii);
                ungetc(currentChar, inputFile);
                token.type = TOKEN_IDENTIFIER;
                return token;
            }

            /* Get current state */
            // If a-z --> set state = 10
            else if (ascii >= 97 && ascii <= 122) {
                currentState = table[currentState][97];
                // printf("Token: %c w/ ascii: %d is a-z --> currentState = %d\n", currentChar, ascii, currentState);
            }

            // If ws or \n --> set state 100, 
            else if (ascii == 32 || ascii == 10) {
                currentState = table[currentState][ascii];
                // printf("Token: %c w/ ascii: %d is ws --> currentState = %d\n", currentChar, ascii, currentState);
            }

            // Double Logic:
            else if (ascii >=  48 && ascii <= 57) {
                currentState = table[currentState][48];
                // printf("Token: %c w/ ascii: %d is 0-9 --> currentState = %d\n", currentChar, ascii, currentState);
            }

            // If E --> follow transition table (accepted with double)
            else if (ascii == 69) {
                currentState = table[currentState][ascii];
                // printf("Token: %c w/ ascii: %d is E --> currentState = %d\n", currentChar, ascii, currentState);
            }

            // If +, -, .  --> follow transition table (accepted with double)
            else if (ascii == 43 || ascii == 45 || ascii == 46) {
                currentState = table[currentState][ascii];
                if (currentState == 0) {
                    if (bufferIndex < BUFFER_SIZE) {token.buffer_val1[bufferIndex] = currentChar;}
                    else {token.buffer_val2[bufferIndex - BUFFER_SIZE] = currentChar;}
                    bufferIndex += 1;     
                    token.type = TOKEN_OPERATOR;
                    return token;
                }
                // printf("Token: %c w/ ascii: %d is +-. --> currentState = %d\n", currentChar, ascii, currentState);
            }

            else if (ascii >= 60 && ascii <= 62) {
                currentState = table[currentState][ascii];
                // printf("Token: %c w/ ascii: %d is <,>,= --> currentState = %d\n", currentChar, ascii, currentState);
            }
                
            // other special
            else {
                currentState = table[currentState][50];
                // printf("Token: %c w/ ascii: %d is Other --> currentState = %d\n", currentChar, ascii, currentState);
            }

            // printf("currentState = %d\n\n", currentState);

            /* Automaton Decisions*/
            // States 1, 6, 10, 13 --> add char to buffer
            if (currentState == 1  || currentState == 5 || currentState == 6 || currentState == 10 || currentState == 13 || currentState == 14 || currentState == 15) {
                // Use buffer 2 if index surpases max size
                if (bufferIndex < BUFFER_SIZE) {token.buffer_val1[bufferIndex] = currentChar;}
                else {token.buffer_val2[bufferIndex - BUFFER_SIZE] = currentChar;}
                bufferIndex += 1;                
            }

            // Accept single special keyword (add char to buffer)
            else if (currentState == 51) {
                // Use buffer 2 if index surpases max size
                if (bufferIndex < BUFFER_SIZE) {token.buffer_val1[bufferIndex] = currentChar;}
                else {token.buffer_val2[bufferIndex - BUFFER_SIZE] = currentChar;}
                bufferIndex += 1;     

                token.type = TOKEN_KEYWORD;
                return token;
            }

            // Accept Operators --> 2, 3, 5, 7, 9 (add to buffer first)
            else if (currentState == 2 || currentState == 3 || currentState == 7 || currentState == 9) {
                if (bufferIndex < BUFFER_SIZE) {token.buffer_val1[bufferIndex] = currentChar;}
                else {token.buffer_val2[bufferIndex - BUFFER_SIZE] = currentChar;}
                bufferIndex += 1;     

                token.type = TOKEN_OPERATOR;
                return token;
            }

            // Accept Single Operator --> 4, 8 --> Need to fix this because its not accepting the char that comes after
            else if (currentState == 4 || currentState == 8) {
                token.type = TOKEN_OPERATOR;
                return token;
            }

            // Accept Identifer if at 100
            else if (currentState == 100) {
                token.type = TOKEN_IDENTIFIER;

                return token;
            }
        }
    }
    return token;
}

/* Functions: */
// Generates Transition Table
void generateTable() {
    FILE *file = fopen("table.txt", "r"); // Open the file for reading
    if (file == NULL) {
        printf("Error opening file.\n");
        return NULL;
    }

    int state, input, next_state;
    while (fscanf(file, "%d, %d, %d\n", &state, &input, &next_state) == 3) {
        table[state][input] = next_state; // Assign next_state to index
    }
    fclose(file);
}

// Generates Reserved/Keyword Array
char** generateKeywords() {
    FILE *file = fopen("keywords.txt", "r");
    if (file == NULL) {
        printf("Error opening file");
        return NULL;
    }

    char* val;
    int i = 0;
    while (fgets(currentBuffer, BUFFER_SIZE, file) != NULL && i < MAX_KEYWORDS) {
        val = malloc(BUFFER_SIZE * sizeof(char)); // Allocate memory for each keyword
        if (val == NULL) {
            printf("Memory allocation failed\n");
            return NULL;
        }
        strcpy(val, currentBuffer); // Copy keyword into allocated memory
        keywords[i] = val;
        i++;

        // Swap Buffers:
        if (currentBuffer == buffer1) {
            currentBuffer = buffer2;
        }
        else {
            currentBuffer = buffer1;
        }
    }
    fclose(file);
    return keywords;
}

/* Phases: */
void lexicalAnalysis() {
    // Initialize: temp token for storing, line and character for tracking position
    Token token;
    
    while(1) {
        token = getNextToken();

        if (!is_blank(token.buffer_val1)) {
            for (int i = 0; i < 30; i++) {
                if ((strncmp(token.buffer_val1, keywords[i], strlen(token.buffer_val1)) == 0) && strlen(token.buffer_val1) == strlen(keywords[i]) - 1) {
                    // printf("matched %s with %s\n", token.buffer_val1, keywords[i]);
                    token.type = TOKEN_KEYWORD;
                    break;
                }
            }

            printf("%s --> %s\n", token.buffer_val1, tokenTypeStrings[token.type]);

            // Write Token to File
            fprintf(tokenFile, "<%s", token.buffer_val1); // Write token to file
            fprintf(tokenFile, "%s", token.buffer_val2); // Write token to file   
            fprintf(tokenFile, ", %s>\n", tokenTypeStrings[token.type]);
        }

        // Check if token is keyword
        if (token.type == TOKEN_EOF) {
            break;
        } 
    }
}

/* Driver Code: */
int main() {

    // Open files --> input file (to compile), tokenFile (symbol table), errorFile
    // char inputFName[100];
    // printf("Enter path of file to compile: ");
    // scanf("%s", inputFName);
    // inputFile = fopen(inputFName, "r");
    inputFile = fopen("test cases/Test7.cp", "r");

    tokenFile = fopen("tokens.txt", "w");
    errorFile = fopen("errors.txt", "w");

    if (inputFile == NULL || tokenFile == NULL || errorFile == NULL) {
        printf("Error opening files\n");
        return NULL;
    }

    // Generate keywords and transition table
    char **keywordsArray = generateKeywords();
    generateTable();
    lexicalAnalysis(); // run parsing

    // Close files and Exit Program
    fclose(inputFile);
    fclose(tokenFile);
    fclose(errorFile);
    return 0;
}