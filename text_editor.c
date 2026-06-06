#include<stdio.h>
#include<unistd.h> 
#include<termios.h>
#include<ctype.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/ioctl.h> 
#include<sys/types.h>
#include<time.h>
#include <stdarg.h>
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
#define ctrl(k) ((k) & 0x1f) 
#define version "0.0.1"
#define tab_spaces 8

typedef struct row_input{ 
	int size ; 
	char *data ; 
	char *render  ; 
	int render_size ; 
} row_input ; 

 
struct editor_global { 
	int cursor_rows ; 
	int cursor_cols ; 
	int rows  ; 
	int cols ; 
      int row_offset ; 
	int col_offset ; 
	row_input *ri ;
	int row_length ; 
	struct termios original  ; 
      int render_cols ; 
	char *filename ; 
	char status_messege[80] ; 
	time_t messege_time ; 
} ;



struct editor_global  edit ; 

struct dynamic_buffer{
 	char *data ; 
	int size ; 

} ;

int first = 0 ; 

enum settings_keys {
    Arrow_left = 1000 ,
    Arrow_right ,
    Arrow_up ,
    Arrow_down ,  
    Page_up , 
    Page_down , 
    Home_key , 
    End_key  , 
    delete 
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



void (row_input *row ) { 
	int tabs  = 0 ; 
	for ( int i = 0 ; i < row->size ; i++ ) { 
		if ( row->data[i] == '\t' ) { 
			tabs = tabs + 1 ; 
		} 
	} 
	free ( row->render) ; 
	row->render = malloc( row->size + ( tabs*( tab_spaces -1  )  + 1  ) ) ; 

	int k = 0 ; 
	for ( int i = 0 ; i < row->size ; i++ ) { 
		if ( row->data[i] != '\t') { 
			row->render[k] = row->data[i] ; 
			row->render_size += 1  ; 
		      k = k + 1 ; 
		} 
		else { 
		   row->render[k] = ' '  ;
			k = k + 1 ;   
			while(k % tab_spaces != 0  )  { 
				row->render[k] = ' '  ; 
				row->render_size += 1  ; 
		      	k = k + 1 ; 
			} 
	} 
    } 
	  k = k +1 ; 
	  row->render[k] = '\0' ; 
	row->render_size = k ; 
	} 



	       
void die(const char *s) { 
   write(STDOUT_FILENO , "\x1b[2J" , 4 ) ; 
   write(STDOUT_FILENO , "\x1b[H" , 3 ) ; 
   perror( s ) ; 
   exit(1) ; 
} 
 
void append_lines( char *line , size_t len) { 
    edit.ri = realloc( edit.ri , sizeof(row_input)*(edit.row_length+1) )  ;  
    edit.ri[edit.row_length].size = len ; 
    edit.ri[edit.row_length].data = malloc( len+1 ) ;  
    memcpy(edit.ri[edit.row_length].data , line , len) ; 
    edit.ri[edit.row_length].data[len] = '\0' ; 
    edit.ri[edit.row_length].render = NULL ; 
    edit.ri[edit.row_length].render_size = 0 ; 
    render_input(&edit.ri[edit.row_length]) ; 
    edit.row_length++   ; 
}  

void text_in_input_buffer(char *file){ 
    FILE *fp  = fopen(file , "r") ;
    edit.filename = strdup(file) ; 
    if (!fp) {									
      die("fopen") ; 
    }
    char *line  = NULL ; 
    size_t mem = 0  ; 
    ssize_t len  ; 
    while ( ( len =  getline(&line , &mem , fp  ) )  != -1  ) { 
    while( len > 0 && (  ( line[len-1] == '\n' ) ||  ( line[len-1] == '\r' ) ) )  {
	    len-- ; 	
	} 
    append_lines( line , len )  ; 
   } 
   free( line) ; 
   fclose( fp ) ; 
   first = 1 ; 

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

    first = 1 ; 

     if ( c == '\x1b') { 
		char buf[3] ; 
	   if ( read(STDIN_FILENO , &buf[0] , 1) != 1) { 
		return '\x1b' ; 
	      } 
	   if ( read(STDIN_FILENO , &buf[1] , 1) != 1) { 
		return '\x1b' ; 
	     } 
	   if ( buf[0] == '[' ) { 
	       if ( buf[1] >= '0' && buf[1] <= '9' ) { 
		   if ( read(STDIN_FILENO , &buf[2] , 1) != 1) { 
		      return '\x1b' ; 
	           } 
		   if ( buf[2] == '~') { 
 		      switch( buf[1] ) { 
			case '1' : return Home_key;
           	       case '4' : return End_key;
			case '5' : return Page_up ; 
			case '6' : return Page_down ; 	
			case '7' : return Home_key;
           	       case '8' : return End_key;
			case '3' : return delete  ; 
                   } 
		   } 
                  }
		else { 
 	          switch ( buf[1] ) {
	        	case 'A' : return  Arrow_up ; 
	        	case 'B' : return Arrow_down ; 
		       case 'C' : return  Arrow_right ; 
		       case 'D' : return Arrow_left ; 
			case 'H' : return Home_key;
           	       case 'F' : return End_key;
		    }
		}
		if ( buf[0] == 'O') { 
		  switch ( buf[1] ) {
			case 'H' : return Home_key;
           	       case 'F' : return End_key;

                 } 
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
			  if ( edit.cursor_cols >  edit.ri[edit.cursor_rows -1  ].size ) { 
			     edit.cursor_cols = edit.ri[edit.cursor_rows - 1  ].size ; 			
			} 
		   edit.cursor_rows -= 1  ; 
		} 
		   break ; 

	  case Arrow_down : 
		
		if (edit.cursor_rows < edit.row_length) {
		  if ( edit.cursor_rows < edit.row_length -1 ) { 
			if ( edit.cursor_cols >  edit.ri[edit.cursor_rows + 1  ].size ) { 
			   edit.cursor_cols = edit.ri[edit.cursor_rows + 1 ].size ; 
			} 
			} 
		  edit.cursor_rows += 1 ; 
		} 
		   break ; 



	 case Arrow_left : 
		if ( edit.cursor_cols == 0  ) { 
			if ( edit.cursor_rows != 0 ) { 
			    edit.cursor_rows -= 1 ; 
			    edit.cursor_cols = edit.ri[edit.cursor_rows].size  ; 
			 } 
		   break ; 
		} 
		   edit.cursor_cols -= 1  ;  
		   break ; 


	case Arrow_right : 
		if ( edit.cursor_cols < edit.ri[edit.cursor_rows].size ) { 
			edit.cursor_cols += 1 ; 
		} 
		else { 
		if ( edit.cursor_rows < edit.row_length  -1 ) { 
		      edit.cursor_rows += 1 ; 
			edit.cursor_cols = 0 ; 
			break ; 
			} 
		} 
		
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

	  case Page_up : 
	  case Page_down :
             if ( word == Page_up ) { 
			if ( edit.row_offset - edit.rows > 0 ) { 
				edit.cursor_rows = edit.row_offset - edit.rows  ; 
			   } 
			else { 
				edit.cursor_rows = 0 ; 
			}
		   } 
		  if (word == Page_down ) { 
			if ( edit.row_offset + edit.rows < edit.row_length ){ 
				edit.cursor_rows = edit.row_offset + edit.rows ; 
		   } 
			else { 
			   edit.cursor_rows = edit.row_length - edit.rows ; 
		 } 
	    	} 
		 
 		int times  = edit.rows ; 
		while(times > 0 ) { 
		  if ( word == Arrow_up) { 
			cursor_change(Arrow_up) ; 
			times-- ; 
		}
		if ( word == Arrow_down) { 
			cursor_change(Arrow_down) ; 
			times-- ; 
		}
	  case End_key : 
		if ( edit.cursor_rows < edit.row_offset + edit.rows ) { 
			edit.cursor_cols = edit.ri[edit.cursor_rows].size ; 
			break ; 
		  } 

	  case Home_key : 
		edit.cols = 0  ; 


	  case Arrow_up : 
	  case Arrow_down : 
	  case Arrow_left : 
	  case Arrow_right : 
		 cursor_change(word) ; 
		break ; 
	} 

} 
}



int position_as_per_cursor( row_input *row ){ 
	int num  = 0 ; 
	 for ( int i = 0 ; i < edit.cursor_cols ; i++ ) { 
		if ( row->data[i] == '\t' ) { 
			num = num + (tab_spaces  -1 ) - ( num % tab_spaces ); 
		} 
			num = num + 1 ; 	
	} 
return num ; 
} 




void scroll_offset(){
	edit.render_cols = 0 ; 
            
	if ( edit.cursor_rows < edit.rows ) { 
      edit.render_cols = position_as_per_cursor( &edit.ri[edit.cursor_rows] )  ; 
	} 
		
     if ( edit.cursor_rows <  edit.row_offset) { 
            edit.row_offset  = edit.cursor_rows ; 
      }
      if ( edit.cursor_rows >= edit.row_offset + edit.rows ) { 
		edit.row_offset = edit.cursor_rows - edit.rows + 1 ; 
       } 
      if ( edit.render_cols < edit.col_offset) { 
		edit.col_offset = edit.render_cols ; 
	} 
	if ( edit.render_cols >= edit.cols + edit.col_offset ) { 
		edit.col_offset = edit.render_cols - edit.cols + 1  ; 
	} 
} 

void status_msg_input (const char* msg , ...  ){ 
    va_list temp   ; 
	va_start(temp  , msg) ; 
	vsnprintf( edit.status_messege , sizeof(edit.status_messege ) , msg , temp) ; 
	va_end(temp) ; 
    edit.messege_time = time(NULL) ; 
}


void status_msg(struct dynamic_buffer *temp ) { 
	dynamic_buffer_append(temp, "\x1b[7m" , 4) ; 
	int msglen = 0 ;
	msglen = strlen(edit.status_messege) ; 
	if ( msglen > edit.cols){
		msglen = edit.cols ; 
	}
	int k = 0 ; 
	if (msglen > 0 &&  time(NULL) - edit.messege_time  < 5 ){ 
		dynamic_buffer_append(temp, edit.status_messege , msglen) ; 
		k = 1 ; 
	} 
	int tes = 0  ; 
	if ( k == 1 ) tes = msglen ; 
	for (int i = tes ; i < edit.cols ; i++ ){ 
		dynamic_buffer_append(temp, " " , 1) ; 
	}
	dynamic_buffer_append(temp, "\x1b[m" , 3) ; 
}



void status_line(struct dynamic_buffer *temp ) { 
	dynamic_buffer_append(temp, "\x1b[7m" , 4) ; 
	int len = 0 ; 
	 char file[80] ; 
	char position[80] ; 
	if ( edit.filename == NULL ) { 
	  edit.filename = "NAME NOT DETECTED"  ;   
	} 
	int pos_len = 0 ; 
	len = snprintf( file , sizeof(file)  , "%s - %d - %d " , edit.filename , edit.cursor_rows , edit.cursor_cols )  ; 
	pos_len = snprintf( position , sizeof(position) , "%d-%d" , edit.cursor_rows + 1  , edit.row_length ) ; 
      if(len > edit.cols) { 
		len = edit.cols ; 
	} 
	if ( len <= edit.cols ) { 
	dynamic_buffer_append(temp, file , len) ; 
	} 
	for (int i = len ; i < edit.cols - pos_len  ; i++) { 	
			dynamic_buffer_append(temp ," "  ,  1 ) ; 
		}  
	dynamic_buffer_append(temp, position , pos_len) ; 

	

	dynamic_buffer_append(temp, "\x1b[m" , 3) ; 		

} 



void txt_print(struct dynamic_buffer *temp  ){ 
	char welcome[100] ; 
      for(int i = 0 ; i < edit.rows ; i++){
		 int correct_row = i + edit.row_offset ; 
         if ( correct_row >= edit.row_length) {
		if ( first == 0 ) { 
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
		}
		else {  
              dynamic_buffer_append(temp, "~" , 1) ; 
			} 
		} 
          else { 
               int temp_len  = edit.ri[correct_row].render_size - edit.col_offset ; 
			if (temp_len < 0 ) { 
				temp_len = 0 ; 
			} 
                 if (temp_len > edit.cols) { 
				temp_len = edit.cols ; 
			} 
             	dynamic_buffer_append(temp, &edit.ri[correct_row].render[edit.col_offset] , temp_len ) ; 

          }
               dynamic_buffer_append(temp, "\x1b[K" , 3) ; 
              dynamic_buffer_append(temp, "\r\n", 2) ; 
}
}


void insert_char( row_input *line ,  int pos ,  int c ){ 
	if ( pos < 0 || pos > line->size ){ 
		pos  = line->size ; 
	}
	line->data = realloc( line->data , line->size + 2 ) ; 
    memmove( &line->data[pos+ 1 ] , &line->data[pos] ,  line->size - pos + 2 ) ; 
	line->size++ ; 
	line->data[pos] = c ; 
	render_input(line)  ; 
}

void screen_ready(){
        scroll_offset() ; 
        struct  dynamic_buffer temp = dynamic_buffer_starter ; 
	dynamic_buffer_append(&temp, "\x1b[?25l" , 6 ) ; 
	dynamic_buffer_append(&temp, "\x1b[H" , 3) ; 
	txt_print( &temp ) ; 
	status_line(&temp) ; 
	status_msg(&temp) ; 
        char buf[32] ; 
	snprintf(buf , sizeof(buf) , "\x1b[%d;%dH" , (edit.cursor_rows - edit.row_offset )+ 1  ,(edit.render_cols - edit.col_offset )+ 1  ) ; 
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
	edit.row_length = 0 ; 
      edit.ri = NULL ;  
      edit.row_offset = 0 ; 
	edit.col_offset = 0 ; 
      if(get_window_size(&edit.rows , &edit.cols) == -1 ) { 
		die("get_window_size") ; 
	} 
	edit.status_messege[0] = '\0' ; 
	edit.messege_time = 0 ; 
	edit.rows  = edit.rows - 2  ; 
	edit.filename = NULL ; 
}


int main(int argc , char *argv[] ){
    rawmode() ; 
    starter() ;
    if (argc >= 2 ){ 
	 text_in_input_buffer(argv[1]) ; 
	} 
	status_msg_input( "ctrl + q for quitting ") ; 
    while(1) {
     screen_ready() ; 
    process_raw_key_press() ; 
    }
    return 0 ; 
} 

