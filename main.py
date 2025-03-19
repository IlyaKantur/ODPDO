from UI import Ui_MainWindow
from PyQt6 import QtWidgets
from PyQt6.QtWidgets import (QMainWindow)
from PyQt6.QtCore import QTime
import sys
import os
import pyqtgraph as pg

class window(QMainWindow):
    def __init__(self):
        super(window, self).__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Устанавливаем текстовое поле консоли как только для чтения
        self.ui.Console_textEdit.setReadOnly(True)

        # Переменные
        self.Folder_path_1D = ""
        self.Name_File_1D = ""
        self.File_path_1D = ""
        self.cor_X_File_1D = []
        self.mas_Y_File_1D = []
        self.mas_sum_1D = []
        self.mas_new_sum_1D = []

        self.table_X = []
        self.table_Y = []
        
        self.calibrated = False #показывает был ли откалиброван
        self.smoothed_1D = False #показывает был ли сглажен спектр
        self.count_smoothed_1D = False #показывает сколько раз был сглажен
        # self.Title_1D
        
        # self.Timer_1D

        # Тригеры к кнопкам
        self.ui.ClearConsole_pushButton.clicked.connect(self.console_clear) #Очистка консоли
        self.ui.Folder_pushButton.clicked.connect(self.Folder_pushButton)

        # Создаем PlotWidget для результатов
        self.plot_widget_resoult = pg.PlotWidget()
        self.ui.graphResoult_gridLayout.addWidget(self.plot_widget_resoult)
        
        # Создаем PlotWidget для таблицы
        self.plot_widget_table = pg.PlotWidget()
        self.ui.graphTable_gridLayout.addWidget(self.plot_widget_table)

        # Создаем PlotWidget для графиков
        self.plot_widget_graphs = pg.PlotWidget()
        self.ui.graphs_gridLayout.addWidget(self.plot_widget_graphs)

        # # Создаем линию графика таблицы
        # self.curve = self.plot_widget_table.plot(self.table_X, self.table_Y)


        self.console("Программа запущена", False)

    def Folder_pushButton(self):
        # Открываем диалоговое окно для выбора папки
        folder_path = QtWidgets.QFileDialog.getExistingDirectory(self, "Выберите папку")

        # Проверяем, была ли выбрана папка
        if folder_path:
            # Записываем путь к выбранной папке
            self.Folder_path_1D = folder_path
            
            # Получаем список всех файлов в выбранной папке
            fileList = os.listdir(folder_path)
            if fileList:
                # Фильтруем файлы по расширениям .dat и .txt
                valid_extensions = ('.dat', '.txt')
                self.File_path_1D = [os.path.join(folder_path, file) for file in fileList if file.endswith(valid_extensions)]
                self.Name_File_1D = [file for file in fileList if file.endswith(valid_extensions)]

                # Проверяем, есть ли подходящие файлы
                if not self.Name_File_1D:
                    self.console("Файлов подходящего формата (dat, txt) нет", True)  # Выводим сообщение об ошибке
                else:
                    self.ui.table_tableWidget.clear()  # Очищаем таблицу перед добавлением новых данных
                    self.ui.table_tableWidget.setRowCount(len(self.Name_File_1D))  # Устанавливаем количество строк

                    # Заполняем таблицу именами файлов
                    for row, file_name in enumerate(self.Name_File_1D):
                        self.ui.table_tableWidget.setItem(row, 0, QtWidgets.QTableWidgetItem(file_name))  # Добавляем имя файла в первую колонку
                    text = f'Загружено: {len(self.Name_File_1D)} файлов'
                    self.console(text, False)
            else:
                self.console("Папка пустая", False)
                    

    #  Выводит иформацию в консоль
    def console(self, text: str = "", error = False):
        current_time = QTime.currentTime().toString("HH:mm:ss")  # Получаем текущее время в формате ЧЧ:ММ:СС
        if error:
            style_text = 'font-size:11pt;color: red'
        else:
            style_text = 'font-size:11pt;'
        formatted_text = (
            f"<span style='font-size:9pt;'>{current_time}</span>: "
            f"<span style={style_text}>{text}</span>"
        )
        self.ui.Console_textEdit.append(formatted_text)
        self.ui.Console_textEdit.append(" ")
        self.ui.Console_textEdit.ensureCursorVisible()

    # Очистка консоли
    def console_clear(self):
        self.ui.Console_textEdit.clear() 

            



app = QtWidgets.QApplication([])
mainWin = window()
mainWin.showMaximized()
sys.exit(app.exec())


# pyuic6 UI/window.ui -o ui/window.py