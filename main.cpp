#include <unistd.h>
#include <csignal>
#include <termios.h>
#include <atomic>
#include <iostream>

using namespace std;
using termios_t = struct termios;

using Key = struct {
    int Character = 0;
    bool Alt = false;
};

class Keyboard {
    private:
    termios_t t_old, t_new;

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

    Key getCh() {
        Key k;
        char buf[1];
        read(0, &buf, 1);
        if (buf[0] == 27) {
            k.Alt = true;
            k.Character = getc(0);
        }
        else
            k.Character = buf[0];
        return k;
    }
};

atomic<bool> flag_interrupt(false);

void break_intercept(int sig) {
    flag_interrupt = true;
}

int main() {
    if (SIG_ERR == signal(SIGINT, break_intercept)) {
        cout << "Failed to register exit function\n";
        return 1; 
    }

    auto kbd = Keyboard();
    cout << "Awaiting input (Ctrl+C to quit)..." << endl;
    while (!flag_interrupt) {
        Key k = kbd.getCh();
        if (k.Alt && k.Character == EOF)
            break;
        cout << k.Character << " - " << (char)k.Character << endl;
    }

    return 0;
}