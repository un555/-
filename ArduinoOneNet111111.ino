//引入ESP8266.h头文件，建议使用教程中修改后的文件
#include "ESP8266.h"
#include "dht11.h"
#include "SoftwareSerial.h"
#include "HX711.h"//压力传感器
float Weight = 0;
//自动控制路灯

int threshold =400; //光强值，根据实际情况调整
void setup ( )
{
Serial.begin(9600); //设置串口波特率
pinMode(10, OUTPUT); //设置输出端口
}
void loop( )
{
int n = analogRead(A3); //读取模拟口A3
 //Serial.println(n); //串口打印，输出n的值
if (n>threshold ) //晚上光线暗，n值变大
 digitalWrite(10, HIGH); //点亮路灯
else
digitalWrite(10, LOW); //关闭路灯
delay(300);
}
//压力
void setup1()
{
	Init_Hx711();				//初始化HX711模块连接的IO设置

	Serial.begin(9600);
	Serial.print("Welcome to use!\n");

	delay(3000);
	Get_Maopi();
}

//配置ESP8266WIFI设置
#define SSID "hhy"    //填写2.4GHz的WIFI名称，不要使用校园网
#define PASSWORD "20041009"//填写自己的WIFI密码
#define HOST_NAME "api.heclouds.com"  //API主机名称，连接到OneNET平台，无需修改
#define DEVICE_ID "buASMQfVj6"       //填写自己的OneNet设备ID
#define HOST_PORT (80)                //API端口，连接到OneNET平台，无需修改
String APIKey = "UzNubFpmT1dVaE5naFRpNUZuSmhPdnBkUk9odVZTRkU="; //与设备绑定的APIKey

#define INTERVAL_SENSOR 5000 //定义传感器采样及发送时间间隔

//创建dht11示例

dht11 DHT11;

//定义DHT11接入Arduino的管脚
#define DHT11PIN 4

//定义ESP8266所连接的软串口
/*********************
 * 该实验需要使用软串口
 * Arduino上的软串口RX定义为D3,
 * 接ESP8266上的TX口,
 * Arduino上的软串口TX定义为D2,
 * 接ESP8266上的RX口.
 * D3和D2可以自定义,
 * 但接ESP8266时必须恰好相反
 *********************/
SoftwareSerial mySerial(3, 2);
ESP8266 wifi(mySerial);

void setup2()
{
 
  //以下为ESP8266初始化的代码
  Serial.print("FW Version: ");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStation()) {
    Serial.print("to station ok\r\n");
  } else {
    Serial.print("to station err\r\n");
  }

  //ESP8266接入WIFI
  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print("Join AP success\r\n");
    Serial.print("IP: ");
    Serial.println(wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  Serial.println("");
  Serial.print("DHT11 LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);

  mySerial.println("AT+UART_CUR=9600,8,1,0,0");
  mySerial.begin(9600);
  Serial.println("setup end\r\n");
}

unsigned long net_time1 = millis(); //数据上传服务器时间
void loop2(){


  if (net_time1 > millis())
    net_time1 = millis();

  if (millis() - net_time1 > INTERVAL_SENSOR) //发送数据时间间隔
  {

    int chk = DHT11.read(DHT11PIN);

    Serial.print("Read sensor: ");
    switch (chk) {
      case DHTLIB_OK:
        Serial.println("OK");
        break;
      case DHTLIB_ERROR_CHECKSUM:
        Serial.println("Checksum error");
        break;
      case DHTLIB_ERROR_TIMEOUT:
        Serial.println("Time out error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }

    float sensor_hum = (float)DHT11.humidity;
    float sensor_tem = (float)DHT11.temperature;
    Weight = Get_Weight();	//计算放在传感器上的重物重量
	  Serial.print(float(Weight),3);	//串口显示重量
	  Serial.print(" g\n");	//显示单位
    Serial.print("humidity (%): ");
    Serial.println(sensor_hum, 2);

    Serial.print("temp (oC): ");
    Serial.println(sensor_tem, 2);
    Serial.println("");

    if (wifi.createTCP(HOST_NAME, HOST_PORT)) { //建立TCP连接，如果失败，不能发送该数据
      Serial.print("create tcp ok\r\n");
      char buf[10];
      //拼接发送data字段字符串
      jsonToSend = "{\"Temperature\":";
      dtostrf(sensor_tem, 1, 2, buf);
      jsonToSend += "\"" + String(buf) + "\"";
      jsonToSend += ",\"Humidity\":";
      dtostrf(sensor_hum, 1, 2, buf);
      jsonToSend += "\"" + String(buf) + "\"";
      jsonToSend += ",\"Weight\":";
      dtostrf(float(Weight),1,2,buf);
      jsonToSend += "\"" + String(buf) + "\"";
      jsonToSend+="}";

      //拼接POST请求字符串
      String postString = "POST /devices/";
      postString += DEVICE_ID;
      postString += "/datapoints?type=3 HTTP/1.1";
      postString += "\r\n";
      postString += "api-key:";
      postString += APIKey;
      postString += "\r\n";
      postString += "Host:api.heclouds.com\r\n";
      postString += "Connection:close\r\n";
      postString += "Content-Length:";
      postString += jsonToSend.length();
      postString += "\r\n";
      postString += "\r\n";
      postString += jsonToSend;
      postString += "\r\n";
      postString += "\r\n";
      postString += "\r\n";

      const char *postArray = postString.c_str(); //将str转化为char数组

      Serial.println(postArray);
      wifi.send((const uint8_t *)postArray, strlen(postArray)); //send发送命令，参数必须是这两种格式，尤其是(const uint8_t*)
      Serial.println("send success");
      if (wifi.releaseTCP()) { 
      
        Serial.print("release tcp ok\r\n");
      } else {
        Serial.print("release tcp err\r\n");
      }
      postArray = NULL; //清空数组，等待下次传输数据
    } else {
      Serial.print("create tcp err\r\n");
    }

    Serial.println("");

    net_time1 = millis();
  }
}
