# 基于腾讯云及WOL的PC远程开机系统
本系统基于腾讯云物联网（IoT）平台和Wake-on-LAN（WOL）技术，结合单片机实现远程PC开机、状态监控以及用户自定义网络配置功能。通过微信小程序，用户可以远程唤醒PC，并实时查看其开机状态。系统还允许用户通过串口助手配置待唤醒的PC MAC地址，腾讯云连接信息和Wi-Fi连接信息，并将这些信息保存在SD卡中供单片机使用。

### 系统功能与流程：

1. 系统配置：通过串口助手（如XShell），用户输入Wi-Fi  SSID和密码、腾讯云连接信息和PC的MAC地址配置信息。单片机将这些信息保存在SD卡中，便于设备启动时自动加载使用。
2. 系统流程及连接：单片机运行RT-Thread系统，启动时读取SD卡中的配置信息，自动连接指定的Wi-Fi网络，获取NTP时间并写入RTC中，连接腾讯云。
3. 腾讯云物联网平台：腾讯云作为中间通信平台，接收用户通过微信小程序发出的指令，并将指令转发至单片机。
4. WOL魔术包发送：单片机在接收到来自腾讯云的远程开机指令后，依据SD卡中保存的MAC地址，通过WOL协议发送魔术包，实现PC的远程唤醒。
5. UI显示：显示时间，WIFI配置信息，MAC地址及作者微信二维码。
6. PC远程控制：PC开机后自动启动向日葵远程桌面软件，用户通过该软件可以远程登录并操作PC。
7. 状态监控与开机检测：
   - 单片机周期性向局域网内的所有IP地址发送ICMP包（Ping），以更新局域网的ARP表。
   - 单片机根据收到的ARP表，查找是否有与保存的MAC地址相匹配的IP地址。如果找到匹配，则表示PC已开机；否则，认为PC仍未开机。
   - 单片机将检测结果在发生改变时上报至腾讯云物联网平台，供用户在微信小程序中实时查看PC的开关机状态。

### 系统特色：

- 灵活配置：用户可通过串口助手（如Xshell）配置PC的MAC地址和Wi-Fi连接信息，便于灵活设置和修改。
- 自动化管理：单片机启动时自动读取SD卡中的配置，自动连接Wi-Fi并管理远程PC的唤醒和状态监控。
- 状态监控：通过发送ICMP包检测PC是否开机，并通过ARP表解析MAC地址，判断PC状态。
- 远程操作：PC开机后，自动启动向日葵远程桌面软件，用户可以随时通过远程桌面操作PC。

该系统集成了远程开机、状态监控、自定义配置和远程操作，为用户提供了完整的PC远程管理解决方案。
### 系统框图：

![Snipaste_2024-09-07_23-37-18.bmp](https://oss-club.rt-thread.org/uploads/20240907/57311053ef1fc73e5a2860662352928c.bmp.webp)
### 微信小程序截图：

![微信图片_20240907233551.jpg](https://oss-club.rt-thread.org/uploads/20240907/515dfad5ef7973a450a9e1a2d65da9aa.jpg.webp)
### UI界面截图：

![图片1.png](https://oss-club.rt-thread.org/uploads/20240907/2811bb3879583ccea5777b48530e6575.png.webp)

![图片2.png](https://oss-club.rt-thread.org/uploads/20240907/b4f2920ed7ded1a2333f661f2cebc0dd.png.webp)

![图片3.png](https://oss-club.rt-thread.org/uploads/20240907/448c7d60bfdd24c59c170feaa4238fd8.png.webp)

![图片4.png](https://oss-club.rt-thread.org/uploads/20240907/cc410af2c3f6305bcb27762afd249fde.png.webp)
### xshell配置界面：

![图片5.png](https://oss-club.rt-thread.org/uploads/20240907/153a344f53543d2703808e7d72c8feca.png)
### 视频连接：
[https://www.bilibili.com/video/BV1EVpeekErx/?vd_source=ebac7a60d5c549ac184aab64ca7fbc4b](https://www.bilibili.com/video/BV1EVpeekErx/?vd_source=ebac7a60d5c549ac184aab64ca7fbc4b)