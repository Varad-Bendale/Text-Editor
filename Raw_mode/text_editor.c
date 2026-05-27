#include<stdio.h>
#include<unistd.h> 
#include<termios.h>
#include<ctype.h>
#include<stdlib.h>
#include<errno.h>
struct termios original  ; 

void disable_raw_mode(){
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, & original) == -1){
  	  die("tcsetattr");
	}
}

void die(const char *s) { 
   perror( s ) ; 
   exit(1) ; 
} 

void rawmode(){
    if (tcgetattr(STDIN_FILENO, &original) == -1){
       die("tcgetattr");
    } 
    struct termios temp = original ; 
    temp.c_iflag &= ~(IXON | ICRNL ) ; 
    temp.c_oflag &= ~(OPOST ) ; 
    temp.c_lflag &= ~(IEXTEN|ECHO|ICANON|ISIG ) ; 
    temp.c_cc[VMIN] = 0
    temp.c_cc[VTIME] = 1
    temp.c_oflag &= ~(OPOST ) ; 
    if (tcsetattr(STDIN_FILENO, &original) == -1){
       die("tcsetattr");
    } 
    if (atexit(disable_raw_mode) == -1){
       die("atexit");
    } 
}

int main(){
    rawmode() ; 
    while(1) {
    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN){
       die("read");
    } 
    if (c == 'q'){
       break ;
    } 
    if (iscntrl(c)){ 
       printf("%d\r\n" ,c) ; 
    }
    else { 
        printf("%d (%c) \r \n " , c , c ) ; 
    }
}
    return 0 ; 
} 

