// Include necessary libraries for input/output, process control, and inter-process communication
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

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
   int count = 0; // Counter for received messages
   key_t key; // Key to be passed to msgget()

   // Generate a key for the message queue
   if ((key = ftok("msgq.txt", 'B')) == -1) {
      perror("ftok"); // Print a system error message
      exit(1); // Terminate the program
   }

   // Connect to the message queue
   if ((msqid = msgget(key, PERMS)) == -1) {
      perror("msgget"); // Print a system error message
      exit(1); // Terminate the program
   }
   printf("Message queue: ready to receive messages.\n"); // Print a status message

   // Receive 50 integers
   while(count < 50) {
      // Receive a message from the queue
      if (msgrcv(msqid, &buf, sizeof(buf.mtext), 0, 0) == -1) {
         perror("msgrcv"); // Print a system error message
         exit(1); // Terminate the program
      }
      printf("Received: %d\n", buf.mtext); // Print the received integer
      count++; // Increment the counter
   }

   // Print a status message and exit
   printf("Message queue: received 50 integers, now exiting.\n");
   return 0; // Return 0 to indicate successful execution
}