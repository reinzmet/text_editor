/*** Includes ***/
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** Data ***/ 

struct editorConfig {
	int screenrows;
	int screencols;
	struct termios orig_termios;
};

struct editorConfig E;

/*** Terminal ***/
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);	
	
	perror(s);
	exit(1); 
}


void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1 )
	die("tcsetattr");
}

void enableRawMode() { 											// This functions enables the raw mode and makes it so the text isn't echoed, like in su
	//struct termios raw;

	//tcgetattr(STDIN_FILENO, &raw);

	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL| INPCK | ISTRIP | IXON); 	// Disables Ctrl-S and Ctrl-Q functions (they are used for flow control), ICRNL disables ctrl-m
    raw.c_oflag &= ~(OPOST);	
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 			// c_lflag is for local flags, and a dumping ground for other states, ICANON allows us to turn off canonical mode and input is then taken byte-by-byte, ISIG turns off Ctrl-c and ctrl-z signals, IEXTEN for disabling Ctrl-V 
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;	
	
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) 
		die("tsetattr");

}

char editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;


	if (write(STDOUT_FILENO	, "\x1b[6n", 4) != 4) return -1; 				// we use the x1b n command to get the device status report and get the cursor postion, we give argument 6 to ask for the cursor position

	// printf("\r\n");
	// char c;
	// while (read(STDIN_FILENO, &c, 1) == 1) {
	// 	if (iscntrl(c)) {
	// 		printf("%d\r\n",c);
	// 	} else {
	// 		printf("%d ('%c')\r\n",c ,c);
	// 	}
	// }
	while (i < sizeof(buf) -1 ) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}	
	buf[i] = '\0';
	printf("\r\n&buf[1]: '%s' \r\n",&buf[1]);
	editorReadKey();

	return -1;
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;

	if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) { 		//TIOCGWINSZ stands for terminal input/output control get windwow sizek
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B",12) != 12) return -1;		// we are sending two escape sequences one after the other, C command moves the cursor to the right and the B command moves it down. We use 999 (large value) to make sure it reaches the end of the screen
		return getCursorPosition(rows, cols);	
		// editorRedKey();	
		// return -1;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}
/*** Output ***/
void editorDrawRows() {
	int y;
	for (y = 0; y < E.screenrows; ++y) {
		write(STDOUT_FILENO, "~\r\n",3);
	}
}
void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4);			// x1b(27) is the escape character followed by J which is the command for erase in display and 2 for arguemtn which clears the entire screen
	write(STDOUT_FILENO, "\x1b[H", 3);			//x1b[h repositions the cursor to the top left of the screen

	 editorDrawRows();

	write(STDOUT_FILENO, "\x1b[H",3);
}


/*** Input ***/

void editorProcessKeypress() {
	char c = editorReadKey();
	
	switch (c) {
		case CTRL_KEY('c'):
		write(STDOUT_FILENO, "\x1b[2J",4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(0);
		break;
	}
}

/*** init ***/
void initEditor() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1 ) 
		die("getWindowSize");
}

int main () {
	enableRawMode();	
	initEditor();	

/*	while (1) {
		char c = '\0';
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
				//while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
		if (iscntrl(c)) {				//iscntrl() comes tests whether the character is a control character. 
			printf("%d\r\n",c);
		}
		else {
			printf("%d ('%c')\r\n",c,c);
		}
		if (c == CTRL_KEY('q')) break;
	}	
*/
	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}	
	return 0; 
}