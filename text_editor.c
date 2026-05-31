#include<stdio.h>
#include<unistd.h> 
#include<termios.h>
#include<ctype.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/ioctl.h> 
#define ctrl(k) ((k) & 0x1f) 
#define version "0.0.1"

struct editor_global { 
	int cursor_rows ; 
	int cursor_cols ; 
	int rows  ; 
	int cols ; 
	struct termios original  ; 
} ;

struct editor_global  edit ; 

struct dynamic_buffer{
 	char *data ; 
	int size ; 
} ;

enum settings_keys {
    Arrow_left = 1000 ,
    Arrow_right ,
    Arrow_up ,
    Arrow_down 
} ;


#define dynamic_buffer_starter { NULL , 0 } 

void dynamic_buffer_append( struct dynamic_buffer *temp  , const char *s , int len  ){ 
    char *need = realloc(temp->data , temp->size+len) ;
     if (need == NULL) {
         return ;  
     } 
     memcpy( &need[temp->size] , s , len ) ; 
     temp->data = need ; 
     temp->size += len ; 
} 

void free_dynamic_buffer ( struct dynamic_buffer *temp){
      free(temp->data)  ; 
}

 
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

int raw_key_press(){
     int temp ; 
	char c = '\0' ; 

     while((temp = read(STDIN_FILENO, &c, 1)) != 1 )   {
	if (temp == -1 && errno != EAGAIN){
       die("read");
    } 
    }

    if ( c == '\x1b') { 
		char buf[3] ; 
	   if ( read(STDIN_FILENO , &buf[0] , 1) != 1) { 
		return '\x1b' ; 
	      } 
	   if ( read(STDIN_FILENO , &buf[1] , 1) != 1) { 
		return '\x1b' ; 
	     } 
	if ( buf[0] == '[' ) { 
 	  switch ( buf[1] ) {
		case 'A' : return  Arrow_up ; 
		case 'B' : return Arrow_down ; 
		case 'C' : return  Arrow_right ; 
		case 'D' : return Arrow_left ; 
		}
       } 
	return '\x1b'  ; 
    }
    else { 
	return c ; 
    } 
    

}





void cursor_change(int c ){ 
	switch (c)  {
	  case Arrow_up : 
		if( edit.cursor_rows  != 0 ) { 
		   edit.cursor_rows -= 1  ; 
		} 
		   break ; 
	  case Arrow_down : 
		if (edit.cursor_rows != edit.rows - 1) {
		  edit.cursor_rows += 1 ; 
		} 
		   break ; 
	 case Arrow_left : 
		if( edit.cursor_cols  != 0 ) { 
		   edit.cursor_cols -= 1  ;  
		} 
		   break ; 
	case Arrow_right : 
		if (edit.cursor_cols != edit.cols - 1) {
		  edit.cursor_cols += 1  ; 
		}		
		   break ; 
} 
} 


void process_raw_key_press(){
	int word  = raw_key_press() ; 
	switch (word) {
	  case ctrl('q') : 
 		write(STDOUT_FILENO , "\x1b[2J" , 4 ) ; 
	       write(STDOUT_FILENO , "\x1b[H" , 3 ) ; 
	       exit(0)  ; 
              break ; 
	  case Arrow_up : 
	  case Arrow_down : 
	  case Arrow_left : 
	  case Arrow_right : 
		 cursor_change(word) ; 
		break ; 
	} 

} 



void tlides(struct dynamic_buffer *temp  ){ 
	char welcome[100] ; 
      for(int i = 0 ; i < edit.rows ; i++){
		if ( i == edit.rows / 2 ) {
            int  welcome_len = snprintf(welcome , sizeof(welcome) , "Type anything PalX Text editor %s", version ) ; 
		if (welcome_len > edit.cols){
			welcome_len = edit.cols ; 
		}
		int extra =  (edit.cols - welcome_len ) /2 ; 
		if (extra) { 
			 dynamic_buffer_append(temp, "~" , 1) ; 
			extra-- ; 
		} 
		while(extra > 0 ) { 
		   dynamic_buffer_append(temp, " " , 1 ) ; 
		   extra-- ; 
		} 
		dynamic_buffer_append(temp, welcome  , welcome_len ) ; 
		}
		else { 
              dynamic_buffer_append(temp, "~" , 1) ; 
		} 
              dynamic_buffer_append(temp, "\x1b[K" , 3) ; 
         if( i<edit.rows-1) {
              dynamic_buffer_append(temp, "\r\n", 2) ; 
         }
}
}


void screen_ready(){
        struct  dynamic_buffer temp = dynamic_buffer_starter ; 
	dynamic_buffer_append(&temp, "\x1b[?25l" , 6 ) ; 
	dynamic_buffer_append(&temp, "\x1b[H" , 3) ; 
	tlides( &temp ) ; 
        char buf[32] ; 
	snprintf(buf , sizeof(buf) , "\x1b[%d;%dH" , edit.cursor_rows + 1  , edit.cursor_cols + 1 ) ; 
       	dynamic_buffer_append(&temp, buf , strlen(buf) ) ; 
	dynamic_buffer_append(&temp, "\x1b[?25h" , 6 ) ; 
        write(STDOUT_FILENO , temp.data , temp.size)  ; 
	free_dynamic_buffer(&temp) ; 
} 


int cursor_position(int *rows, int *cols) {
     char  temp[32];
     unsigned int i  =   0;
     if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4){
        return -1;
     } 

     while (i < sizeof(temp) - 1) {
         if (read(STDIN_FILENO, &temp[i], 1) != 1) break;
   	 if (temp[i] == 'R') break;
   	 i++;
     }

     temp[i] = '\0';
     if (temp[0] != '\x1b' || temp[1] != '[' ) {
        return -1 ; 
      } 
     if(sscanf(&temp[2]  , "%d;%d" , rows , cols ) !=2 ) {
          return -1 ; 
      } 
     return 0;
}
	

int get_window_size(int *rows , int *columns){ 
	struct winsize ws ; 
	if(ioctl(STDOUT_FILENO , TIOCGWINSZ , & ws ) == -1 || ws.ws_col  == 0  || ws.ws_row == 0 ) {
		if (write(STDOUT_FILENO , "\x1b[999C\x1b[999B" , 12) != 12){ 
			return -1 ; 
		} 
   		return  cursor_position (rows , columns ) ; 

	}
	else { 
		*columns = ws.ws_col ; 
		*rows = ws.ws_row ; 
       	return 0 ; 
	}
	
} 

void starter(){
      edit.cursor_rows = 0 ; 
      edit.cursor_cols = 0 ; 
      if(get_window_size(&edit.rows , &edit.cols) == -1 ) { 
		die("get_window_size") ; 
	} 
}


int main(){
    rawmode() ; 
    starter() ;
 

    while(1) {
     screen_ready() ; 
    process_raw_key_press() ; 
    }
    return 0 ; 
} 

