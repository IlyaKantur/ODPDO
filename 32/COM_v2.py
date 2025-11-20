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
import random
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

@dataclasses.dataclass(slots=True)
class SerialConfig:
    """Параметры COM-порта, сопоставимые с PORTCFG в C++."""

    port: str
    baudrate: int = 9600
    parity: str = "N"  # pyserial ожидает 'N', 'E', 'O' и т.д.
    stopbits: int = 1
    timeout: float = 0.2


@dataclasses.dataclass(slots=True)
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


# --- Симуляция сигнала --------------------------------------------------------

def _encode_pair(first: int, second: int) -> List[int]:
    """Разбивает два 12-битных значения на 4 байта по протоколу."""

    return [
        (first & MASK_VALUE) | MASK_LOW,
        ((first >> 6) & MASK_VALUE) | MASK_HIGH,
        (second & MASK_VALUE) | MASK_LOW,
        ((second >> 6) & MASK_VALUE) | MASK_HIGH2,
    ]


def generate_simulated_bytes(
    samples: int,
    *,
    seed: Optional[int] = None,
    noise: float = 0.08,
) -> Iterator[int]:
    """Формирует поток байтов, имитирующий работу прибора."""

    rng = random.Random(seed)
    samples = max(samples, 1)

    for idx in range(samples):
        # базируемся на синусоиде + шум -> условный сдвиг канала
        phase = 2.0 * math.pi * (idx / samples)
        ratio = 0.5 + 0.35 * math.sin(phase) + rng.uniform(-noise, noise)
        ratio = max(0.05, min(0.95, ratio))

        total = RANGE_SPAN - 2
        first = int(total * ratio)
        second = total - first
        second = max(1, min(RANGE_SPAN - 1, second))
        first = max(1, min(RANGE_SPAN - 1, first))

        for byte in _encode_pair(first, second):
            yield byte


def simulated_measurement_stream(
    samples: int,
    *,
    seed: Optional[int] = None,
    chunk_size: int = 4,
    delay: float = 0.2,
) -> Iterator[RangeMeasurement]:
    """Отдаёт измерения в темпе, приближенном к реальному устройству."""

    parser = RangeParser()
    chunk: List[int] = []

    for byte in generate_simulated_bytes(samples, seed=seed):
        chunk.append(byte)
        if len(chunk) >= chunk_size:
            for measurement in parser.feed(chunk):
                yield measurement
                if delay > 0:
                    time.sleep(delay)
            chunk.clear()

    if chunk:
        for measurement in parser.feed(chunk):
            yield measurement
            if delay > 0:
                time.sleep(delay)


# --- Live график --------------------------------------------------------------

def plot_live_histogram(
    measurements: Iterable[RangeMeasurement],
    *,
    refresh_interval: float = 0.2,
    max_points: Optional[int] = None,
) -> collections.Counter[int]:
    """Строит график (канал -> количество попаданий) в режиме реального времени."""

    try:
        import matplotlib.pyplot as plt  # type: ignore
    except ImportError as exc:  # pragma: no cover - требуется matplotlib
        raise RuntimeError("Для графика установите пакет matplotlib") from exc

    plt.ion()
    fig, ax = plt.subplots()
    plt.show(block=False)
    ax.set_xlabel("Номер канала")
    ax.set_ylabel("Количество")
    ax.set_title("Live histogram (канал / попадания)")

    counts: collections.Counter[int] = collections.Counter()
    xs: List[int] = []
    ys: List[int] = []
    last_refresh = time.monotonic()

    def _render() -> None:
        xs.clear()
        ys.clear()
        for key in sorted(counts):
            xs.append(key)
            ys.append(counts[key])

        ax.clear()
        ax.bar(xs, ys, width=1.0)
        ax.set_xlabel("Номер канала")
        ax.set_ylabel("Количество")
        ax.set_title("Live histogram (канал / попадания)")
        ax.set_xlim(0, RANGE_SPAN)
        fig.canvas.draw()
        fig.canvas.flush_events()

    for idx, meas in enumerate(measurements, start=1):
        counts[meas.channel_value] += 1
        current = time.monotonic()
        if current - last_refresh >= refresh_interval:
            _render()
            last_refresh = current

        if max_points is not None and idx >= max_points:
            break

    _render()
    plt.ioff()
    plt.show(block=True)

    return counts


# --- CLI ---------------------------------------------------------------------

def _build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Считывает данные из COM как wDetect и выводит каналы.",
    )
    parser.add_argument("--port", help="Например, COM3 или /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--parity", default="N", choices=["N", "E", "O", "M", "S"])
    parser.add_argument("--stopbits", type=float, default=1.0)
    parser.add_argument("--timeout", type=float, default=0.2)
    parser.add_argument("--duration", type=float, default=2.0,
                        help="Секунды чтения до отправки STOP")
    parser.add_argument("--chunk", type=int, default=512,
                        help="Размер блока чтения (байт)")
    parser.add_argument("--simulate", action="store_true",
                        help="Использовать генератор данных вместо порта")
    parser.add_argument("--samples", type=int, default=2000,
                        help="Число виртуальных измерений в симуляции")
    parser.add_argument("--sim-delay", type=float, default=0.2,
                        help="Задержка между измерениями симуляции, сек. (0.2 ≈ 5 Гц)")
    parser.add_argument("--plot", action="store_true",
                        help="Показать live-график (канал -> количество)")
    parser.add_argument("--refresh", type=float, default=0.2,
                        help="Частота обновления графика, сек.")
    parser.add_argument("--seed", type=int, default=None,
                        help="Seed для симулятора (повторяемость).")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = _build_argparser()
    args = parser.parse_args(argv)

    if not args.simulate and not args.port:
        parser.error("Нужно указать --port или включить --simulate.")

    if args.simulate:
        measurement_iter: Iterable[RangeMeasurement] = simulated_measurement_stream(
            samples=args.samples,
            seed=args.seed,
            delay=args.sim_delay,
        )

        if args.plot:
            counts = plot_live_histogram(
                measurements=measurement_iter,
                refresh_interval=args.refresh,
                max_points=args.samples,
            )
            print(f"Итого измерений: {sum(counts.values())}")
            return 0

        measurements = list(measurement_iter)
    else:
        cfg = SerialConfig(
            port=args.port,
            baudrate=args.baud,
            parity=args.parity,
            stopbits=float(args.stopbits),
            timeout=args.timeout,
        )

        reader = ComChannelReader(cfg)

        try:
            if args.plot:
                counts = plot_live_histogram(
                    measurements=reader.iter_measurements(
                        duration=args.duration,
                        chunk_size=args.chunk,
                    ),
                    refresh_interval=args.refresh,
                )
                print(f"Итого измерений: {sum(counts.values())}")
                return 0

            measurements = reader.collect(duration=args.duration, chunk_size=args.chunk)
        except Exception as exc:  # pragma: no cover - требует железа
            print(f"Ошибка при чтении из {args.port}: {exc}", file=sys.stderr)
            return 1

    if not measurements:
        print("Данные не получены.")
        return 0

    for idx, meas in enumerate(measurements, start=1):
        print(
            f"[{idx:04d}] channel={meas.channel_value:04X} "
            f"a={meas.a_value:04X} b={meas.b_value:04X}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

