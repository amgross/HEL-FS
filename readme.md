hel-fs - is a heap like file system, which construct in manner to minimize writes to memory, be simple and avoid corruption upon crashes.

Requirements - implementation of memory driver as defined at mem_driver.h, where the triky part is that for avoiding corruption upon crash in writes the first 4 bytes should be written atomically at the end of the write.

The file system is built from chunks of memories in flexible sizes, like a heap. file is linked list of chunks where the first and last chunk are signed with special flags. chunks that are not part of file linked list are free to use/fragment/defragment. This implementation minimizes writes due to the fact that there is no metadata on file stored in memory outside the file and due to the flexible chunks sizes the matadata may be very little per file (depends on memory fragmentation). Furthermore corruption upon power down acoided by design as file is created/deleted just by writing the first chunk flag, all writes inside chunk will be ignored if the flag in the first chunk of the file not signs it as first chunk.

Limitations:
- The file system requiers lots of memory reads, to overcome the fact there is no metadata outside files about them.
- There is no file naming neither directories out of the box, one should implement its own way to differ between file or use our applicable application layer.


Directory hirarchy:
- kernel: the kernel of the file system, include the code + API + mem driver API needed to implemented by user.
- tests: the tests for CI.
- naming_wrapper: basic application layer that using the kernel for files with names (in different than the kernel that files has just id).

Critical things still missings:
- Option to change file after first creation.

Getting started:
- run 'make full' from the project root directory, to ensure all tests are running successfully on your computer.
- Change the defines at /kernel/hel_kernel_user_defines.h according your needs.
- You can change tests on the tests directory to see if the system is OK for your needs, it suggested to start from "naming_wrapper_tests.c"
- Create your memory driver according to /kernel/mem_driver.h, in first step it is suggested not to follow the instruction that needed for power down corruption avoidance (i.e. writing the atomic write atomically and in the end of the write).
