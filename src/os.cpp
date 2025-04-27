#include "os.hpp"

std::string OS::detect_os()
{
    std::string result;
    #if defined(OS_WINDOWS)
        result = "Windows";
    #elif defined(OS_MACOS)
        result = "MacOS";
    #elif defined(OS_LINUX)
        result = "Linux";
    #elif defined(OS_UNIX)
        result = "Unix";
    #elif defined(OS_FREEBSD)
        result = "FreeBSD";
    #else
        result = "Unknown";
    #endif
    return result;
}

#if OS_UNIX_LIKE_DEFINED
std::pair<int, std::string> OS::run_command_unix(const std::vector<std::string>& args)
{
    // Convert std::vector<std::string> to an array of char* required by execvp
    std::vector<char*> cargs;
    for (const std::string& arg : args) {
        cargs.push_back(strdup(arg.c_str()));
    }
    cargs.push_back(nullptr);  // execvp expects a null-terminated array
    int pipefd[2];  // Pipe for capturing output
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return {-1, ""};
    }
    pid_t pid = fork();  // Fork the current process
    if (pid == -1) {
        // Error occurred during fork
        perror("fork failed");
        return {-1, ""};
    }
    else if (pid == 0) {
        // Child process: redirect stdout to the pipe
        close(pipefd[0]);  // Close unused read end
        dup2(pipefd[1], STDOUT_FILENO);  // Redirect stdout to pipe write end
        dup2(pipefd[1], STDERR_FILENO);  // Optionally redirect stderr to pipe write end
        close(pipefd[1]);  // Close original write end after duplication

		std::string full_command;
		for (const auto& arg : args) {
        if (!full_command.empty()) full_command += " ";
        full_command += "'" + arg + "'";  // WRAP EACH ARG IN SINGLE QUOTES
    }
		const char* bash_args[] = {"/bin/bash", "-c", full_command.c_str(), nullptr};
		execvp(bash_args[0], const_cast<char* const*>(bash_args));

        // execvp only returns if an error occurred
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
    else {
        // Parent process
        close(pipefd[1]);  // Close unused write end
        // Read output from the pipe
        std::string output;
        char buffer[128];
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        close(pipefd[0]);  // Close read end when done
        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);  // Wait for the child process to terminate
        // Free allocated memory for cargs
        for (char* carg : cargs) {
            free(carg);
        }
        // Return child's exit status and captured output
        if (WIFEXITED(status)) {
            return {WEXITSTATUS(status), output};  // Return exit code and output
        }
        else {
            return {-1, output};  // Child did not exit normally, return -1 with captured output
        }
    }
}
#elif OS_WINDOWS_DEFINED || !OS_UNIX_LIKE_DEFINED
std::pair<int, std::string> OS::run_command_windows(const std::string& command)
{
    PROCESS_INFORMATION processInfo = {};
    STARTUPINFO startupInfo = { sizeof(STARTUPINFO) };
    SECURITY_ATTRIBUTES securityAttrs = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead = NULL, hWrite = NULL;

    // Create pipe for capturing output
    if (!CreatePipe(&hRead, &hWrite, &securityAttrs, 0)) {
        return {-1, "Failed to create pipe"};
    }

    startupInfo.hStdOutput = hWrite;
    startupInfo.hStdError = hWrite;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Use cmd.exe to execute the command
    std::string fullCommand = "cmd.exe /C " + command;

    // Create a modifiable command string
    char commandBuffer[2048];
    strncpy(commandBuffer, fullCommand.c_str(), sizeof(commandBuffer) - 1);
    commandBuffer[sizeof(commandBuffer) - 1] = '\0';

    // Execute command
    if (!CreateProcess(
            NULL,               // Application name
            commandBuffer,      // Command line
            NULL,               // Process security attributes
            NULL,               // Thread security attributes
            TRUE,               // Inherit handles
            0,                  // Creation flags
            NULL,               // Environment
            NULL,               // Current directory
            &startupInfo,       // Startup info
            &processInfo))      // Process info
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        DWORD error = GetLastError();
        return {-1, "Failed to create process. Error code: " + std::to_string(error)};
    }

    // Close the write end of the pipe in the parent process
    CloseHandle(hWrite);

    // Wait for the process to complete and read the output
    WaitForSingleObject(processInfo.hProcess, INFINITE);

    char buffer[128];
    std::string output;
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate
        output += buffer;
    }
    CloseHandle(hRead);

    // Get the exit code of the process
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
        exitCode = -1; // Error retrieving exit code
    }

    // Clean up handles
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return {static_cast<int>(exitCode), output};
}
#endif

std::pair<int, std::string> OS::run_command(std::string& arg)
{
    std::pair<int, std::string> result;
    std::vector<std::string> args = split_preserve_quotes(arg, ' ');
    #if defined(OS_UNIX_LIKE)
        result = OS::run_command_unix(args);
    #else
        result = OS::run_command_windows(arg);
    #endif
    return result;
}

// Overload: run_command(vector<string>)
std::pair<int, std::string> OS::run_command(const std::vector<std::string>& args)
{
    #if defined(OS_UNIX_LIKE)
        return OS::run_command_unix(args);
    #else
        return OS::run_command_windows(args);  // You can stub this if needed
    #endif
}

std::vector<std::string> split(std::string str, char delim)
{
    std::vector<std::string> result;
    std::string temp = "";
    for (char c : str) {
        if (c == delim) {
            result.emplace_back(temp);
            temp = "";
            continue;
        }
        temp += c;
    }
    if (temp.size() > 0) result.emplace_back(temp);
    return result;
}

std::vector<std::string> split_preserve_quotes(std::string str, char delim)
{
    std::vector<std::string> result;
    std::string temp;
    bool inside_quotes = false;

    for (char c : str) {
        if (c == '\"') {
            inside_quotes = !inside_quotes;  // Toggle quote state
        }
        else if (c == delim && !inside_quotes) {
            result.emplace_back(temp);
            temp = "";
            continue;
        }
        temp += c;
    }
    if (!temp.empty()) result.emplace_back(temp);
    return result;
}

