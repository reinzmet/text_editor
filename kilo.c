/*** Includes ***/
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** Data ***/ 
struct termios orig_termios;
this is a keyboard stroke

/*** Terminal ***/
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);	
	
	perror(s);
	exit(1); 
}


void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1 )
	die("tcsetattr");
}

void enableRawMode() { 											// This functions enables the raw mode and makes it so the text isn't echoed, like in su
	//struct termios raw;

	//tcgetattr(STDIN_FILENO, &raw);

	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
	atexit(disableRawMode);

	struct termios raw = orig_termios;
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

/*** Output ***/
void editorDrawRows() {
	int y;
	for (y = 0; y < 24; ++y) {
		write(STDOUT_FILENO, "~\r\n",3);
	}
}
void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

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

int main () {
	enableRawMode();	
	

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