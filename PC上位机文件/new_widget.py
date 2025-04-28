from PyQt5 import QtCore, QtWidgets
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont


class Set_question:
    def set_return(self,ico,text,dir,user=False):  #头像，文本，方向
        self.widget = QtWidgets.QWidget(self.scrollAreaWidgetContents)
        self.widget.setLayoutDirection(dir) #方向
        self.widget.setObjectName("widget")
        self.horizontalLayout = QtWidgets.QHBoxLayout(self.widget)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.label = QtWidgets.QLabel(self.widget)
        self.label.setMaximumSize(QtCore.QSize(60, 60)) #头像占据大小
        self.label.setText("")
        self.label.setPixmap(ico) #头像
        self.label.setScaledContents(True)
        self.label.setObjectName("label")
        self.horizontalLayout.addWidget(self.label, alignment=Qt.AlignTop)
        #文本部分 更换为label
        self.text_label = QtWidgets.QLabel(text)
        self.text_label.setWordWrap(True)  # 自动换行
        font = QFont()
        font.setPointSize(12)
        self.text_label.setFont(font)
        if user == True:
            self.text_label.setStyleSheet("""
                                padding: 10px;
                                background-color: #2BA245; /* 微信聊天气泡绿 */
                                 font-family: "PingFang SC", "Microsoft YaHei", sans-serif;
                                font-size: 28px;
                                            """)
        else:
            self.text_label.setStyleSheet("""
                                   padding: 10px;
                                   background-color: rgba(71,121,214,20);
                                    font-family: "PingFang SC", "Microsoft YaHei", sans-serif;
                                   font-size: 28px;
                               """)
        #控制气泡大小
        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Maximum, QtWidgets.QSizePolicy.Preferred)
        self.text_label.setSizePolicy(size_policy)
        self.text_label.setMaximumWidth(500)  # 限制最大宽度
        self.text_label.adjustSize()  # 关键！自适应大小

        self.horizontalLayout.addWidget(self.text_label)
        self.verticalLayout.addWidget(self.widget)