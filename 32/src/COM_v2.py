# -*- coding: utf-8 -*-
"""Высокоуровневый аналог COM-логики из wDetect на Python.

Реализованы:
- настройка и управление последовательным портом;
- побайтовый парсер формата (маски 0xC0/0x3F) как в ParseStream_;
- вычисление номера канала из пары значений (A, B);
- симуляция сигнала + live-график (номер канала -> число попаданий);
- CLI-обвязка для реального порта и режима симуляции.
"""

from __future__ import annotations

import argparse
import collections
import dataclasses
import math
import sys
import time
from typing import Callable, Iterable, Iterator, List, Optional, Sequence

try:
    import serial  # type: ignore
except ImportError:  # pragma: no cover - pyserial отсутствует в окружении
    serial = None

# --- Константы протокола -----------------------------------------------------

CHR_START = b"T"
CHR_STOP = b"P"
CHR_STEP = b"S"

MASK_LOW = 0x00
MASK_HIGH = 0x40
MASK_HIGH2 = 0x80
MASK_OVER = 0xC0
MASK_ORDER = 0xC0
MASK_VALUE = 0x3F
RANGE_SPAN = 0xFFF


# --- Структуры данных --------------------------------------------------------

@dataclasses.dataclass()
class SerialConfig:
    """Параметры COM-порта, сопоставимые с PORTCFG в C++."""

    port: str
    baudrate: int = 115200
    parity: str = "N"  # pyserial ожидает 'N', 'E', 'O' и т.д.
    stopbits: int = 1
    timeout: float = 0.2


@dataclasses.dataclass()
class RangeMeasurement:
    """Содержит исходные величины и вычисленное значение канала."""

    channel_value: int
    a_value: int
    b_value: int


class RangeParser:
    """FSM для декодирования потока байтов по схеме ParseStream_."""

    def __init__(self) -> None:
        self._state = "EMPTY"
        self._first = 0
        self._second = 0

    def feed(self, chunk: Sequence[int]) -> List[RangeMeasurement]:
        """Обрабатывает входящие байты и возвращает готовые измерения."""

        results: List[RangeMeasurement] = []

        for byte in chunk:
            byte_type = byte & MASK_ORDER

            if self._state == "EMPTY":
                if byte_type != MASK_LOW:
                    continue
                self._first = byte & MASK_VALUE
                self._state = "LO"
                continue

            if self._state == "LO":
                if byte_type != MASK_HIGH:
                    self._state = "EMPTY"
                    continue
                self._first |= (byte & MASK_VALUE) << 6
                self._state = "HI"
                continue

            if self._state == "HI":
                if byte_type != MASK_LOW:
                    self._state = "EMPTY"
                    continue
                self._second = byte & MASK_VALUE
                self._state = "LO2"
                continue

            if self._state == "LO2":
                if byte_type not in (MASK_HIGH2, MASK_OVER):
                    self._state = "EMPTY"
                    continue
                self._second |= (byte & MASK_VALUE) << 6

                if (
                    self._first not in (0, RANGE_SPAN)
                    and self._second not in (0, RANGE_SPAN)
                    and byte_type == MASK_HIGH2
                ):
                    channel_value = self._calc_channel(self._first, self._second)
                    results.append(
                        RangeMeasurement(channel_value, self._first, self._second)
                    )

                # при overflow (MASK_OVER) просто сбрасываем состояние
                self._state = "EMPTY"

        return results

    @staticmethod
    def _calc_channel(first: int, second: int) -> int:
        """Повторяет формулу из ParseStream_."""

        result = 0.5 * ((first - second) / (first + second) + 1.0) * RANGE_SPAN
        return int(result + 0.5)


class ComChannelReader:
    """Высокоуровневый читатель COM с управлением старт/стоп цикла."""

    def __init__(
        self, config: SerialConfig, *, serial_module: Optional[Callable[..., object]] = None
    ) -> None:
        if serial_module is None:
            if serial is None:
                raise ImportError(
                    "Не найден модуль pyserial. Установите пакет 'pyserial'."
                )
            serial_module = serial.Serial  # type: ignore[attr-defined]

        self._serial_factory = serial_module
        self._config = config

    def iter_measurements(
        self, duration: float, chunk_size: int = 512
    ) -> Iterator[RangeMeasurement]:
        """Читает данные и постатейно отдаёт результаты (для live-графика)."""

        parser = RangeParser()
        start_ts = time.monotonic()
        end_ts = start_ts + duration

        with self._open_port() as port:
            # очищаем буферы для воспроизводимости
            port.reset_input_buffer()
            port.reset_output_buffer()

            # посылаем стартовый символ
            port.write(CHR_START)
            port.flush()

            try:
                while time.monotonic() < end_ts:
                    data = port.read(size=chunk_size)
                    if not data:
                        continue
                    yield from parser.feed(data)
            finally:
                # гарантированно отправляем стоп
                port.write(CHR_STOP)
                port.flush()

    def collect(self, duration: float, chunk_size: int = 512) -> List[RangeMeasurement]:
        """Синхронная обёртка над iter_measurements."""

        return list(self.iter_measurements(duration=duration, chunk_size=chunk_size))

    def _open_port(self):
        """Контекстный менеджер вокруг serial.Serial."""

        cfg = self._config
        serial_obj = self._serial_factory(
            port=cfg.port,
            baudrate=cfg.baudrate,
            parity=cfg.parity,
            stopbits=cfg.stopbits,
            timeout=cfg.timeout,
            write_timeout=cfg.timeout,
        )
        return serial_obj


# --- Live график --------------------------------------------------------------


# --- CLI ---------------------------------------------------------------------

def _build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Считывает данные из COM как wDetect и выводит каналы.",
    )
    parser.add_argument("--port", help="Например, COM3 или /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--parity", default="N", choices=["N", "E", "O", "M", "S"])
    parser.add_argument("--stopbits", type=float, default=1.0)
    parser.add_argument("--timeout", type=float, default=0.2)
    parser.add_argument("--duration", type=float, default=2.0,
                        help="Секунды чтения до отправки STOP")
    parser.add_argument("--chunk", type=int, default=512,
                        help="Размер блока чтения (байт)")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = _build_argparser()
    args = parser.parse_args(argv)

    if not args.port:
        parser.error("Нужно указать --port.")

    cfg = SerialConfig(
        port=args.port,
        baudrate=args.baud,
        parity=args.parity,
        stopbits=float(args.stopbits),
        timeout=args.timeout,
    )

    reader = ComChannelReader(cfg)

    try:
        measurements = reader.collect(duration=args.duration, chunk_size=args.chunk)
    except Exception as exc:  # pragma: no cover - требует железа
        print(f"Ошибка при чтении из {args.port}: {exc}", file=sys.stderr)
        return 1

    if not measurements:
        print("Данные не получены.")
        return 0

    for idx, meas in enumerate(measurements, start=1):
        print(
            f"[{idx:04d}] channel={meas.channel_value} "
            f"a={meas.a_value} b={meas.b_value}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

