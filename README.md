Implementation of the first 3 levels (or the first 2 phases) of the Pandos operating system as described at:
https://wiki.virtualsquare.org/#!education/pandos.md

Pandos is run through the µMPS3 emulator. To learn more about µMPS3, see:
https://github.com/virtualsquare/umps3

Phase 1 just has the Queues manager, which supports the management of queues of structures. Phase 2 contains 
both the Queues manager and the Kernel. The kernel consists of 8 system calls, device interupthandling, 
round-robin process scheduling, and deadlock detection.

This project was completed in Xavier's CSCI 320 operating systems course in a team of 3.
