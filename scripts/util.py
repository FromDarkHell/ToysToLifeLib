import struct
from dataclasses import dataclass
from typing import Tuple


@dataclass(frozen=True)
class Color:
    r: int
    g: int
    b: int

    def as_tuple(self) -> Tuple[int, int, int]:
        return (self.r, self.g, self.b)

    def __iter__(self):
        return iter(self.as_tuple())

    @classmethod
    def from_hex(cls, hex_str: str) -> "Color":
        hex_str = hex_str.strip().lstrip("#")
        r, g, b = struct.unpack("BBB", bytes.fromhex(hex_str))
        return cls(r, g, b)


class Colors:
    BLACK = Color(0, 0, 0)
    WHITE = Color(255, 255, 255)
    RED = Color(255, 0, 0)
    GREEN = Color(0, 255, 0)
    BLUE = Color(0, 0, 255)
    YELLOW = Color(255, 255, 0)
    MAGENTA = Color(255, 0, 255)
    CYAN = Color(0, 255, 255)
