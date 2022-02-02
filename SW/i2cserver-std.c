#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>

static const int default_device = 0;
char busmap[] = {11,12,13,14,15,16,17,18,3,4,5,6,7,8,9,10,27,28,29,30,31,32,33,34,19,20,21,22,23,24,25,26};

void error(char *msg)
{
    perror(msg);
    exit(1);
}

static void pabort(const char *s)
{
    perror(s);
    abort();
}

void print_usage(char *prg)
{
    fprintf(stderr, "\nUsage: %s [options]\n", prg);
    fprintf(stderr, "  (use 'q' or CTRL-C to terminate %s)\n\n", prg);
    fprintf(stderr, "Options: -d <device 0..31>\n");
    fprintf(stderr, "Options: -v   Verbose write commands\n\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  To write: \"W address Byte1 Byte2 .. ByteN\"\n");
    fprintf(stderr, "  To read:  \"R address NUM_BYTES_TO_READ\"\n");
    fprintf(stderr, "  To sleep: \"S miliseconds\"\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
    char i2c_device[30];
    static char busnum = 0;
    char buff[512];
    char answ[512];
    int i, address, file, n, cnt, opt;
    int size = 0;
    int delay = 0;
    int vwrite = 0;
    char buf[32];
    char rw;
    char bytes[512];
    char msg[512];
    msg[0]=0;
    cnt=0;

    sprintf(i2c_device, "/dev/i2c-%d", busmap[default_device]);

    while ((opt = getopt(argc, argv, "vd:?")) != -1)
        {
            switch (opt)
                {
                case 'd':
                    sscanf(optarg, "%d", &busnum);
                    sprintf(i2c_device, "/dev/i2c-%d", busmap[busnum]);
                    break;
                case 'v':
                    vwrite = 1;
                    break;
                default:
                    print_usage(basename(argv[0]));
                    exit(1);
                    break;
                }
        }

    if (optind < argc)
        {
            print_usage(basename(argv[0]));
            exit(0);
        }

    file = open(i2c_device, O_RDWR);

    while((buff[0]=getchar())!=0)
        {
            if (buff[0]=='\n')
                {
                    //EOL
                    if (msg[cnt-1]=='\r')
                        {
                            msg[cnt-1]='\n';
                            msg[cnt]=0;
                        }
                    else
                        {
                            msg[cnt]='\n';
                            msg[cnt+1]=0;
                        }

                    sscanf(msg, "%c %[^\n]", &rw, bytes);

                    switch(rw)
                        {
                        case 'R':
                        case 'r':
                            sscanf(bytes, "%x %x %[^\n]", &address, &size, bytes); 
                            if (ioctl(file, I2C_SLAVE, address) < 0)
                            {
                                fprintf(stderr, "Error: Could not set address to 0x%02x: %s\n", address, strerror(errno));
                                break;
                            }
                            if (read(file,buf,size) != size)
                                {
                                    /* ERROR HANDLING: i2c transaction failed */
                                    fprintf(stderr,"Error: R %x\n", address);
                                }
                            else
                                {       
                                    answ[0]='\0';
                                    for (i=0; i<size; i++)
                                        {
                                            sprintf(answ, "%s%02x ", answ, buf[i]);
                                        }
                                    answ[(size*3)-1] = '\n';
                                    printf("R %s", answ);
                                }
                            break;
                        case 'W':
                        case 'w':
                            sscanf(bytes, "%x %[^\n]", &address, bytes); 
                            if (ioctl(file, I2C_SLAVE, address) < 0)
                            {
                                fprintf(stderr, "Error: Could not set address to 0x%02x: %s\n", address, strerror(errno));
                                break;
                            }                               
                            size = 0;
                            while (sscanf(bytes, "%x %[^\n]", &buf[size], bytes) == 2)
                            {
                                size++;
                            }
                            if (size>0) size++;
                            answ[0]='\0';
                            for (i=0; i<size; i++)
                                {
                                    sprintf(answ, "%s%02x ", answ, buf[i]);
                                }
                            answ[(size*3)-1] = '\n';

                            if (write(file,buf,size) < 0)
                                {
                                    /* ERROR HANDLING: i2c transaction failed */
                                    fprintf(stderr,"Error: W %x %s", address, answ);
                                }
                            else
                                {
                                    if (vwrite)
                                        printf("W %x %s", address, answ);
                                }
                            break;
                        case 'q':
                        case 'Q':
                            close(file);
                            return 0;
                            break;
                        case 'S':
                        case 's':
                            sscanf(bytes, "%d %[^\n]", &delay, bytes); 
                            usleep(delay*1000);
                            break;
                        default:
                            break;
                        }
                    fflush(stdout);
                    msg[0]=0;
                    cnt=0;
                }

            else
                {
                    msg[cnt]=buff[0];
                    cnt++;
                }
        }
    close(file);
    return 0;
}
