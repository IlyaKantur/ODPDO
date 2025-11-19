from enum import Enum
from typing import Iterable, Iterator, Tuple


class FragmentType(Enum):
    EMPTY = 0
    LO = 1
    HI = 2
    LO2 = 3


MASK_ORDER = 0xC0
MASK_VALUE = 0x3F
MASK_LOW = 0x00      # RST_LO
MASK_HIGH = 0x40     # RST_HI
MASK_HIGH2 = 0x80    # RST_LO2
RANGE_SPAN = 0xFFF   # 4095


def _decode_pair(stream: Iterator[int]) -> Iterator[Tuple[int, int]]:
    """
    Принимает поток байтов (0..255). На выходе — готовые пары (A, B)
    из двух 12‑битных чисел.
    """
    e_type = FragmentType.EMPTY
    first = second = 0

    for raw in stream:
        raw &= 0xFF
        order = raw & MASK_ORDER
        value = raw & MASK_VALUE

        if e_type is FragmentType.EMPTY:
            if order != MASK_LOW:
                raise ValueError("MASK_LOW expected")
            first = value
            e_type = FragmentType.LO

        elif e_type is FragmentType.LO:
            if order != MASK_HIGH:
                raise ValueError("MASK_HIGH expected")
            first |= value << 6
            e_type = FragmentType.HI

        elif e_type is FragmentType.HI:
            if order != MASK_LOW:
                raise ValueError("MASK_LOW expected")
            second = value
            e_type = FragmentType.LO2

        elif e_type is FragmentType.LO2:
            if order != MASK_HIGH2:
                raise ValueError("MASK_HIGH2 expected")
            second |= value << 6
            e_type = FragmentType.EMPTY

            if 0 < first < RANGE_SPAN and 0 < second < RANGE_SPAN:
                yield first, second
        else:
            raise RuntimeError("Unknown state")

    # Если стрим оборвался посередине, оставляем фрагменты как есть.


def _combine_to_channel(a: int, b: int) -> int:
    """Формула из C++: round(0.5 * ((A - B)/(A + B) + 1) * RANGE_SPAN)."""
    total = a + b
    if total == 0:
        raise ZeroDivisionError("A + B = 0, формулу нельзя применить")
    value = 0.5 * (((a - b) / total) + 1.0) * RANGE_SPAN
    return int(value + 0.5)  # округление до ближайшего целого


def parse_stream(byte_iterable: Iterable[int]) -> Iterator[Tuple[int, int, int]]:
    """
    Высокоуровневая обёртка.
    На вход — iterable байтов. На выход — тройки (канал, A, B).
    """
    for a, b in _decode_pair(iter(byte_iterable)):
        channel = _combine_to_channel(a, b)
        yield channel, a, b


# Пример использования:
if __name__ == "__main__":
    import random

    def encode_sample(a: int, b: int) -> bytes:
        """Генератор тестового потока с тем же фреймингом."""
        def encode_chunk(value: int, low_mask: int) -> Tuple[int, int]:
            lo = (value & 0x3F) | low_mask
            hi = ((value >> 6) & 0x3F) | (low_mask + 0x40)
            return lo, hi

        a_lo, a_hi = encode_chunk(a, MASK_LOW)
        b_lo = (b & 0x3F) | MASK_LOW
        b_hi = ((b >> 6) & 0x3F) | MASK_HIGH2
        return bytes([a_lo, a_hi, b_lo, b_hi])

    # строим тестовый поток из 3 валидных пар + шум (сброс ошибки)
    sample_stream = bytearray()
    for _ in range(3):
        a = random.randint(1, RANGE_SPAN - 1)
        b = random.randint(1, RANGE_SPAN - 1)
        sample_stream.extend(encode_sample(a, b))

    for channel, a, b in parse_stream(sample_stream):
        print(f"Channel={channel} (A={a}, B={b})")