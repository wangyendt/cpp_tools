#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <vector>
#include <signal.h>


class ADBLogcatReader {
public:
    ADBLogcatReader() = default;
    ~ADBLogcatReader() {
        if (child_pid > 0) {
            // Terminate the child process if it's running
            kill(child_pid, SIGTERM);
            waitpid(child_pid, nullptr, 0);
        }
    }

    void clearLogcat() {
        system("adb logcat -c");
    }

    bool startLogcat() {
        if (pipe(fd) == -1) {
            std::cerr << "Pipe failed" << std::endl;
            return false;
        }

        child_pid = fork();

        if (child_pid == -1) {
            std::cerr << "Fork failed" << std::endl;
            return false;
        } else if (child_pid == 0) {
            // Child process
            close(fd[0]); // Close the unused read end
            dup2(fd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
            close(fd[1]);

            execlp("adb", "adb", "logcat", (char *)NULL);
            // If execlp returns, it means it failed
            std::cerr << "execlp failed" << std::endl;
            _exit(1);
        } else {
            // Parent process
            close(fd[1]); // Close the unused write end
            return true;
        }
    }

    std::string readLine() {
        char buffer[1024];
        ssize_t bytes_read = read(fd[0], buffer, sizeof(buffer) - 1);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            return std::string(buffer);
        } else if (bytes_read == -1) {
            std::cerr << "Read failed" << std::endl;
        }

        return std::string();
    }

private:
    int fd[2];
    pid_t child_pid = -1;
};


int main() {
    ADBLogcatReader logcatReader;

    logcatReader.clearLogcat();

    if (!logcatReader.startLogcat()) {
        std::cerr << "Failed to start logcat" << std::endl;
        return 1;
    }

    while (true) {
        std::string line = logcatReader.readLine();
        if (!line.empty()) {
            std::cout << line << std::endl;
        } else {
            // Handle end of stream or error
            break;
        }
    }

    return 0;
}
