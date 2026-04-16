/* Julius Dimaranan, jdima016@ucr.edu
   Discussion Section 23
   Custom Lab Project
   
   I acknowledge all content contained herein,
   excluding template or example code, is my 
   own original work.
   
   Demo Link: https://youtu.be/AtWQCDrdzUA  

*/
//timer2- timerISR
//timer1- passive buzzers
//timer0- IR remote

#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
#include "LCD.h"
#include "irAVR.h"


const char *str1   = "Note: ";
const char *str2   = "REC: ";
const char *str3   = "BPM:";
const char *on     = "ON";
const char *off    = "OFF";
const char *clr    = "       ";
const char *clr2   = "   ";
const char *bpm1   = "50";
const char *bpm2   = "75";
const char *bpm3   = "100";
const char *bpm4   = "125";
bool tmp1=0;
bool tmp2=0;
bool tmp3=0;
unsigned char cnt=0;
long time=0;
bool trackSaved=0;
bool recording=0;
bool playTrack=0;
bool met=0;
unsigned char j=0; //led duty timer
double k=0;
bool shift,key1,key2,key3,key4,key5,key6,key7,key8,key9,key10,key11,key12;
unsigned char Track[1550];
long Tracklngth;
unsigned char dataArr[15];
unsigned char timeSig=4; //2 - 2/4, 3 - 3/4, 4 - 4/4
bool recordCountIn=0;
int bpm=2; //1 - 50, 2 - 75, 3 - 100, 4 - 125
int bpmInbpm=75;
int beatMS;
unsigned char note;
decode_results results;


#define NUM_TASKS 8


//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


const unsigned long REMOTE_PERIOD = 500;
const unsigned long LCD_PERIOD = 250;
const unsigned long KEYS_PERIOD = 25;
const unsigned long BUZZER1_PERIOD = 25;
const unsigned long BUZZER2_PERIOD = 25;
const unsigned long LED_PERIOD   = 250;
const unsigned long SHIFTREG_PERIOD = 25;
const unsigned long MET_PERIOD = 1;
unsigned long gcdtemp=findGCD(REMOTE_PERIOD,findGCD(LCD_PERIOD,findGCD(KEYS_PERIOD,findGCD(BUZZER1_PERIOD,findGCD(LED_PERIOD,findGCD(SHIFTREG_PERIOD,findGCD(MET_PERIOD,BUZZER2_PERIOD)))))));
const unsigned long GCD_PERIOD = gcdtemp;

task tasks[NUM_TASKS]; // declared task array with  tasks

void checkShift(){
    if(dataArr[11])
        shift=1;
    else if(dataArr[11]==0)
        shift=0;
}

void countInEnd(){
    recordCountIn=0;
    cnt=0;
    recording=1;
    time=0;
    trackSaved=1;
}

void dispBPM(unsigned int bpm){
    lcd_goto_xy(1,12);
    if(bpm==1){
        lcd_write_str((char*)bpm1);
        bpmInbpm=50;
        lcd_write_character(' ');
    }
    else if(bpm==2){
        lcd_write_str((char*)bpm2);
        bpmInbpm=75;
        lcd_write_character(' ');
    }
    else if(bpm==3){
        lcd_write_str((char*)bpm3);
        bpmInbpm=100;
    }
    else if(bpm==4){
        lcd_write_str((char*)bpm4);
        bpmInbpm=125;
    }

}

void newNoteLCD(){
    if(shift)
        lcd_write_character('5');
    else
        lcd_write_character('4');
    if(key1||key3||key5||key6||key8||key10||key12)
        lcd_write_character(' ');
}

double findICR1(int fwant){
    return ((16000000-(8*fwant))/(8*fwant));
}


void playTrk(unsigned char buzzTemp){
    if(buzzTemp==1)
        ICR1=findICR1(261.63);
    else if(buzzTemp==2)
        ICR1=findICR1(277.18);
    else if(buzzTemp==3)
        ICR1=findICR1(293.66);
    else if(buzzTemp==4)
        ICR1=findICR1(311.13);
    else if(buzzTemp==5)
        ICR1=findICR1(329.63);
    else if(buzzTemp==6)
        ICR1=findICR1(349.23);
    else if(buzzTemp==7)
        ICR1=findICR1(369.99);
    else if(buzzTemp==8)
        ICR1=findICR1(392);
    else if(buzzTemp==9)
        ICR1=findICR1(415.3);
    else if(buzzTemp==10)
        ICR1=findICR1(440);        
    else if(buzzTemp==11)
        ICR1=findICR1(466.16);
    else if(buzzTemp==12)
        ICR1=findICR1(493.88);
    else if(buzzTemp==13)
        ICR1=findICR1(523.25);
    else if(buzzTemp==14)
        ICR1=findICR1(554.37);    
    else if(buzzTemp==15)
        ICR1=findICR1(587.33);    
    else if(buzzTemp==16)
        ICR1=findICR1(622.25);        
    else if(buzzTemp==17)
        ICR1=findICR1(659.25);
    else if(buzzTemp==18)
        ICR1=findICR1(698.46);
    else if(buzzTemp==19)
        ICR1=findICR1(739.99);
    else if(buzzTemp==20)
        ICR1=findICR1(783.99);
    else if(buzzTemp==21)
        ICR1=findICR1(830.61);  
    else if(buzzTemp==22)
        ICR1=findICR1(880);
    else if(buzzTemp==23)
        ICR1=findICR1(932.33);
    else if(buzzTemp==24)
        ICR1=findICR1(987.77);
    if(buzzTemp){
        OCR1B=ICR1/2;
    }
    else
        OCR1B=ICR1;
}

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

enum LCDStates{LCDINIT, disp};
int Tick_LCD(int state){
    switch(state){
        case LCDINIT:
        lcd_clear();
        state=disp;
        break;

        case disp:
        state=disp;
        break;

        default:
        break;
    }

    switch(state){

        case disp:
        if(tmp1==0){
            lcd_write_str((char *)str1);
            tmp1=1;
        }
        if(tmp2==0){
            lcd_goto_xy(1,0);
            lcd_write_str((char *)str2);
            tmp2=1;
        }
        
        if(tmp3==0){
            lcd_goto_xy(1,8);
            lcd_write_str((char *)str3);
            lcd_write_str((char*)bpm2);
            tmp3=1;
        }
 
        dispBPM(bpm);

        if(recording){
            lcd_goto_xy(1,4);//lcd_goto_xy(1,4);
            lcd_write_str((char *)on);
            lcd_write_character(' ');
        }
        else{
            lcd_goto_xy(1,4);//lcd_goto_xy(1,4);
            lcd_write_str((char *)off);
        }
        if(key1){
            lcd_goto_xy(0,6);
            lcd_write_character('C');
            lcd_goto_xy(0,7);
            newNoteLCD();

        }
        else if(key2){
            lcd_goto_xy(0,6);
            lcd_write_character('C');
            lcd_goto_xy(0,7);
            lcd_write_character('#');
            newNoteLCD();
        }
        else if(key3){
            lcd_goto_xy(0,6);
            lcd_write_character('D');
            lcd_goto_xy(0,7);
            newNoteLCD();
        }
        else if(key4){
            lcd_goto_xy(0,6);
            lcd_write_character('D');
            lcd_goto_xy(0,7);
            lcd_write_character('#');
            newNoteLCD();
        }
        else if(key5){
            lcd_goto_xy(0,6);
            lcd_write_character('E');
            lcd_goto_xy(0,7);
            newNoteLCD();
        }
        else if(key6){
            lcd_goto_xy(0,6);
            lcd_write_character('F');
            lcd_goto_xy(0,7);
            newNoteLCD();
        }
        else if(key7){
            lcd_goto_xy(0,6);
            lcd_write_character('F');
            lcd_goto_xy(0,7);
            lcd_write_character('#');
            newNoteLCD();
        }
        else if(key8){
            lcd_goto_xy(0,6);
            lcd_write_character('G');
            lcd_goto_xy(0,7);
            newNoteLCD();
        }
        else if(key9){
            lcd_goto_xy(0,6);
            lcd_write_character('G');
            lcd_goto_xy(0,7);
            lcd_write_character('#');
            newNoteLCD();
        }
        else if(key10){
            lcd_goto_xy(0,6);
            lcd_write_character('A');
            lcd_goto_xy(0,7);
            newNoteLCD();
        }
        else if(key11){
            lcd_goto_xy(0,6);
            lcd_write_character('A');
            lcd_goto_xy(0,7);
            lcd_write_character('#');
            newNoteLCD();
        }
        else if(key12){
            lcd_goto_xy(0,6);
            lcd_write_character('B');
            lcd_goto_xy(0,7);
            newNoteLCD();
        }
        else{
            lcd_goto_xy(0,6);
            lcd_write_str((char *)clr);
        }
        if(met||recordCountIn){
            lcd_goto_xy(0,15);
            if(cnt==1)
                lcd_write_character('1');
            else if(cnt==2)
                lcd_write_character('2');
            else if(cnt==3)
                lcd_write_character('3');
            else if(cnt==4)
                lcd_write_character('4');
            }
        else if(!met&&!recordCountIn){
            lcd_goto_xy(0,15);
            lcd_write_character(' ');
        }
        
        
        break;

        default:
        break;
    }

    return state;
}

enum RemoteStates{REMOTEINIT,remote,metronome};
int Tick_Remote(int state){
    switch(state){
        case REMOTEINIT:
        state=remote;
        break;

        case remote:
        if(IRdecode(&results)){
            if(results.value==16753245)
                state=metronome;
            else if(results.value==16748655){
                if(bpm<4)
                    bpm++;
                else
                    bpm=4;
            }
            else if(results.value==16769055){
                if(bpm>1)
                    bpm--;
                else    
                    bpm=1;
            }
            else if(results.value==16716015){
                timeSig=4;
                serial_println("4");
                cnt=0;
            }
            else if(results.value==16743045){
                timeSig=3;
                serial_println("3");
                cnt=0;
            }
            else if(results.value==16718055){
                timeSig=2;
                serial_println("2");
                cnt=0;
            }
        }
        IRresume();
        break;

        case metronome:
        //serial_println(met);
        met=!met;
        serial_println(met);
        state=remote;
        break;

        default:
        break;
    }

    switch(state){

        case remote:       
        //if(IRdecode(&results))
        //    serial_println(results.value);

        //button codes:
        //power - 16753245
        //up arrow - 16748655
        //down arrow - 16769055
        //4 - 16716015
        //3 - 16743045
        //2 - 16718055

        //serial_println(results.value);
        //serial_println(results.bits);

        //IRresume();
        break;

        case metronome:
        break;

        default:
        break;
    }

    return state;
}

enum KeyStates{KEYSINIT,keysIDLE,k1,k2,k3,k4,k5,k6,k7,k8,k9,k10,k11,k12,strendRecord,playRecord};
int Tick_Keys(int state){
    switch(state){
        case KEYSINIT:
        state=keysIDLE;
        break;

        case keysIDLE:
        if(dataArr[12])
            state=k1;
        else if(dataArr[13])
            state=k2;
        else if(dataArr[14])
            state=k3;
        else if((PINC>>4)&0x01)
            state=strendRecord;
        else if(((PINC>>5)&0x01)&&recording==0){
            //serial_println("Play Recording");
            state=playRecord;
        }
        else if(dataArr[10])
            state=k4;
        else if(dataArr[0])
            state=k5;
        else if(dataArr[1])
            state=k6;
        else if(dataArr[2])
            state=k7;
        else if(dataArr[3])
            state=k8;
        else if(dataArr[7])
            state=k9;
        else if(dataArr[6])
            state=k10;
        else if(dataArr[5])
            state=k11;
        else if(dataArr[4])
            state=k12;
        break;

        case strendRecord:
        if(((PINC>>4)&0x01)==0){
            if(recording==1)
                recording=0;
            else
                recordCountIn=1;
            if(recording==0)
                Tracklngth=time;    //save length of recorded track
            state=keysIDLE;

        }
        
        break;

        case playRecord:
        if(((PINC>>5)&0x01)==0){
            playTrack=1;
            state=keysIDLE;
            time=0;
            met=0;
        }
        break;

        case k1:
        if(dataArr[12]==0)
            state=keysIDLE;
        break;

        case k2:
        if(dataArr[13]==0)
            state=keysIDLE;
        break;

        case k3:
        if(dataArr[14]==0)
            state=keysIDLE;
        break;

        case k4:
        if(dataArr[10]==0)
            state=keysIDLE;
        break;

        case k5:
        if(dataArr[0]==0)
            state=keysIDLE;
        break;

        case k6:
        if(dataArr[1]==0)
            state=keysIDLE;
        break;

        case k7:
        if(dataArr[2]==0)
            state=keysIDLE;
        break;

        case k8:               
        if(dataArr[3]==0)
            state=keysIDLE;
        break;

        case k9:
        if(dataArr[7]==0)
            state=keysIDLE;
        break;

        case k10:
        if(dataArr[6]==0)
            state=keysIDLE;
        break;
        
        case k11:
        if(dataArr[5]==0)
            state=keysIDLE;
        break;

        case k12:
        if(dataArr[4]==0)
            state=keysIDLE;
        break;

        default:
        break;
    }

    switch(state){

        case keysIDLE:
        key1=key2=key3=key4=key5=key6=key7=key8=key9=key10=key11=key12=0;
        break;

        
        case k1:
        key1=1;
        break;

        case k2:
        key2=1;
        break;

        case k3:
        key3=1;
        break;

        case k4:
        key4=1;
        break;

        case k5:
        key5=1;
        break;

        case k6:
        key6=1;
        break;

        case k7:
        key7=1;
        break;

        case k8:
        key8=1;
        break;

        case k9:
        key9=1;
        break;

        case k10:
        key10=1;
        break;

        case k11:
        key11=1;
        break;

        case k12:
        key12=1;
        break;

        default:
        break;
    }

    return state;
}

enum Buzzer1States{BUZZER1INIT,buzzer1IDLE, C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B};
int Tick_Buzzer1(int state){
    switch(state){
        case BUZZER1INIT:
        state=buzzer1IDLE;
        break;

        case buzzer1IDLE:
        if(key1)
            state=C;
        else if(key2)
            state=Cs;
        else if(key3)
            state=D;
        else if(key4)
            state=Ds;
        else if(key5)
            state=E;
        else if(key6)
            state=F;
        else if(key7)
            state=Fs;
        else if(key8)
            state=G;
        else if(key9)
            state=Gs;
        else if(key10)
            state=A;
        else if(key11)
            state=As;
        else if(key12)
            state=B;
        break;

        case C:
        if(key1==0)
            state=buzzer1IDLE;
        break;

        case Cs:
        if(key2==0)
            state=buzzer1IDLE;
        break;

        case D:
        if(key3==0)
            state=buzzer1IDLE;
        break;

        case Ds:
        if(key4==0)
            state=buzzer1IDLE;
        break;

        case E:
        if(key5==0)
            state=buzzer1IDLE;
        break;

        case F:
        if(key6==0)
            state=buzzer1IDLE;
        break;

        case Fs:
        if(key7==0)
            state=buzzer1IDLE;
        break;

        case G:
        if(key8==0)
            state=buzzer1IDLE;
        break;

        case Gs:
        if(key9==0)
            state=buzzer1IDLE;
        break;

        case A:
        if(key10==0)
            state=buzzer1IDLE;
        break;

        case As:
        if(key11==0)
            state=buzzer1IDLE;
        break;

        case B:
        if(key12==0)
            state=buzzer1IDLE;
        break;

        default:
        break;
    }

    switch(state){
        //fPWM = fclk / (N * (1+TOP))
        //Fwant = 16/(8*1+TOP)
        //(16000000-(8*Fwant))/(Fwant*8)
        case buzzer1IDLE:
        OCR1A=ICR1;
        if(recording){
            Track[time]=0;
            time++;
        }
        break;

        case C:     //261.63hz/523.25hz
        checkShift();
        if(shift==0){
                ICR1=7643.38;
                OCR1A=ICR1/2;
            if(recording){
                Track[time]=1;
                time++;
            }
        }
        else{
            ICR1=3821.26469183;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=13;
                time++;
            }
        }
        break;

        case Cs:    //277.18hz/554.37hz
        checkShift();
        if(shift==0){
            ICR1=7124.527;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=2;
                time++;
            }
        }
        else{
            ICR1=3606.6988293;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=14;
                time++;
            }
        }
        break;

        case D:    //293.66hz/587.33hz
        checkShift();
        if(shift==0){
            ICR1=6809.597;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=3;
                time++;
            }
        }
        else{
            ICR1=3404.24066538;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=15;
                time++;
            }
        }
        break;

        case Ds:    //311.13hz/622.25hz
        checkShift();
        if(shift==0){
            ICR1=6427.181;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=4;
                time++;
            }
        }
        else{
            ICR1=3213.14222579;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=16;
                time++;
            }
        }
        break;

        case E:    //329.63hz/659.25hz
        checkShift();
        if(shift==0){
            ICR1=6066.408;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=5;
                time++;
            }
        }
        else{
            ICR1=3032.75047402;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=17;
                time++;
            }
        }
        break;

        case F:    //349.23hz/698.46hz
        checkShift();
        if(shift==0){
            ICR1=5725.884;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=6;
                time++;
            }
        }
        else{
            ICR1=findICR1(698.46);
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=18;
                time++;
            }
        }
        break;

        case Fs:    //369.99hz/739.99hz
        checkShift();
        if(shift==0){
            ICR1=5404.551;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=7;
                time++;
            }
        }
        else{
            ICR1=findICR1(739.99);
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=19;
                time++;
            }
        }
        break;

        case G:    //392hz/783.99hz
        checkShift();
        if(shift==0){
            ICR1=5101.04;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=8;
                time++;
            }
        }
        else{
            ICR1=findICR1(783.99);
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=20;
                time++;
            }
        }   
        break;

        case Gs:    //415.3hz/830.61hz
        checkShift();
        if(shift==0){
            ICR1=4814.79581026;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=9;
                time++;
            }
        }
        else{
            ICR1=findICR1(830.61);
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=21;
                time++;
            }
        }   
        break;
        

        case A:    //440hz/880hz
        checkShift();
        if(shift==0){
            ICR1=4544.45454545;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=10;
                time++;
            }
        }
        else{
            ICR1=findICR1(880);
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=22;
                time++;
            }
        }   
        break;

        case As:    //466.16hz/932.33hz
        checkShift();
        if(shift==0){
            ICR1=4289.37240432;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=11;
                time++;
            }
        }
        else{
            ICR1=findICR1(932.33);
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=23;
                time++;
            }
        }   
        break;

        case B:    //493.88hz/987.77hz
        checkShift();
        if(shift==0){
            ICR1=4048.56669636;
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=12;
                time++;
            }
        }
        else{
            ICR1=findICR1(987.77);
            OCR1A=ICR1/2;
            if(recording){
                Track[time]=24;
                time++;
            }
        }   
        break;

        default:
        break;
    }

    return state;
}

enum Buzzer2States{BUZZER2INIT,buzzer2IDLE, buzzer2PlayTrack};
int Tick_Buzzer2(int state){
    switch(state){
        case BUZZER2INIT:
        state=buzzer2IDLE;
        break;

        case buzzer2IDLE:
        if(playTrack){
            state=buzzer2PlayTrack;
            time=0;
        }
        break;

        case buzzer2PlayTrack:
        if(time>=Tracklngth){
            playTrack=0;
            state=buzzer2IDLE;
        }
        break;
    
        default:
        break;
    }

    switch(state){
        //fPWM = fclk / (N * (1+TOP))
        //Fwant = 16/(8*1+TOP)
        //(16000000-(8*Fwant))/(Fwant*8)
        case buzzer2IDLE:
        OCR1B=ICR1;
        break;

        case buzzer2PlayTrack:        
        note=Track[time];
        playTrk(note);
        time++;
        break;
 

        default:
        break;
    }

    return state;
}

enum LEDStates{LEDINIT,LEDIDLE,LEDon};
int Tick_LED(int state){
    switch(state){
        case LEDINIT:
        state=LEDIDLE;
        j=0;
        break;

        case LEDIDLE:
        if(recording)
            state=LEDon;
        else if(trackSaved)
            state=LEDon;
        break;

        case LEDon:
        break;

        default:
        break;
    }

    switch(state){

        case LEDon:
        if(recording){
        if(j==2)
            j=0;
        if(j<1)
            PORTB=PORTB&0x0F;
        else
            PORTB=PORTB|0x10;
        j++;
        }
        else if(trackSaved){
            PORTB=PORTB&0x0F;
            PORTB=PORTB|0x20;
            j=0;
        }
        else 
            PORTB=PORTB&0x0F;

        break;

        default:
        break;
    }

    return state;
}

enum ShiftRegStates{SHIFTREGINIT,readShiftReg};
int Tick_ShiftReg(int state){
    switch(state){
        case SHIFTREGINIT:
        state=readShiftReg;
        break;

        default:
        break;
    }

    switch(state){

        case readShiftReg:
        PORTC=PORTC&0xFB;   //turn latch off then on
        PORTC=PORTC|0x04;

        for(int i=0;i<15;i++){

            dataArr[i]=PINC&0x01;

            PORTC=PORTC|0x02;     //turn clock on then off
            PORTC=PORTC&0xFD;
            
        }
        /*for(int i=0;i<16;i++){
            serial_println(dataArr[i]);

            
        }
        serial_println("");
        */
        //serial_println(met);

       /* buzzI++;        //alternate buzzers
        if(buzzI>1)         //thought audio multiplexing would work but nope
            buzzI=0;
            */
        //serial_println(time);

        if(time==1550){     //end recording if not stopped before reaching max array length
            recording=0;
            trackSaved=1;
            Tracklngth=1550;
        }

        //serial_println(buzzI);
        break;


        default:
        break;
    }

    return state;
}

enum MetStates{METINIT,metOFF,metON,countIn};
int Tick_Met(int state){
    switch(state){
        case METINIT:
        state=metOFF;
        break;

        case metOFF:
        if(recordCountIn){
            cnt=k=0;
            state=countIn;
        }
        else if(met){
            cnt=k=0;
            state=metON;
        }
        break;

        case metON:
        if(recordCountIn){
            cnt=k=0;
            state=countIn;
        }
        else if(!met)
            state=metOFF;
        
        break;

        case countIn:
        if(!recordCountIn){
            if(met)
                state=metON;
            else if(!met)
                state=metOFF;
        }
        break;
            

        default:
        break;
    }

    switch(state){

        case metOFF:
        //serial_println("metOff");
        break;

        case metON:
        //serial_println("metON");
        beatMS=60000/bpmInbpm;
        if(k>=beatMS){
            k=0;
            if(timeSig==2){
                if(cnt==2)
                    cnt=0;
            }
            else if(timeSig==3){
                if(cnt==3)
                    cnt=0;
            }
            else if(timeSig==4){
                if(cnt==4)
                    cnt=0;
            }
            cnt++;
            //serial_println(cnt);

        }
        k++;
        //serial_println(k);
        //serial_println(beatMS);
        break;

        case countIn:
        beatMS=60000/bpmInbpm;
        if(k>=beatMS){
            k=0;
            if(timeSig==2){
                if(cnt==2)
                    countInEnd();
            }
            else if(timeSig==3){
                if(cnt==3)
                    countInEnd();
            }
            else if(timeSig==4){
                if(cnt==4)
                    countInEnd();
            }
            cnt++;
            //serial_println(cnt);

        }
        k++;
        //serial_println(k);
        //serial_println(beatMS);
        break;
        
        default:
        break;
    }
    return state;
}

int main(void) {

    DDRC=0xFE; PORTC=0x01;   //sets A0 to input and A1-A5 to outputs
    DDRB=0xFE; PORTB=0x01;   //sets port B0 to input and B1-B5 to outputs
    DDRD=0xFF; PORTD=0x00;   //sets all of port D to outputs


    //ADC_init();   // initializes ADC
    lcd_init(); // initializes lcd
    lcd_clear();
    serial_init(9600);

    unsigned char i=0;



    //Initialize buzzer1 - pin9
    TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
   //WGM11, WGM12, WGM13 set timer to fast pwm mode

   //Initialize buzzer2 - pin10
    TCCR1A |= (1 << WGM11) | (1 << COM1B1); //COM1B1 opens channel B
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
   //WGM11, WGM12, WGM13 set timer to fast pwm mode

    //initialize IR Receiver 
    IRinit(&PORTB,&PINB,0);


    tasks[i].period = LCD_PERIOD;
    tasks[i].state = LCDINIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_LCD;
    i++;

    tasks[i].period = REMOTE_PERIOD;
    tasks[i].state = REMOTEINIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Remote;
    i++;

    tasks[i].period = KEYS_PERIOD;
    tasks[i].state = KEYSINIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Keys;
    i++;

    tasks[i].period = BUZZER1_PERIOD;
    tasks[i].state = BUZZER1INIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Buzzer1;
    i++;

    tasks[i].period = BUZZER2_PERIOD;
    tasks[i].state = BUZZER2INIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Buzzer2;
    i++;

    tasks[i].period = LED_PERIOD;
    tasks[i].state = LEDINIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_LED;
    i++;

    tasks[i].period = SHIFTREG_PERIOD;
    tasks[i].state = SHIFTREGINIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_ShiftReg;
    i++;

    tasks[i].period = MET_PERIOD;
    tasks[i].state = METINIT;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Tick_Met;
    i++;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}

    return 0;
}