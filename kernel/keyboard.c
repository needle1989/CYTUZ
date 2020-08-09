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
#include "keyboard.h"
#include "keymap.h"
#include "proto.h"

// 键盘键入状态变量
PRIVATE int shiftL;
PRIVATE int shiftR;
PRIVATE int ctrlL;
PRIVATE int ctrlR;
PRIVATE int altL;
PRIVATE int altR;
PRIVATE int capsLock;
PRIVATE int numLock;
PRIVATE int column;

PRIVATE struct kbInbuf kbIn;
PRIVATE int codeE0;


// 缓冲区处理
PUBLIC void kbHandler(int irq)
{
    u8 code = in_byte(KB_DATA);
    // 扫一遍缓冲区
    if(kbIn.count < KB_IN_BYTES){
        *(kbIn.head) = code;
        kbIn.head++;
        if(kbIn.head == kbIn.buf + KB_IN_BYTES){
            kbIn.head = kbIn.buf;
        }
        kbIn.count++;
    }
    key_pressed = 1;
}

// keyboard相关初始化
PUBLIC void kbInitial()
{
    kbIn.count = 0;
    kbIn.head = kbIn.tail = kbIn.buf;
    
    shiftL = shiftR = ctrlL = ctrlR = altL = altR = 0;
    capsLock = numLock = 0;
    column = 0;

    setLeds();

    put_irq_handler(KEYBOARD_IRQ, kbHandler);
    enable_irq(KEYBOARD_IRQ);
}

// 缓冲区读操作
PUBLIC void kbRead(TTY* tty)
{
    // 扫描码
    u8 code;

    // 1: make
    // 0: break
    int make;

    // 循环处理用的键位记录
    u32 key;

    u32* keyRow;

    while(kbIn.count > 0){
        codeE0 = 0;
        code = getByteFromKbBuffer();

        // 0xE1和0xE0单独处理，双字节编码
        if(code == 0xE0){
            codeE0 = 1;
            code = getByteFromKbBuffer();
            // printScreen键 pressed
            if(code == 0x2A){
                codeE0 = 0;
                if((code = getByteFromKbBuffer()) == 0xE0){
                    codeE0 = 1;
                    if((code = getByteFromKbBuffer()) == 0x37){
                        key = PRINTSCREEN;
                        make = 1;
                    }
                }
            }
            // release
            else if(code == 0xB7){
                codeE0 = 0;
                if((code = getByteFromKbBuffer()) == 0xE0){
                    codeE0 = 1;
                    if((code = getByteFromKbBuffer()) == 0xAA){
                        key = PRINTSCREEN;
                        make = 0;
                    }
                }
            }
        }
        else if(code == 0xE1){
            u8 pauseCodeArr[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};
            int isPause = 1;
            for(int i = 0; i < 6; ++i){
                if(getByteFromKbBuffer() != pauseCodeArr[i]){
                    isPause = 0;
                    break;
                }
            }
            if(isPause){
                key = PAUSEBREAK;
            }
        }

        if((key != PAUSEBREAK) && (key != PRINTSCREEN)){
            if(code & FLAG_BREAK){
                make = 0;
            }
            else make = 1;
            keyRow = &keymap[(code & 0x7F) * MAP_COLS];
            column = 0;

            // 大小写
            int caps = shiftL || shiftR;
            if(capsLock && keyRow[0] >= 'a' && keyRow[0] <= 'z'){
                caps = !caps;
            }
            
            if(caps)   column = 1;
            if(codeE0) column = 2;

            key = keyRow[column];

            switch(key){
            case SHIFT_L: shiftL = make; break;
            case SHIFT_R: shiftR = make;break;

            case CTRL_L: ctrlL = make; break;
            case CTRL_R: ctrlR = make; break;

            case ALT_L: altL = make; break;
            case ALT_R: altR = make; break;

            case CAPS_LOCK:
                if(make){
                    capsLock = !capsLock;
                    setLeds();
                }
                break;
            case NUM_LOCK:
                if(make){
                    numLock = !numLock;
                    setLeds();
                }
                break;
            default: break;
            }
        }

        // pressed code
        if(make){
            int pad = 0;

            if((key >= PAD_SLASH) && (key <= PAD_9)){
                pad = 1;
                switch(key){
                case PAD_SLASH: key = '/';   break;
                case PAD_STAR:  key = '*';   break;
                case PAD_MINUS: key = '-';   break;
                case PAD_PLUS:  key = '+';   break;
                case PAD_ENTER: key = ENTER; break;
                default:
                    if(numLock){
                        if(key >= PAD_0 && key <= PAD_9){
                            key = key - PAD_0 + '0';
                        }
                        else if(key == PAD_DOT){
                            key = '.';
                        }
                    }
                    else{
                        switch(key){
                        case PAD_HOME:      key = HOME;     break;
                        case PAD_END:       key = END;      break;
                        case PAD_PAGEUP:    key = PAGEUP;   break;
                        case PAD_PAGEDOWN:  key = PAGEDOWN; break;
                        case PAD_UP:        key = UP;       break;
                        case PAD_DOWN:      key = DOWN;     break;
                        case PAD_LEFT:      key = LEFT;     break;
                        case PAD_RIGHT:     key = RIGHT;    break;
                        case PAD_DOT:       key = DELETE;   break;
                        case PAD_INS:       key = INSERT;   break;
                        default: break;
                        }
                    }
                    break;
                }
            }
            key |= shiftL	? FLAG_SHIFT_L	: 0;
			key |= shiftR	? FLAG_SHIFT_R	: 0;
			key |= ctrlL	? FLAG_CTRL_L	: 0;
			key |= ctrlR	? FLAG_CTRL_R	: 0;
			key |= altL	    ? FLAG_ALT_L	: 0;
			key |= altR 	? FLAG_ALT_R	: 0;
			key |= pad	    ? FLAG_PAD	    : 0;

            in_process(tty, key);
        }
    }
}


PRIVATE u8 getByteFromKbBuffer()
{
    u8 code;

    // 等待输入
    while(kbIn.count <= 0);

    disable_int();
    code = *(kbIn.tail);
    kbIn.tail++;
    if(kbIn.tail == kbIn.buf + KB_IN_BYTES){
        kbIn.tail = kbIn.buf;
    }
    kbIn.count--;
    enable_int();

    return code;
}

// 等待8042缓冲区清空
PRIVATE void kbWait()
{
    u8 kbStat = in_byte(KB_CMD);
    while(kbStat & 0x02){
        kbStat = in_byte(KB_CMD);
    }
}

// 读，直到接收 KB_ACK 
PRIVATE void kbAck()
{
    u8 kbStat = in_byte(KB_DATA);
    while(kbStat != KB_ACK){
        kbStat = in_byte(KB_DATA);
    }
}

PRIVATE void setLeds()
{
    u8 leds = (capsLock << 2) | (numLock << 1);

    kbWait();
    out_byte(KB_DATA, LED_CODE);
    kbAck();

    kbWait();
    out_byte(KB_DATA, leds);
    kbAck();
}
