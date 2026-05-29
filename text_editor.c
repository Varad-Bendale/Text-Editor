#include<stdio.h>
#include<unistd.h> 
#include<termios.h>
#include<ctype.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/ioctl.h> 
#define ctrl(k) ((k) & 0x1f) 


struct editor_global { 
	int rows  ; 
	int cols ; 
	struct termios original  ; 
} ;

struct editor_global  edit ; 

void die(const char *s) { 
   write(STDOUT_FILENO , "\x1b[2J" , 4 ) ; 
   write(STDOUT_FILENO , "\x1b[H" , 3 ) ; 
   perror( s ) ; 
   exit(1) ; 
} 

void disable_raw_mode(){
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &edit.original) == -1){
  	  die("tcsetattr");
	}
}

  

void rawmode(){
    if (tcgetattr(STDIN_FILENO, &edit.original) == -1){
       die("tcgetattr");
    } 
    struct termios temp = edit.original ; 
    temp.c_iflag &= ~(IXON | ICRNL ) ; 
    temp.c_oflag &= ~(OPOST ) ; 
    temp.c_lflag &= ~(IEXTEN|ECHO|ICANON|ISIG ) ; 
    temp.c_cc[VMIN] = 0 ; 
    temp.c_cc[VTIME] = 1 ; 
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH , &temp) == -1){
       die("tcsetattr");
    } 
    if (atexit(disable_raw_mode) != 0 ){
       die("atexit");
    } 
}

char raw_key_press(){
     int temp ; 
	char c = '\0' ; 
     while((temp = read(STDIN_FILENO, &c, 1) != 1 ))   {
	if (temp == -1 && errno != EAGAIN){
       die("read");
    } 
    }
    return c  ; 
}

void process_raw_key_press(){
	char word  = raw_key_press() ; 
	if (word == ctrl('q')){
          write(STDOUT_FILENO , "\x1b[2J" , 4 ) ; 
	   write(STDOUT_FILENO , "\x1b[H" , 3 ) ; 
	   exit(0)  ; 
        return ; 
     } 

} 

void tlides(){ 
      for(int i = 0 ; i < edit.cols ; i++){
         if(i == edit.cols-1 ){ 
              write(STDOUT_FILENO , "~" , 1) ; 
     	   } 
         if( i<edit.cols -1) {
   	      write(STDOUT_FILENO , "~\r\n" , 2) ; 
         }
}
}

void screen_ready(){
	write(STDOUT_FILENO , "\x1b[2J" , 4 ) ; 
	write(STDOUT_FILENO , "\x1b[H" , 3 ) ; 
	tlides() ; 
	write(STDOUT_FILENO , "\x1b[H" , 3 ) ; 
} 


int get_window_size(int *rows , int *columns){ 
	struct winsize ws ; 
	if(1 || ioctl(STDOUT_FILENO , TIOCGWINSZ , & ws ) == -1 || ws.ws_col  == 0  || ws.ws_row == 0 ) {
		if (write(STDOUT_FILENO , "\x1b[999C\x1b[999B" , 12) != 12){ 
			return -1 ; 
		} 
   		raw_key_press() ; 
		return -1 ; 
	}
	else { 
		*columns = ws.ws_col ; 
		*rows = ws.ws_row ; 
       	return 0 ; 
	}
	
} 

void window_size(){
      if(get_window_size(&edit.rows , &edit.cols) == -1 ) { 
		die("get_window_size") ; 
	} 
}
     
int main(){
    rawmode() ; 
    window_size() ;
    screen_ready() ; 
    while(1) {
 
    process_raw_key_press() ; 
    }
    return 0 ; 
} 

