// Include necessary libraries for input/output, process control, inter-process communication, and time
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

// Define permissions for the message queue
#define PERMS 0644

// Define a structure for the message buffer
struct my_msgbuf {
   long mtype; // Message type
   int mtext; // Message text (changed to int to hold an integer)
};

// Main function
int main(void) {
   struct my_msgbuf buf; // Message buffer
   int msqid; // Message queue ID
   key_t key; // Key to be passed to msgget()

   // Create a file named "msgq.txt" if it doesn't exist
   system("touch msgq.txt");

   // Generate a key for the message queue
   if ((key = ftok("msgq.txt", 'B')) == -1) {
      perror("ftok"); // Print a system error message
      exit(1); // Terminate the program
   }

   // Create the message queue
   if ((msqid = msgget(key, PERMS | IPC_CREAT)) == -1) {
      perror("msgget"); // Print a system error message
      exit(1); // Terminate the program
   }
   printf("message queue: ready to send messages.\n"); // Print a status message

   buf.mtype = 1; // Set the message type
   srand(time(NULL)); // Seed the random number generator

   // Send 50 random integers
   for (int i = 0; i < 50; i++) {
      buf.mtext = rand() % 1000; // Generate a random integer
      printf("Sending: %d\n", buf.mtext); // Print the integer to be sent
      if (msgsnd(msqid, &buf, sizeof(int), 0) == -1) // Send the integer
         perror("msgsnd"); // Print a system error message if sending fails
   }

   return 0; // Return 0 to indicate successful execution
}