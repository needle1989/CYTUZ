
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    OS Team for CYTUZ, 2020
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	struct task* p_task;
	struct proc* p_proc= proc_table;
	char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16   selector_ldt = SELECTOR_LDT_FIRST;
        u8    privilege;
        u8    rpl;
	int   eflags;
	int   i, j;
	int   prio;
	for (i = 0; i < NR_TASKS+NR_PROCS; i++) {
	        if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
			prio      = 15;
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
			prio      = 5;
                }

		strcpy(p_proc->name, p_task->name);	/* name of the process */
		p_proc->pid = i;			/* pid */

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		/* p_proc->nr_tty		= 0; */

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_proc->ticks = p_proc->priority = prio;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

        /* proc_table[NR_TASKS + 0].nr_tty = 0; */
        /* proc_table[NR_TASKS + 1].nr_tty = 1; */
        /* proc_table[NR_TASKS + 2].nr_tty = 1; */

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
        kbInitial();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}

PUBLIC void addTwoString(char *to_str,char *from_str1,char *from_str2){
    int i=0,j=0;
    while(from_str1[i]!=0)
        to_str[j++]=from_str1[i++];
    i=0;
    while(from_str2[i]!=0)
        to_str[j++]=from_str2[i++];
    to_str[j]=0;
}

/*======================================================================*
                            welcome animation
*======================================================================*/
void displayWelcomeInfo() 
{
    printf("   |=========================================================================|\n");
    printf("   |=========================================================================|\n");
    printf("   |               **       * *       ***       * *       ***                |\n");
    printf("   |              *          *         *        * *        *                 |\n");
    printf("   |               **        *         *        ***       ***                |\n");
    printf("   |=========================================================================|\n");
    printf("   |                                                                         |\n");
    printf("   |                        OPERATING        SYSTEM                          |\n");
    printf("   |                                                                         |\n");
    printf("   |                        INITIALIZATION COMPLETE                          |\n");
    printf("   |                                                                         |\n");
    printf("   |                           Welcome to CYTUZ                              |\n");
    printf("   |                      Enter 'help' to show commands.                     |\n");
    printf("   |=========================================================================|\n");
    printf("   |=========================================================================|\n");
    printf("\n\n\n\n");


}


/*****************************************************************************
 *                                Custom Command
 *****************************************************************************/
char* findpass(char *src)
{
    char pass[128];
    int flag = 0;
    char *p1, *p2;

    p1 = src;
    p2 = pass;

    while (p1 && *p1 != ' ')
    {
        if (*p1 == ':')
            flag = 1;

        if (flag && *p1 != ':')
        {
            *p2 = *p1;
            p2++;
        }
        p1++;
    }
    *p2 = '\0';

    return pass;
}

void clearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

void printTitle()
{
    clear(); 	

  //  disp_color_str("dddddddddddddddd\n", 0x9);
    if(current_console==0){
    	displayWelcomeInfo();
    }
    else{
    	printf("[TTY #%d]\n", current_console);
    }
    
}

void clear()
{	
	clear_screen(0,console_table[current_console].cursor);
    console_table[current_console].crtc_start = console_table[current_console].orig;
    console_table[current_console].cursor = console_table[current_console].orig;    
}

void doTest(char *path)
{
    struct dir_entry *pde = find_entry(path);
    printf(pde->name);
    printf("\n");
    printf(pde->pass);
    printf("\n");
}

int verifyFilePass(char *path, int fd_stdin)
{
    char pass[128];

    struct dir_entry *pde = find_entry(path);

    /*printf(pde->pass);*/

    if (strcmp(pde->pass, "") == 0)
        return 1;

    printf("Please input the file password: ");
    read(fd_stdin, pass, 128);

    if (strcmp(pde->pass, pass) == 0)
        return 1;

    return 0;
}

void doEncrypt(char *path, int fd_stdin)
{
    //查找文件
    /*struct dir_entry *pde = find_entry(path);*/

    char pass[128] = {0};

    printf("Please input the new file password: ");
    read(fd_stdin, pass, 128);

    if (strcmp(pass, "") == 0)
    {
        /*printf("A blank password!\n");*/
        strcpy(pass, "");
    }
    //以下内容用于加密
    int i, j;

    char filename[MAX_PATH];
    memset(filename, 0, MAX_FILENAME_LEN);
    struct inode * dir_inode;

    if (strip_path(filename, path, &dir_inode) != 0)
        return 0;

    if (filename[0] == 0)   /* path: "/" */
        return dir_inode->i_num;

    /**
     * Search the dir for the file.
     */
    int dir_blk0_nr = dir_inode->i_start_sect;
    int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    int nr_dir_entries =
      dir_inode->i_size / DIR_ENTRY_SIZE; /**
                           * including unused slots
                           * (the file has been deleted
                           * but the slot is still there)
                           */
    int m = 0;
    struct dir_entry * pde;
    for (i = 0; i < nr_dir_blks; i++) {
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
        pde = (struct dir_entry *)fsbuf;
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++)
        {
            if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
            {
                //刷新文件
                strcpy(pde->pass, pass);
                WR_SECT(dir_inode->i_dev, dir_blk0_nr + i);
                return;
                /*return pde->inode_nr;*/
            }
            if (++m > nr_dir_entries)
                break;
        }
        if (m > nr_dir_entries) /* all entries have been iterated */
            break;
    }

}


void help()
{
    printf("   |=========================================================================|\n");
    printf("   |======================= C     Y     T     U     Z =======================|\n");
    printf("   |=========================================================================|\n");
    printf("   |                                                                         |\n");
    printf("   |************************* COMMANDS FOR SYSTEM ***************************|\n");
    printf("   |                                                                         |\n");
    printf("   |                               help : Show CYTUZ commands                |\n");
    printf("   |                              clear : Clear the screen                   |\n");
    printf("   |                            process : Show process manager               |\n");
    printf("   |                                                                         |\n");
    printf("   |************************* COMMANDS FOR FILE SYSTEM **********************|\n");
    printf("   |                                 ls : List files in current directory    |\n");
    printf("   |                             create : Create a new file                  |\n");
    printf("   |                                cat : Print the file                     |\n");
    printf("   |                                 vi : Modify the content of the file     |\n");
    printf("   |                             delete : Delete a file                      |\n");
    printf("   |                                 cp : Copy a file                        |\n");
    printf("   |                                 mv : Move a file                        |\n");   
    printf("   |                             encypt : Encrypt a file                     |\n");
    printf("   |                                 cd : Change the directory               |\n");
    printf("   |                              mkdir : Create a new directory             |\n");
    printf("   |                                                                         |\n");
    printf("   |************************* COMMANDS FOR USER APPLICATIONS ****************|\n");
    printf("   |                        minesweeper : Launch Minesweeper                 |\n");
    printf("   |                              snake : Launch SnakeGame                   |\n");
    printf("   |                              gobang : Launch GoBangGame                   |\n");
    printf("   |=========================================================================|\n");
}

void ProcessManage()
{
    int i;
    printf("=============================================================================\n");
    printf("      processID      |    name       | spriority    | running?\n");
    //进程号，进程名，优先级，是否是系统进程，是否在运行
    printf("-----------------------------------------------------------------------------\n");
    for ( i = 0 ; i < NR_TASKS + NR_PROCS ; ++i )//逐个遍历
    {
        /*if ( proc_table[i].priority == 0) continue;//系统资源跳过*/
        printf("        %d           %s            %d                yes\n", proc_table[i].pid, proc_table[i].name, proc_table[i].priority);
    }
    printf("=============================================================================\n");
}

//游戏运行库
unsigned int _seed2 = 0xDEADBEEF;

void srand(unsigned int seed){
	_seed2 = seed;
}

int rand() {
	int next = _seed2;
	int result;

	next *= 1103515245;
	next += 12345;
	result = (next / 65536) ;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (next / 65536) ;

	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (next / 65536) ;

	_seed2 = next;

	return result>0 ? result : -result;
}
/*****************************************************************************
 *                                game code for Minesweeper
 *****************************************************************************/
#define rows 11
#define cols 11
#define Count 10

char mine[rows][cols];
char show[rows][cols];

void m_init()
{
	int i = 0;
	int j = 0;
	for (i = 0; i < rows - 1; i++)
	{
		for (j = 0; j < cols - 1; j++)
		{
			mine[i][j] = '0';
			show[i][j] = '*';
		}
	}
}

void m_set_mine()
{
    int x = 0;
    int y = 0;
    int count = 10;//雷总数
    while (count)//雷布完后跳出循环
    {
        int x = rand() % 10 + 1;//产生1到10的随机数，在数组下标为1到10的范围内布雷
        int y = rand() % 10 + 1;//产生1到10的随机数，在数组下标为1到10的范围内布雷
        if (mine[x][y] == '0')//找不是雷的地方布雷
        {
            mine[x][y] = '1';
            count--;
        }
    }
}


void m_display(char a[rows][cols])
{
	clear();
	printf("   |=========================================================================|\n");
	printf("   |======================= Minesweeper for CYTUZ ===========================|\n");
	printf("   |=========================================================================|\n");
	printf("   |                 A long time ago in a galaxy far,far away...             |\n");
	printf("   | YOU need to sweep Imperial minefield to deliver Death Star blueprints!  |\n");
	printf("   |                   The fate of galaxy is at your hands!                  |\n");
	printf("   |                           Enter q to quit                               |\n");
	printf("   |=========================================================================|\n");
	printf("   |=========================================================================|\n");
	int i = 0;
	int j = 0;
	printf("\n");
	printf(" ");
	for (i = 1; i < cols - 1; i++)
	{
		printf(" %d ", i);
	}
	printf("\n");
	for (i = 1; i < rows - 1; i++)
	{
		printf("%d", i);
		for (j = 1; j < cols - 1; j++)
		{
			printf(" %c ", a[i][j]);
		}
		printf("\n");
	}
}

  
int m_get_num(int x, int y)
{
	int count = 0;
	if (mine[x - 1][y - 1] == '1')
	{
		count++;
	}
	if (mine[x - 1][y] == '1')
	{
		count++;
	}
	if (mine[x - 1][y + 1] == '1')
	{
		count++;
	}
	if (mine[x][y - 1] == '1')  
	{
		count++;
	}
	if (mine[x][y + 1] == '1')  
	{
		count++;
	}
	if (mine[x + 1][y - 1] == '1')  
	{
		count++;
	}
	if (mine[x + 1][y] == '1')  
	{
		count++;
	}
	if (mine[x + 1][y + 1] == '1')  
	{
		count++;
	}
	return  count;
}
  
int m_sweep()
{
	int count = 0;
	int x = 0, y = 0;
	char cx[2], cy[2];
	while (count != ((rows - 2)*(cols - 2) - Count))
	{
		printf("Please input row number: ");
		int r = read(0, cx, 2);
		if (cx[0] == 'q')
			return 0;
		x = cx[0] - '0';
		while (x <= 0 || x > 9)
		{
			printf("Wrong Input!\n");
			printf("Please input row number: ");
			r = read(0, cx, 2);
			if (cx[0] == 'q')
				return 0;
			x = cx[0] - '0';
		}

		printf("Please input col number: ");
		r = read(0, cy, 2);
		if (cy[0] == 'q')
			return 0;
		y = cy[0] - '0';
		while (y <= 0 || y > 9)
		{
			printf("Wrong Input!\n");
			printf("Please input col number: ");
			r = read(0, cy, 2);
			if (cy[0] == 'q')
				return 0;
			y = cy[0] - '0';
		}

		if (mine[x][y] == '1')
		{
			m_display(mine);
			printf("Nerf this! Game Over!\n");

			return 0;
		}
		else
		{
			int ret = m_get_num(x, y);
			show[x][y] = ret + '0';
			m_display(show);
			count++;
		}
	}
	printf("YOU WIN!\n");
	m_display(mine);
	return 0;
}

int runMine(fd_stdin, fd_stdout)
{
	printf("   |=========================================================================|\n");
	printf("   |======================= Minesweeper for CYTUZ ===========================|\n");
	printf("   |=========================================================================|\n");
	printf("   |                 A long time ago in a galaxy far,far away...             |\n");
	printf("   | YOU need to sweep Imperial minefield to deliver Death Star blueprints!  |\n");
	printf("   |                   The fate of galaxy is at your hands!                  |\n");
	printf("   |                           Enter q to quit                               |\n");
	printf("   |=========================================================================|\n");
	printf("   |=========================================================================|\n");
        
	
	

	m_init();
	m_set_mine();
	m_display(show);
	m_sweep();

	printf("\nEnter anything to continue...");
	char rdbuf[128];
	int r = read(fd_stdin, rdbuf, 70);
	rdbuf[r] = 0;
	while (r < 1)
	{
		r = read(fd_stdin, rdbuf, 70);
	}
	clear();
	printf("\n");
	return 0;
}

/*======================================================================*
                            snake game
 *======================================================================*/

int quitSnake = 0;  

PUBLIC void judgeInpt(u32 key) 
{
    char output[2] = {'\0', '\0'};
    if (!key) 
    {
        output[0] = key & 0xFF;
        if(output[0] == 'a') changeToLeft();
        if(output[0] == 's') changeToDown();
        if(output[0] == 'd') changeToRight();
        if(output[0] == 'w') changeToUp();
    }
}

int listenerStart = 0;

struct Snake
{   //every node of the snake 
    int x, y;  
    int now;   //0,1,2,3 means left right up down   
}Snake[15*35];  //Snake[0] is the head，and the other nodes are recorded in inverted order，eg: Snake[1] is the tail


//change the direction of circle
void changeToLeft()
{
    if(listenerStart == 1)
    {
        Snake[0].now = 0;
        listenerStart = 0;
    }
}

void changeToDown()
{
    if(listenerStart == 1)
    {
        Snake[0].now = 3;
        listenerStart = 0;
    }
}

void changeToRight()
{
    if(listenerStart == 1)
    {
        Snake[0].now = 1;
        listenerStart = 0;
    }
}
void changeToUp()
{
    if(listenerStart == 1)
    {
        Snake[0].now = 2;
        listenerStart = 0;
    }
}

const int mapH = 15;
const int mapW = 35;
char sHead = '@';
char sBody = 'O';
char sFood = '$';
char sNode = '.';    
char Map[15][35]; // the map of snake
int food[15][2] = {{5, 13},{6, 10}, {17, 15}, {8, 9}, {3, 4}, {1,12}, {0, 2}, {5, 23},
                   {15, 13},{16, 10}, {7, 15}, {8, 19}, {3, 14}, {11,12}, {10, 2}};
int foodNum = 0;
int eat = -1;
int win = 15;  // the length of win
 
int sLength = 1;
int overOrNot = 0;
int dx[4] = {0, 0, -1, 1};  
int dy[4] = {-1, 1, 0, 0}; 

void initGame(); 
void initFood();
void s_show();
void move();
void checkBorder();
void checkHead(int x, int y);
void action();
void showGameSuccess();
void showGameSuccess();
/**
 * enter the snake game
 */
void snakeGame()
{
    clear();
    initGame();  
    s_show(); 
}
/**
 * init game
 */
void initGame() 
{
    printf("===============================================================================\n");
    printf("                                        Snake                                  \n");
    printf("===============================================================================\n");
    printf("                                                                               \n");
    printf("                     press 'A''D''W''S' to control snake's direction.          \n");
    printf("                                                                               \n");
    printf("===============================================================================\n\n");

    int i, j;  
    int headx = 5;
    int heady = 10;
    memset(Map, '.', sizeof(Map));   //init map with '.'
    Map[headx][heady] = sHead;  
    Snake[0].x = headx;  
    Snake[0].y = heady;  
    Snake[0].now = -1;
    initFood();   //init target 
    for(i = 0; i < mapH; i++)
    {
        for(j = 0; j < mapW; j++) printf("%c", Map[i][j]);  
        printf("\n");  
    } 
   
    listenerStart = 1;
    while(listenerStart);
}

/**
 * the food location
 */
void initFood()
{
    int fx, fy;
    int tick;  
    while(1)
    {
        tick = get_ticks();
        fx = tick%mapH;
        fy = tick%mapW;     
        if(Map[fx][fy] == '.')
        {
            eat++;
            Map[fx][fy] = sFood;  
            break;  
        }
        foodNum++;
    }
}

/**
 * show game situation
 */
void s_show()
{
    int i, j; 
    printf("Load snake game ...");
    while(1)
    {
        listenerStart = 1;

        if(eat < 5)
        {
            milli_delay(8000);
        }
        else if(eat < 10)
        {
            milli_delay(6000);
        }
        else
        {
            milli_delay(5000);
        }
        move();  
        if(overOrNot || quitSnake) 
        {
            showGameOver();
            milli_delay(50000);
            clear();
            break;  
        } 
        if(eat == win)
        {
            showGameSuccess();
            milli_delay(50000);
            clear();
            break;
        }
        clear();
        for(i = 0; i < mapH; i++)   
        {   
            for(j = 0; j < mapW; j++)printf("%c", Map[i][j]);  
            printf("\n");  
        }  

        printf("           Have fun!\n");
        printf("       You have ate:%d\n",eat);
    }  
}
/**
 * snake move function
 */
void move()
{
    int i, x, y;  
    int t = sLength;
    x = Snake[0].x;  
    y = Snake[0].y;  
    Snake[0].x = Snake[0].x + dx[Snake[0].now]; 
    Snake[0].y = Snake[0].y + dy[Snake[0].now];  

    Map[x][y] = '.'; 
    checkBorder(); 
    checkHead(x, y);   
    if(sLength == t)  //did not eat
        for(i = 1; i < sLength; i++)
        {
            if(i == 1) Map[Snake[i].x][Snake[i].y] = '.'; //tail 
     
            if(i == sLength-1)  //the node after the head 
            {  
                Snake[i].x = x;  
                Snake[i].y = y;  
                Snake[i].now = Snake[0].now;  
            }  
            else 
            {  
                Snake[i].x = Snake[i+1].x;  
                Snake[i].y = Snake[i+1].y;  
                Snake[i].now = Snake[i+1].now;  
            }  
            Map[Snake[i].x][Snake[i].y] = sBody;  
        }  
}
/**
 *
 */
void checkBorder()
{
    if(Snake[0].x < 0 || Snake[0].x >= mapH || Snake[0].y < 0 || Snake[0].y >= mapW)
    {
        printl("game over!");
        overOrNot = 1;  
    }
    
}
/**
 *
 * @param x
 * @param y
 */
void checkHead(int x, int y)
{
    if(Map[Snake[0].x][Snake[0].y] == '.') Map[Snake[0].x][Snake[0].y] = sHead ;  
    else if(Map[Snake[0].x][Snake[0].y] == sFood) 
    {
        Map[Snake[0].x][Snake[0].y] = sHead ;    
        Snake[sLength].x = x;                //new node
        Snake[sLength].y = y;  
        Snake[sLength].now = Snake[0].now;  
        Map[Snake[sLength].x][Snake[sLength].y] = sBody;   
        sLength++;  
        initFood();  
    }  
    else
    { 
        overOrNot = 1; 
    }
}
/**
 *
 */
void showGameOver()
{
    printf("===============================================================================\n");
    printf("                                  Game Over                                    \n");
    printf("                                  will exit soon...                            \n");
    printf("===============================================================================\n");
}

/**
 *
 */
void showGameSuccess()
{
    printf("===============================================================================\n");
    printf("                               Congratulation!                                 \n");
    printf("                                  will exit soon...                            \n");
    printf("===============================================================================\n");
}


void calculator()
{
    printf("===============================================================================\n");
    printf("                                     Calculator                                \n");
    printf("===============================================================================\n");
    printf("                                                                               \n");
    printf("                                   Enter e to quit                             \n");
    printf("                                                                               \n");
    printf("===============================================================================\n\n");
    while(1)
    {   
        char result;

        char bufr[128];
        read(0, bufr, 128);


        switch(bufr[1])
        {

            case '+':result=(bufr[0] - '0')+(bufr[2] - '0');break;

            case '-':result=(bufr[0] - '0')-(bufr[2] - '0');break;

            case '*':result=(bufr[0] - '0')*(bufr[2] - '0');break;

            case '/':result=(bufr[0] - '0')/(bufr[2] - '0');break;
        
        }
        if(bufr[0]=='e') break;
        else printf("%d %c %d = %d\n",(bufr[0] - '0'),bufr[1],(bufr[2] - '0'),result);
    }
}
/*========================================================================*
                            Gobang game
 *=========================================================================*/

#define N  9
int chessboard[N + 1][N + 1] = { 0 };

//用来记录轮到玩家1还是玩家2
int whoseTurn = 0;

void initGame(void);
void printChessboard(void);
void playChess(void);
int GobangJudge(int, int);
int stop=0;

void Gobang()
{
    //初始化游戏
    printf("===============================================================================\n");
    printf("                                       Gobang                                  \n");
    printf("===============================================================================\n");
    printf("                                                                               \n");
    printf("                      Enter 'e' to quit, enter x and y to put chess.           \n");
    printf("                                                                               \n");
    printf("===============================================================================\n\n");
    //清空棋盘
    int i, j;
    for (i = 0; i <= N; i++)
        for (j = 0; j <= N; j++)
        {
            chessboard[i][j] = 0;
        }
    printChessboard();

    whoseTurn = 0;
    stop=0;
    chessboard[N + 1][N + 1] =0;
    printf("Player1:");
    while (1)
    {
        //玩家轮流下子
        whoseTurn++;
        playChess();
        if(stop==1)break;
    }
    stop=0;
    return 0;
}

void printChessboard(void)
{
    int i, j;

    for (i = 0; i <= N; i++)
    {
        for (j = 0; j <= N; j++)
        {
            if (0 == i) printf("%3d", j);
            else if (j == 0) printf("%3d", i);
            else if (1 == chessboard[i][j]) printf("  F");
            else if (2 == chessboard[i][j]) printf("  T");
            else printf("  -");
        }
        printf("\n");
    }
}

void playChess(void)
{
    int i, j;
    char bufr[128];
    read(0, bufr, 128);

    if (1 == whoseTurn % 2)
    {
        
        i = bufr[0] - '0';
        j = bufr[2] - '0';
        chessboard[i][j] = 1;
    }
    if (0 == whoseTurn % 2)
    {
        
        i = bufr[0] - '0';
        j = bufr[2] - '0';
        chessboard[i][j] = 2;
    }

    if(bufr[0]=='e') stop=1;
    else
    {
        printChessboard();
        if (1 == whoseTurn % 2) printf("Player2:");
        else printf("Player1:");
    }
    if (GobangJudge(i, j))
    {
        if (1 == whoseTurn % 2) printf("Player1 win!\n");
        else printf("Player2 win!\n");
        stop=1;
    }
    
}

int GobangJudge(int x, int y)
{
    int i, j;
    int t = 2 - whoseTurn % 2;

    for (i = x - 4, j = y; i <= x; i++)
    {
        if (i >= 1 && i <= N - 4 && t == chessboard[i][j] && t == chessboard[i + 1][j] && t == chessboard[i + 2][j] && t == chessboard[i + 3][j] && t == chessboard[i + 4][j])
            return 1;
    }
    for (i = x, j = y - 4; j <= y; j++)
    {
        if (j >= 1 && j <= N - 4 && t == chessboard[i][j] && t == chessboard[i][j + 1] && t == chessboard[i][j + 1] && t == chessboard[i][j + 3] && t == chessboard[i][j + 4])
            return 1;
    }
    for (i = x - 4, j = y - 4; i <= x, j <= y; i++, j++)
    {
        if (i >= 1 && i <= N - 4 && j >= 1 && j <= N - 4 && t == chessboard[i][j] && t == chessboard[i + 1][j + 1] && t == chessboard[i + 2][j + 2] && t == chessboard[i + 3][j + 3] && t == chessboard[i + 4][j + 4])
            return 1;
    }
    for (i = x + 4, j = y - 4; i >= 1, j <= y; i--, j++)
    {
        if (i >= 1 && i <= N - 4 && j >= 1 && j <= N - 4 && t == chessboard[i][j] && t == chessboard[i - 1][j + 1] && t == chessboard[i - 2][j + 2] && t == chessboard[i - 3][j + 3] && t == chessboard[i - 4][j + 4])
            return 1;
    }

    return 0;
}

 /*****************************************************************************
 *                                shell
 *****************************************************************************/
void shell(char *tty_name){
	 int fd;

    //int isLogin = 0;

    char rdbuf[512];
    char cmd[512];
    char arg1[512];
    char arg2[512];
    char buf[1024];
    char temp[512];
   
          
    

    int fd_stdin  = open(tty_name, O_RDWR);
    assert(fd_stdin  == 0);
    int fd_stdout = open(tty_name, O_RDWR);
    assert(fd_stdout == 1);

    //animation();
    clear();
    //animation_l();
    
     displayWelcomeInfo();
   
    
   	

   char current_dirr[512] = "/";
   
    while (1) {  
        //清空数组中的数据用以存放新数据
        clearArr(rdbuf, 512);
        clearArr(cmd, 512);
        clearArr(arg1, 512);
        clearArr(arg2, 512);
        clearArr(buf, 1024);
        clearArr(temp, 512);

        printf("~%s$ ", current_dirr);

        int r = read(fd_stdin, rdbuf, 512);

        if (strcmp(rdbuf, "") == 0)
            continue;

        //解析命令
        int i = 0;
        int j = 0;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            cmd[i] = rdbuf[i];
            i++;
        }
        i++;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            arg1[j] = rdbuf[i];
            i++;
            j++;
        }
        i++;
        j = 0;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            arg2[j] = rdbuf[i];
            i++;
            j++;
        }
        //清空缓冲区
        rdbuf[r] = 0;

        if (strcmp(cmd, "process") == 0)
        {
            ProcessManage();
        }
        else if (strcmp(cmd, "help") == 0)
        {
            help();
        }      
        else if (strcmp(cmd, "clear") == 0)
        {
            printTitle();
        }
        else if (strcmp(cmd, "snake") == 0)
        {
        	snakeGame();
        }
        else if (strcmp(cmd, "gobang") == 0)
        {
        	Gobang();
        }
        else if (strcmp(cmd, "ls") == 0)
        {
            ls(current_dirr);
        }
        else if (strcmp(cmd, "create") == 0)
        {
            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }

            fd = open(arg1, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                printf("Failed to create file! Please check the filename!\n");
                continue ;
            }
            write(fd, buf, 1);
            printf("File created: %s (fd %d)\n", arg1, fd);
            close(fd);
        }
        else if (strcmp(cmd, "cat") == 0)
        {
            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }

            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("Failed to open file! Please check the filename!\n");
                continue ;
            }
            if (!verifyFilePass(arg1, fd_stdin))
            {
                printf("Authorization failed\n");
                continue;
            }
            read(fd, buf, 1024);
            close(fd);
            printf("%s\n", buf);
        }
        else if (strcmp(cmd, "vi") == 0)
        {
            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }

            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("Failed to open file! Please check the filename!\n");
                continue ;
            }
            if (!verifyFilePass(arg1, fd_stdin))
            {
                printf("Authorization failed\n");
                continue;
            }
            int tail = read(fd_stdin, rdbuf, 512);
            rdbuf[tail] = 0;

            write(fd, rdbuf, tail+1);
            close(fd);
        }
        else if (strcmp(cmd, "delete") == 0)
        {

            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }

            int result;
            result = unlink(arg1);
            if (result == 0)
            {
                printf("File deleted!\n");
                continue;
            }
            else
            {
                printf("Failed to delete file! Please check the filename!\n");
                continue;
            }
        } 
        else if (strcmp(cmd, "cp") == 0)
        {
            //首先获得文件内容
            if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
            
            int tail = read(fd, buf, 1024);
            close(fd);

            if(arg2[0]!='/'){
                addTwoString(temp,current_dirr,arg2);
                memcpy(arg2,temp,512);                
            }
            
            fd = open(arg2, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                //文件已存在，什么都不要做
            }
            else
            {
                //文件不存在，写一个空的进去
                char temp2[1024];
                temp2[0] = 0;
                write(fd, temp2, 1);
                close(fd);
            }
             
            //给文件赋值
            fd = open(arg2, O_RDWR);
            write(fd, buf, tail+1);
            close(fd);
        } 
        else if (strcmp(cmd, "mv") == 0)
        {
             if(arg1[0]!='/'){
                addTwoString(temp,current_dirr,arg1);
                memcpy(arg1,temp,512);                
            }
            //首先获得文件内容
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
           
            int tail = read(fd, buf, 1024);
            close(fd);

            if(arg2[0]!='/'){
                addTwoString(temp,current_dirr,arg2);
                memcpy(arg2,temp,512);                
            }
            
            fd = open(arg2, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                //文件已存在，什么都不要做
            }
            else
            {
                //文件不存在，写一个空的进去
                char temp2[1024];
                temp2[0] = 0;
                write(fd, temp2, 1);
                close(fd);
            }
             
            //给文件赋值
            fd = open(arg2, O_RDWR);
            write(fd, buf, tail+1);
            close(fd);
            //最后删除文件
            unlink(arg1);
        }   
        else if (strcmp(cmd, "encrypt") == 0)
        {
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
            if (!verifyFilePass(arg1, fd_stdin))
            {
                printf("Authorization failed\n");
                continue;
            }
            doEncrypt(arg1, fd_stdin);
        } 
        else if (strcmp(cmd, "test") == 0)
        {
            doTest(arg1);
        }
        else if (strcmp(cmd, "minesweeper") == 0){
        	runMine(fd_stdin, fd_stdout);
        }
        else if (strcmp(cmd, "mkdir") == 0){
            i = j =0;
            while(current_dirr[i]!=0){
                arg2[j++] = current_dirr[i++];
            }
            i = 0;
            
            while(arg1[i]!=0){
                arg2[j++]=arg1[i++];
            }
            arg2[j]=0;
            printf("%s\n", arg2);
            fd = mkdir(arg2);            
        }
        else if (strcmp(cmd, "cd") == 0){
            if(arg1[0]!='/'){ // not absolute path from root
                i = j =0;
                while(current_dirr[i]!=0){
                    arg2[j++] = current_dirr[i++];
                }
                i = 0;
               
                while(arg1[i]!=0){
                    arg2[j++]=arg1[i++];
                }
                arg2[j++] = '/';
                arg2[j]=0;
                memcpy(arg1, arg2, 512);
		
            }
            else if(strcmp(arg1, "/")!=0){
                for(i=0;arg1[i]!=0;i++){}
                arg1[i++] = '/';
                arg1[i] = 0;
            }
            printf("%s\n", arg1);
	    int nump=0;
           char eachf[512];
		memset(eachf,0,sizeof(eachf)); 
		const char *s = arg1;
		char *t = eachf;
				if(arg1[0]=="/")
					*s++;
				while (*s) {	
					*t++ = *s++;
				}
		t = eachf;
		while (*t) {	
			char *ps = t+1;
			if(*ps==0){
				if(*t == '/')
				*t = 0;			
			}		
			t++;
		}	
            fd = open(eachf, O_RDWR);
            printf("%s\n", eachf);
            if(fd == -1){
                printf("The path not exists!Please check the pathname!\n");
            }
            else{
                memcpy(current_dirr, arg1, 512);
                printf("Change dir %s success!\n", current_dirr);
            }
        }
        else
            printf("Command not found, please check!\n");
            
    }
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{

	
	//while(1);
	char tty_name[] = "/dev_tty0";
	shell(tty_name);
	assert(0); /* never arrive here */
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	char tty_name[] = "/dev_tty1";
	shell(tty_name);
	
	assert(0); /* never arrive here */
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	//char tty_name[] = "/dev_tty2";
	//shell(tty_name);
	spin("TestC");
	/* assert(0); */
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

