#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void sig_handler(int sig)
{
	if (sig == SIGINT)
	{
		write(STDOUT_FILENO, "hello", 5);
	}
}

int main(void)
{
	signal(SIGINT, sig_handler);

	while (1)
	{
		write(STDOUT_FILENO, "a", 1);
		sleep(5);
	}
}