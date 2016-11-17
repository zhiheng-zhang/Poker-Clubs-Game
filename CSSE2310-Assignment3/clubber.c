#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


#define FALSE 0
#define TRUE (!FALSE)

int check_hub_message(char* message);
/*
 * Unrecoverable error types (passed to fatal())
 */
enum error {
    E_USAGE = 1, /* incorrect no. of cmd line args */
    E_PLAYERS,   /* invalid player count           */
    E_PLAYER_ID, /* Invalid player ID          */
    E_LOAD,      /* error reading grid file        */
    E_EOF,       /* EOF when user input expected   */
    E_NOMEM = 9  /* malloc returns NULL            */
};


char *cards_table[4][13] = {
    {"2S", "3S", "4S", "5S", "6S", "7S", "8S", "9S", "TS", "JS", "QS", "KS", "AS"},
    {"2C", "3C", "4C", "5C", "6C", "7C", "8C", "9C", "TC", "JC", "QC", "KC", "AC"},
    {"2D", "3D", "4D", "5D", "6D", "7D", "8D", "9D", "TD", "JD", "QD", "KD", "AD"},
    {"2H", "3H", "4H", "5H", "6H", "7H", "8H", "9H", "TH", "JH", "QH", "KH", "AH"}
};


/*
 * Prints the error message corresponding to 'err' and exits the calling
 * process.
 */
static void fatal(enum error err)
{
    static const char *const msgs[] = {
        0,
        "Usage: player number_of_players myid\n",
        "Invalid player count\n",
        "Invalid player ID\n",
        "Invalid grid file\n",
        "Error reading grid contents\n",
        "End of user input\n",
        0,
        0,
        "System call failure\n"
    };
    
    fputs(msgs[err], stderr);
    exit(err);
}



/* for easy pass argument use struct */
struct Game{
    int  playerNumber;
    char player;
    int **hand;
    int **played;
    
};

typedef struct Game Game;


void check_command_arugments_format(int argc, char** argv, Game *g) {
    char *endPointer;
    if(argc != 3) {
        fatal(E_USAGE);
    }
    g->playerNumber = strtol(argv[1], &endPointer, 10);
    if(g->playerNumber < 2 || g->playerNumber > 4 || *endPointer != 0) {
        fatal(E_PLAYERS);
    }
    
    if(strcmp("A", argv[2]) && strcmp("B", argv[2]) && strcmp("C", argv[2]) && strcmp("D", argv[2])) {
        fatal(E_PLAYER_ID);
    }
    g->player = argv[2][0];
    
}

void allocate_game(Game *g) {
    int i;
    g->hand = (int **)malloc(sizeof(int *) * g->playerNumber);
    g->played = (int **)malloc(sizeof(int *) * g->playerNumber);
    for(i = 0; i < g->playerNumber; i++) {
        g->hand[i] = malloc(sizeof(int)*13);
        g->played[i] = malloc(sizeof(int)*13);
    }
}

/* display cards by suits (S,C,D,H), th?n by rank within each suit */
void display_hand(Game *g) {
    int i, j, comma_flag;
    comma_flag = FALSE;
    fprintf(stderr, "Hand: ");
    for(i = 0; i < g->playerNumber; i++) {
        for(j = 0; j < 13; j++) {
            if(g->hand[i][j]) {
                if(comma_flag) {
                    fprintf(stderr, ",");
                }
                
                if(!comma_flag) {
                    comma_flag = TRUE;
                }
                fprintf(stderr, "%s", cards_table[i][j]);
            }
        }
    }
    fprintf(stderr, "\n");
    fflush(stdout);
    
}

/* display each played suit in increasing rank order. */
void display_played(Game *g) {
    int i, j, comma_flag;
    comma_flag = FALSE;
    
    for(i = 0; i < g->playerNumber; i++) {
        
        if(i == 0) {
            fprintf(stderr, "Played (S): ");
        } else if (i == 1) {
            fprintf(stderr, "Played (C): ");
        } else if (i == 2) {
            fprintf(stderr, "Played (D): ");
        } else if (i == 3) {
            fprintf(stderr, "Played (H): ");
        }
        for(j = 0; j < 13; j++) {
            if(g->played[i][j]) {
                if(comma_flag) {
                    fprintf(stderr, ",");
                }
                
                if(!comma_flag) {
                    comma_flag = TRUE;
                }
                fprintf(stderr, "%s", cards_table[i][j]);
            }
        }
        fprintf(stderr, "\n");
    }
    
    fflush(stdout);
    
}

void loss_of_hub() {
    fprintf(stderr, "Unexpected loss of hub\n");
    exit(4);
}

int main(int argc, char **argv) {
    Game *g = malloc(sizeof(Game));
    char hubMessage[21];
    
    check_command_arugments_format(argc, argv, g);
    allocate_game(g);
    //send dash to std
    fprintf(stdout, "-");
    fflush(stdout);
    
    //listen on stdin
    if(fgets(hubMessage, 21, stdin) == NULL) {
        loss_of_hub();
    }
    
    // check if the hub message is valid
    if(check_hub_message(hubMessage)){
        fprintf(stderr, "Bad message from hub\n");
        exit(5);
    }
    
    //print_message_from_hub
    hubMessage[20] = 0;
    fprintf(stderr, "From hub:%s\n", hubMessage);
    fflush(stderr);
    
    //
    
    
    //display to stderr
    g->hand[0][0] = 1;
    g->played[0][2] = 1;
    display_hand(g);
    display_played(g);
    
    return 0;
}

//check the validity
int check_hub_message(char* message) {
    char firstChar[9];
    sscanf(message, "%s", firstChar);
    
    if(strcmp(firstChar, "newround") == 0) {
        return 0;
    }
    if(strcmp(firstChar, "newtrick") == 0) {
        return 0;
    }
    if(strcmp(firstChar, "trickover") == 0) {
        return 0;
    }
    if(strcmp(firstChar, "yourturn") == 0) {
        return 0;
    }
    if(strcmp(firstChar, "played") == 0) {
        return 0;
    }
    if(strcmp(firstChar, "scores") == 0) {
        return 0;
    }
    if(strcmp(firstChar, "end") == 0) {
        return 0;
    }
    return 1; 
}

