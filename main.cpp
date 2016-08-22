#include <unistd.h>
#include <csignal>
#include <termios.h>
#include <atomic>
#include <iostream>
#include <iomanip>

using namespace std;
using termios_t = struct termios;
using timeval_t = struct timeval;

using Key = struct {
    int Character = 0;
    bool Alt = false;
    bool Ctrl = false;
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
        k.Character = buf[0];

        if (k.Character < 27 && k.Character > 0) {
            k.Ctrl = true;
            k.Character += 96; // Convert to actual
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
        cout << "Failed to register exit function\n";
        return 1; 
    }

    auto kbd = Keyboard();
    cout << "Awaiting input (Ctrl+C to quit)..." << endl;
    while (!flag_interrupt) {
        Key k = kbd.getCh();
        if (k.Alt && k.Character == 0)
            break;
        if (k.Character == 27)
            cout << "0x1b\t27\t[Alt] or [Esc]" << endl;
        else {
            cout << "0x" << hex << k.Character << "\t" 
                 << dec << k.Character << "\t";
            if (k.Ctrl)
                cout << "Ctrl-" << (char)k.Character;
            else
                cout << (char)k.Character;
            cout << endl;
        }
    }

    return 0;
}