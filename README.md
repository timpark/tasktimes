tasktimes - Simple command-line time tracking
=============================================
----------------------------------------------------------------------------
This is a program to help track time taken on tasks.  After starting a
task, you can rename it, alter the start time, or delete it.  After
ending a task, but before starting a new one, you can alter the end time.
You can create reports from other files, so if you want to archive by
month or project, you can move the relevant lines from the active
file to an archive one.

There are defines at the top of the program where you can change the
delimiter between the project and the task and the filename used.  You
may wish to give it an absolute filename in a backed-up or synced
directory.

Usage:

    task                  : Show current task
    task help             : Show usage
    task "prj - stuff"    : Start task called "stuff" with project "prj"
    task on "prj - stuff" : Start task called "stuff" with project "prj"
    task on 8:24          : Change start time of current task to X:Y
    task on reset         : Change start time of current task to now
    task off              : Stop tracking current task
    task off 8h38m        : Add XhYm to current task start time and stop tracking
    task off 17:36        : Change end time of previous task to X:Y
    task off reset        : Change end time of previous task to now
    task resume           : Restart previous task
    task rename "task"    : Change name of current task to "task"
    task delete           : Delete current task
    task times            : Give report of tasks
    task times [file]     : Give report of tasks in another file

Example output:

    > task "prj - name of task"
    "2013/01/01 21:43:54 prj - name of task" added

    > task off
    "2013/01/01 21:43:59 off" added

    > task times
    --- prj ---
    2013/01/01 21:43:54 Mo ( 0:00:05) name of task
                     Total   0:00:05

License
=======
----------------------------------------------------------------------------

Copyright (c) 2013 Tim Park

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
