# Arduino_clock
Using DFROBOT Intermediate Kit for Arduino.
使用DFRobot Arduino中级套件制作。

![image](https://github.com/xianfei/Arduino_clock/raw/master/img/DSC01408small.jpg)


Component: Arduino Uno broad & I/O extended broad， A1602 I2C screen module, IR remote module & remote control, Buzzer module(if you want to use alarm function), DHT-11 temperature and humidity sensor module(if you want to display temperature). 需要的组件：ArduinoUno开发板及扩展版，屏幕模块，IR接收模块，遥控器，蜂鸣器，（v2新增）温湿度传感器。

If you want to use auto brightness function, you need change something on the screen module.(only in v3 and newer software) 如果你想使用自动亮度调节，你需要稍稍更改一下电路。

![image](https://github.com/xianfei/Arduino_clock/raw/master/img/DSC01453small.jpg)

（在img文件夹下可以查看高清大图）

Firstly, you need download those library in Arduino APP.
首先，你需要在arduino中下载如下库。如图所示。

![image](https://github.com/xianfei/Arduino_clock/raw/master/img/QQ20190715-132620%402x.png)

Then, you need change the pin number in the Define Area(at the top of the code) to apdapt your connection.
然后你需要在define区域（代码的最上面那点儿）改一下你连接设备的插口号，来适配你链接的方式。

按键功能（你可以在define区域对按键进行修改）：电源键：返回主界面  ； 1键：修改时间 ； 2键：设定闹钟  ； 3键：修改日期 ； 4键：取消闹钟

v2版本增加：5键：切换24小时制  ；  6键：显示温度  ；   7键：切换背光开关

修改时间日期及设置闹钟使用0-9键输入，主界面显示alamr则表示闹钟已设定。

更新日志：

V2：2018.12.25

加入温度计、24小时制切换、开光屏幕背光功能，优化代码易读性及使用宏定义更加方便对代码进行个性化调整。

V1.1：2018.11.23

优化时序逻辑降低接收信号延时，优化算法提高接收信号成功率，优化系统稳定性并降低内存占用提高执行效率。

V1.0： 2018.10.31

完成第一版，具备闹钟、修改时间日期、显示时间日期功能。使用直接输入的方式输入时间日期及设定闹钟。
