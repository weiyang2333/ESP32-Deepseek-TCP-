import threading
from PyQt5.QtCore import Qt, pyqtSignal
import sys
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5 import QtCore, QtGui, QtWidgets
from new_widget import Set_question
from chat import Ui_MainWindow
import socket

# ESP32 的 IP 地址和端口（注意替换为实际的 ESP32 IP）
ESP32_IP = "172.20.10.3"
PORT = 8083


class MainWindow(QMainWindow,Ui_MainWindow):

    update_wifi_status = QtCore.pyqtSignal(str)
    response_received = pyqtSignal(str)  # 定义一个服务器接收信号

    def __init__(self,parent=None):
        super(MainWindow,self).__init__(parent)
        self.setupUi(self)
        self.retranslateUi(self)
        self.chat_layout = QtWidgets.QVBoxLayout()
        self.chat_layout.setAlignment(Qt.AlignTop)
        self.scrollAreaWidgetContents.setLayout(self.chat_layout)
        self.chat_layout.addStretch(1)
        #网络标签
        self.wifi1 = QtGui.QPixmap("wifi1.png")
        self.wifi_status.setText("无网络连接...")
        self.label_wifi_symbol.setPixmap(self.wifi1)  # 头像
        self.label_wifi_symbol.setMaximumSize(QtCore.QSize(30, 30))  #限制大小
        self.label_wifi_symbol.setScaledContents(True) # 自动调节网络标签图像占据大小
        #信号连接部分
        self.update_wifi_status.connect(self.wifi_status.setText)
        #多线程网络 启动！
        self.client_socket = None  # 初始化
        network_thread = threading.Thread(target=self.connect_network)
        network_thread.start()
        #聊天框头像
        self.icon = QtGui.QPixmap("ds.jpg")  # ds头像
        self.icon_use = QtGui.QPixmap("mita.jpg") #用户头像
        # 设置聊天窗口样式 隐藏滚动条
        self.chat_scrollArea.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.chat_scrollArea.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff) #Qt.ScrollBarAsNeeded需要的时候会出现
        # 信号与槽
        self.Confirmation_button.clicked.connect(self.send_message)  # 用户发送
        self.plainTextEdit.undoAvailable.connect(self.Event)  # 监听输入框状态
        self.response_received.connect(self.display_received_message) #服务器接收
        # 可滚动区域
        scrollbar = self.chat_scrollArea.verticalScrollBar()
        scrollbar.rangeChanged.connect(self.adjustScrollToMaxValue)  # 监听窗口滚动条范围
        self.scrollAreaWidgetContents.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Maximum)

    def connect_network(self):
        self.wifi_status.setText("无网络连接...")
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            client_socket.connect((ESP32_IP, PORT)) #连接ESP32服务器
            self.client_socket = client_socket
            self.update_wifi_status.emit("已连接到网络")  #网络标签
            self.wifi3 = QtGui.QPixmap("wifi3.png")
            self.label_wifi_symbol.setPixmap(self.wifi3)  # 标签3
            self.label_wifi_symbol.setScaledContents(True)  # 自动调节网络标签图像占据大小
            print("连接成功！")
        except socket.error as e:
            self.update_wifi_status.emit("连接失败")
            print(f"连接失败: {e}")
            return
        self.buffer = b""
        self.chunk = b""
        while True:
            self.client_socket.settimeout(None)  # 超时一损俱损 第一个进行超时豁免
            self.buffer = self.client_socket.recv(1024)
            if not self.buffer:
                print("连接中断 断开服务")
                break
            while True:  # END是结束标识符
                try:
                    self.buffer += self.chunk
                    if b"END" in self.buffer:
                        break
                    self.client_socket.settimeout(6)
                    self.chunk = self.client_socket.recv(124)
                except socket.timeout:
                    print("超时，强制结束接收")
                    break

            response = self.buffer.replace(b"END", b"").decode("utf-8", errors="ignore")
            self.buffer = b""
            print("Ds:", response,len(response))
            self.response_received.emit(response)  # 跨线程发出信号 子线程不可调用GUI组件

    def display_received_message(self, response):
        #之所以要多此一举 是因为子线程不可修改GUI组件 表现为 服务器回传信息不会打印，所以采用主线程调用的方式
        Set_question.set_return(self,self.icon, response, QtCore.Qt.LeftToRight)
        QApplication.processEvents()
    def send_message(self):
        message = self.plainTextEdit.toPlainText()
        print("用户输入内容:", message)

        # 将输入框中的文字显示在 QTextBrowser 中
        if message:
            Set_question.set_return(self,self.icon_use, message, QtCore.Qt.RightToLeft,user=True)  # 调用new_widget.py中方法生成右气泡
            QApplication.processEvents()
            self.client_socket.sendall(message.encode('utf-8'))#同时向esp32发送数据
            self.plainTextEdit.setPlainText("") #清空

    def Event(self):
        if not self.plainTextEdit.isEnabled():  #这里通过文本框的是否可输入
            self.plainTextEdit.setEnabled(True)
            self.pushButton.click()
            self.plainTextEdit.setFocus()
    #窗口滚到最底部
    def adjustScrollToMaxValue(self):
        scrollbar = self.chat_scrollArea.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())

    def closeEvent(self, event):
        # 关闭连接
        self.client_socket.close()
        event.accept()



if __name__ == '__main__':
    app = QApplication(sys.argv)
    myshow = MainWindow()
    myshow.show()
    sys.exit(app.exec_())
