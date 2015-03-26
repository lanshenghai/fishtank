#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define PI

#ifdef PI
#include <wiringPi.h>
#endif

#define MAX_CHAR_PER_LINE       100
#define MAX_TIMER_NUM           50
#define MAX_PORT_NUM            7

typedef struct {
        int port;
        int start;      //second
        int end;        //second
}PORT_TIMER;

PORT_TIMER g_timer[MAX_TIMER_NUM];
int g_timer_num = 0;
int g_status_reload = 1;

int is_space(char c)
{
        return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

char* strip(char *line)
{
        char *p = line;
        char *rp;

        while (*p != '\0' && is_space(*p)) p++;

        rp = p + strlen(p) - 1;
        while (rp != p && is_space(*rp)) rp--;
        *rp = '\0';

        return p;
}

void insert_timer(int port, int start, int end)
{
        if (g_timer_num >= MAX_TIMER_NUM)
        {
                printf("Error: the timers are too many (%d)", g_timer_num);
                return;
        }

        g_timer[g_timer_num].port = port;
        g_timer[g_timer_num].start = start;
        g_timer[g_timer_num].end = end;

        printf("%d: %d %d - %d\n", g_timer_num, g_timer[g_timer_num].port, g_timer[g_timer_num].start, g_timer[g_timer_num].end);
        g_timer_num++;
}

int isvalid_min(int min)
{
        return 0 <= min && min < 60;
}

int isvalid_hour(int hour)
{
        return 0 <= hour && hour <= 24;
}

int isvalid_sec(int sec)
{
        return 0 <= sec && sec < 60;
}

// 1    8:00:00-18:00:00
// 2    0:00:00-24:00:00
int decode_line(char *line)
{
        int port;
        int hour_s, min_s, sec_s;
        int hour_e, min_e, sec_e;
        int start, end;

        char *p = strip(line);
        if (strlen(p) == 0)
                return 0; // skip empty line

        if (-1 == sscanf(line, "%d %d:%d:%d-%d:%d:%d", &port, &hour_s, &min_s, &sec_s, &hour_e, &min_e, &sec_e))
        {
                printf("Error: sscanf %s return error", line);
                return -1;
        }
        //printf("%s\n %d %d:%d:%d-%d:%d:%d\n", line, port, hour_s, min_s, sec_s, hour_e, min_e, sec_e);

        if (port < 0 || MAX_PORT_NUM <= port)
        {
                printf("Error: invlid port number %d, valid range is [%d, %d)", port, 0, MAX_PORT_NUM);
                return -1;
        }

        if (!isvalid_hour(hour_s) || !isvalid_min(min_s) || !isvalid_sec(sec_s) || !isvalid_hour(hour_e) || !isvalid_min(min_e) || !isvalid_sec(sec_e))
        {
                printf("Rrrror: Invlid time");
                return -1;
        }

        start = (hour_s * 60 + min_s) * 60 + sec_s;
        end = (hour_e * 60 + min_e) * 60 + sec_e;

        if (start <= end)
        {
                insert_timer(port, start, end);
        }
        else
        {
                // generate 2 periods for the cross mid night mode
                insert_timer(port, start, 24 * 60 * 60);
                insert_timer(port, 0, end);
        }


        return 0;
}

int load_config(char *cfg_name)
{
        FILE *fp;
        int c;
        char line[MAX_CHAR_PER_LINE];
        char *p;
        int ret = 0;
        int line_num = 1;

        fp = fopen(cfg_name, "r");
        if (fp == NULL)
        {
                printf("Error: can't open the %s file!\n", cfg_name);
                return -1;
        }

        p = line;
        while((c = fgetc(fp)) != EOF)
        {
                if (c != '\n')
                {
                        *p++ = c;
                }
                else
                {
                        *p = '\0';
                        ret = decode_line(line);
                        p = line;
                        if (ret != 0)
                        {
                                printf(" at line %d\n", line_num);
                                break;
                        }
                        line_num++;
                }
        }

        fclose(fp);

        return ret;
}

void sig_callback(int signum)
{
        printf("Received signal:%d.\n", signum);
        g_status_reload = 1;
}

int open_gpio()
{
        int fd = -1;

        while (1)
        {
                fd = open("/dev/gpio", O_RDWR);
                if (fd == -1)
                        sleep(1);
                else
                        break;
        }
        return fd;
}

void set_gpio(int port, int status)
{
#ifndef PI  // for s3c2410
  	char write_buf[2];
  	write_buf[0] = port + '0';
  	write_buf[1] = status;
  	fd = open_gpio();
  	write(fd, write_buf, 2);
	close(fd);
#else 	// for pi
typedef enum {
        PORT1,
        PORT2,
        PORT3,
        PORT4,
        PORT5,
        PORT6,
        PORT7,
        MAX_PORT
}PORTNUM;

static unsigned int ports_mapping[MAX_PORT] = {
        [PORT1] = 22,
        [PORT2] = 23,
        [PORT3] = 24,
        [PORT4] = 27,
        [PORT5] = 25,
        [PORT6] = 28,
        [PORT7] = 29
};

  wiringPiSetup () ;
  pinMode (ports_mapping[port], OUTPUT) ;
  digitalWrite (ports_mapping[port], (status == '0') ? 1 : 0);

  printf("Port #%d change to %c.\n", port, status);
#endif
}


int get_current_sec()
{
        time_t t;
        struct tm *local;

        t = time(NULL);
        local = localtime(&t);
        return (local->tm_hour * 60 + local->tm_min) * 60 + local->tm_sec;
}

int main(int argc, char* argv[])
{
        int cur;
        int i;
        char current_status[MAX_PORT_NUM];
        char local_status[MAX_PORT_NUM];
        char *cfg_name = "timer.cfg";
        int fd, wdt_fd;
        int has_change;
        size_t len;
        int port;

        if (argc > 1 && strlen(argv[1]) > 0)
                cfg_name = argv[1];

        if (load_config(cfg_name) != 0)
        {
                printf("Load configure file error.\n");
                return -1;
        }
#if 1
        if(daemon(0,0) == -1)
        {
                printf("daemon error\n");
                return -1;
        }
#endif
        if (signal(SIGUSR1, sig_callback) ==  SIG_ERR)
        {
                printf("Register the SIGUSR1 failed.\n");
        }

    sleep(5); // delay 5s to let the bootup is more stable.

#ifndef PI
    wdt_fd = open("/dev/watchdog",O_WRONLY);

    if(wdt_fd==-1)
    {
        perror("Can't open watchdog.");
        exit(1);
    }
    else
    {
                printf("Watchdog open succeed.\n");
        }

#endif
        while(1)
        {
                if (g_status_reload)
                {
#ifndef PI
                        // load real status
                        fd = open_gpio();
                        len = read(fd, current_status, sizeof(current_status));
                        close(fd);
#else
		memset(current_status, '1', sizeof(current_status));
                for (i = 0; i < g_timer_num; i++)
                {
                        int port = g_timer[i].port;
                        if (g_timer[i].start <= cur && cur < g_timer[i].end)
                                current_status[port] = '1'; // set open status
                        else
                                current_status[port] = '0'; // set close status
                }
			for (i = 0; i < MAX_PORT_NUM; i++)
				set_gpio(i, current_status[i]);
			len = i;
#endif
                        current_status[len] = '\0';
                        g_status_reload = 0;

                        printf("Reload real status: %s\n", current_status);
                }

                cur = get_current_sec();

                // default same as before
                memcpy(local_status, current_status, sizeof(current_status));

                has_change = 0;
                for (i = 0; i < g_timer_num; i++)
                {
                        int port = g_timer[i].port;
                        if (g_timer[i].start <= cur && cur < g_timer[i].end)
                                local_status[port] = '1'; // set open status
                        else
                                local_status[port] = '0'; // set close status

                        if (local_status[port] != current_status[port])
                                has_change = 1;
                }

                if (has_change)
                {
                        // write status to gpio
                        for (port = 0;port < MAX_PORT_NUM; port++)
                        {
                                if (current_status[port] != local_status[port])
                                {
									set_gpio(port, local_status[port]);

                                        printf("Port %d: changed from %c to %c\n", port, current_status[port], local_status[port]);
                                        current_status[port] = local_status[port];
                                        sleep(1); // sleep sometime between each operation for gpio.

#ifndef PI
                        write(wdt_fd,"\0",1);
                        fsync(wdt_fd);
#endif
                }
                        }
                }
                else
                {
#ifndef PI
                write(wdt_fd,"\0",1);
                fsync(wdt_fd);
#endif
                }
                sleep(1);
        }

        return 0;
}

