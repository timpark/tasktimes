/***********************************************************************
* tasktimes - Simple command-line time tracking
* Copyright (c) 2013 Tim Park. Licensed under the MIT License
* "task help" - Show usage (see printUsage function)
***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define FILENAME "times.txt"
#define DELIMITER " - "
#define VERSION "1.0"

#define MAXLINE 256
#define DATESIZE 20
#define START 0
#define END 1
#define TOTALWPRJ 2
#define TOTALNOPRJ 3

char now[DATESIZE];
struct Task {
	struct Task *next;
	struct Task *prev;
	char startTime[DATESIZE];
	char endTime[DATESIZE];
	double taskTime;
	char project[MAXLINE];
	char task[MAXLINE];
	bool reported;
} head, tail;

//*** Print how to use the program and the current default filename.
void printUsage(char *cmd)
{
	printf("tasktimes v%s - Simple command-line time tracking\n\n", VERSION);
	printf("%s                  : Show current task\n", cmd);
	printf("%s help             : Show usage\n", cmd);
	printf("%s \"prj%sstuff\"    : Start task called \"stuff\" with project \"prj\"\n", cmd, DELIMITER);
	printf("%s on \"prj%sstuff\" : Start task called \"stuff\" with project \"prj\"\n", cmd, DELIMITER);
	printf("%s on 8:24          : Change start time of current task to X:Y\n", cmd);
	printf("%s on reset         : Change start time of current task to now\n", cmd);
	printf("%s off              : Stop tracking current task\n", cmd);
	printf("%s off 8h38m        : Add XhYm to current task start time and stop tracking\n", cmd);
	printf("%s off 17:36        : Change end time of previous task to X:Y\n", cmd);
	printf("%s off reset        : Change end time of previous task to now\n", cmd);
	printf("%s resume           : Restart previous task\n", cmd);
	printf("%s rename \"task\"    : Change name of current task to \"task\"\n", cmd);
	printf("%s delete           : Delete current task\n", cmd);
	printf("%s times            : Give report of tasks\n", cmd);
	printf("%s times [file]     : Give report of tasks in another file\n", cmd);
	printf(" Current default file : %s\n", FILENAME);
}

//*** Print error and exit. Don't really need variable args for our use.
void errorExit(char *error, char *arg)
{
	struct Task *task, *currentTask;
	for (task = head.next; task != &tail; task = currentTask)
	{
		currentTask = task->next;
		free(task);
	}
	fprintf(stderr, error, arg);
	exit(1);
}

//*** Return the day of the week. (0 = Sunday)
int dayOfWeek(int y, int m, int d)
{
	struct tm tm1 = {0,0,0,0,0,0,0,0,0};
	tm1.tm_year = y - 1900;
	tm1.tm_mon = m - 1;
	tm1.tm_mday = d;
	tm1.tm_hour = tm1.tm_min = tm1.tm_sec = 1;
	mktime(&tm1);
	return tm1.tm_wday;
}

//*** Convert "YYYY/MM/DD HH:MM:SS" to time_t. (seconds) Return false on invalid date/time.
bool stringToSecs(char *buf, time_t *time1)
{
	struct tm tm1 = {0,0,0,0,0,0,0,0,0};
	char days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
//Example task entry:
//2013/03/11 01:21:20 prj - stuff

	if ((buf[4] != '/') || (buf[7] != '/') || (buf[10] != ' ') ||
		(buf[13] != ':') || (buf[16] != ':'))
		return false;

	// Convert and validate date/time
	sscanf(buf, "%d/%d/%d %d:%d:%d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday,
		&tm1.tm_hour, &tm1.tm_min, &tm1.tm_sec);
	if (((tm1.tm_year % 400) == 0) || (((tm1.tm_year % 4) == 0) && ((tm1.tm_year % 100) != 0)))
		days[1] = 29; // Adjust for leap year
	tm1.tm_year -= 1900;
	tm1.tm_mon--;
	if ((tm1.tm_year < 0) ||
		(tm1.tm_mon < 0) || (tm1.tm_mon > 11) ||
		(tm1.tm_mday < 1) || (tm1.tm_mday > days[tm1.tm_mon]) ||
		(tm1.tm_hour < 0) || (tm1.tm_hour > 23) ||
		(tm1.tm_min < 0) || (tm1.tm_min > 59) ||
		(tm1.tm_sec < 0) || (tm1.tm_sec > 59))
		return false;
	tm1.tm_isdst = -1; // unknown DST status

	*time1 = mktime(&tm1);
	if (*time1 == -1)
		return false;
	return true;
}

//*** Convert "H:MM" or "HH:MM" to integer values. Exits on invalid hour/minutes values since only
//*** used for command arguments. Returns false if the string doesn't match either of the time formats.
bool stringToTime(char *buf, int *hour, int *minutes)
{
	if ((strlen(buf) == 4) && (isdigit(buf[0])) && (buf[1] == ':') &&
		(isdigit(buf[2])) && (isdigit(buf[3])))
	{
		*hour = atoi(buf);
		*minutes = atoi(&buf[2]);
		if ((*hour < 0) || (*hour > 23) || (*minutes < 0) || (*minutes > 59))
			errorExit("Invalid time\n", "");
		return true;
	}
	if ((strlen(buf) == 5) && (isdigit(buf[0])) && (isdigit(buf[1])) &&
		(buf[2] == ':') && (isdigit(buf[3])) && (isdigit(buf[4])))
	{
		*hour = atoi(buf);
		*minutes = atoi(&buf[3]);
		if ((*hour < 0) || (*hour > 23) || (*minutes < 0) || (*minutes > 59))
			errorExit("Invalid time\n", "");
		return true;
	}
	return false;
}

//*** Change the time in the string passed in. Caller should validate values.
void changeTime(char *buf, int hour, int minutes)
{
	char tmp[3];
	sprintf(tmp, "%02d", hour);
	buf[11] = tmp[0];
	buf[12] = tmp[1];
	sprintf(tmp, "%02d", minutes);
	buf[14] = tmp[0];
	buf[15] = tmp[1];
	buf[17] = '0';
	buf[18] = '0';
}

//*** Add a task to the task file. If last task is currently unfinished, complete it.
void addTask(char *filename, time_t time1, char *task, struct Task *currentTask)
{
	struct tm *tm1;
	FILE *file = fopen(filename, "a");
	if (file == NULL)
		errorExit("Can't open file for appending: %s\n", filename);
	if (currentTask)
	{
		fprintf(stderr, "WARNING: Completing current task... %s\n", currentTask->task);
		tm1 = localtime(&time1);
		strftime(now, DATESIZE, "%Y/%m/%d %H:%M:%S", tm1);
		fprintf(file, "%s off\n", now);
		time1 += 1; // Avoid datetime collisions
		printf("\"%s off\" added\n", now);
	}
	tm1 = localtime(&time1);
	strftime(now, DATESIZE, "%Y/%m/%d %H:%M:%S", tm1);
	fprintf(file, "%s %s\n", now, task);
	fclose(file);
	printf("\"%s %s\" added\n", now, task);
}

//*** Find difference between two times and store the difference.
void calculateTaskTime(struct Task *task)
{
	time_t time1, time2;
	stringToSecs(task->startTime, &time1);
	stringToSecs(task->endTime, &time2);
	task->taskTime = difftime(time2, time1);
}

//*** Print a task for the log file (START, END) or for reporting. (TOTALWPRJ, TOTALNOPRJ)
void printTask(FILE *handle, struct Task *task, int timeFlag)
{
	static char days[7][3] = {"Su","Mo","Tu","We","Th","Fr","Sa"};
	int y, m, d;
	if (task == NULL)
		return;
	if ((timeFlag == END) && (task->endTime[0] == 0))
		return; // Don't print "off" if task not completed
	if (timeFlag == START)
	{
		if (task->project[0] == 0)
			fprintf(handle, "%s %s\n", task->startTime, task->task);
		else
			fprintf(handle, "%s %s%s%s\n", task->startTime, task->project, DELIMITER, task->task);
	}
	else if (timeFlag == END)
		fprintf(handle, "%s off\n", task->endTime);
	else if ((timeFlag == TOTALWPRJ) || (timeFlag == TOTALNOPRJ))
	{
		sscanf(task->startTime, "%d/%d/%d", &y, &m, &d);
		fprintf(handle, "%s %s (%2d:%02d:%02d)%c", task->startTime, days[dayOfWeek(y,m,d)],
			(int)(task->taskTime/3600), (int)(task->taskTime/60)%60, (int)task->taskTime%60,
			(strcmp(task->endTime, now) == 0) ? '*' : ' ');
		if ((task->project[0] == 0) || (timeFlag == TOTALNOPRJ))
			fprintf(handle, "%s\n", task->task);
		else
			fprintf(handle, "%s%s%s\n", task->project, DELIMITER, task->task);
	}
}

//*** Print all tasks for a given project and return the total time.
double reportProject(char *project)
{
	struct Task *task;
	double totalTime = 0;
	printf("\n--- %s ---\n", (project[0] == 0) ? "Misc" : project);
	for (task = head.next; task != &tail; task = task->next)
		if (strcmp(task->project, project) == 0)
		{
			printTask(stdout, task, TOTALNOPRJ);
			totalTime += task->taskTime;
			task->reported = true;
		}
	printf("                 Total %3d:%02d:%02d\n",
		(int)(totalTime/3600), (int)(totalTime/60)%60, (int)totalTime%60);
	return totalTime;
}

//*** Generate a report of all projects.
void printReport()
{
	struct Task *task;
	char *project;
	int numProjects = 0;
	double totalTime = 0;
	while (true)
	{
		project = NULL;
		for (task = head.next; task != &tail; task = task->next)
			if (task->reported == false)
			{
				project = task->project;
				break;
			}
		if (project == NULL)
			break;
		totalTime += reportProject(project);
		numProjects++;
	}
	if (numProjects > 1)
		printf("     %2d Projects Total %3d:%02d:%02d\n", numProjects,
			(int)(totalTime/3600), (int)(totalTime/60)%60, (int)totalTime%60);
}

//*** Read in all tasks from a file.
void readTasks(char *filename)
{
	FILE *file;
	char *c, *d, line[MAXLINE];
	time_t time1;
	struct Task *currentTask = NULL;

	file = fopen(filename, "r");
	if (file == NULL)
	{
		// Create empty file
		file = fopen(filename, "w");
		if (file == NULL)
			errorExit("Can't open file for writing: %s\n", filename);
		fclose(file);
		return;
	}
	while (fgets(line, MAXLINE, file) != NULL) {
		// Strip trailing whitespace
		c = line + strlen(line) - 1;
		while ((c > line) && (isspace(*c)))
			c--;
		*(c + 1) = 0;

		// Skip leading whitespace
		c = line;
		while (isspace(*c))
			c++;

		// Skip if line is blank
		if (*c == 0)
			continue;

		c[19] = 0;
		if (stringToSecs(c, &time1) == false)
		{
			fclose(file);
			errorExit("Invalid time: %s\n", c);
		}
		if (currentTask == NULL)
		{
			if (strcmp(&c[20], "off") == 0)
				errorExit("ERROR: \"off\" found with no task... %s\n", c);
			else
			{
				// Add task to list
				currentTask = calloc(1, sizeof(struct Task));
				if (currentTask == NULL)
				{
					fclose(file);
					errorExit("Couldn't allocate memory\n", "");
				}
				currentTask->prev = tail.prev;
				currentTask->next = tail.prev->next;
				tail.prev->next = currentTask;
				tail.prev = currentTask;

				strcpy(currentTask->startTime, c);
				c += 20;
				d = strstr(c, DELIMITER);
				if (d == NULL)
					strcpy(currentTask->task, c);
				else
				{
					*d = 0;
					d += strlen(DELIMITER);
					strcpy(currentTask->project, c);
					strcpy(currentTask->task, d);
				}
			}
		}
		else
		{
			if (strcmp(&c[20], "off") != 0)
				errorExit("ERROR: Task not completed... %s\n", c);
			else
			{
				strcpy(currentTask->endTime, c);
				calculateTaskTime(currentTask);
				if (currentTask->taskTime < 0)
					fprintf(stderr, "WARNING: Negative time for task... %s\n", c);
				currentTask = NULL;
			}
		}
	}
	fclose(file);
}

//*** Write all tasks to a file.
void writeTasks(char *filename)
{
	FILE *file;
	struct Task *task;

	file = fopen(filename, "w");
	if (file == NULL)
		errorExit("Can't open file for writing: %s\n", filename);
	for (task = head.next; task != &tail; task = task->next)
	{
		printTask(file, task, START);
		printTask(file, task, END);
	}
	fclose(file);
}

int main(int argc, char **argv)
{
	FILE *file;
	time_t time1, diff;
	struct tm *tm1;
	int hour, minutes;
	char *c, *filename = FILENAME;
	struct Task *task, *currentTask = NULL;

	head.endTime[0] = 0;
	head.next = &tail;
	tail.prev = &head;

	if (time(&time1) == -1)
		errorExit("Couldn't get current time\n", "");
	tm1 = localtime(&time1);
	strftime(now, DATESIZE, "%Y/%m/%d %H:%M:%S", tm1);

	if ((argc > 1) && (strcmp(argv[1], "help") == 0))
	{
		printUsage(argv[0]);
		exit(0);
	}

	if ((argc > 2) && (strcmp(argv[1], "times") == 0))
		filename = argv[2];
	readTasks(filename);
	// If there's at least one item in list and last item doesn't have ending time
	if ((tail.prev != &head) && (tail.prev->endTime[0] == 0))
		currentTask = tail.prev;

	if (argc <= 1)
	{
		printf("tasktimes - Simple command-line time tracking (\"%s help\" for usage)\n\n", argv[0]);
		if (currentTask)
		{
			strcpy(currentTask->endTime, now);
			calculateTaskTime(currentTask);
			printf("Current task: ");
			printTask(stdout, currentTask, TOTALWPRJ);
		}
		else
		{
			printf("No current task\n");
			if (tail.prev == &head)
				printf("No previous task\n");
			else
			{
				printf("Previous task: ");
				printTask(stdout, tail.prev, TOTALWPRJ);
			}
		}
	}
	else
	{
		if (strcmp(argv[1], "on") == 0)
		{
			if ((argc > 2) && (strcmp(argv[2], "reset") == 0))
			{
				if (currentTask == NULL)
					errorExit("No current task\n", "");
				printf("Changing current task from:\n");
				printTask(stdout, currentTask, START);
				strcpy(currentTask->startTime, now);
				printf("to:\n");
				printTask(stdout, currentTask, START);
				writeTasks(filename);
			}
			else if ((argc > 2) && (stringToTime(argv[2], &hour, &minutes)))
			{
				if (currentTask == NULL)
					errorExit("No current task\n", "");
				printf("Changing current task from:\n");
				printTask(stdout, currentTask, START);
				changeTime(currentTask->startTime, hour, minutes);
				printf("to:\n");
				printTask(stdout, currentTask, START);
				writeTasks(filename);
			}
			else if (argc < 3)
				errorExit("No task given\n", "");
			else if (strcmp(argv[2], "off") == 0)
				errorExit("Invalid task name\n", "");
			else
				addTask(filename, time1, argv[2], currentTask);
		}
		else if (strcmp(argv[1], "off") == 0)
		{
			if (currentTask == NULL)
			{
				currentTask = tail.prev;
				if ((argc > 2) && (strcmp(argv[2], "reset") == 0))
				{
					if (tail.prev == &head)
						errorExit("No previous task\n", "");
					printf("Changing previous task from:\n");
					printTask(stdout, currentTask, END);
					strcpy(tail.prev->endTime, now);
					printf("to:\n");
					printTask(stdout, currentTask, END);
					writeTasks(filename);
				}
				else if ((argc > 2) && (stringToTime(argv[2], &hour, &minutes)))
				{
					if (tail.prev == &head)
						errorExit("No previous task\n", "");
					printf("Changing task previous from:\n");
					printTask(stdout, currentTask, END);
					changeTime(tail.prev->endTime, hour, minutes);
					printf("to:\n");
					printTask(stdout, currentTask, END);
					writeTasks(filename);
				}
				else
					errorExit("No current task\n", "");
			}
			else
			{
				if (argc > 2)
				{
					if (strcmp(argv[2], "reset") == 0)
						errorExit("Current task not completed\n", "");
					else if (stringToTime(argv[2], &hour, &minutes))
						errorExit("Current task not completed\n", "");
					// Try to parse XhYm and add to start time
					diff = 0;
					if ((c = strchr(argv[2], 'h')) != 0)
					{
						c--;
						while ((c >= argv[2]) && (isdigit(*c))) c--;
						diff += (time_t)(60 * 60 * atoi(c + 1));
					}
					if ((c = strchr(argv[2], 'm')) != 0)
					{
						c--;
						while ((c >= argv[2]) && (isdigit(*c))) c--;
						diff += (time_t)(60 * atoi(c + 1));
					}
					stringToSecs(currentTask->startTime, &time1);
					time1 += diff;
					tm1 = localtime(&time1);
					strftime(now, DATESIZE, "%Y/%m/%d %H:%M:%S", tm1);
				}
				file = fopen(filename, "a");
				if (file == NULL)
					errorExit("Can't open file for appending: %s\n", filename);
				fprintf(file, "%s off\n", now);
				fclose(file);
				printf("\"%s off\" added\n", now);
			}
		}
		else if (strcmp(argv[1], "resume") == 0)
		{
			if (currentTask)
				errorExit("Current task not completed\n", "");
			else
			{
				if (tail.prev == &head)
					errorExit("No previous task\n", "");
				file = fopen(filename, "a");
				if (file == NULL)
					errorExit("Can't open file for appending: %s\n", filename);
				strcpy(tail.prev->startTime, now);
				printTask(file, tail.prev, START);
				fclose(file);
				printf("Added task:\n");
				printTask(stdout, tail.prev, START);
			}
		}
		else if (strcmp(argv[1], "rename") == 0)
		{
			if (argc < 3)
				errorExit("No new name given\n", "");
			if (currentTask == NULL)
				errorExit("No current task\n", "");
			printf("Changing current task from:\n");
			printTask(stdout, currentTask, TOTALWPRJ);
			c = strstr(argv[2], DELIMITER);
			if (c == NULL)
			{
				currentTask->project[0] = 0;
				strcpy(currentTask->task, argv[2]);
			}
			else
			{
				strcpy(currentTask->project, argv[2]);
				*(currentTask->project + (c - argv[2])) = 0;
				strcpy(currentTask->task, c + strlen(DELIMITER));
			}
			printf("to:\n");
			printTask(stdout, currentTask, TOTALWPRJ);
			writeTasks(filename);
		}
		else if (strcmp(argv[1], "delete") == 0)
		{
			if (currentTask == NULL)
				errorExit("No current task\n", "");
			printf("Removing task:\n");
			printTask(stdout, currentTask, TOTALWPRJ);
			currentTask->next->prev = currentTask->prev;
			currentTask->prev->next = currentTask->next;
			free(currentTask);
			writeTasks(filename);
		}
		else if (strcmp(argv[1], "times") == 0)
		{
			// Give current task an end time for the report
			if (currentTask)
			{
				strcpy(currentTask->endTime, now);
				calculateTaskTime(currentTask);
			}
			printReport();
		}
		else
			addTask(filename, time1, argv[1], currentTask);
	}

	for (task = head.next; task != &tail; task = currentTask)
	{
		currentTask = task->next;
		free(task);
	}
	return 0;
}
