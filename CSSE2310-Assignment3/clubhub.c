
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

struct Deck {
    char data[3];
    struct Deck *next;
};


typedef struct Deck porkDeck;
void insertend(porkDeck* card);
void detect_the_dash(int);
void handler_gameover(int, int*);
void open_file_descriptors(int playernumber);
void set_up_sigpipe_handler();
void set_up_sigint_handler();
void fork_player(int playernumber, char* argv[]);
void new_round(int playernumber);
int if_only_one_deck() ;

char hand[4][52];

//pipe for each player
int fd[4][2];
//file descriptor for each player
int pid[4];
// input handler from each player
FILE* fpin[4] = {0};
// output handler to each player
FILE* fpout[4] = {0};
// output handler from player to hub
int fdCtop[4][2];
// null handler
int fdNull;
// file pointer for deck
FILE* fpForDeck;
// point to card deck
porkDeck* card;
// head pointer to card deck
porkDeck* head = NULL;
FILE* fp;

int main(int argc, char *argv[]) {
    int winScore;
    int playernumber;
    char* endptr;
    /*check the number of arguments*/
    if (argc > 7 || argc < 5) {
        fprintf(stderr, "Usage: clubhub deckfile winscore prog1 prog2 [prog3 [prog4]]\n");
        exit(1);
    }
    winScore = strtol(argv[2], &endptr, 10);
    errno = 0;
    if ((errno == ERANGE && (winScore == INT_MAX || winScore == INT_MIN)) || (errno
                                                                              != 0 && winScore == 0) || endptr == argv[2] || *endptr != 0) {
        fprintf(stderr, "Invalid score\n");
        exit(2);
    }
    
    if (winScore < 0 || winScore == 0) {
        fprintf(stderr, "Invalid score\n");
        exit(2);
    }
    
    
    //check deck file and put into struct deck
    fpForDeck = fopen(argv[1], "r");
    if (fpForDeck == NULL) {
        fprintf(stderr, "Unable to access deckfile\n");
        exit(3);
    }
    
    char *porkDeck;
    porkDeck = (char *)malloc(80 * sizeof(char));
    
    /* check if the deckfile  is valid - suppose always valid*/
    /*
     while (fgets(porkDeck, 18, fp)) {
     if (porkDeck[16] != '\n') {
     fprintf(stderr, "Error reading deck\n");
     fflush(stderr);
     exit(4);
     }
     }
     */
    
    char* p;
    fseek(fpForDeck, 0, SEEK_SET);
    while (fgets(porkDeck, 80, fp)) {
        int j;
        p = strtok(porkDeck, ",");
        while (p != NULL) {
            card = (struct Deck *)malloc(sizeof(porkDeck));
            card->data[0] = p[0];
            card->data[1] = p[1];
            card->data[2] = '\0';
            insertend(card);
            p = strtok(NULL, ",");
        }
    }
    free(porkDeck);
    playernumber = argc - 3;
    int j;
    int a = 52;
    if (playernumber == 3) {
        a = 34;
    }
    if (playernumber == 4) {
        a = 26;
    }
    int i;
    for(i = 0; i < playernumber; i++) {
        for (j = 0; j < a; j++) {
            hand[i][j] = card->data[0];
            hand[i][j+1] = card->data[1];
            card = card->next;
            j++;
        }
        
    }
    fork_player(playernumber, argv);
    
    set_up_sigpipe_handler();
    set_up_sigint_handler();
    //open flie descriptors
    open_file_descriptors(playernumber);
    
    //detect '-' before game
    detect_the_dash(playernumber);
    
    //play game
    card = head;
    int b,c,d;
    int lead = 0;
    char leadType;
    char bigValue;
    int score[4] = {0};
    
    while(1) {
        //new round
        char mCtop[22] = {0};
        new_round(playernumber);
        d = 0;
        while(1) {
            
            c = lead;
            b = 0;
            int thisRunScore = 0;
            //this is the start of a single trick/lead
            //need send hand of each player in order, to stdout?
            
            
            //new trick
            //send newtrick to lead player
            if (fpin[lead]) {
                fprintf(fpin[lead], "newtrick\n");
                fflush(fpin[lead]);
            }
            
            while(1) {
                //receive from child
                if (fpout[c]) {
                    while(1) {
                        if (fgets(mCtop, 22, fpout[c]) != NULL) {
                            break;
                        }
                        if (feof(fpout[c])) {
                            fprintf(stderr, "Player quit\n");
                            fflush(stderr);
                            exit(5);
                        }
                        sleep(1);
                    }
                    //check the form - what is proper formed?
                    if (strlen(mCtop) != 2) {
                        fprintf(stderr,
                                "Invalid message received from player");
                        exit(7);
                    }
                    //check if legal
                    if ((mCtop[0] > '2' && mCtop[0] < '9') || mCtop[0] == 'T') {
                    } else {
                        fprintf(stderr,
                                "Invalid play by player\n");
                        exit(8);
                    }
                }
                
                if (b == 0) {
                    leadType = mCtop[1];
                    bigValue = mCtop[0];
                    //send to stdout who led what
                    printf("Player %c led %s", (char)c + 65, mCtop);
                } else {
                    //send to stdout who played what
                    printf("Player %c played %s", (char)c + 65, mCtop);
                }
                
                if (mCtop[1] == leadType && mCtop[0] > bigValue) {
                    lead = c;
                }
                if (mCtop[1] == 'S') {
                    thisRunScore++;
                }
                
                int j;
                //process the valid message from child
                //send message about which card is played
                for (j = 0; j < playernumber; j++) {
                    if (fpin[j]) {
                        fprintf(fpin[j], "played %s\n", mCtop);
                        fflush(fpin[j]);
                    }
                }
                //send yourturn to the next player
                b++;
                if (b == playernumber) {
                    break;
                }
                c++;
                if (c  == 4) {
                    c = 0;
                }
                if (fpin[c]) {
                    fprintf(fpin[c], "yourturn\n");
                    fflush(fpin[c]);
                }
                
                // wait for response and process the message from child
                
            }
            
            score[lead] =score[lead] + thisRunScore;
            
            //hub to stdout
            
            //announce trickover and report scores to each player after each trick
            for (j = 0; j < playernumber; j++) {
                if (fpin[j]) {
                    fprintf(fpin[j], "trickover\n");
                    fprintf(fpin[j], "scores %d, %d", score[0], score[1]);
                    if (playernumber == 3) {
                        fprintf(fpin[j], ", %d", score[2]);
                    }
                    if (playernumber == 4) {
                        fprintf(fpin[j], ", %d, %d", score[2], score[3]);
                    }
                    fprintf(fpin[j], "\n");
                    fflush(fpin[j]);
                }
            }
            
            
            d++;
            if (d == a/2) {
                break;
            }
            
            
            
            
            
            
            
            
            //check if the end of the deck line
            
        }
        
        
        //sent score message to children
        for (j = 0; j < playernumber; j++) {
            if (fpin[j]) {
                fprintf(fpin[j], "trickover\n");
                fprintf(fpin[j], "scores %d, %d", score[0], score[1]);
                if (playernumber == 3) {
                    fprintf(fpin[j], ", %d", score[2]);
                }
                if (playernumber == 4) {
                    fprintf(fpin[j], ", %d, %d", score[2], score[3]);
                }
                fprintf(fpin[j], "\n");
                fflush(fpin[j]);
            }
        }
        
        if (if_only_one_deck()) {
            handler_gameover(playernumber, score);
            break;
        }
        
        
        // gameover  if someone get more than X
        if (score[0] >= winScore || score[1] >= winScore || score[2] >= winScore || score[3] >= winScore) {
            handler_gameover(playernumber, score);
            //report winners to stdout
            
            break;
            
        }
    }
    
    //close pipe
    int k;
    for (k = 0; k < playernumber; k++) {
        fclose(fpout[k]);
        fclose(fpin[k]);
    }
    for (k = 0; k < playernumber; k++) {
        waitpid(pid[k], NULL, 0);
    }
    return 0;
}

//INT signal handler
void my_handler(int signo) {
    fprintf(stderr, "SIGINT caught\n");
    exit(9);
}

//PIPE signal handler
void my_pipe_handler(int signo) {
    fprintf(stderr, "Invalid message received from player\n");
    exit(7);
}

int if_only_one_deck() {
    return 1;
}


//insert node to the list tail
void insertend(porkDeck *card) {
    porkDeck *lp;
    //char b;
    
    //b = card->data; // should not exist
    if (head == NULL) {
        card->next = card;
        head = card;
    } else {
        lp = head;
        while (lp->next != head) {
            lp = lp->next;
        }
        lp->next = card;
        card->next = head;
    }
    return;
}



//update score and send end
void handler_gameover(int playernumber, int* score) {
    int winner;
    int k;
    int m;
    m = score[0];
    for (k = 1; k < playernumber; k++) {
        if (score[k] > m) {
            m = score[k];
            winner = k;
        }
    }
    
    //report the game final winners to stdout
    fprintf(stdout, "Winner(s): %c", (char)winner + 65);
    
    for ( k = 1; k < m; k++) {
        if (score[k] == m) {
            fprintf(stdout, " %c", (char)k + 65);
        }
    }
    fprintf(stdout, " \n");
    
    for (k = 0; k < playernumber; k++) {//does the sequence matter?
        if (fpin[k]) {
            fprintf(fpin[k], "end\n");
        }
    }
}

//open the pipe for each player
void open_file_descriptors(int playernumber) {
    int i;
    for (i = 0; i < playernumber; i++) {
        fpin[i] = fdopen(fd[i][1], "w");
    }
    
    for (i = 0; i < playernumber; i++) {
        fpout[i] = fdopen(fdCtop[i][0], "r");
        if (fpout[i] == NULL) {
            fprintf(stderr, "Unable to start subprocess\n");
            fflush(stderr);
            exit(5);
        }
    }
}

//enable sigint handler
void set_up_sigint_handler() {
    struct sigaction mySignalInt;
    
    mySignalInt.sa_handler = my_handler;
    mySignalInt.sa_flags = 0;
    sigemptyset(&mySignalInt.sa_mask);
    
    if (sigaction(SIGINT, &mySignalInt, NULL) < 0) {
        fprintf(stderr, "sigaction error\n");
        exit(1);
    }
}

//enable sigpipe handler
void set_up_sigpipe_handler() {
    struct sigaction mySignalPipe;
    
    mySignalPipe.sa_handler = my_pipe_handler;
    mySignalPipe.sa_flags = 0;
    sigemptyset(&mySignalPipe.sa_mask);
    
    if (sigaction(SIGPIPE, &mySignalPipe, NULL) < 0) {
        fprintf(stderr, "sigaction error\n");
        exit(1);
    }
}

//detect the dash at the begin of the game
void detect_the_dash(int playernumber) {
    int flag[4] = {0};
    char start[4] = {0};
    int i;
    
    for (i = 0; i < playernumber; i++) {
        if (fpout[i]) {
            while (flag[i] == 0) {
                start[i] = fgetc(fpout[i]);
                
                if (feof(fpout[i])) {
                    fprintf(stderr, "Unable to start subprocess\n");
                    fflush(stderr);
                    exit(4);
                }
                
                if (start[i] == '-') {
                    flag[i] = 1;
                } else {
                    fprintf(stderr, "Invalid message received from player\n");
                    exit(7);
                }
                sleep(1);
            }
        }
    }
}

// start each player
void fork_player(int playernumber, char* argv[]) {
    int i;
    char playerName[2] = "A";
    char playerNumber[2];
    sprintf(playerNumber, "%d", playernumber);
    
    for (i = 0; i < playernumber; i++) {
        if (pipe(fd[i]) == -1) {
            perror("Can't create pipe");
            exit(1);
        }
        if (pipe(fdCtop[i]) == -1) {
            perror("Can't create pipe");
            exit(1);
        }
        pid[i] = fork();
        if (pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid[i] == 0) {
            if (i > 0) {
                close(fd[i - 1][1]);
                close(fdCtop[i - 1][0]); //added
            }
            close(fd[i][1]);
            dup2(fd[i][0], STDIN_FILENO);
            dup2(fdCtop[i][1], STDOUT_FILENO);
            close(fdCtop[i][1]);
            close(fd[i][0]);
            
            fdNull = open("/dev/null", 0);
            if (fdNull == -1) {
                fprintf(stderr, "Error in open '/dev/null', 0\n");
                exit(EXIT_FAILURE);
            }
            dup2(STDERR_FILENO, fdNull);
            close(STDERR_FILENO);
            if (execl(argv[i + 2], argv[i + 2], playerNumber, playerName,
                      NULL) == -1) {
                fprintf(stderr, "Unable to start subprocess\n");
                exit(4);
            }
            break;
        } else {
            close(fd[i][0]);
            close(fdCtop[i][1]);
            playerName[0]++;
        }
    }
}


// start a new round
void new_round(int playernumber) {
    int i, j;
    int a = 52;
    if (playernumber == 3) {
        a = 34;
    }
    if (playernumber == 4) {
        a = 26;
    }
    
    for (i = 0; i < playernumber; i++) {
        //stdout and pipe to player
        if (fpin[i]) {
            fprintf(fpin[i], "newround %c%c", hand[i][0], hand[i][1]);
            printf("%c%c", hand[i][0], hand[i][1]);
            
            for (j = 2; j < a; j++) {
                fprintf(fpin[i], ", %c%c", hand[i][j], hand[i][j+1]);
                fflush(fpin[i]);
                printf(", %c%c", hand[i][j], hand[i][j+1]);
                j++;
            }
            fprintf(fpin[i], "\n");
            printf("\n");
        }
        
    }
}
