
/********** 需要用到的库（在Arduino库管理器中安装）**********
    IRremote      LiquidCrystal I2C     Rtc by Makuna
 *******************************************************/
//在这里自定义数字区遥控按键
#define edittimeKey 1 // 1键：修改时间
#define setAlarmKey 2 // 2键：设定闹钟
#define editdayKey 3 // 3键：修改日期
#define cancelAlarmKey 4 // 4键：取消闹钟
#define use24hKey 5 // 5键：切换24小时制
#define showtempKey 6 // 6键：显示温度
#define backlightKey 7 // 7键：打开或关闭背光

//在这里设置端口信息
#define DHT11_PIN 5  //温度计模块接口 删除这句话会禁用温度相关代码
#define buzzpin 4  //蜂鸣器接收端口
#define RECV_PIN 10 //红外接收端口

#define AMBIENT_Light1 9
#define AMBIENT_Light2 3

#define LCD_BK_PIN 6  //屏幕背光接口（如果使用自动亮度调节） 删除这句话会禁用自动亮度相关代码
#define LIGHT_SENSOR A0 //光线传感器接口（如果使用自动亮度调节） 删除这句话会禁用自动亮度相关代码

#define TIMEOFFSET 10 // 时钟时间与编译时间补偿  具体数值取决于编译及上传速度等

#define bootinf "by xianfei"  //自定义启动画面显示的信息 如不想显示任何信息请直接删除或注释掉该行

// #define RTC_DS3231  //在这里定义是否要启动外置时间模块  注释掉这句话则不使用外置时钟模块
//外置时间模块需要自己单独购买，淘宝搜DS3231我买的六块八包邮

/*******************************************************/

//以下是代码部分
#if defined(RTC_DS3231)
  #include <Wire.h> // must be included here so that Arduino library object file references work
  #include <RtcDS3231.h>
  RtcDS3231<TwoWire> Rtc(Wire);
#endif
#include <inttypes.h>
#include <Arduino.h>
#if defined(DHT11_PIN)
  #include "dht11.h"
  dht11 DHT;
#endif
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
IRrecv irrecv(RECV_PIN); // 创建红外线接受变量
decode_results results;  // 用于保存红外线接收结果
LiquidCrystal_I2C lcd(0x20, 16, 2); //设置LCD的地址，2行，每行16字符
static char t[9], d[12];
static short h, m, s, y, mon, day, xq; // 系统时间 时 分 秒 年 月 日 星期
static short h_, m_, s_, y_, mon_, day_; // 调整时间的临时变量 时 分 秒 年 月 日 星期
static short edittime = 0, editdate = 0, editalarm = 0, curs,backLight=1;
static short ah = 25, am = 0;
unsigned long oldmillis=0;
bool use24h=1,showtemp=0; //是否使用24h制、显示温度的初始状态
#define editmode (edittime || editdate || editalarm)

unsigned char ambientLight1 = 0;
bool ambientLight1Waved = false;
unsigned char ambientLight2 = 0;
bool ambientLight2On = false;

bool noOutputOnce = false, needClear=false;
#if defined(LIGHT_SENSOR)&&(LCD_BK_PIN)
  unsigned long oldmillis2=0;
  unsigned char currentBright=255;  //  当前显示屏亮度  用于亮度缓慢升降
  unsigned char lcd_bk = 255;  //  应设定的显示屏亮度
  unsigned char bright[100]={0};  // 用于亮度采样  防止PWM影响亮度检测
  unsigned char bright_i=0;  // 用于亮度采样的循环变量
  short brightOffset=0;  // 亮度偏移量
#endif
unsigned char keepBKOn=0; //  保持背光时间（s）


void setup()
{
  lcd.init();         // LCD初始化设置
  lcd.backlight();    // 打开 LCD背光
  #if defined(LIGHT_SENSOR)&&(LCD_BK_PIN)
    pinMode(LCD_BK_PIN, OUTPUT);
    analogWrite(LCD_BK_PIN, lcd_bk);
  #endif
  pinMode(AMBIENT_Light1, OUTPUT);
  //analogWrite(AMBIENT_Light1, 255);
  //Serial.begin(9600); // 设置串口波特率9600
  irrecv.enableIRIn();
  pinMode(buzzpin, OUTPUT);
  #if defined(bootinf)
    lcd.setCursor(0, 0);
    lcd.print("Booting...");
    lcd.setCursor(0, 1);
    lcd.print(bootinf);
    delay(1000);
  #endif
  lcd.clear();
  #if defined(RTC_DS3231)
    Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__)+TIMEOFFSET;
    if (Rtc.GetDateTime() < compiled) Rtc.SetDateTime(compiled);
  #else
    inittime();
  #endif
}



void loop()
{
  recv(); //接受遥控信号
  if(ambientLight1Waved){
    //double light=255*0.5*(1.0+sin(millis()/2000.0));
    //double light=255*pow(sin(millis()/2000.0),2);
    //double light=255*sqrt(0.5*(1.0+sin(millis()/2000.0)));
    double light=ambientLight1*pow(0.5*(1.0+sin(millis()/2000.0)),2);
    analogWrite(AMBIENT_Light1, light);
    //Serial.println(light);
    }
  #if defined(LIGHT_SENSOR)&&(LCD_BK_PIN)
  unsigned long millis_2=millis();
  if(oldmillis2!=millis_2){
    //Serial.println("Start");
      int sensorValue = analogRead(LIGHT_SENSOR); // 0-1023
      //Serial.println(sensorValue);
      //Serial.println(sqrt(sensorValue*64.0));
      bright[bright_i++]=sqrt(sensorValue*64.0);  // 0-255
      if(bright_i==100){
        bright_i=0;
      }
    oldmillis2=millis_2;
    //Serial.println(bright[1]);
    unsigned char freq[256]={0};
    for(int i=0;i<100;i++)freq[bright[i]]++;
    int freqMax=freq[0],index=0;
    for(int i=0;i<255;i++){
      if(freq[i+1]>freqMax){
        freqMax=freq[i+1];
        index=i+1;
      }
    }
    //Serial.println(sum/100);
    if(keepBKOn!=0&&currentBright<=50)currentBright=50;// 黑暗下 遥控等操作出发背光短暂亮起
    else {
      if(freqMax>=10)lcd_bk=index;  // 
    //Serial.println("lcd_bk");
    //Serial.println(lcd_bk);
    if(lcd_bk<currentBright&&currentBright>0)currentBright--;
    if(lcd_bk>currentBright&&currentBright<255)currentBright++;}
    //if(keepBKOn!=0&&currentBright<100)analogWrite(LCD_BK_PIN, 100);
    //Serial.println(pow(currentBright/255.0,brightOffset>0?(1/(1.0+0.2*brightOffset)):(1.0-0.2*brightOffset))*255.0);
    // 写入亮度时加上亮度偏移系数
    analogWrite(LCD_BK_PIN, pow(currentBright/255.0,brightOffset>0?(1/(1.0+0.2*brightOffset)):(1.0-0.2*brightOffset))*255.0);
   }
  #endif

  
  unsigned long millis_=millis();
  if(millis_-oldmillis>=1000){
    oldmillis=millis_;
    //Serial.println(millis_);
    if(keepBKOn>0)keepBKOn--;
    s++;
    formatTime(); //获取时间及时间流动
    if (!(editmode || (ah == h && am == m))) {
      if(noOutputOnce)noOutputOnce=false;
      else output();
    } //如果不在编辑模式 将输出时间日期
    if (ah == h && am == m) alarmrun(); //闹钟响铃部分
   }
   
}

void recv()
{
      if (irrecv.decode(&results))
    {
      if(results.value>0xFD0000&&results.value<0xFDFFFF)keepBKOn=5;
      //Serial.println(results.value, HEX);
      if (results.value == 0xFD00FF)  // 电源键：返回主界面
      {
        edittime = 0;
        editdate = 0;
        editalarm = 0;
        lcd.clear();
        lcd.noBlink();
        formatTime();
        output();
        ah = 25, am = 0;
        digitalWrite(buzzpin, LOW);
      }else if (results.value == 0xFD807F) // VOL+ 键
      {
        // do sth...
      }else if (results.value == 0xFD40BF) // FUNC/STOP  切换非PWM灯光控制
      {
        if(ambientLight2==0){
          ambientLight2=1;
          noOutputOnce = true;
          lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Backgrd Light");
        lcd.setCursor(6,1);
        lcd.print("OFF");
          pinMode(3, OUTPUT);
          pinMode(4, OUTPUT);
          digitalWrite(3, LOW);
          digitalWrite(4, LOW);
         }else if(ambientLight2==1){
          ambientLight2=2;
          noOutputOnce = true;
          lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Backgrd Light");
        lcd.setCursor(6,1);
        lcd.print("LOW");
          pinMode(3, OUTPUT);
          pinMode(4, INPUT);
          digitalWrite(3, HIGH);
          //digitalWrite(4, LOW);
          }else if(ambientLight2==2){
            ambientLight2=0;
          noOutputOnce = true;
          lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Backgrd Light");
        lcd.setCursor(6,1);
        lcd.print("HIGH");  
          pinMode(3, OUTPUT);
          pinMode(4, OUTPUT);
          digitalWrite(3, HIGH);
          digitalWrite(4, HIGH);
          }
      }else if (results.value == 0xFD20DF) // |<<
      {
        keepBKOn=0;
        noOutputOnce = true;
        lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Bright Offset");
        lcd.setCursor(7,1);
        if(brightOffset>-5)brightOffset--;
        lcd.print(brightOffset);
      }else if (results.value == 0xFDA05F) // >||
      {
        // do sth...
      }else if (results.value == 0xFD609F) // >>|
      {
        keepBKOn=0;
        noOutputOnce = true;
        lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Bright Offset");
        lcd.setCursor(7,1);
        if(brightOffset<5)brightOffset++;
        lcd.print(brightOffset);
        // do sth...
      }else if (results.value == 0xFD10EF) // 下箭头  切换PWM控制灯光亮度
      {
        if(ambientLight1>=10) ambientLight1 -= 50;
        if(ambientLight1==5) ambientLight1 = 0;
        analogWrite(AMBIENT_Light1, ambientLight1);
        noOutputOnce = true;
        lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Ambient Light");
        lcd.setCursor(6,1);
        lcd.print(ambientLight1/50);
        lcd.print(" / 5");
      }else if (results.value == 0xFD906F) // VOL-   切换PWM控制灯光呼吸效果
      {
        if(!ambientLight1Waved){
          ambientLight1Waved=true;
          noOutputOnce = true;
          lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Ambient Light");
        lcd.setCursor(4,1);
        lcd.print("Waved On");
          }else{
            ambientLight1Waved=false;
          noOutputOnce = true;
          lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Ambient Light");
        lcd.setCursor(4,1);
        lcd.print("WavedOFF");
            }
      }else if (results.value == 0xFD50AF) // 上箭头   切换PWM控制灯光亮度
      {
        if(ambientLight1<=245) ambientLight1 += 50;
        if(ambientLight1==250) ambientLight1 = 255;
        analogWrite(AMBIENT_Light1, ambientLight1);
        //Serial.println(ambientLight);
        noOutputOnce = true;
        lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Ambient Light");
        lcd.setCursor(6,1);
        lcd.print(ambientLight1/50);
        lcd.print(" / 5");
      }else if (results.value == 0xFDB04F) // EQ  显示当前温湿度
      {
        #if defined(DHT11_PIN)
        noOutputOnce = true;
        lcd.clear();
        DHT.read(DHT11_PIN);
        lcd.setCursor(0,0);
        lcd.print("Temperature:");
        lcd.print(DHT.temperature,1);
        lcd.setCursor(1,1);
        lcd.print("Humidity:");
        lcd.print(DHT.humidity,1);
        lcd.print("%");
        needClear=1;
        #endif
        // do sth...
      }else if (results.value == 0xFD708F) // ST/REPT
      {
        // do sth...
      }else if (results.value == 0xFD30CF)
      /*****数字键区功能请不要从这里修改*****/
      {
        remote(0);
      }else if (results.value == 0xFD08F7)
      {
        remote(1);
      }else if (results.value == 0xFD8877)
      {
        remote(2);
      }else if (results.value == 0xFD48B7)
      {
        remote(3);
      }else if (results.value == 0xFD28D7)
      {
        remote(4);
      }else if (results.value == 0xFDA857)
      {
        remote(5);
      }else if (results.value == 0xFD6897)
      {
        remote(6);
      }else if (results.value == 0xFD18E7)
      {
        remote(7);
      }else if (results.value == 0xFD9867)
      {
        remote(8);
      }else if (results.value == 0xFD58A7)
      {
        remote(9);
      }
      irrecv.resume();
    }
}

void output()
{
  if(needClear){
    needClear=false;
    lcd.clear();
  }
  //显示闹钟标识及时间
  if ((ah == 25)&&use24h)
    lcd.setCursor(4, 1);
  if ((ah == 25)&&!use24h)
    lcd.setCursor(3, 1);
  if ((ah != 25))
    lcd.setCursor(1, 1);
  lcd.print(t);
  if ((ah != 25)&&use24h)
  lcd.print(" alarm");  
  if ((ah != 25)&&!use24h){
    if(h>12)lcd.print("PM"); 
    else lcd.print("AM");
    lcd.print(" alm");  
  }
  if ((ah == 25)&&!use24h)
    {
      if(h>12)lcd.print("PM"); 
      else lcd.print("AM");
    }
  
  //显示日期部分
  lcd.setCursor(1, 0);
  if(showtemp){

  lcd.print(&d[1]);
  lcd.print(" ");
  }
  else{
    lcd.print(y);
  lcd.print(d);
  lcd.print(" ");
  }
  //显示星期部分
  switch (xq)
  {
  case 1:
    lcd.print("Mon");
    break;
  case 2:
    lcd.print("Tue");
    break;
  case 3:
    lcd.print("Wes");
    break;
  case 4:
    lcd.print("Thu");
    break;
  case 5:
    lcd.print("Fri");
    break;
  case 6:
    lcd.print("Sat");
    break;
  case 7:
    lcd.print("Sun");
    break;
  default:
    lcd.print("Err");
  }
  #if defined(DHT11_PIN)
  if(showtemp&&(s%2==0)){
    DHT.read(DHT11_PIN);
    lcd.print(" ");
    if(0<DHT.temperature&&DHT.temperature<50)
    lcd.print(DHT.temperature,1);
  }
  #endif
}

void alarmrun()
{
  static bool bee;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alarm is beeing...");
  lcd.setCursor(0, 1);
  lcd.print("Stop after ");
  lcd.print(60 - s);
  lcd.print("s");
  if (bee)
    digitalWrite(buzzpin, HIGH);
  if (!bee)
    digitalWrite(buzzpin, LOW);
  bee = !bee;
  if (s == 59)
    lcd.clear();
}

int remote(int recv)
{
  if (recv == backlightKey && !editmode){
    backLight=!backLight;
    if(backLight)lcd.backlight(); 
    else lcd.noBacklight();
    return 0;
  }
  #if defined(DHT11_PIN)
  if (recv == showtempKey && !editmode){
    showtemp=!showtemp;
    lcd.clear();
    formatTime();
    output();
    return 0;
  }
  #endif
  if (recv == use24hKey && !editmode){
    use24h=!use24h;
    lcd.clear();
    formatTime();
    output();
    return 0;
  }
  if (recv == cancelAlarmKey && !editmode)
  {
    ah = 25;
    digitalWrite(buzzpin, LOW);
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Alarm canceled");
    noOutputOnce=true;
    return 0;
  }
  if (recv == editdayKey && !editmode)
  {
    editdate = 1;
    curs = 2;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Input day:");
    lcd.setCursor(6, 1);
    lcd.print("-");
    lcd.setCursor(9, 1);
    lcd.print("-");
    lcd.setCursor(curs, 1);
    lcd.blink();
    delay(50);
    return 0;
  }
  if (recv == edittimeKey && !editmode)
  {
    edittime = 1;
    curs = 3;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Input time:");
    lcd.setCursor(5, 1);
    lcd.print(":");
    lcd.setCursor(8, 1);
    lcd.print(":");
    lcd.setCursor(curs, 1);
    lcd.blink();
    delay(50);
    return 0;
  }
  if (recv == setAlarmKey && !editmode)
  {
    editalarm = 1;
    curs = 3;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Input alarm time:");
    lcd.setCursor(5, 1);
    lcd.print(":");
    lcd.setCursor(8, 1);
    lcd.print(":00");
    lcd.setCursor(curs, 1);
    lcd.blink();
    delay(50);
    return 0;
  }
  if (editalarm)
  {
    lcd.setCursor(curs, 1);
    lcd.print(recv);
    switch (curs)
    {
    case 3:
      ah = recv * 10;
      break;
    case 4:
      ah = ah + recv;
      break;
    case 6:
      am = recv * 10;
      break;
    case 7:
      am = am + recv;
      break;
    }
    if (curs == 7)
    {
      lcd.clear();
      editalarm = 0;
      lcd.noBlink();
      formatTime();
      output();
      return 0;
    }
    if (curs == 4)
    {
      lcd.setCursor(curs + 1, 1);
      lcd.print(":");
      curs++;
    }
    curs++;
    lcd.setCursor(curs, 1);
    lcd.blink();
    // delay(50);
  }
  if (edittime)
  {
    lcd.setCursor(curs, 1);
    lcd.print(recv);
    switch (curs)
    {
    case 3:
      h_ = recv * 10;
      break;
    case 4:
      h_ = h_ + recv;
      break;
    case 6:
      m_ = recv * 10;
      break;
    case 7:
      m_ = m_ + recv;
      break;
    case 9:
      s_ = recv * 10;
      break;
    case 10:
      s_ = s_ + recv;
      break;
    }
    if (curs == 10)
    {
      h=h_;
      s=s_;
      m=m_;
      #if defined(RTC_DS3231)
      Rtc.SetDateTime(RtcDateTime{y,mon,day,h_,m_,s_});
      #endif
      lcd.clear();
      edittime = 0;
      lcd.noBlink();
      formatTime();
      output();
      return 0;
    }
    if (curs == 4 || curs == 7)
    {
      lcd.setCursor(curs + 1, 1);
      lcd.print(":");
      curs++;
    }
    curs++;
    lcd.setCursor(curs, 1);
    lcd.blink();
    delay(50);
  }
  if (editdate)
  {
    lcd.setCursor(curs, 1);
    lcd.print(recv);
    switch (curs)
    {
    case 2:
      y_ = recv * 1000;
      break;
    case 3:
      y_ = recv * 100 + y_;
      break;
    case 4:
      y_ = recv * 10 + y_;
      break;
    case 5:
      y_ = recv + y_;
      break;
    case 7:
      mon_ = recv * 10;
      break;
    case 8:
      mon_ = recv + mon_;
      break;
    case 10:
      day_ = 10 * recv;
      break;
    case 11:
      day_ = day_ + recv;
      break;
    }
    if (curs == 11)
    {
      curs++;
    }

    if (curs == 12)
    {
      y=y_;
      day=day_;
      mon=mon_;
      #if defined(RTC_DS3231)
        Rtc.SetDateTime(RtcDateTime{y_,mon_,day_,h,m,s});
      #endif
      lcd.clear();
      editdate = 0;
      lcd.noBlink();
      formatTime();
      output();
      return 0;
    }
    if (curs == 5 || curs == 8)
    {
      lcd.setCursor(curs + 1, 1);
      lcd.print("-");
      curs++;
    }
    curs++;
    lcd.setCursor(curs, 1);
    lcd.blink();
    delay(50);
  }
}

void formatTime()
{
  #if defined(RTC_DS3231)
    RtcDateTime now = Rtc.GetDateTime();
    y=now.Year();
    mon=now.Month();
    day=now.Day();
    h=now.Hour();
    m=now.Minute();
    s=now.Second();
  #else
  if (s > 59) 
  {
    m+=s/60;
    s %=60;
    
  }
  if (m > 59) 
  {
    h+=m/60;
    m %=60;
  }
  if (h > 23)
  {
    h = 0;
    day++;
  }
  #endif
  if (s < 10)
  {
    t[7] = '0' + s;
    t[6] = '0';
  }
  else
  {
    t[7] = '0' + s % 10;
    t[6] = '0' + s / 10;
  }
  if (m < 10)
  {
    t[4] = '0' + m;
    t[3] = '0';
  }
  else
  {
    t[4] = '0' + m % 10;
    t[3] = '0' + m / 10;
  }
  if((!use24h)&&(h>12)){
    if (h-12 < 10)
  {
    t[1] = '0' + h-12;
    t[0] = '0';
  }
  else
  {
    t[1] = '0' + h-12 % 10;
    t[0] = '0' + h-12 / 10;
  }
  
  }else{
      if (h < 10)
  {
    t[1] = '0' + h;
    t[0] = '0';
  }
  else
  {
    t[1] = '0' + h % 10;
    t[0] = '0' + h / 10;
  }
  }
  t[2] = ':';
  t[5] = ':';
  #if defined(RTC_DS3231)
  #else
  short daymax[13] = {100, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (y / 100 != 0 && y % 4 == 0 || y / 400 == 0)
  {
    daymax[2] = 29;
  }
  else
  {
    daymax[2] = 28;
  }
  if (day > daymax[mon])
  {
    day = 1;
    mon++;
  }
  if (mon > 12)
  {
    mon = 1;
    y++;
  }
  #endif
  if (day < 10)
  {
    d[5] = '0' + day;
    d[4] = '0';
  }
  else
  {
    d[5] = '0' + day % 10;
    d[4] = '0' + day / 10;
  }

  if (mon < 10)
  {
    d[2] = '0' + mon;
    d[1] = '0';
  }
  else
  {
    d[2] = '0' + mon % 10;
    d[1] = '0' + mon / 10;
  }
  d[0] = '-';
  d[3] = '-';
  xq=ReturnWeekDay(y,mon,day);
}


uint8_t StringToInt(const char* pString)
{
    uint8_t value = 0;

    // skip leading 0 and spaces
    while ('0' == *pString || *pString == ' ')
    {
        pString++;
    }

    // calculate number until we hit non-numeral char
    while ('0' <= *pString && *pString <= '9')
    {
        value *= 10;
        value += *pString - '0';
        pString++;
    }
    return value;
}


void inittime()  // 用于初始化时间为编译时间
{   char *date=__DATE__,*time=__TIME__;
    // sample input: date = "Dec 06 2009", time = "12:34:56"
    y = StringToInt(date+9)+2000;
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (date[0])
    {
    case 'J':
        if ( date[1] == 'a' )
            mon = 1;
        else if ( date[2] == 'n' )
            mon = 6;
        else
            mon = 7;
        break;
    case 'F':
        mon = 2;
        break;
    case 'A':
        mon = date[1] == 'p' ? 4 : 8;
        break;
    case 'M':
        mon = date[2] == 'r' ? 3 : 5;
        break;
    case 'S':
        mon = 9;
        break;
    case 'O':
        mon = 10;
        break;
    case 'N':
        mon = 11;
        break;
    case 'D':
        mon = 12;
        break;
    }
    day = StringToInt(date + 4);
    h = StringToInt(time);
    m = StringToInt(time + 3);
    s = StringToInt(time + 6);
    xq=ReturnWeekDay(y,mon,day);
   s+=TIMEOFFSET;
}


int ReturnWeekDay( unsigned int iYear, unsigned int iMonth, unsigned int iDay )
{
  int iWeek = 0;
  unsigned int y = 0, c = 0, m = 0, d = 0;
 
  if ( iMonth == 1 || iMonth == 2 )
  {
    c = ( iYear - 1 ) / 100;
    y = ( iYear - 1 ) % 100;
    m = iMonth + 12;
    d = iDay;
  }
  else
  {
    c = iYear / 100;
    y = iYear % 100;
    m = iMonth;
    d = iDay;
  }
  
  iWeek = y + y / 4 + c / 4 - 2 * c + 26 * ( m + 1 ) / 10 + d - 1;    //蔡勒公式
  iWeek = iWeek >= 0 ? ( iWeek % 7 ) : ( iWeek % 7 + 7 );    //iWeek为负时取模
  if ( iWeek == 0 )    //星期日不作为一周的第一天
  {
    iWeek = 7;
  }
  return iWeek;
}
