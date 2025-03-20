from UI import Ui_MainWindow
from PyQt6 import QtWidgets
from PyQt6.QtWidgets import (QMainWindow)
from PyQt6.QtCore import QTime
import sys
import os
import pyqtgraph as pg
import numpy as np

class window(QMainWindow):
    def __init__(self):
        super(window, self).__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Устанавливаем текстовое поле консоли как только для чтения
        self.ui.Console_textEdit.setReadOnly(True)

        # Переменные
        self.Folder_path_1D = ""
        self.Name_File_1D = []
        self.File_path_1D = []
        self.data_files = []  # Массив для хранения данных из файлов
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
        self.ui.Files_pushButton.clicked.connect(self.Files_pushButton)

        # Создаем PlotWidget для результатов
        self.plot_widget_resoult = pg.PlotWidget()
        self.ui.graphResoult_gridLayout.addWidget(self.plot_widget_resoult)
        
        # Создаем PlotWidget для таблицы
        self.plot_widget_table = pg.PlotWidget()
        self.ui.graphTable_gridLayout.addWidget(self.plot_widget_table)

        # Создаем PlotWidget для графиков
        self.plot_widget_graphs = pg.PlotWidget()
        self.ui.graphs_gridLayout.addWidget(self.plot_widget_graphs)

        # Подключаем обработчик события для нажатия на ячейку таблицы
        self.ui.table_tableWidget.cellClicked.connect(self.on_table_cell_clicked)

        self.console("Программа запущена", False)

    def Folder_pushButton(self):
        # Открываем диалоговое окно для выбора папки
        folder_path = QtWidgets.QFileDialog.getExistingDirectory(self, "Выберите папку")

        # Проверяем, была ли выбрана папка
        if folder_path:
            # Нормализуем путь к папке
            self.Folder_path_1D = os.path.normpath(folder_path)

            # Получаем список всех файлов в выбранной папке
            self.loadFilesFromDirectory(self.Folder_path_1D)

    def Files_pushButton(self):
        # Открываем диалоговое окно для выбора файлов
        file_paths, _ = QtWidgets.QFileDialog.getOpenFileNames(self, "Выберите файлы", "", "Data Files (*.dat *.txt)")

        # Проверяем, были ли выбраны файлы
        if file_paths:
            # Нормализуем пути к файлам
            self.File_path_1D = [os.path.normpath(file) for file in file_paths]
            self.Name_File_1D = [os.path.basename(file) for file in self.File_path_1D]

            # Загружаем данные из выбранных файлов
            self.loadFilesData()

    def loadFilesFromDirectory(self, folder_path):
        # Получаем список всех файлов в выбранной папке
        fileList = os.listdir(folder_path)
        if fileList:
            # Фильтруем файлы по расширениям .dat и .txt
            valid_extensions = ('.dat', '.txt')
            self.File_path_1D = [os.path.normpath(os.path.join(folder_path, file)) 
                                for file in fileList if file.endswith(valid_extensions)]
            self.Name_File_1D = [file for file in fileList if file.endswith(valid_extensions)]

            # Проверяем, есть ли подходящие файлы
            if not self.Name_File_1D:
                self.console("Файлов подходящего формата (dat, txt) нет", True)  # Выводим сообщение об ошибке
            else:
                self.data_files = []  # Очищаем массив данных
                self.ui.table_tableWidget.clear()  # Очищаем таблицу перед добавлением новых данных
                self.ui.table_tableWidget.setRowCount(len(self.Name_File_1D))  # Устанавливаем количество строк

                # Заполняем таблицу именами файлов и загружаем данные
                for row, file_name in enumerate(self.Name_File_1D):
                    self.ui.table_tableWidget.setItem(row, 0, QtWidgets.QTableWidgetItem(file_name))  # Добавляем имя файла в первую колонку
                    file_path = self.File_path_1D[row]
                    data = self.readDataFromFile(file_path)
                    if data is not None:
                        self.data_files.append(data)  # Сохраняем данные в массив

                text = f'Загружено: {len(self.Name_File_1D)} файлов'
                self.console(text, False)

                # Строим графики для всех загруженных данных
                self.loadAndPlotData()
        else:
            self.console("Папка пустая", False)

    def loadFilesData(self):
        # Очищаем массив данных
        self.data_files = []
        self.ui.table_tableWidget.clear()  # Очищаем таблицу перед добавлением новых данных
        self.ui.table_tableWidget.setRowCount(len(self.Name_File_1D))  # Устанавливаем количество строк

        # Заполняем таблицу именами файлов и загружаем данные
        for row, file_name in enumerate(self.Name_File_1D):
            self.ui.table_tableWidget.setItem(row, 0, QtWidgets.QTableWidgetItem(file_name))  # Добавляем имя файла в первую колонку
            file_path = self.File_path_1D[row]
            data = self.readDataFromFile(file_path)
            if data is not None:
                self.data_files.append(data)  # Сохраняем данные в массив

        text = f'Загружено: {len(self.Name_File_1D)} файлов'
        self.console(text, False)

        # Строим графики для всех загруженных данных
        self.loadAndPlotData()

    def loadAndPlotData(self):
        # Очищаем предыдущие графики
        self.plot_widget_graphs.clear()

        for data, file_path in zip(self.data_files, self.File_path_1D):
            # Строим график для каждого файла
            self.plotData(data, file_path)

    def plotData(self, data, file_path):
        # Предполагаем, что данные имеют два столбца: X и Y
        x, y = data
        # Устанавливаем начальное смещение по оси Y
        y_offset = 30
        # Получаем текущее количество графиков
        current_plot_count = len(self.plot_widget_graphs.listDataItems())
        # Применяем смещение к Y
        y_shifted = y + current_plot_count * y_offset
        # Строим график с учетом смещения
        self.plot_widget_graphs.plot(x, y_shifted, pen='r', name=os.path.basename(file_path))

    def readDataFromFile(self, file_path):
        try:
            # Для хранения информации из заголовка
            header_info = []
            header_lines = 0         
            with open(file_path, 'r') as file:
                for line in file:
                    try:
                        values = [float(val) for val in line.strip().split()]
                        if len(values) >= 2:
                            break
                    except ValueError:
                        header_info.append(line.strip())
                        header_lines += 1
            # Читаем данные, пропуская строки заголовка
            data = np.loadtxt(file_path, delimiter=' ', skiprows=header_lines)           
            return data[:, 0], data[:, 1]
        except Exception as e:
            self.console(f"Ошибка при чтении файла {os.path.basename(file_path)}: {str(e)}", True)
            return None

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

    def on_table_cell_clicked(self, row, column):
        # Получаем данные из массива по выбранной ячейке
        if row < len(self.data_files):
            data = self.data_files[row]
            # Строим график в plot_widget_table
            self.plot_widget_table.clear()  # Очищаем предыдущий график
            x, y = data
            self.plot_widget_table.plot(x, y, pen='b', name=self.Name_File_1D[row])  # Строим график

            # Выводим координаты в CoordinatTable_tableWidget
            self.ui.CoordinatTable_tableWidget.clear()  # Очищаем предыдущие координаты
            self.ui.CoordinatTable_tableWidget.setRowCount(len(y))  # Устанавливаем количество строк
            for i in range(len(y)):
                self.ui.CoordinatTable_tableWidget.setItem(i, 0, QtWidgets.QTableWidgetItem(str(x[i])))  # X координаты
                self.ui.CoordinatTable_tableWidget.setItem(i, 1, QtWidgets.QTableWidgetItem(str(y[i])))  # Y координаты

app = QtWidgets.QApplication([])
mainWin = window()
mainWin.showMaximized()
sys.exit(app.exec())


# pyuic6 UI/window.ui -o ui/window.py