from UI import Ui_MainWindow
from PyQt6 import QtWidgets
from PyQt6.QtWidgets import (QMainWindow)
from PyQt6.QtCore import QTime, QDateTime
import sys
import os
import pyqtgraph as pg
import numpy as np
import scipy as scipy
from scipy.signal import savgol_filter

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
        self.cor_Y_File_1D = []
        self.mas_sum_1D = []
        self.mas_new_sum_1D = []

        self.table_X = []
        self.table_Y = []
        
        self.calibrated = False #показывает был ли откалиброван
        self.smoothed = False #показывает был ли сглажен спектр
        self.count_smoothed = 0 #показывает сколько раз был сглажен
        self.transition = None #хранит выбранный переход (Ka или Kb)
        # self.Title_1D
        
        # self.Timer_1D

        # Сохраняем оригинальные данные для возможности отмены операций
        self.original_X = []  # Оригинальные значения X (до калибровки)
        self.original_Y = []  # Оригинальные значения Y (до сглаживания)

        # Элементы для отображения точек координат
        self.text_item_graphs = pg.TextItem(anchor=(0.5, -1.0), color=(0, 0, 0), fill=(255, 255, 255, 200))
        self.text_item_graphs.setZValue(100)  # Устанавливаем высокое значение Z для отображения поверх графика
        self.text_item_table = pg.TextItem(anchor=(0.5, -1.0), color=(0, 0, 0), fill=(255, 255, 255, 200))
        self.text_item_table.setZValue(100)  # Устанавливаем высокое значение Z для отображения поверх графика
        self.text_item_result = pg.TextItem(anchor=(0.5, -1.0), color=(0, 0, 0), fill=(255, 255, 255, 200))
        self.text_item_result.setZValue(100)  # Устанавливаем высокое значение Z для отображения поверх графика
        
        # Тригеры к кнопкам
        self.ui.ClearConsole_pushButton.clicked.connect(self.console_clear) # Очистка консоли
        self.ui.Folder_pushButton.clicked.connect(self.Folder_pushButton) # Загрузка из папки
        self.ui.Files_pushButton.clicked.connect(self.Files_pushButton) # Загрузка выбраных файлов
        self.ui.Save_pushButton.clicked.connect(self.save) # Сохранение
        self.ui.sum_pushButton.clicked.connect(self.sum_pushButton)
        self.ui.search_pushButton.clicked.connect(self.search_pushButtom)
        self.ui.Calibration_pushButton.clicked.connect(self.calibration_pushButton)
        self.ui.CancelCalibrate_pushButton.clicked.connect(self.cancel_calibrate_pushButton)
        self.ui.Smooth_pushButton.clicked.connect(self.smooth_pushButton)
        self.ui.CancelSmooth_pushButton.clicked.connect(self.cancel_smooth_pushButton)

        # Создаем PlotWidget для результатов
        self.plot_widget_resoult = pg.PlotWidget()
        self.ui.graphResoult_gridLayout.addWidget(self.plot_widget_resoult)
        self.plot_widget_resoult.addItem(self.text_item_result)
        self.plot_widget_resoult.scene().sigMouseMoved.connect(self.mouse_moved_result)
        self.plot_widget_resoult.getViewBox().setMouseMode(pg.ViewBox.RectMode)
        
        # Создаем PlotWidget для таблицы
        self.plot_widget_table = pg.PlotWidget()
        self.ui.graphTable_gridLayout.addWidget(self.plot_widget_table)
        self.plot_widget_table.addItem(self.text_item_table)
        self.plot_widget_table.scene().sigMouseMoved.connect(self.mouse_moved_table)
        self.plot_widget_table.getViewBox().setMouseMode(pg.ViewBox.RectMode)

        # Создаем PlotWidget для графиков
        self.plot_widget_graphs = pg.PlotWidget()
        self.ui.graphs_gridLayout.addWidget(self.plot_widget_graphs)
        self.plot_widget_graphs.addItem(self.text_item_graphs)
        self.plot_widget_graphs.scene().sigMouseMoved.connect(self.mouse_moved_graphs)
        self.plot_widget_graphs.getViewBox().setMouseMode(pg.ViewBox.RectMode)

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
        # Строим график с учетом смещения (линия с точками)
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

    def save(self):
        # Проверяем, есть ли данные для сохранения
        if len(self.cor_X_File_1D) == 0 or len(self.cor_Y_File_1D) == 0:
            self.console("Нет данных для сохранения", True)
            return
            
        # Получаем значения из полей
        compound = self.ui.Compound_lineEdit.text().strip()
        element = self.ui.Element_lineEdit.text().strip()
        
        # Если поля пустые, используем значения по умолчанию
        if not compound:
            compound = "NoCompound"
        if not element:
            element = "element"
            
        # Формируем путь для сохранения результатов
        if self.transition:
            # Если переход известен, используем его в пути
            result_dir = os.path.join("Resoult", compound, element, self.transition)
            transition_str = self.transition
        else:
            # Если переход не известен, сохраняем просто по compound/element
            result_dir = os.path.join("Resoult", compound, element)
            transition_str = "NoTransition"
        
        # Создаем директорию, если она не существует
        os.makedirs(result_dir, exist_ok=True)
        
        # Формируем имя файла с соединением, элементом, переходом и датой
        current_date = QDateTime.currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")
        file_name = f"{compound}_{element}_{transition_str}_{current_date}.dat"
        file_path = os.path.join(result_dir, file_name)
        
        try:
            # Сохраняем данные в файл
            with open(file_path, 'w') as file:
                for x, y in zip(self.cor_X_File_1D, self.cor_Y_File_1D):
                    file.write(f"{x} {y}\n")
            
            # Создаем директорию для логов
            if self.transition:
                logs_dir = os.path.join("Logs", compound, element, self.transition)
            else:
                logs_dir = os.path.join("Logs", compound, element)
                
            os.makedirs(logs_dir, exist_ok=True)
            
            # Создаем файл логов
            log_file_path = os.path.join(logs_dir, f"{current_date}.log")
            
            # Записываем информацию в лог
            with open(log_file_path, 'w') as log_file:
                log_file.write(f"Время сохранения: {QDateTime.currentDateTime().toString('yyyy-MM-dd HH:mm:ss')}\n")
                log_file.write(f"Соединение: {compound}\n")
                log_file.write(f"Элемент: {element}\n")
                log_file.write(f"Переход: {self.transition if self.transition else 'Не указан'}\n")
                log_file.write(f"Количество файлов: {len(self.data_files)}\n")
                log_file.write(f"Суммирование по точкам: {self.ui.sum_spinBox.value()}\n")
                
                # Информация о калибровке
                log_file.write(f"Калибровка: {'Да' if self.calibrated else 'Нет'}\n")
                if self.calibrated:
                    log_file.write(f"Энергия 1: {self.ui.E_one_doubleSpinBox.value()}\n")
                    log_file.write(f"Энергия 2: {self.ui.E_two_doubleSpinBox.value()}\n")
                    log_file.write(f"Номер пика 1: {self.ui.N_one_doubleSpinBox.value()}\n")
                    log_file.write(f"Номер пика 2: {self.ui.N_two_doubleSpinBox.value()}\n")
                
                # Информация о сглаживании
                log_file.write(f"Сглаживание: {'Да' if self.smoothed else 'Нет'}\n")
                if self.smoothed:
                    log_file.write(f"Количество точек сглаживания: {self.count_smoothed}\n")
            
            self.console(f"Данные сохранены в {file_path}")
            self.console(f"Лог сохранен в {log_file_path}")
            
        except Exception as e:
            self.console(f"Ошибка при сохранении: {str(e)}", True)

    #  Выводит иформацию в консоль
    def console(self, text: str = "", error = False):
        current_time = QTime.currentTime().toString("HH:mm:ss")  # Получаем текущее время в формате ЧЧ:ММ:СС
        if error:
            style_text = 'font-size:11pt;color: "red"'
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
            self.plot_widget_table.addItem(self.text_item_table)  # Возвращаем текстовый элемент
            x, y = data
            self.plot_widget_table.plot(x, y, pen='b', name=self.Name_File_1D[row], 
                                        symbol='o', symbolSize=3, symbolBrush='b')  # Строим график с точками

            # Выводим координаты в CoordinatTable_tableWidget
            self.ui.CoordinatTable_tableWidget.clear()  # Очищаем предыдущие координаты
            self.ui.CoordinatTable_tableWidget.setRowCount(len(y))  # Устанавливаем количество строк
            for i in range(len(y)):
                self.ui.CoordinatTable_tableWidget.setItem(i, 0, QtWidgets.QTableWidgetItem(str(x[i])))  # X координаты
                self.ui.CoordinatTable_tableWidget.setItem(i, 1, QtWidgets.QTableWidgetItem(str(y[i])))  # Y координаты

    def sum_pushButton(self):
        self.calibrated = False
        self.smoothed = False
        self.count_smoothed = 0
        # Получаем значение из spinBox
        sum_points = self.ui.sum_spinBox.value()
        
        # Проверяем, что значение больше 0
        if sum_points <= 0:
            self.console("Количество точек для суммирования должно быть больше 0", True)
            return

        # Создаем массив для хранения суммированных данных
        total_y = None
        total_x = None

        # Суммируем данные по X
        for data in self.data_files:
            if data is not None:
                x, y = data
                # Суммируем по указанному количеству точек
                summed_y = self.sumData(y, sum_points)

                # Если это первый файл, инициализируем total_y
                if total_y is None:
                    total_y = np.zeros_like(summed_y)  # Инициализируем массив нулями
                    total_x = np.arange(len(summed_y)) # Создаем X для суммированных данных 
                    self.cor_X_File_1D = total_x

                # Добавляем суммированные значения к total_y
                total_y += summed_y

        self.cor_Y_File_1D = total_y
        # Сохраняем оригинальные данные
        self.original_X = self.cor_X_File_1D.copy()
        self.original_Y = self.cor_Y_File_1D.copy()
        
        # Теперь можно построить график для суммированных данных
        self.plotSummedData(total_x, total_y)

        text = f'Просуммировано {len(self.data_files)} файлов'
        self.console(text, False)

    def sumData(self, y, sum_points):
        # Суммируем данные с использованием скользящего окна
        summed_y = []
        for i in range(0, len(y), sum_points):
            # Суммируем значения в окне
            window_sum = sum(y[i:i + sum_points])
            summed_y.append(window_sum)
        return summed_y

    def plotSummedData(self, x, y):
        # Очищаем предыдущие графики
        self.plot_widget_resoult.clear()
        self.plot_widget_resoult.addItem(self.text_item_result)  # Возвращаем текстовый элемент

        # Если калибровка выполнена, округляем значения X больше 1 до 3 знаков после запятой
        if self.calibrated:
            x_display = [round(val, 3) if abs(val) > 1 else val for val in x]
        else:
            x_display = x

        # Строим график для суммированных данных с точками
        self.plot_widget_resoult.plot(x_display, y, pen='g', name='Суммированные данные', 
                                    symbol='o', symbolSize=3, symbolBrush='g')
    
    # Функции для обработки движения мыши и отображения координат
    def mouse_moved_result(self, pos):
        self.show_point_coordinates(self.plot_widget_resoult, pos, self.text_item_result)       
    def mouse_moved_table(self, pos):
        self.show_point_coordinates(self.plot_widget_table, pos, self.text_item_table)
    def mouse_moved_graphs(self, pos):
        self.show_point_coordinates(self.plot_widget_graphs, pos, self.text_item_graphs)
          
    def show_point_coordinates(self, plot_widget, pos, text_item):
        # Преобразование позиции мыши в координаты графика
        plot_point = plot_widget.plotItem.vb.mapSceneToView(pos)
        x, y = plot_point.x(), plot_point.y()
        
        # Получаем все элементы графика
        data_items = plot_widget.listDataItems()
        
        # Получаем диапазоны осей для определения адаптивного порога близости
        view_box = plot_widget.getViewBox()
        view_range = view_box.viewRange()
        x_range = view_range[0]  # Диапазон по X
        y_range = view_range[1]  # Диапазон по Y
        
        # Вычисляем адаптивный порог расстояния (3% от диапазона осей)
        x_threshold = (x_range[1] - x_range[0]) * 0.03
        y_threshold = (y_range[1] - y_range[0]) * 0.03
        threshold_distance = min(x_threshold, y_threshold)
        # Ограничиваем минимальное и максимальное значение порога
        threshold_distance = max(min(threshold_distance, 10), 3)
        
        # Проверяем близость к точкам
        closest_point = None
        min_distance = float('inf')
        
        for item in data_items:
            data_x = item.xData
            data_y = item.yData
            
            if data_x is None or data_y is None:
                continue
                
            for i in range(len(data_x)):
                # Вычисляем расстояние до точки графика
                distance = ((data_x[i] - x) ** 2 + (data_y[i] - y) ** 2) ** 0.5
                if distance < min_distance and distance < threshold_distance:  # Используем адаптивный порог
                    min_distance = distance
                    closest_point = (data_x[i], data_y[i])
        
        # Если нашли ближайшую точку - показываем координаты
        if closest_point:
            # Определяем границы для позиционирования
            x_width = x_range[1] - x_range[0]
            y_height = y_range[1] - y_range[0]
            
            # Отступы от границ (15% от размера видимой области)
            x_margin = x_width * 0.15
            y_margin = y_height * 0.15
            
            # Правая и левая границы
            right_bound = x_range[1] - x_margin
            left_bound = x_range[0] + x_margin
            # Верхняя граница с отступом
            upper_bound = y_range[1] - y_margin
            
            # Определяем положение точки относительно границ
            near_right = closest_point[0] > right_bound
            near_left = closest_point[0] < left_bound
            near_top = closest_point[1] > upper_bound
            
            # Устанавливаем якорь в зависимости от положения точки
            if near_top:
                if near_right:
                    text_item.setAnchor((1.0, 0.0))  # Точка в правом верхнем углу
                elif near_left:
                    text_item.setAnchor((0.0, 0.0))  # Точка в левом верхнем углу
                else:
                    text_item.setAnchor((0.5, 0.0))  # Точка вверху по центру
            else:
                if near_right:
                    text_item.setAnchor((1.0, 1.0))  # Точка в правом нижнем углу
                elif near_left:
                    text_item.setAnchor((0.0, 1.0))  # Точка в левом нижнем углу
                else:
                    # В центральной области используем стандартную логику
                    if closest_point[0] > (x_range[0] + x_range[1]) / 2:
                        text_item.setAnchor((0.0, 1.0))  # Якорь слева внизу текста
                    else:
                        text_item.setAnchor((1.0, 1.0))  # Якорь справа внизу текста
            
            text_item.setText(f"X: {closest_point[0]:.2f}\nY: {closest_point[1]:.2f}")
            text_item.setPos(closest_point[0], closest_point[1])
            text_item.show()
        else:
            text_item.hide()

    def search_pushButtom(self):
        # Получаем значение из поля Element_lineEdit
        element_text = self.ui.Element_lineEdit.text().strip()

        if element_text == '':
            self.console("Введите элемент", True)
            return
        
        # Делаем первый символ заглавным, остальные строчными
        element = element_text[0].upper() + element_text[1:].lower()
        
        # Формируем путь к папке и файлу
        folder_path = os.path.join('Base', element)
        file_path = os.path.join(folder_path, f"{element}.dat")
        
        # Проверяем существование папки и файла
        if not os.path.exists(folder_path):
            self.console(f"Папка {folder_path} не найдена", True)
            return
            
        if not os.path.exists(file_path):
            self.console(f"Файл {file_path} не найден", True)
            return
        
        try:
            # Открываем файл и читаем последнюю строку
            with open(file_path, 'r') as file:
                lines = file.readlines()
                
                if not lines:
                    self.console(f"Файл {file_path} пуст", True)
                    return
                last_line = lines[-1].strip()
                
                # Разбиваем строку по символу "/"
                parts = last_line.split('/')
                
                if len(parts) < 2:
                    self.console(f"Неверный формат данных в файле {file_path}", True)
                    return
                
                # Получаем числа из второй части
                try:
                    energy_values = [float(val) for val in parts[1].strip().split()]

                    
                    if len(energy_values) < 5:  # Нам нужно минимум 5 чисел для Ka и Kb
                        self.console(f"Недостаточно значений в файле {file_path}", True)
                        return
                        
                    # Создаем диалоговое окно для выбора линии
                    msg_box = QtWidgets.QMessageBox()
                    msg_box.setWindowTitle("Выбор линии")
                    msg_box.setText("Какая линия?")
                    
                    # Добавляем кнопки Ka и Kb
                    ka_button = msg_box.addButton("Ka", QtWidgets.QMessageBox.ButtonRole.ActionRole)
                    kb_button = msg_box.addButton("Kb", QtWidgets.QMessageBox.ButtonRole.ActionRole)
                    cancel_button = msg_box.addButton("Отмена", QtWidgets.QMessageBox.ButtonRole.RejectRole)
                    
                    # Показываем диалоговое окно и ждем ответа
                    msg_box.exec()
                    
                    clicked_button = msg_box.clickedButton()
                    
                    # Устанавливаем значения в зависимости от выбора пользователя
                    if clicked_button == ka_button:
                        # Для Ka используем первые два числа, умноженные на 1000
                        self.ui.E_one_doubleSpinBox.setValue(energy_values[0] * 1000)
                        self.ui.E_two_doubleSpinBox.setValue(energy_values[1] * 1000)
                        self.transition = "Ka"
                        self.console(f"Установлены значения для линии Ka: {energy_values[0] * 1000}, {energy_values[1] * 1000}")
                    elif clicked_button == kb_button:
                        # Для Kb используем 4-е и 5-е числа, умноженные на 1000
                        self.ui.E_one_doubleSpinBox.setValue(energy_values[3] * 1000)
                        self.ui.E_two_doubleSpinBox.setValue(energy_values[4] * 1000)
                        self.transition = "Kb"
                        self.console(f"Установлены значения для линии Kb: {energy_values[3] * 1000}, {energy_values[4] * 1000}")
                    else:
                        # Пользователь отменил операцию
                        self.console("Операция отменена")
                        
                except ValueError:
                    self.console(f"Ошибка при чтении числовых значений из файла {file_path}", True)
                    
        except Exception as e:
            self.console(f"Ошибка при обработке файла {file_path}: {str(e)}", True)

    def calibration_pushButton(self):
        # Получаем значения из полей
        E_one = self.ui.E_one_doubleSpinBox.value()
        E_two = self.ui.E_two_doubleSpinBox.value()
        N_one = self.ui.N_one_doubleSpinBox.value()
        N_two = self.ui.N_two_doubleSpinBox.value()
        
        # Проверяем, чтобы значения были не нулевые
        if E_one == 0 or E_two == 0 or N_one == 0 or N_two == 0:
            self.console("Ошибка: Все значения должны быть ненулевыми.", True)
            return
        
        # Если переход не был выбран через поиск, спрашиваем о нем
        if self.transition is None:
            # Создаем диалоговое окно для выбора линии
            msg_box = QtWidgets.QMessageBox()
            msg_box.setWindowTitle("Выбор линии")
            msg_box.setText("Какая линия?")
            
            # Сохраняем оригинальные X перед калибровкой, если еще не сохранены
            if not self.original_X:
                self.original_X = self.cor_X_File_1D.copy()
            
            # Добавляем кнопки Ka и Kb
            ka_button = msg_box.addButton("Ka", QtWidgets.QMessageBox.ButtonRole.ActionRole)
            kb_button = msg_box.addButton("Kb", QtWidgets.QMessageBox.ButtonRole.ActionRole)
            cancel_button = msg_box.addButton("Отмена", QtWidgets.QMessageBox.ButtonRole.RejectRole)
            
            # Показываем диалоговое окно и ждем ответа
            msg_box.exec()
            
            clicked_button = msg_box.clickedButton()
            
            if clicked_button == ka_button:
                self.transition = "Ka"
                self.console("Выбрана линия Ka")
            elif clicked_button == kb_button:
                self.transition = "Kb"
                self.console("Выбрана линия Kb")
            elif clicked_button == cancel_button:
                self.console("Операция отменена")
                return
        
        # Калибровка графика
        [self.cor_X_File_1D, energy_step] = self.convert_to_energy(self.cor_X_File_1D, E_one, E_two, N_one, N_two)
        
        self.calibrated = True
        # Обновляем график
        self.plotSummedData(self.cor_X_File_1D, self.cor_Y_File_1D)
        self.console(f'График откалиброван, шаг по энергии: {energy_step}', False)

    def convert_to_energy(self, cor_X_File_1D, E_one, E_two, N_one, N_two):
        # Вычисляем шаг по энергии
        energy_step = (E_two - E_one) / (N_two - N_one)
        energy_step = np.round(energy_step, 3) 
        
        # Вычисляем энергию первой точки
        first_energy = E_one - energy_step * (N_one - cor_X_File_1D[0])
        
        # Вычисляем энергию для каждой точки
        calibrated_values = [first_energy]  # Начинаем с первой энергии
        for i in range(1, len(cor_X_File_1D)):
            energy = first_energy + energy_step * i  # Прибавляем шаг для каждой следующей точки
            # Округляем значения больше 1 до 3 знаков после запятой
            if abs(energy) > 1:
                energy = round(energy, 3)
            calibrated_values.append(energy)
        
        return [calibrated_values, energy_step]

    def smooth_pushButton(self):
        # Проверяем, есть ли данные для сглаживания
        if len(self.cor_X_File_1D) == 0 or len(self.cor_Y_File_1D) == 0:
            self.console("Нет данных для сглаживания", True)
            return
            
        # Получаем количество точек для сглаживания
        points_count = self.ui.smooth_spinBox.value()
        
        # Проверяем, чтобы количество точек не было слишком большим
        if points_count >= len(self.cor_Y_File_1D):
            self.console(f"Количество точек для сглаживания слишком большое. Максимальное значение: {len(self.cor_Y_File_1D) - 1}", True)
            return
        
        # Количество точек должно быть нечетным для Savitzky-Golay
        if points_count % 2 == 0:
            points_count += 1
            self.ui.smooth_spinBox.setValue(points_count)
            self.console(f"Количество точек увеличено до {points_count} (должно быть нечетным)", False)
        
        # Показываем диалоговое окно для выбора метода сглаживания
        msg_box = QtWidgets.QMessageBox()
        msg_box.setWindowTitle("Выбор метода сглаживания")
        msg_box.setText("Выберите метод сглаживания:")
        
        # Добавляем кнопки для выбора метода
        adjacent_button = msg_box.addButton("Adjacent-Averaging", QtWidgets.QMessageBox.ButtonRole.ActionRole)
        savgol_button = msg_box.addButton("Savitzky-Golay", QtWidgets.QMessageBox.ButtonRole.ActionRole)
        cancel_button = msg_box.addButton("Отмена", QtWidgets.QMessageBox.ButtonRole.RejectRole)
        
        # Показываем диалоговое окно и ждем ответа
        msg_box.exec()
        
        clicked_button = msg_box.clickedButton()
        
        # Сохраняем оригинальные данные Y перед первым сглаживанием
        if not self.smoothed and self.original_Y.size == 0:
            self.original_Y = self.cor_Y_File_1D.copy()
            
        try:
            if clicked_button == adjacent_button:
                # Применяем метод Adjacent-Averaging
                smoothed_data = self.adjacent_averaging(self.cor_Y_File_1D, points_count)
                self.cor_Y_File_1D = smoothed_data
                self.smoothed = True
                self.count_smoothed = points_count
                self.console(f"Применено сглаживание методом Adjacent-Averaging по {points_count} точкам")
                
            elif clicked_button == savgol_button:
                # Для Savitzky-Golay нужен дополнительный параметр - порядок полинома
                order, ok = QtWidgets.QInputDialog.getInt(
                    self, "Порядок полинома", 
                    "Введите порядок полинома (должен быть < количества точек):",
                    min=1, max=points_count-1, step=1, value=3
                )
                
                if ok:
                    # Проверяем, чтобы порядок был меньше количества точек
                    if order >= points_count:
                        order = points_count - 1
                        self.console(f"Порядок полинома уменьшен до {order} (должен быть < количества точек)", False)
                    

                    try:
                        # Применяем фильтр Savitzky-Golay
                        smoothed_data = savgol_filter(self.cor_Y_File_1D, points_count, order)
                        self.cor_Y_File_1D = smoothed_data
                        self.smoothed = True
                        self.count_smoothed = points_count
                        self.console(f"Применено сглаживание методом Savitzky-Golay по {points_count} точкам с порядком {order}")
                    except ImportError:
                        self.console(f'Ошибка: {e}', True)
                        return
                else:
                    # Пользователь отменил ввод порядка
                    self.console("Операция сглаживания отменена")
                    return
                    
            elif clicked_button == cancel_button:
                # Пользователь отменил операцию
                self.console("Операция сглаживания отменена")
                return
                
            # Обновляем график
            self.plotSummedData(self.cor_X_File_1D, self.cor_Y_File_1D)
            
        except Exception as e:
            self.console(f"Ошибка при сглаживании: {str(e)}", True)

    def adjacent_averaging(self, data, points_count):
        """Метод сглаживания Adjacent-Averaging (скользящее среднее)"""
        result = np.zeros_like(data)
        n = len(data)
        half_window = points_count // 2
        
        # Обрабатываем граничные точки (начало массива)
        for i in range(half_window):
            result[i] = np.mean(data[:i + half_window + 1])
            
        # Обрабатываем основную часть массива
        for i in range(half_window, n - half_window):
            result[i] = np.mean(data[i - half_window:i + half_window + 1])
            
        # Обрабатываем граничные точки (конец массива)
        for i in range(n - half_window, n):
            result[i] = np.mean(data[i - half_window:])
            
        return result

    def cancel_calibrate_pushButton(self):
        """Отмена калибровки"""
        if not self.calibrated:
            self.console("Калибровка не была применена", True)
            return
            
        if self.original_X.size == 0:
            self.console("Невозможно отменить калибровку: исходных данных нет", True)
            return
            
        # Восстанавливаем оригинальные значения X
        self.cor_X_File_1D = self.original_X.copy()
        self.calibrated = False
        
        # Обновляем график
        self.plotSummedData(self.cor_X_File_1D, self.cor_Y_File_1D)
        self.console("Калибровка отменена")

    def cancel_smooth_pushButton(self):
        """Отмена сглаживания"""
        if not self.smoothed:
            self.console("Сглаживание не было применено", True)
            return
            
        if self.original_Y.size == 0:
            self.console("Невозможно отменить сглаживание: исходных данных нет", True)
            return
            
        # Восстанавливаем оригинальные значения Y
        self.cor_Y_File_1D = self.original_Y.copy()
        self.smoothed = False
        self.count_smoothed = 0
        
        # Обновляем график
        self.plotSummedData(self.cor_X_File_1D, self.cor_Y_File_1D)
        self.console("Сглаживание отменено")

app = QtWidgets.QApplication([])
mainWin = window()
mainWin.showMaximized()
sys.exit(app.exec())


# pyuic6 UI/window.ui -o ui/window.py