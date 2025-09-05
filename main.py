from UI import Ui_MainWindow
from PyQt6 import QtWidgets
from PyQt6.QtWidgets import (QMainWindow)
from PyQt6.QtCore import QTime, QDateTime, QTimer
from PyQt6.QtGui import QFont
from PyQt6 import QtCore
import sys
import os
import pyqtgraph as pg
import numpy as np
import scipy as scipy
from scipy.signal import savgol_filter
from scipy.signal import find_peaks
import scipy.interpolate as interpolate
from scipy.optimize import curve_fit
from scipy.special import voigt_profile

import matplotlib.pyplot as plt
import xraydb

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
        self.ui.Calculation_pushButton.clicked.connect(self.calculation_pushButton)
        self.ui.ApplyCoordinat_pushButton.clicked.connect(self.applyCoordinat_pushButton)
        self.ui.CancleCoordinat_pushButton.clicked.connect(self.cancleCoordinat_pushButton)
        self.ui.AddSpectra_pushButton.clicked.connect(self.addSpectra_pushButton)
        self.ui.DelSpectra_pushButton.clicked.connect(self.delSpectra_pushButton)

        self.ui.Kristal_action.triggered.connect(self.kristalAnalization_pushButton)

        # Создаем PlotWidget для результатов
        self.plot_widget_resoult = pg.PlotWidget()
        self.ui.graphResoult_gridLayout.addWidget(self.plot_widget_resoult)
        self.plot_widget_resoult.addItem(self.text_item_result)
        
        # Подключаем сигнал для обновления координат при наведении
        self.plot_widget_resoult.scene().sigMouseMoved.connect(self.mouse_moved_result)
        
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

        # Добавляем таймер для режима наблюдения
        self.observation_timer = QTimer()
        self.observation_timer.timeout.connect(self.check_new_files)
        self.observation_folder = ""  # Папка для наблюдения
        self.processed_files = []  # Множество уже обработанных файлов

        # Добавляем обработчик изменения состояния чекбокса наблюдения
        self.ui.Observ_checkBox.stateChanged.connect(self.observation_checkbox_changed)

        # Добавляем флаг для отслеживания состояния паузы
        self.observation_paused = False

        # Добавляем переменную для хранения результатов FWHM
        self.fwhm_results = []

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

    def loadFilesData(self, ):
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
        """Сохранение результатов"""
        try:
            # Получаем путь для сохранения
            file_path, _ = QtWidgets.QFileDialog.getSaveFileName(self, "Сохранить файл", "", "Text Files (*.txt)")
            
            if not file_path:
                return
                
            with open(file_path, 'w', encoding='utf-8') as file:
                # Записываем информацию о соединении и элементе
                compound = self.ui.Compound_lineEdit.text()
                element = self.ui.Element_lineEdit.text()
                file.write(f"Соединение: {compound}\n")
                file.write(f"Элемент: {element}\n\n")
                
                # Записываем информацию о калибровке
                if self.calibrated:
                    file.write(f"Калибровка: {self.transition}\n")
                    file.write(f"E1 = {self.ui.E_one_doubleSpinBox.value():.3f} эВ\n")
                    file.write(f"E2 = {self.ui.E_two_doubleSpinBox.value():.3f} эВ\n")
                    file.write(f"N1 = {self.ui.N_one_doubleSpinBox.value():.0f}\n")
                    file.write(f"N2 = {self.ui.N_two_doubleSpinBox.value():.0f}\n\n")
                
                # Записываем информацию о сглаживании
                if self.smoothed:
                    file.write(f"Сглаживание: {self.count_smoothed} точек\n\n")
                
                # Записываем результаты FWHM, если они есть
                if self.fwhm_results:
                    file.write("Результаты расчета FWHM:\n")
                    for result in self.fwhm_results:
                        file.write(f"{result}\n")
                    file.write("\n")
                
                # Записываем данные
                file.write("Данные:\n")
                file.write("X\tY\n")
                for x, y in zip(self.cor_X_File_1D, self.cor_Y_File_1D):
                    file.write(f"{x}\t{y}\n")
                
            self.console(f"Файл сохранен: {file_path}")
            
        except Exception as e:
            self.console(f"Ошибка при сохранении файла: {str(e)}", True)

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
        """Обработчик нажатия кнопки суммирования"""
        # Если наблюдение активно, ставим на паузу
        if self.observation_timer.isActive():
            self.pause_observation()
            return
        # Если на паузе, возобновляем
        elif self.observation_paused:
            self.resume_observation()
            return
            
        self.calibrated = False
        self.smoothed = False
        self.count_smoothed = 0
        
        # Проверяем режим наблюдения
        if self.ui.Observ_checkBox.isChecked():
            self.start_observation_mode()
        else:
            self.normal_sum_mode()

    def normal_sum_mode(self):
        """Обычный режим суммирования"""
        # Убеждаемся, что кнопка имеет стандартный цвет
        self.ui.sum_pushButton.setStyleSheet("")
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

    def start_observation_mode(self):
        """Запуск режима наблюдения"""
        self.reset_state()
        # Запрашиваем папку для наблюдения
        folder_path = QtWidgets.QFileDialog.getExistingDirectory(self, "Выберите папку для наблюдения")
        if not folder_path:
            self.console("Папка для наблюдения не выбрана", True)
            return
        
        self.observation_folder = folder_path
        # Получаем период обновления в минутах и переводим в миллисекунды
        update_period = self.ui.time_spinBox.value() * 60 * 1000
        
        if update_period <= 0:
            self.console("Неверный период обновления", True)
            return

        self.start_observation(update_period)
        # # Выполняем первичное суммирование
        # self.process_files()

    def reset_state(self):
        """Полный сброс состояния данных и флагов"""
        # Останавливаем наблюдение и сбрасываем связанные флаги
        if hasattr(self, 'observation_timer'):
            self.observation_timer.stop()
        self.observation_folder = ""
        self.processed_files = []
        self.observation_paused = False

        # Сбрасываем данные и служебные коллекции
        self.Folder_path_1D = ""
        self.Name_File_1D = []
        self.File_path_1D = []
        self.data_files = []
        self.cor_X_File_1D = []
        self.cor_Y_File_1D = []
        self.mas_sum_1D = []
        self.mas_new_sum_1D = []
        self.table_X = []
        self.table_Y = []

        # Сбрасываем флаги обработки
        self.calibrated = False
        self.smoothed = False
        self.count_smoothed = 0
        self.transition = None

        # Сбрасываем оригинальные массивы и результаты
        self.original_X = []
        self.original_Y = []
        self.fwhm_results = []

        # Очищаем UI-таблицы
        self.ui.table_tableWidget.clear()
        self.ui.table_tableWidget.setRowCount(0)
        self.ui.CoordinatTable_tableWidget.clear()
        self.ui.CoordinatTable_tableWidget.setRowCount(0)
        self.ui.Coordinat_tableWidget.clear()
        self.ui.Coordinat_tableWidget.setRowCount(0)

        # Очищаем графики и возвращаем текстовые элементы
        self.plot_widget_graphs.clear()
        self.plot_widget_graphs.addItem(self.text_item_graphs)
        self.plot_widget_table.clear()
        self.plot_widget_table.addItem(self.text_item_table)
        self.plot_widget_resoult.clear()
        self.plot_widget_resoult.addItem(self.text_item_result)

        # Сбрасываем стиль кнопки суммирования
        self.ui.sum_pushButton.setStyleSheet("")

    def sumData(self, y, sum_points):
        """Суммирование данных с использованием скользящего окна"""
        # Суммируем данные с использованием скользящего окна
        summed_y = []
        for i in range(0, len(y), sum_points):
            # Суммируем значения в окне
            window_sum = sum(y[i:i + sum_points])
            summed_y.append(window_sum)
        return summed_y

    def start_observation(self, period):
        """Запуск режима наблюдения"""
        self.observation_timer.start(period)
        # Меняем цвет кнопки на зеленый
        self.ui.sum_pushButton.setStyleSheet("background-color: #90EE90;")  # Светло-зеленый цвет
        self.console(f"Режим наблюдения запущен. Период проверки: {period/60000:.1f} мин")
        
    def stop_observation(self):
        """Остановка режима наблюдения"""
        self.observation_timer.stop()
        self.observation_folder = ""
        self.processed_files.clear()
        self.observation_paused = False
        # Возвращаем стандартный цвет кнопки
        self.ui.sum_pushButton.setStyleSheet("")
        self.console("Режим наблюдения остановлен")
        
    def check_new_files(self):
        """Проверка новых файлов в папке наблюдения"""
        try:
            new_files = []
            # Получаем список всех .dat и .txt файлов в папке
            valid_extensions = ('.dat', '.txt')
            fileList = os.listdir(self.observation_folder)
            self.File_path_1D = [os.path.normpath(os.path.join(self.observation_folder, file)) 
                                for file in fileList if file.endswith(valid_extensions)]
            self.Name_File_1D = [file for file in fileList if file.endswith(valid_extensions)]

            # Находим новые файлы (исключаем уже обработанные)
            new_files = [f for f in self.File_path_1D if f not in self.processed_files]
            count_proc_files = len(self.processed_files)
            if new_files:
                self.console(f"Найдено новых файлов: {len(new_files)}")
                
                # Сохраняем текущие данные
                current_x = self.cor_X_File_1D.copy() if len(self.cor_X_File_1D) > 0 else None
                current_y = self.cor_Y_File_1D.copy() if len(self.cor_Y_File_1D) > 0 else None
                
                self.ui.table_tableWidget.setRowCount(len(self.File_path_1D))
                # Загружаем и обрабатываем новые файлы
                for i, file_path in enumerate(new_files):
                    data = self.readDataFromFile(file_path)
                    if data is not None:
                        self.data_files.append(data)
                        self.processed_files.append(file_path)

                        # Добавляем в таблицу
                        self.ui.table_tableWidget.setItem(i + count_proc_files, 0, QtWidgets.QTableWidgetItem(self.Name_File_1D[i + count_proc_files]))  # Добавляем имя файла в первую колонку
                        # Строим графики для всех загруженных данных
                        self.loadAndPlotData()
                
                # Выполняем суммирование с учетом новых файлов
                self.process_files(current_x, current_y)
        
        except Exception as e:
            self.console(f"Ошибка при проверке новых файлов: {str(e)}", True)
            self.stop_observation()

    def process_files(self, prev_x=None, prev_y=None):
        """Обработка файлов с учетом предыдущих данных"""
        try:
            if not self.data_files:
                self.console("Нет данных для суммирования", True)
                return

            # Получаем количество точек для суммирования
            sum_points = self.ui.sum_spinBox.value()
            
            if sum_points <= 0:
                self.console("Неверное количество точек для суммирования", True)
                return

            # Создаем массивы для хранения суммированных данных
            total_y = None
            total_x = None

            # Суммируем данные
            for data in self.data_files:
                if data is not None:
                    x, y = data
                    # Суммируем по указанному количеству точек
                    summed_y = self.sumData(y, sum_points)

                    # Если это первый файл, инициализируем total_y
                    if total_y is None:
                        total_y = np.zeros_like(summed_y)
                        total_x = np.arange(len(summed_y))

                    # Добавляем суммированные значения к total_y
                    total_y += summed_y

            if prev_x is not None and prev_y is not None:
                # Используем предыдущие данные
                self.cor_X_File_1D = prev_x
                self.cor_Y_File_1D = total_y
            else:
                # Используем новые данные
                self.cor_X_File_1D = total_x
                self.cor_Y_File_1D = total_y

            # Сохраняем оригинальные данные при первом суммировании
            if not hasattr(self, 'original_X') or len(self.original_X) == 0:
                self.original_X = self.cor_X_File_1D.copy()
                self.original_Y = self.cor_Y_File_1D.copy()

            # Обновляем график
            self.plotSummedData(self.cor_X_File_1D, self.cor_Y_File_1D)
            
            text = f'Просуммировано {len(self.data_files)} файлов'
            self.console(text, False)
            
        except Exception as e:
            self.console(f"Ошибка при обработке файлов: {str(e)}", True)

    def plotSummedData(self, x, y):
        # Очищаем предыдущие графики
        self.plot_widget_resoult.clear()
        self.plot_widget_resoult.addItem(self.text_item_result)  # Возвращаем текстовый элемент

        # Если калибровка выполнена, округляем значения X больше 1 до 3 знаков после запятой
        if self.calibrated:
            x_display = [round(val, 3) if abs(val) > 1 else val for val in x]
        else:
            x_display = x

        # Получаем имя первого файла для легенды
        legend_name = os.path.basename(self.Name_File_1D[0])
        legend_name = legend_name.split('-')
        legend_name = legend_name[0]

        # Строим график для суммированных данных с точками
        self.plot_widget_resoult.plot(x_display, y, pen='g', name=legend_name, 
                                    symbol='o', symbolSize=3, symbolBrush='g')

        # Получаем текущий график и его данные
        plot_item = self.plot_widget_resoult.getPlotItem()
        current_plots = plot_item.listDataItems()

        # Удаляем старую легенду
        if plot_item.legend is not None:
            plot_item.legend.scene().removeItem(plot_item.legend)
            plot_item.legend = None

        # Создаем новую легенду
        legend = plot_item.addLegend(offset=(30, 30))

        # Добавляем текущие графики в легенду
        for plot in current_plots:
            if plot.name() is not None:
                legend.addItem(plot, plot.name())

        # Обновляем таблицу координат
        self.updateCoordinatTable(x_display, y)

    def updateCoordinatTable(self, x, y):
        """Обновление таблицы координат"""
        self.ui.Coordinat_tableWidget.setColumnCount(2)
        self.ui.Coordinat_tableWidget.setColumnWidth(0, 72)
        self.ui.Coordinat_tableWidget.setColumnWidth(1, 72)
        self.ui.Coordinat_tableWidget.setHorizontalHeaderLabels(["X", "Y"])
        self.ui.Coordinat_tableWidget.setRowCount(len(x))  # Устанавливаем количество строк

        for i in range(len(x)):
            self.ui.Coordinat_tableWidget.setItem(i, 0, QtWidgets.QTableWidgetItem(f"{x[i]:.2f}"))  # X
            self.ui.Coordinat_tableWidget.setItem(i, 1, QtWidgets.QTableWidgetItem(f"{y[i]:.2f}"))  # Y

    # Функции для обработки движения мыши и отображения координат
    def mouse_moved_result(self, evt):
        self.show_point_coordinates(self.plot_widget_resoult, evt, self.text_item_result)
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
            # Устанавливаем якорь в правый верхний угол графика
            text_item.setAnchor((1.0, 0.0))
            
            # Устанавливаем шрифт с увеличенным размером
            font = QFont()
            font.setPointSize(12)  # Устанавливаем размер шрифта
            text_item.setFont(font)
            
            text_item.setText(f"X: {closest_point[0]:.2f}\nY: {closest_point[1]:.2f}")
            
            # Вычисляем отступы от краев (5% от размера видимой области)
            x_margin = (x_range[1] - x_range[0]) * 0.05
            y_margin = (y_range[1] - y_range[0]) * 0.05
            
            # Устанавливаем позицию в правом верхнем углу графика с отступами
            text_item.setPos(x_range[1] - x_margin, y_range[1] - y_margin)
            text_item.show()
        else:
            text_item.hide()

    def search_pushButtom(self):
        # Получаем значение из поля Element_lineEdit
        energy_values = {} 
        element_text = self.ui.Element_lineEdit.text().strip()

        if element_text == '':
            self.console("Введите элемент", True)
            return
        
        # Делаем первый символ заглавным, остальные строчными
        element = element_text[0].upper() + element_text[1:].lower()

        # Обращение к базе данных
        self.console(f'# Atomic Symbol: {element}')
        self.console(f'# Atomic Number: {xraydb.atomic_number(element)}')
        self.console(f'# Atomic Moss:   {xraydb.atomic_mass(element):.4f}')

        self.console('# X-ray Lines:')
        self.console('#  Line     Energy  Intensity       Levels')
        for key, val in xraydb.xray_lines(element).items():
            energy_values[key] = val.energy  
            levels = '%s-%s' % (val.initial_level, val.final_level)
            self.console(f'{key} {val.energy} {val.intensity} {levels}')
        
        try:
          
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
                self.ui.E_one_doubleSpinBox.setValue(energy_values['Ka2'])
                self.ui.E_two_doubleSpinBox.setValue(energy_values['Ka1'])
                self.transition = "Ka"
                self.console(f"Установлены значения для линии Ka: {energy_values['Ka2']}, {energy_values['Ka1']}")
            elif clicked_button == kb_button:
                self.ui.E_one_doubleSpinBox.setValue(energy_values['Kb1'])
                self.ui.E_two_doubleSpinBox.setValue(energy_values['Kb5'])
                self.transition = "Kb"
                self.console(f"Установлены значения для линии Kb: {energy_values['Kb1']}, {energy_values['Kb5']}")
            else:
                # Пользователь отменил операцию
                self.console("Операция отменена")
                        
        except ValueError:
            self.console(ValueError, True)

    def kristalAnalization_pushButton(self):
        # Создаем диалоговое окно для расчета угра
        dialog = QtWidgets.QDialog(self)
        dialog.setWindowTitle("Расчет угла")
        dialog.setModal(True)

        # Создаем вертикальный layout
        layout = QtWidgets.QVBoxLayout()

        # Создаем горизонтальный layout для ввода параметров расчета
        search_box = QtWidgets.QFrame()
        search_box.setFrameShape(QtWidgets.QFrame.Shape.StyledPanel)
        search_layout = QtWidgets.QHBoxLayout()
        search_box.setLayout(search_layout)

        # Поле ввода названия элемента
        element_lineEdit = QtWidgets.QLineEdit(placeholderText="Элемент")
        element_lineEdit.setMaximumWidth(100)

        # Блок с выбором линии
        line_radioButton_group = QtWidgets.QButtonGroup()
        line_radioButton_Ka = QtWidgets.QRadioButton("Ka")
        line_radioButton_Kb = QtWidgets.QRadioButton("Kb")
        line_radioButton_group.addButton(line_radioButton_Ka)
        line_radioButton_group.addButton(line_radioButton_Kb)
        # line_radioButton_Ka.toggled.connect(self.line_radioButton_toggled)
        # line_radioButton_Kb.toggled.connect(self.line_radioButton_toggled)
        
        # Выбор решетки
        grid_comboBox = QtWidgets.QComboBox()
        grid_comboBox.setEditable(True)
        grid_comboBox.setMinimumWidth(65)
        grid_comboBox.addItem("1.17")
        grid_comboBox.addItem("3.33")

        # Ввод радиуса
        radius_lineEdit = QtWidgets.QLineEdit(placeholderText = "Радиус")
        radius_lineEdit.setMaximumWidth(100)

        # Кнопка поиска
        search_kristal_button = QtWidgets.QPushButton("🔍")
        search_kristal_button.setFixedWidth(40)

        search_layout.addWidget(element_lineEdit)
        search_layout.addWidget(line_radioButton_Ka)
        search_layout.addWidget(line_radioButton_Kb)
        search_layout.addWidget(grid_comboBox)
        search_layout.addWidget(radius_lineEdit)
        search_layout.addWidget(search_kristal_button)

        # Заполняем layout для первой точки
        first_line_E_lineEdit = QtWidgets.QLineEdit(placeholderText = "Энергия 1 пика")
        first_line_l_lineEdit = QtWidgets.QLineEdit(placeholderText = "Длина волны 1 пика")
        first_line_d_lineEdit = QtWidgets.QLineEdit(placeholderText = "Угол 1 пика")
        first_line_r_lineEdit = QtWidgets.QLineEdit(placeholderText = "Радиус 1 пика")
        first_line_n_lineEdit = QtWidgets.QLineEdit(placeholderText = "Порядок дифракции")

        # Заполняем layout для второй точки
        second_line_E_lineEdit = QtWidgets.QLineEdit(placeholderText = "Энергия 2 пика")
        second_line_l_lineEdit = QtWidgets.QLineEdit(placeholderText = "Длина волны 2 пика")
        second_line_d_lineEdit = QtWidgets.QLineEdit(placeholderText = "Угол 2 пика")
        second_line_r_lineEdit = QtWidgets.QLineEdit(placeholderText = "Радиус 2 пика")
        second_line_n_lineEdit = QtWidgets.QLineEdit(placeholderText = "Порядок дифракции")

        # Список всех QLineEdit, которым нужно задать ширину
        line_edits = [
            first_line_E_lineEdit, first_line_l_lineEdit, first_line_d_lineEdit, first_line_r_lineEdit, first_line_n_lineEdit, second_line_E_lineEdit, second_line_l_lineEdit, second_line_d_lineEdit, second_line_r_lineEdit, second_line_n_lineEdit
        ]

        # Задаём максимальную ширину всем сразу
        for edit in line_edits:
            edit.setMaximumWidth(80)
            edit.setReadOnly(True)

        # Сетка
        grid_layout = QtWidgets.QGridLayout()
        grid_layout.addWidget(QtWidgets.QLabel("№", alignment=QtCore.Qt.AlignmentFlag.AlignCenter), 0, 0)
        grid_layout.addWidget(QtWidgets.QLabel("Энергия (эВ)", alignment=QtCore.Qt.AlignmentFlag.AlignCenter), 0, 1)
        grid_layout.addWidget(QtWidgets.QLabel("Д. волны (Å)", alignment=QtCore.Qt.AlignmentFlag.AlignCenter), 0, 2)
        grid_layout.addWidget(QtWidgets.QLabel("Угол (град)", alignment=QtCore.Qt.AlignmentFlag.AlignCenter), 0, 3)
        grid_layout.addWidget(QtWidgets.QLabel("Радиус (см)", alignment=QtCore.Qt.AlignmentFlag.AlignCenter), 0, 4)
        grid_layout.addWidget(QtWidgets.QLabel("Пор. дифр.", alignment=QtCore.Qt.AlignmentFlag.AlignCenter), 0, 5)

        grid_layout.addWidget(QtWidgets.QLabel("1"), 1, 0)
        grid_layout.addWidget(first_line_E_lineEdit, 1, 1)
        grid_layout.addWidget(first_line_l_lineEdit, 1, 2)
        grid_layout.addWidget(first_line_d_lineEdit, 1, 3)
        grid_layout.addWidget(first_line_r_lineEdit, 1, 4)
        grid_layout.addWidget(first_line_n_lineEdit, 1, 5)

        grid_layout.addWidget(QtWidgets.QLabel("2"), 2, 0)
        grid_layout.addWidget(second_line_E_lineEdit, 2, 1)
        grid_layout.addWidget(second_line_l_lineEdit, 2, 2)
        grid_layout.addWidget(second_line_d_lineEdit, 2, 3)
        grid_layout.addWidget(second_line_r_lineEdit, 2, 4)
        grid_layout.addWidget(second_line_n_lineEdit, 2, 5)

        layout.addWidget(search_box)
        layout.addLayout(grid_layout)
        dialog.setLayout(layout)

        def search_clicked():
            con = True
            if not line_radioButton_Ka.isChecked() and not line_radioButton_Kb.isChecked():
                self.console("Выберите линию", True)
                con = False
            if not element_lineEdit.text():
                self.console("Введите назнание", True)
                con = False
            if not grid_comboBox.currentText():
                self.console("Выберите решетку", True)
                con = False
            if not radius_lineEdit.text():
                self.console("Введите радиус", True)
                con = False
            if con:
                # Получаем значение из поля Element_lineEdit
                energy_values = {}
                l = {}
                chord = {}
                angle = {}
                angle_deg = {}
                d = float(grid_comboBox.currentText())
                line = line_radioButton_Ka.text() if line_radioButton_Ka.isChecked() else line_radioButton_Kb.text()
                n_diffraction = 0

                element_text = element_lineEdit.text().strip()
                element = element_text[0].upper() + element_text[1:].lower()
                
                for key, val in xraydb.xray_lines(element).items():
                    if (line == "Ka" and (key == "Ka1" or key == "Ka2")) or (line == "Kb" and (key == "Kb1" or key == "Kb5")):
                        energy_values[key] = val.energy 
                        l[key] = np.round(12398.41984 / val.energy, 4)
                        for i in [1, 2, 3]:
                            angle[key] = np.round(np.arcsin(i * l[key] / (2 * d)), 3)
                            angle_deg[key] = np.round(np.degrees(angle[key]), 1)
                            if angle_deg[key] > 30 and angle_deg[key] < 60:
                                chord[key] = np.round(float(radius_lineEdit.text()) * np.sin(angle[key]), 1)
                                if chord[key] > 80 and chord[key] < 115:
                                    if key.find("1") != -1:
                                        n_diffraction = i
                                        first_line_E_lineEdit.setText(str(energy_values[key]))
                                        first_line_l_lineEdit.setText(str(l[key]))
                                        first_line_d_lineEdit.setText(str(angle_deg[key]))
                                        first_line_r_lineEdit.setText(str(chord[key]))
                                        first_line_n_lineEdit.setText(str(i))
                                    else:
                                        n_diffraction = i
                                        second_line_E_lineEdit.setText(str(energy_values[key]))
                                        second_line_l_lineEdit.setText(str(l[key]))
                                        second_line_d_lineEdit.setText(str(angle_deg[key]))
                                        second_line_r_lineEdit.setText(str(chord[key]))
                                        second_line_n_lineEdit.setText(str(i))
                                    break
                
                self.console(f"Элемент: {element}", False)
                self.console(f"Линия: {line_radioButton_Ka.text() if line_radioButton_Ka.isChecked() else line_radioButton_Kb.text()}", False)
                self.console(f"Решетка: {grid_comboBox.currentText()}", False)
                self.console(f"Энергия: {energy_values}", False)
                self.console(f"Д. волны: "+ str({k : float(v) for k,v in l.items()}), False)
                self.console(f"Угол: "+ str({k : float(v) for k,v in angle_deg.items()}), False)
                self.console(f"Хорда: "+ str({k : float(v) for k,v in chord.items()}), False)
                self.console(f"Пор. дифр.: {n_diffraction}", False)
                

        search_kristal_button.clicked.connect(search_clicked)
        element_lineEdit.returnPressed.connect(search_clicked)

        result = dialog.exec()


    # def line_radioButton_toggled(self):
    #     if self.line_radioButton_Ka.isChecked():
    #         self.line_radioButton_Ka.setChecked(True)
    #         self.line_radioButton_Kb.setChecked(False)
    #     elif self.line_radioButton_Kb.isChecked():
    #         self.line_radioButton_Kb.setChecked(True)
    #         self.line_radioButton_Ka.setChecked(False)

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

    def calculation_pushButton(self):
        """Расчет FWHM для пиков"""
        if not self.calibrated:
            self.console("Необходимо сначала выполнить калибровку", True)
            return

        # Показываем диалоговое окно для выбора метода расчета FWHM
        msg_box = QtWidgets.QMessageBox()
        msg_box.setWindowTitle("Выбор метода расчета FWHM")
        msg_box.setText("Выберите метод расчета:")
        
        # Добавляем кнопки для выбора метода
        interp_button = msg_box.addButton("Интерполяция", QtWidgets.QMessageBox.ButtonRole.ActionRole)
        lorentz_button = msg_box.addButton("Лоренц", QtWidgets.QMessageBox.ButtonRole.ActionRole)
        cancel_button = msg_box.addButton("Отмена", QtWidgets.QMessageBox.ButtonRole.RejectRole)
        
        # Показываем диалоговое окно и ждем ответа
        msg_box.exec()
        
        clicked_button = msg_box.clickedButton()
        
        if clicked_button == cancel_button:
            return
            
        # Сохраняем выбранный метод
        if clicked_button == interp_button:
            self.fwhm_method = "interpolation"
        elif clicked_button == lorentz_button:
            self.fwhm_method = "lorentz"

        # Остальная логика поиска пиков
        if len(self.cor_X_File_1D) == 0 or len(self.cor_Y_File_1D) == 0:
            self.console("Нет данных для расчета", True)
            return

        # Получаем значения энергий из полей
        E_one = self.ui.E_one_doubleSpinBox.value()
        E_two = self.ui.E_two_doubleSpinBox.value()
        N_one = self.ui.N_one_doubleSpinBox.value()
        N_two = self.ui.N_two_doubleSpinBox.value()

        if E_one == 0 or E_two == 0:
            self.console("Не заданы значения энергий", True)
            return

        # Вычисляем шаг по энергии
        energy_step = abs((E_two - E_one) / (N_two - N_one))
        # Устанавливаем окно поиска как удвоенный шаг
        search_window = energy_step * 2

        # Находим пики выше 40% от максимума
        peaks, _ = find_peaks(self.cor_Y_File_1D, height=np.max(self.cor_Y_File_1D)*0.4)
        
        if len(peaks) == 0:
            self.console("Пики не найдены", True)
            return

        def find_max_peak_near_energy(energy, peaks, window):
            """Находит максимальный пик в окрестности заданной энергии"""
            # Фильтруем пики, которые находятся в пределах окна от заданной энергии
            nearby_peaks = [p for p in peaks if abs(self.cor_X_File_1D[p] - energy) <= window]
            if not nearby_peaks:
                return None
            # Возвращаем индекс пика с максимальной амплитудой
            return max(nearby_peaks, key=lambda x: self.cor_Y_File_1D[x])

        if self.transition == "Ka":
            # Для Ka ищем оба пика
            if len(peaks) < 2:
                self.console("Не найдено достаточное количество пиков для Ka", True)
                return

            # Находим максимальные пики около заданных энергий
            ka2_peak_idx = find_max_peak_near_energy(E_one, peaks, search_window)
            ka1_peak_idx = find_max_peak_near_energy(E_two, peaks, search_window)

            if ka2_peak_idx is None or ka1_peak_idx is None:
                self.console("Не удалось найти пики вблизи заданных энергий", True)
                return

            # Проверяем, что найденные пики достаточно близки к заданным энергиям
            ka2_energy = self.cor_X_File_1D[ka2_peak_idx]
            ka1_energy = self.cor_X_File_1D[ka1_peak_idx]

            if abs(ka2_energy - E_one) > search_window or abs(ka1_energy - E_two) > search_window:
                self.console("Предупреждение: найденные пики значительно отличаются от заданных энергий", True)

            # Вычисляем FWHM для обоих пиков
            if self.fwhm_method == "interpolation":
                self.calculate_fwhm_interpolation(ka2_peak_idx, "Ka₂")
                self.calculate_fwhm_interpolation(ka1_peak_idx, "Ka₁")
            elif self.fwhm_method == "lorentz":
                self.calculate_fwhm_lorentz(ka2_peak_idx, "Ka₂")
                self.calculate_fwhm_lorentz(ka1_peak_idx, "Ka₁")
        else:
            # Для Kb или если переход не указан, ищем один пик
            peak_idx = find_max_peak_near_energy(E_one, peaks, search_window)
            
            if peak_idx is None:
                self.console("Не удалось найти пик вблизи заданной энергии", True)
                return

            peak_energy = self.cor_X_File_1D[peak_idx]

            if abs(peak_energy - E_one) > search_window:
                self.console("Предупреждение: найденный пик значительно отличается от заданной энергии", True)

            # Вычисляем FWHM для найденного пика
            if self.fwhm_method == "interpolation":
                self.calculate_fwhm_interpolation(peak_idx, "Kb" if self.transition == "Kb" else "")
            elif self.fwhm_method == "lorentz":
                self.calculate_fwhm_lorentz(peak_idx, "Kb" if self.transition == "Kb" else "")

    def calculate_fwhm_interpolation(self, peak_idx, peak_name=""):
        """Вычисление FWHM методом интерполяции"""
        try:
            peak_height = self.cor_Y_File_1D[peak_idx]
            half_max = peak_height / 2

            # Ищем точки пересечения слева от пика
            left_idx = peak_idx
            while left_idx > 0 and self.cor_Y_File_1D[left_idx - 1] > half_max:
                left_idx -= 1

            # Интерполяция для точного нахождения левой точки
            if left_idx > 0:
                x_left = np.array([self.cor_X_File_1D[left_idx - 1], self.cor_X_File_1D[left_idx]])
                y_left = np.array([self.cor_Y_File_1D[left_idx - 1], self.cor_Y_File_1D[left_idx]])
                interp_left = np.interp(half_max, y_left, x_left)
            else:
                interp_left = self.cor_X_File_1D[left_idx]

            # Ищем точки пересечения справа от пика
            right_idx = peak_idx
            while right_idx < len(self.cor_Y_File_1D) - 1 and self.cor_Y_File_1D[right_idx + 1] > half_max:
                right_idx += 1

            # Интерполяция для точного нахождения правой точки
            if right_idx < len(self.cor_Y_File_1D) - 1:
                x_right = np.array([self.cor_X_File_1D[right_idx], self.cor_X_File_1D[right_idx + 1]])
                y_right = np.array([self.cor_Y_File_1D[right_idx], self.cor_Y_File_1D[right_idx + 1]])
                interp_right = np.interp(half_max, y_right, x_right)
            else:
                interp_right = self.cor_X_File_1D[right_idx]

            # Вычисляем FWHM
            fwhm = abs(interp_right - interp_left)

            # Отрисовываем результаты
            inter = int(len(self.cor_X_File_1D)*0.2)
            x_data = self.cor_X_File_1D[max(0, peak_idx-inter):min(len(self.cor_X_File_1D), peak_idx+inter)]
            y_data = self.cor_Y_File_1D[max(0, peak_idx-inter):min(len(self.cor_Y_File_1D), peak_idx+inter)]
            
            self.plot_fwhm_results(
                x_data, y_data,
                interp_left, interp_right, half_max,
                None, None,
                peak_name
            )

            # Формируем сообщение
            peak_x = self.cor_X_File_1D[peak_idx]

            message = f"FWHM{' ' + peak_name if peak_name else ''} = {fwhm:.3f} при энергии {peak_x:.3f} эВ"

            # Сохраняем результат
            self.fwhm_results.append(message)
            
            self.console(message, False)

        except Exception as e:
            self.console(f"Ошибка при расчете FWHM: {str(e)}", True)

    def calculate_fwhm_lorentz(self, peak_idx, peak_name=""):
        """Вычисление FWHM с помощью аппроксимации функцией Лоренца"""
        try:
            # Определяем область вокруг пика для фитирования
            window = int(len(self.cor_X_File_1D) * 0.1)  # 10% от общего количества точек
            start_idx = max(0, peak_idx - window)
            end_idx = min(len(self.cor_X_File_1D), peak_idx + window)

            x_data = self.cor_X_File_1D[start_idx:end_idx]
            y_data = self.cor_Y_File_1D[start_idx:end_idx]

            # Получаем реальную высоту пика
            peak_height = self.cor_Y_File_1D[peak_idx]

            def lorentzian(x, x0, gamma, amplitude, offset):
                """Функция Лоренца с амплитудой и смещением"""
                return amplitude * gamma**2 / ((x - x0)**2 + gamma**2) + offset

            # Начальные параметры для фитирования
            p0 = [
                self.cor_X_File_1D[peak_idx],  # x0 - центр пика
                1.0,  # gamma - полуширина на половине высоты
                peak_height,  # amplitude - высота пика
                0.0   # offset - смещение
            ]

            # Ограничения для параметров
            bounds = (
                [self.cor_X_File_1D[peak_idx] - 5, 0.01, peak_height * 0.5, -peak_height * 0.1],  # нижние границы
                [self.cor_X_File_1D[peak_idx] + 5, 10, peak_height * 1.5, peak_height * 0.1]   # верхние границы
            )

            # Фитируем функцию Лоренца
            popt, _ = curve_fit(lorentzian, x_data, y_data, p0=p0, bounds=bounds)
            x0, gamma, amplitude, offset = popt

            # Создаем точки для построения аппроксимирующей кривой
            x_fit = np.linspace(min(x_data), max(x_data), 1000)
            y_fit = lorentzian(x_fit, x0, gamma, amplitude, offset)

            # Вычисляем FWHM для функции Лоренца
            fwhm = 2 * gamma

            # Находим точки пересечения для визуализации
            half_max_y = amplitude / 2 + offset
            left_x = x0 - gamma
            right_x = x0 + gamma

            # Отрисовываем результаты
            self.plot_fwhm_results(
                x_data, y_data,
                left_x, right_x, half_max_y,
                x_fit, y_fit,
                peak_name
            )


            message = f"FWHM{' ' + peak_name if peak_name else ''} (Lorentz) = {fwhm:.3f} при энергии {x0:.3f} эВ"

            # Сохраняем результат
            self.fwhm_results.append(message)
            self.console(message, False)

        except Exception as e:
            self.console(f"Ошибка при расчете FWHM (Lorentz): {str(e)}", True)

    def plot_fwhm_results(self, x_data, y_data, left_x, right_x, half_max_y, x_fit=None, y_fit=None, peak_name=""):
        """Отрисовка результатов расчета FWHM"""
        try:
            # Создаем копию основного графика
            main_plot_item = self.plot_widget_resoult.getPlotItem()
            current_items = main_plot_item.listDataItems()
            
            # Сохраняем текущие настройки отображения
            current_range = self.plot_widget_resoult.viewRange()
            
            # Очищаем график
            self.plot_widget_resoult.clear()
            self.plot_widget_resoult.addItem(self.text_item_result)  # Возвращаем текстовый элемент
            
            # Восстанавливаем основной график
            for item in current_items:
                self.plot_widget_resoult.addItem(item)
            
            # Определяем интервал для отображения
            x_range = max(x_data) - min(x_data)
            x_min = min(x_data) - x_range * 0.2
            x_max = max(x_data) + x_range * 0.2
            
            # Если есть аппроксимация (метод Лоренца), рисуем её
            if x_fit is not None and y_fit is not None:
                self.plot_widget_resoult.plot(x_fit, y_fit, pen='r', name='Lorentz fit')
            
            # Рисуем линию половины максимума
            self.plot_widget_resoult.plot(
                [x_min, x_max],
                [half_max_y, half_max_y],
                pen=pg.mkPen('g', style=QtCore.Qt.PenStyle.DashLine),
                name='Half Maximum'
            )
            
            # Рисуем точки пересечения
            self.plot_widget_resoult.plot(
                [left_x, right_x],
                [half_max_y, half_max_y],
                pen=None,
                symbol='o',
                symbolSize=10,
                symbolBrush='r',
                name='FWHM points'
            )
            
            # Рисуем вертикальные линии для обозначения FWHM
            self.plot_widget_resoult.plot(
                [left_x, left_x],
                [0, half_max_y],
                pen=pg.mkPen('r', style=QtCore.Qt.PenStyle.DashLine)
            )
            self.plot_widget_resoult.plot(
                [right_x, right_x],
                [0, half_max_y],
                pen=pg.mkPen('r', style=QtCore.Qt.PenStyle.DashLine)
            )
            
            # Добавляем легенду
            # self.plot_widget_resoult.addLegend()
            
            # Устанавливаем диапазон отображения с фокусом на область пика
            padding = x_range * 1.2 
            self.plot_widget_resoult.setXRange(x_min - padding, x_max + padding)
            
        except Exception as e:
            self.console(f"Ошибка при отрисовке результатов FWHM: {str(e)}", True)

    def observation_checkbox_changed(self, state):
        """Обработчик изменения состояния чекбокса наблюдения"""
        if not state:  # Если чекбокс выключен
            self.stop_observation()  # Полностью останавливаем наблюдение

    def pause_observation(self):
        """Поставить наблюдение на паузу"""
        self.observation_timer.stop()
        self.observation_paused = True
        # Меняем цвет кнопки на желтый
        self.ui.sum_pushButton.setStyleSheet("background-color: #FFD700;")  # Gold color
        self.console("Режим наблюдения приостановлен")

    def resume_observation(self):
        """Возобновить наблюдение"""
        # Получаем текущий период обновления
        update_period = self.ui.time_spinBox.value() * 60 * 1000
        self.observation_timer.start(update_period)
        self.observation_paused = False
        # Возвращаем зеленый цвет
        self.ui.sum_pushButton.setStyleSheet("background-color: #90EE90;")
        self.console("Режим наблюдения возобновлен")

    def applyCoordinat_pushButton(self):
        """Применяет изменения из таблицы координат к данным"""
        try:
            # Получаем количество строк в таблице
            row_count = self.ui.Coordinat_tableWidget.rowCount()
            
            # Проверяем, что количество строк совпадает с текущими данными
            if row_count != len(self.cor_Y_File_1D):
                self.console("Количество строк в таблице не совпадает с данными", True)
                return
                
            # Создаем новый массив Y
            new_y = []
            
            # Читаем данные из таблицы
            for i in range(row_count):
                item = self.ui.Coordinat_tableWidget.item(i, 1)
                if item is None:
                    self.console(f"Отсутствует значение в строке {i+1}", True)
                    return
                try:
                    y_value = float(item.text())
                    new_y.append(y_value)
                except ValueError:
                    self.console(f"Некорректное значение в строке {i+1}", True)
                    return
                
            # Применяем новые значения Y
            self.cor_Y_File_1D = np.array(new_y)
            
            # Обновляем график
            self.plotSummedData(self.cor_X_File_1D, self.cor_Y_File_1D)
            
            self.console("Изменения координат Y успешно применены")
            
        except Exception as e:
            self.console(f"Ошибка при применении изменений: {str(e)}", True)

    def cancleCoordinat_pushButton(self):
        """Отменяет изменения и возвращает оригинальные данные"""
        try:
            # Проверяем, есть ли оригинальные данные
            if len(self.original_Y) == 0:
                self.console("Нет данных для отмены изменений", True)
                return
            
            # Восстанавливаем оригинальные значения Y
            self.cor_Y_File_1D = self.original_Y.copy()
            
            # Обновляем график
            self.plotSummedData(self.cor_X_File_1D, self.cor_Y_File_1D)
            
            # Обновляем таблицу координат
            self.updateCoordinatTable(self.cor_X_File_1D, self.cor_Y_File_1D)
            
            self.console("Изменения координат Y отменены")
            
        except Exception as e:
            self.console(f"Ошибка при отмене изменений: {str(e)}", True)

    def addSpectra_pushButton(self):
        """Добавление спектра для сравнения"""
        try:
            # Создаем диалоговое окно для выбора метода загрузки
            dialog = QtWidgets.QDialog(self)
            dialog.setWindowTitle("Сравнительный спектр")
            dialog.setModal(True)
            
            # Создаем вертикальный layout
            layout = QtWidgets.QVBoxLayout()
            
            # Создаем горизонтальный layout для спинбокса суммирования
            sum_layout = QtWidgets.QHBoxLayout()
            
            # Добавляем label для спинбокса суммирования
            sum_label = QtWidgets.QLabel("Количество точек для суммирования:")
            sum_layout.addWidget(sum_label)
            
            # Создаем спинбокс для суммирования
            sum_spin = QtWidgets.QSpinBox()
            sum_spin.setMinimum(1)
            sum_spin.setMaximum(1000)
            sum_spin.setValue(self.ui.sum_spinBox.value())  # Берем значение из основного окна
            sum_layout.addWidget(sum_spin)
            
            # Добавляем layout суммирования в основной layout
            layout.addLayout(sum_layout)
            
            # Создаем горизонтальный layout для спинбокса сдвига
            shift_layout = QtWidgets.QHBoxLayout()
            
            # Добавляем label для спинбокса сдвига
            shift_label = QtWidgets.QLabel("Коэффициент сдвига (%):")
            shift_layout.addWidget(shift_label)
            
            # Создаем спинбокс для сдвига
            shift_spin = QtWidgets.QSpinBox()
            shift_spin.setMinimum(15)
            shift_spin.setMaximum(75)
            shift_spin.setValue(50)  # Значение по умолчанию
            shift_layout.addWidget(shift_spin)
            
            # Добавляем layout сдвига в основной layout
            layout.addLayout(shift_layout)
            
            # Создаем кнопки
            button_layout = QtWidgets.QHBoxLayout()
            
            folder_button = QtWidgets.QPushButton("Загрузить из папки")
            files_button = QtWidgets.QPushButton("Выбрать файлы")
            cancel_button = QtWidgets.QPushButton("Отмена")
            
            button_layout.addWidget(folder_button)
            button_layout.addWidget(files_button)
            button_layout.addWidget(cancel_button)
            
            # Добавляем кнопки в layout
            layout.addLayout(button_layout)
            
            # Устанавливаем layout для диалога
            dialog.setLayout(layout)
            
            # Подключаем сигналы кнопок
            folder_button.clicked.connect(dialog.accept)
            files_button.clicked.connect(dialog.accept)
            cancel_button.clicked.connect(dialog.reject)
            
            # Сохраняем ссылку на нажатую кнопку
            dialog.clicked_button = None
            folder_button.clicked.connect(lambda: setattr(dialog, 'clicked_button', 'folder'))
            files_button.clicked.connect(lambda: setattr(dialog, 'clicked_button', 'files'))
            
            # Показываем диалог и ждем ответа
            result = dialog.exec()
            
            if result == QtWidgets.QDialog.DialogCode.Accepted:
                # Получаем значения
                sum_points = sum_spin.value()
                shift_percent = shift_spin.value() / 100.0  # Переводим проценты в десятичную дробь
                
                if dialog.clicked_button == 'folder':
                    # Открываем диалоговое окно для выбора папки
                    folder_path = QtWidgets.QFileDialog.getExistingDirectory(self, "Выберите папку")
                    if not folder_path:
                        return
                        
                    # Получаем список файлов из папки
                    fileList = os.listdir(folder_path)
                    valid_extensions = ('.dat', '.txt')
                    file_paths = [os.path.normpath(os.path.join(folder_path, file)) 
                                 for file in fileList if file.endswith(valid_extensions)]
                    
                elif dialog.clicked_button == 'files':
                    # Открываем диалоговое окно для выбора файлов
                    file_paths, _ = QtWidgets.QFileDialog.getOpenFileNames(
                        self, "Выберите файлы", "", "Data Files (*.dat *.txt)")
                    if not file_paths:
                        return
                else:
                    return
            else:
                return
                
            # Загружаем данные из файлов
            data_files = []
            for file_path in file_paths:
                data = self.readDataFromFile(file_path)
                if data is not None:
                    data_files.append(data)
                    
            if not data_files:
                self.console("Нет данных для суммирования", True)
                return
            
            # Суммируем данные
            total_y = None
            total_x = None
            
            for data in data_files:
                x, y = data
                # Суммируем по указанному количеству точек
                summed_y = self.sumData(y, sum_points)
                
                # Если это первый файл, инициализируем total_y
                if total_y is None:
                    total_y = np.zeros_like(summed_y)
                    total_x = np.arange(len(summed_y))
                    
                # Добавляем суммированные значения к total_y
                total_y += summed_y
                
            # Получаем текущий график и его данные
            plot_item = self.plot_widget_resoult.getPlotItem()
            current_plots = plot_item.listDataItems()
            
            # Определяем максимальную высоту текущего графика
            if current_plots:
                current_max_y = max(plot.yData.max() for plot in current_plots)
                # Добавляем смещение с учетом заданного коэффициента
                total_y = total_y + (current_max_y * shift_percent)
            
            # Цвета и символы для графиков
            colors = ['r', 'b', 'm', 'c', 'y', 'g']  # Красный, синий, пурпурный, голубой, желтый, зеленый
            symbols = ['o', 's', 't', 'd', '+', 'x']  # Круг, квадрат, треугольник, ромб, плюс, крест
            
            # Определяем индекс для цвета и символа
            color_index = len(current_plots) % len(colors)
            
            # Получаем имя первого файла для легенды
            legend_name = os.path.basename(file_paths[0])
            legend_name = legend_name.split('-')
            legend_name = legend_name[0]
            
            # Удаляем старую легенду
            if plot_item.legend is not None:
                plot_item.legend.scene().removeItem(plot_item.legend)
                plot_item.legend = None
            
            # Создаем новую легенду
            legend = plot_item.addLegend(offset=(30, 30))
            
            # Добавляем текущие графики в легенду
            for plot in current_plots:
                if plot.name() is not None:
                    legend.addItem(plot, plot.name())
            
            # Строим новый график
            new_plot = self.plot_widget_resoult.plot(
                total_x, total_y,
                pen=pg.mkPen(colors[color_index], width=2),
                name=legend_name,
                symbol=symbols[color_index],
                symbolSize=3,
                symbolBrush=colors[color_index]
            )
            
            self.console(f"Добавлен спектр из {len(data_files)} файлов")
            
        except Exception as e:
            self.console(f"Ошибка при добавлении спектра: {str(e)}", True)

    def delSpectra_pushButton(self):
        """Удаление графиков"""
        try:
            # Получаем текущий график и его данные
            plot_item = self.plot_widget_resoult.getPlotItem()
            current_plots = plot_item.listDataItems()
            
            # if len(current_plots) <= 1:
            #     self.console("Нет дополнительных графиков для удаления", True)
            #     return
            
            # Создаем диалоговое окно
            dialog = QtWidgets.QDialog(self)
            dialog.setWindowTitle("Удаление графиков")
            dialog.setModal(True)
            
            # Создаем вертикальный layout
            layout = QtWidgets.QVBoxLayout()
            
            # Создаем список графиков
            list_widget = QtWidgets.QListWidget()
            # Добавляем все графики кроме основного
            for plot in current_plots[0:]:
                item = QtWidgets.QListWidgetItem(plot.name())
                list_widget.addItem(item)
            
            layout.addWidget(list_widget)
            
            # Создаем кнопку "Удалить все"
            delete_all_button = QtWidgets.QPushButton("Удалить все добавленные")
            layout.addWidget(delete_all_button)
            
            # Добавляем кнопку закрытия
            close_button = QtWidgets.QPushButton("Закрыть")
            layout.addWidget(close_button)
            
            # Устанавливаем layout для диалога
            dialog.setLayout(layout)
            
            # Обработчик клика по элементу списка
            def on_item_clicked(item):
                # Показываем диалог подтверждения
                msg_box = QtWidgets.QMessageBox()
                msg_box.setWindowTitle("Подтверждение удаления")
                msg_box.setText(f"Хотите удалить график '{item.text()}'?")
                msg_box.setStandardButtons(
                    QtWidgets.QMessageBox.StandardButton.Yes | 
                    QtWidgets.QMessageBox.StandardButton.No
                )
                msg_box.setDefaultButton(QtWidgets.QMessageBox.StandardButton.No)
                
                if msg_box.exec() == QtWidgets.QMessageBox.StandardButton.Yes:
                    # Находим и удаляем выбранный график
                    for plot in current_plots[0:]:
                        if plot.name() == item.text():
                            self.plot_widget_resoult.removeItem(plot)
                            list_widget.takeItem(list_widget.row(item))
                            break
                    
                    # Обновляем легенду
                    self.update_legend()
                    self.console(f"График '{item.text()}' удален")
            
            # Обработчик кнопки "Удалить все"
            def delete_all():
                msg_box = QtWidgets.QMessageBox()
                msg_box.setWindowTitle("Подтверждение удаления")
                msg_box.setText("Хотите удалить все добавленные графики?")
                msg_box.setStandardButtons(
                    QtWidgets.QMessageBox.StandardButton.Yes | 
                    QtWidgets.QMessageBox.StandardButton.No
                )
                msg_box.setDefaultButton(QtWidgets.QMessageBox.StandardButton.No)
                
                if msg_box.exec() == QtWidgets.QMessageBox.StandardButton.Yes:
                    # Удаляем все графики кроме основного
                    for plot in current_plots[0:]:
                        self.plot_widget_resoult.removeItem(plot)
                    list_widget.clear()
                    
                    # Обновляем легенду
                    self.update_legend()
                    self.console("Все добавленные графики удалены")
                    dialog.close()
            
            # Подключаем обработчики событий
            list_widget.itemClicked.connect(on_item_clicked)
            delete_all_button.clicked.connect(delete_all)
            close_button.clicked.connect(dialog.close)
            
            # Показываем диалог
            dialog.exec()
            
        except Exception as e:
            self.console(f"Ошибка при удалении графиков: {str(e)}", True)
            
    def update_legend(self):
        """Обновление легенды"""
        try:
            plot_item = self.plot_widget_resoult.getPlotItem()
            current_plots = plot_item.listDataItems()
            
            # Удаляем старую легенду
            if plot_item.legend is not None:
                plot_item.legend.scene().removeItem(plot_item.legend)
                plot_item.legend = None
            
            # Создаем новую легенду если есть графики
            if current_plots:
                legend = plot_item.addLegend(offset=(30, 30))
                for plot in current_plots:
                    if plot.name() is not None:
                        legend.addItem(plot, plot.name())
                        
        except Exception as e:
            self.console(f"Ошибка при обновлении легенды: {str(e)}", True)



app = QtWidgets.QApplication([])
mainWin = window()
mainWin.showMaximized()
sys.exit(app.exec())


# pyuic6 UI/window.ui -o UI/window.py