# ESP32-Deepseek-TCP-Onenet

ESP32 使用5.0-Idf
使用ESP作为一个服务中转站 实现 电脑上的qt上位机与deepseek聊天对话  你需要更换一下你自己的API以及IP即可 主要感谢GPT的帮助。
ESP文件夹的内容 是烧录到ESP32中 上位机文件夹则是pycharm中使用。
这里我解释一下因为我是用的手机热点，来使ESP32与电脑联网，因此需要电脑CMD 输入configip查询手机热点分配的IP地址，所有设备都在这个IP地址下才可以通讯。
本来是老登安排给我一个杂活去搞一个水赛避免延毕，因为时间赶，还要取整理课题，所以有些功能还不完全 比如上位机聊天框放大会导致头像与气泡漂移。
还有一个缺点 循环接收有些问题，短句子没问题(800字以内没问题）。基本演示的话没问题。
QT借鉴https://www.cnblogs.com/Rrea/p/17584484.html 大佬 我把Textbrowse 改为label 想着自动缩放一下 。
![image](1.png)

新添加功能可以使用ESP 的MQTT协议连接onenet 云平台 修改物模型数据以及获取下发数据 他们使用的通讯协议并不一样。
在上位机界面输入“设备”或者“平台”可以直接将获取云平台的最新物模型数据（你事先保存上去的）发送给deepseek解析。因为我这个项目是根据纱线瑕点监测来溯源设备故障因此跟纱线相关，你只需要修改我的变量即可。
