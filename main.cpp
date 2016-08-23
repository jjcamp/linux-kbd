#include <unistd.h>
#include <csignal>
#include <termios.h>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <poll.h>
#include <curses.h>

using namespace std;
using termios_t = struct termios;
using pollfd_t = struct pollfd;

using Key = struct {
    int Char = 0;
    int CChar = 0;
    bool Alt = false;
    bool Ctrl = false;
    bool Special = false;
};

class Keyboard {
    private:
    termios_t t_old, t_new;

    typedef struct readbuf_t {
        static const size_t MAX = 4;
        char buf[MAX];
        size_t size = 0;
    } readbuf;

    readbuf readBuf() const {
        readbuf buf;
        // Block on first character
        buf.size = read(0, &buf.buf, buf.MAX);
        if (buf.size < 1)
            throw "read";
        pollfd pfd = { 0, POLLIN };
        while (buf.size < buf.MAX && poll(&pfd, 1, 1))
            buf.size += read(0, &buf.buf[buf.size], buf.MAX - buf.size);

        return buf;
    }

    public:
    Keyboard() {
        if (tcgetattr(0, &t_old) != 0)
            throw "tcgetattr";
        t_new = t_old;
        // Set desired terminal settings
        t_new.c_iflag &= ~IGNBRK;
        t_new.c_lflag &= ~(ICANON | ECHO | ECHOE);
        t_new.c_cc[VMIN] = 1;
        t_new.c_cc[VTIME] = 0;

        if (tcsetattr(0, TCSANOW, &t_new) != 0)
            throw "tcsetattr";
    }

    ~Keyboard() {
        tcsetattr(0, TCSANOW, &t_old);
    }

    Key getCh() const {
        Key k;
        
        readbuf buf = this->readBuf();
        if (buf.size > 1 && buf.buf[0] == 27) {  // Handle multiple-character scancodes
            k.Special = true;
            // ALT
            if (buf.size == 2) {
                k.Char = buf.buf[1];
                k.Alt = true;
                k.CChar = k.Char;  // Shouldn't use this here anyway
            }
            else if (buf.size == 3 && (buf.buf[1] == 79 || buf.buf[1] == 91)) {
                k.Char = buf.buf[2];
                switch (k.Char) {
                    case 'A': k.CChar = KEY_UP; break;
                    case 'B': k.CChar = KEY_DOWN; break;
                    case 'C': k.CChar = KEY_RIGHT; break;
                    case 'D': k.CChar = KEY_LEFT; break;
                    case 70: k.CChar = KEY_END; break;
                    case 72: k.CChar = KEY_HOME; break;
                    default:
                        cout << "27 79|91 x - " << k.Char << endl;
                        k.CChar = k.Char;
                }
            }
            else if (buf.size == 4 && buf.buf[1] == 91 && buf.buf[3] == 126) {
                k.Char = buf.buf[2];
                switch (k.Char) {
                    case 51: k.CChar = KEY_DC; break;
                    case 53: k.CChar = KEY_PPAGE; break;
                    case 54: k.CChar = KEY_NPAGE; break;
                    default:
                        cout << "27 91 x 126 - " << (int)k.Char << endl;
                        k.CChar = k.Char;
                }
            }
            else {
                for (int i = 0; i < buf.size; i++)
                    cout << (short)buf.buf[i] << " ";
                cout << endl;
            }
            return k;
        }
        else if (buf.size > 1)  // how did this happen?
            throw "Keyboard.getCh buffer size";
        else {
            k.Char = buf.buf[0];
            k.CChar = k.Char;

            // Translate certain keys for curses
            switch (k.Char) {
                case 10: k.CChar = KEY_ENTER; break;
                case 127: k.CChar = KEY_BACKSPACE; break;
            }
            if (k.CChar < 27 && k.CChar > 0) { // Remaining Ctrl keys
                k.Ctrl = true;
                k.Char += 96; // Translate to actual letter
            }
        }

        return k;
    }
};

atomic<bool> flag_interrupt(false);

void break_intercept(int sig) {
    flag_interrupt = true;
}

int main() {
    if (SIG_ERR == signal(SIGINT, break_intercept)) {
        cout << "Failed to register exit function" << endl;
        return 1; 
    }

    {
        auto kbd = Keyboard();
        cout << "Awaiting input (Ctrl+C or Esc to quit)..." << endl;
        while (!flag_interrupt) {
            Key k = kbd.getCh();
            if (k.Char == 27)
                break;
            cout << "0x" << hex << k.Char << "\t" 
                << dec << k.Char << "\t";
            if (k.Alt)
                cout << "Alt-";
            else if (k.Ctrl)
                cout << "Ctrl-";
            cout << (char)k.Char << "\t"
                 << keyname(k.CChar)
                 << endl;
        }
    }

    return 0;
}
