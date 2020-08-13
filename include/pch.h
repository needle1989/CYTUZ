// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件

#ifndef PCH_H
#define PCH_H
#include <iostream>
#include<graphics.h>
#include <deque>
#include<mmsystem.h>
#include<conio.h>
#include<time.h>
#include<stdlib.h>
#include<windows.h>
#include<fstream>
#include<string.h>
#include <tchar.h>
#pragma comment(lib, "winmm.lib")
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)
#define n_smlg 4
#define n_L 680
#define n_W 480
#define button_id 10010
using namespace std;
void keyboard();
void initsnake();
void drawsnake();
void movesnake();
void Locfood();
void Drawfood();
void Eatfood();
void locbrick();
void drawbrick();
void encounter();
void spower();
void eatpower();
void end1();
void gscore();
void drawscore();
void output_file();
void input_file();
void end2();
void error_solution();
void main_screen();
void drawrank(int);
void mouses();
void drawbutton();
// TODO: 添加要在此处预编译的标头

#endif //PCH_H
