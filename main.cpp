#include <iostream>
#include <thread>
#include <vector>
#include <wait.h>
#include <unistd.h>
#include <bits/sigaction.h>
#include <signal.h>
#include <termios.h>
#include <list>
#include <atomic>
#include <pthread.h>

class BufferToggle
{
private:
    struct termios t;
public:
    void off()
    {
        tcgetattr(STDIN_FILENO, &t); //get the current terminal I/O structure
        t.c_lflag &= ~ICANON; //Manipulate the flag bits to do what you want it to do
        tcsetattr(STDIN_FILENO, TCSANOW, &t); //Apply the new settings
    }
    void on()
    {
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag |= ICANON;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }
};

pthread_mutex_t io_sync;
bool to_exit;
std::atomic<int> left_to_exit;

void slave(void* args);
void log(const std::string& message = std::string{});
void output_by_char(const std::string& str);

int main() {
    BufferToggle bt;
    bt.off();
    log("started");
    std::list<pthread_t> threads;
    pthread_mutex_init(&io_sync, NULL);

    char val;

    while (val != 'q')
    {
        val = std::getchar();
        switch(val)
        {
            case '+':
                pthread_t buf;
                pthread_create(&buf, NULL, reinterpret_cast<void *(*)(void *)>(&slave), NULL);
                threads.push_back(buf);
                break;
            case '-':
                left_to_exit = 1;
                to_exit = true;

                bool joined = false;
                while(!joined)
                {
                    for (auto i = threads.begin(); i != threads.end(); i++)
                    {
                        if (pthread_tryjoin_np(*i, NULL) == 0)
                        {
                            joined = true;
                            i = threads.erase(i);
                            break;
                        }
                    }
                }
                to_exit = false;
                break;
        }
    }


    left_to_exit = threads.size();
    to_exit = true;


    pthread_mutex_destroy(&io_sync);
    bt.on();
    return 0;
}

void slave(void* args) {
    while (true) {
        if (to_exit)
        {
            int buf = left_to_exit--;
            if (buf > 0)
            {
                pthread_exit(NULL);
            }
        }
        pthread_mutex_lock(&io_sync);
        log("message");
        pthread_mutex_unlock(&io_sync);
    }
}

void log(const std::string& message) {
    static __pid_t main_id = gettid();
    if (gettid() == main_id) {
        output_by_char("main process: ");
    }
    else {
        output_by_char("process " + std::to_string(gettid()) + ": ");
    }
    output_by_char(message);
    if (message.size())
        output_by_char("\n");
}

void output_by_char(const std::string& str) {
    for (auto i : str) {
        std::cout << i;
    }
}