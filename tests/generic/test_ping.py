from tellypy import Client
import random
from string import ascii_letters, digits

import sys
from pathlib import Path

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import wrong_argument, ExtendedTestCase
except ImportError:
    pass


class PingCommand(ExtendedTestCase):
    client: Client

    @classmethod
    def setUpClass(self):
        self.client = Client(host="localhost", port=6379)
        self.client.connect()

    def test_without_arguments(self):
        self.assertSimpleStringEqual("PING", "PONG")

    def test_with_argument(self):
        value = "".join(random.choices(ascii_letters + digits, k=64))
        self.assertBulkStringEqual(f"PING {value}", value)

    def test_with_arguments(self):
        self.assertSimpleErrorEqual("PING a b", wrong_argument("PING"))
