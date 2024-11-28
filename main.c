// #include <stdio.h>
// #include <unistd.h>

// int main() {
//     int i = 0;
//     pid_t pid = fork();

//     if (pid == 0) { // Child process
//         i++;
//         printf("Child process: i = %d\n", i);
//     } else { // Parent process
//         i--;
//         printf("Parent process: i = %d\n", i);
//     }

//     return 0;
// }

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid1, pid2, pid3;  // Variables to store process IDs
    int i;

    // First fork: creates the first child process
    pid1 = fork();

    if (pid1 == 0) {
        // First child process
        for (i = 0; i < 100; i++) {
            printf("A");
        }
        printf("\nFirst child process (A) done.\n");
    } else {
        // Second fork: creates the second child process
        pid2 = fork();

        if (pid2 == 0) {
            // Second child process
            for (i = 0; i < 100; i++) {
                printf("B");
            }
            printf("\nSecond child process (B) done.\n");
        } else {
            // Third fork: creates the third child process
            pid3 = fork();

            if (pid3 == 0) {
                // Third child process
                for (i = 0; i < 100; i++) {
                    printf("C");
                }
                printf("\nThird child process (C) done.\n");
            } else {
                // Parent process prints the process IDs of each child
                printf("Parent process: First child PID = %d\n", pid1);
                printf("Parent process: Second child PID = %d\n", pid2);
                printf("Parent process: Third child PID = %d\n", pid3);

                // Parent process may perform other tasks or wait for children if needed
            }
        }
    }

    return 0;
}






// //launch.json
// {
//     "version": "0.2.0",
//     "configurations": [
//         {
//             "name": "Debug Program",
//             "type": "cppdbg",
//             "request": "launch",
//             "program": "${fileDirname}/${fileBasenameNoExtension}",
//             "args": [],
//             "stopAtEntry": false,
//             "cwd": "${workspaceFolder}",
//             "environment": [],
//             "externalConsole": true,
//             "MIMode": "gdb",
//             "setupCommands": [
//                 {
//                     "description": "Enable pretty-printing for gdb",
//                     "text": "-enable-pretty-printing",
//                     "ignoreFailures": true
//                 }
//             ],
//             "preLaunchTask": "Build active file with g++",
//             "miDebuggerPath": "/usr/bin/gdb"
//         }
//     ]
// }
// //tasks.json
// {
//     "version": "2.0.0",
//     "tasks": [
//         {
//             "type": "cppbuild",
//             "label": "Build active file with g++",
//             "command": "/usr/bin/g++",
//             "args": [
//                 "-fdiagnostics-color=always",
//                 "-g",
//                 "${file}",
//                 "-o",
//                 "${fileDirname}/${fileBasenameNoExtension}"
//             ],
//             "options": {
//                 "cwd": "${fileDirname}"
//             },
//             "problemMatcher": [
//                 "$gcc"
//             ],
//             "group": {
//                 "kind": "build",
//                 "isDefault": true
//             },
//             "detail": "Build task for C and C++ files with g++"
//         }
//     ]
// }
